# Что находится в `GPL` и зачем это нужно Ghidra

Этот документ объясняет папку [`GPL`](../GPL) простыми словами и показывает, как
она связана с обычными модулями [`Ghidra`](../Ghidra). Это продолжение общего
[обзора архитектуры Ghidra](ARCHITECTURE_OVERVIEW_RU.md): сначала полезно понять
модель `Program`, загрузчики и декомпилятор, а затем смотреть на внешние
компоненты из `GPL`.

## Короткий ответ

`GPL` — не «ещё одна часть Ghidra» и не каталог исходников, без которого не
запускается абсолютно вся программа. Это отдельная зона проекта для компонентов,
которые имеют отличные от основного кода лицензии, используют сторонний код или
собираются как отдельные native/standalone-программы.

В текущем репозитории там находятся:

| Путь | Назначение |
|---|---|
| [`GPL/DemanglerGnu`](../GPL/DemanglerGnu) | Два standalone GNU C++ demangler: версии 2.24 и 2.41. Они превращают имена вроде `_ZN3Foo3barEv` обратно в читаемые имена C++. |
| [`GPL/GnuDisassembler`](../GPL/GnuDisassembler) | Обёртка над GNU binutils disassembler, исполняемый файл `gdis`. Нужен главным образом для проверки синтаксиса дизассемблера Sleigh. |
| [`GPL/DMG`](../GPL/DMG) | Отдельный Java-сервер и библиотеки для чтения файловых систем внутри macOS/iOS DMG. |
| [`GPL/licenses`](../GPL/licenses) | Тексты GPL, LGPL и других лицензий, которые нужно поставлять вместе с соответствующими компонентами. |
| [`GPL/Icons`](../GPL/Icons) | Ресурсы иконок с отдельными условиями лицензирования. |
| `GPL/*gradle` | Общие правила сборки native-компонентов, платформ и каталогов `build/os/<platform>`. |

Сам факт включения `GPL` в сборку виден в корневом
[`settings.gradle`](../settings.gradle#L23-L32). При этом корневой проект также
подключает общие скрипты из `GPL` для native-сборки:
[`build.gradle`](../build.gradle#L138-L139).

## Почему не достаточно только `Ghidra/*`

Обычные модули `Ghidra/*` содержат Java-логику Ghidra, интерфейс, модель
программы, загрузчики, Sleigh и интеграцию. Но некоторые возможности требуют
внешнего компонента по трём причинам.

### 1. Лицензия и происхождение кода различаются

Основной код Ghidra распространяется по Apache License 2.0. В `GPL` лежат
скопированные или адаптированные части GNU binutils, сторонние библиотеки и
компоненты с GPL/LGPL-compatible условиями. Сборочные правила отдельно формируют
лицензионный текст для модулей из `GPL`: это видно в
[`gradle/support/ip.gradle`](../gradle/support/ip.gradle#L335-L369).

Поэтому папка является не только техническим каталогом, но и границей учёта
лицензий. Например, при упаковке дистрибутива в него отдельно копируются
`GPL/*`, `GPL/Icons` и `GPL/licenses`:
[`gradle/root/distribution.gradle`](../gradle/root/distribution.gradle#L372-L381).

### 2. Нужен готовый внешний исполняемый backend

Java-класс может организовать запуск процесса, но не обязан содержать реализацию
GNU demangling или binutils disassembly внутри JVM. Такой дизайн позволяет:

```text
Ghidra Java -> stdin внешней программы -> stdout результата -> Ghidra Java
```

Для GNU demangler это прямо видно в
[`GnuDemangler.java`](../Ghidra/Features/GnuDemangler/src/main/java/ghidra/app/util/demangler/gnu/GnuDemangler.java#L108-L121):
Ghidra получает `GnuDemanglerNativeProcess`, вызывает `process.demangle(...)`,
а затем разбирает текстовый результат в объект `DemangledObject`.

В `GPL/DemanglerGnu/build.gradle` описаны два native executable и разные
платформы сборки: `demangler_gnu_v2_41` и `demangler_gnu_v2_24`
([`build.gradle`](../GPL/DemanglerGnu/build.gradle#L20-L24),
[`build.gradle`](../GPL/DemanglerGnu/build.gradle#L72-L125)). Две версии нужны не
для красоты: старая версия понимает некоторые старые форматы и исторические
особенности, которые современная версия не поддерживает. Это отражено в
[`GnuDemanglerOptions.java`](../Ghidra/Features/GnuDemangler/src/main/java/ghidra/app/util/demangler/gnu/GnuDemanglerOptions.java#L31-L45).

### 3. Это отдельные инструменты, а не общий анализатор

Sleigh — собственный механизм описания процессоров Ghidra. `gdis` — независимый
GNU disassembler. Он не заменяет Sleigh и не является обязательным для каждого
импорта. Его ценность в том, что два разных декодера можно сравнить на одних и
тех же байтах. README модуля прямо говорит, что binutils используется для
проверки синтаксиса вывода Sleigh:
[`GPL/GnuDisassembler/README.md`](../GPL/GnuDisassembler/README.md#L3-L6).

## Разбор содержимого `GPL`

### `DemanglerGnu`: имена C++

Компилятор C++ обычно хранит в бинарнике не исходное имя `Foo::bar()`, а
закодированное имя, например `_ZN3Foo3barEv`. Такой формат называется mangling.
Demangler выполняет обратную операцию.

В Ghidra есть Java-интеграция в
[`Ghidra/Features/GnuDemangler`](../Ghidra/Features/GnuDemangler). Она решает
прикладные задачи:

- определяет, подходит ли GNU demangler для ELF, Mach-O или GCC-программы
  ([`GnuDemangler.java`](../Ghidra/Features/GnuDemangler/src/main/java/ghidra/app/util/demangler/gnu/GnuDemangler.java#L47-L65));
- выбирает native executable;
- отбрасывает заведомо неподходящие имена;
- передаёт строку внешнему процессу;
- превращает ответ в namespace, функцию, типы и другие объекты Ghidra.

Сами C-файлы demangler находятся в
[`GPL/DemanglerGnu/src`](../GPL/DemanglerGnu/src), а не в Java-модуле. Важная
деталь — это не полная сборка binutils: исходники специально урезаны до
необходимого набора. README версии 2.24 объясняет, что такой набор позволяет
собирать standalone demangler через Make или Visual Studio без сложного
`configure`, что особенно важно для Windows
([`README.txt`](../GPL/DemanglerGnu/src/demangler_gnu_v2_24/README.txt#L9-L19)).

В том же README описаны локальные изменения Ghidra: например, в старом
demangler добавлена полноценная `main`-логика для поведения `c++filt`, а формат
вывода подстроен под построчное чтение Ghidra
([`README.txt`](../GPL/DemanglerGnu/src/demangler_gnu_v2_24/README.txt#L24-L57)).

Пример цепочки:

```text
_ZN3Foo3barEv
  -> demangler_gnu_v2_41.exe
  -> Foo::bar()
  -> GnuDemanglerParser
  -> имя функции и namespace в Program
```

Без установленного native executable Java-код интеграции останется, но операция
demangle не сможет выполнить внешний backend. Поэтому в поставке важны не только
классы `Ghidra/Features/GnuDemangler`, но и бинарники/исходники из `GPL`.

### `GnuDisassembler`: независимая проверка Sleigh

`GnuDisassembler` состоит из небольшого собственного C-кода
([`src/gdis/c`](../GPL/GnuDisassembler/src/gdis/c)), скриптов сборки и входного
архива binutils. Поддерживаемая версия binutils задаётся в
[`build.gradle`](../GPL/GnuDisassembler/build.gradle#L22-L35), а сборочная
обвязка распаковывает, конфигурирует и собирает binutils перед сборкой `gdis`
([`buildGdis.gradle`](../GPL/GnuDisassembler/buildGdis.gradle#L76-L141)).

В Java Ghidra это подключается через
[`GNUExternalDisassembler.java`](../Ghidra/Extensions/SleighDevTools/src/main/java/ghidra/app/util/disassemble/GNUExternalDisassembler.java).
Класс:

1. ищет модуль `GnuDisassembler` и `gdis`/`gdis.exe`;
2. выбирает архитектуру и machine id;
3. запускает процесс;
4. отправляет байты через stdin;
5. читает текстовый дизассемблированный результат.

Команда запуска собрана в исходнике около
[`GNUExternalDisassembler.java`](../Ghidra/Extensions/SleighDevTools/src/main/java/ghidra/app/util/disassemble/GNUExternalDisassembler.java#L533-L570).
Если расширение не установлено, основной дизассемблер Ghidra не перестаёт
работать: просто недоступен внешний provider, что код отдельно диагностирует
([`GNUExternalDisassembler.java`](../Ghidra/Extensions/SleighDevTools/src/main/java/ghidra/app/util/disassemble/GNUExternalDisassembler.java#L743-L746)).

Практический пример: разработчик добавил инструкцию ARM в
[`Ghidra/Processors`](../Ghidra/Processors), описал её в Sleigh и хочет проверить,
что отображаемый текст совпадает с GNU binutils. Для этого нужен `gdis`; обычного
Java-кода анализатора недостаточно, потому что эталонный декодер является другим
исполняемым инструментом.

### `DMG`: чтение macOS/iOS disk image

DMG — контейнер/образ диска Apple. В `Ghidra/Features/FileFormats` находится
клиентская часть: она распознаёт DMG, при необходимости расшифровывает его и
создаёт файловую систему. В `GPL/DMG` находится отдельный сервер, который
непосредственно читает содержимое образа с помощью библиотек в
[`GPL/DMG/data/lib`](../GPL/DMG/data/lib).

Распознавание и проверка наличия модуля происходят в
[`DmgClientFileSystemFactory.java`](../Ghidra/Features/FileFormats/src/main/java/ghidra/file/formats/ios/dmg/DmgClientFileSystemFactory.java#L44-L57)
и [`DmgClientFileSystemFactory.java`](../Ghidra/Features/FileFormats/src/main/java/ghidra/file/formats/ios/dmg/DmgClientFileSystemFactory.java#L141-L155).
При отсутствии `DMG` Ghidra намеренно сообщает, что открыть такую файловую
систему нельзя, вместо того чтобы ломать всю программу.

Сервер собирается отдельным source set и пакуется как `DMG.jar`:
[`GPL/DMG/build.gradle`](../GPL/DMG/build.gradle#L24-L38),
[`GPL/DMG/build.gradle`](../GPL/DMG/build.gradle#L51-L60). Его точка входа
[`DmgServer.java`](../GPL/DMG/src/dmg/java/mobiledevices/dmg/server/DmgServer.java)
принимает команды через stdin, например `open`, `get_listing`, `get_info` и
`get_data`, а ответы отправляет через stdout
([`DmgServer.java`](../GPL/DMG/src/dmg/java/mobiledevices/dmg/server/DmgServer.java#L55-L145)).

Клиентская часть Ghidra запускает сервер так:

```text
DMG-файл
  -> DmgClientFileSystemFactory
  -> временный расшифрованный файл при необходимости
  -> DMG.jar / mobiledevices.dmg.server.DmgServer
  -> get_listing / get_data
  -> виртуальная файловая система Ghidra
```

Это не дублирование обычного `Ghidra/Features/FileFormats`: там находится
интерфейс файловой системы и orchestration, а в `GPL/DMG` — изолированный
reader/backend. Менеджер процесса поддерживает очередь команд, таймаут и
перезапуск сервера при сбое
([`DmgServerProcessManager.java`](../Ghidra/Features/FileFormats/src/main/java/ghidra/file/formats/ios/dmg/DmgServerProcessManager.java#L31-L47),
[`DmgServerProcessManager.java`](../Ghidra/Features/FileFormats/src/main/java/ghidra/file/formats/ios/dmg/DmgServerProcessManager.java#L95-L179)).

### Лицензии, иконки и служебные Gradle-файлы

Файл [`GPL/certification.local.manifest`](../GPL/certification.local.manifest)
помечает локальную IP-категорию как Public Domain. Однако это не означает, что
всё содержимое каталога имеет одну лицензию. Реальные тексты лежат в
[`GPL/licenses`](../GPL/licenses), включая GPL 2/3, LGPL 2.1/3.0, Public Domain и
GPL 2 с Classpath Exception.

`nativePlatforms.gradle` задаёт имена платформ вроде `win_x86_64`,
`linux_arm_64`, `mac_arm_64` и BSD-вариантов
([`nativePlatforms.gradle`](../GPL/nativePlatforms.gradle#L13-L33)).
`nativeBuildProperties.gradle` подключает C/C++ Gradle-плагины, проверяет
toolchain и предоставляет правило `buildNatives[_PlatformName]`
([`nativeBuildProperties.gradle`](../GPL/nativeBuildProperties.gradle#L15-L35),
[`nativeBuildProperties.gradle`](../GPL/nativeBuildProperties.gradle#L120-L150)).
Это общая инфраструктура, которой пользуются не только каталоги внутри `GPL`:
например, native-сборки Decompiler, PDB и FileFormats импортируют эти скрипты
из `GPL` ([`Ghidra/Features/Decompiler/buildNatives.gradle`](../Ghidra/Features/Decompiler/buildNatives.gradle#L17-L23)).

## Что будет, если удалить `GPL`

Результат зависит от удалённой части:

- Без `DemanglerGnu` исчезнет GNU C++ demangling, особенно полезный для ELF,
  Mach-O и GCC-бинарников.
- Без `GnuDisassembler` базовая Ghidra всё ещё сможет дизассемблировать через
  Sleigh, но исчезнет внешний сравнительный provider для разработчиков языков.
- Без `DMG` нельзя будет открыть DMG-файловую систему через соответствующий
  модуль FileFormats.
- Без `GPL/*gradle` сломается часть общей сборки native-компонентов, включая
  некоторые модули `Ghidra/*`.
- Без `GPL/licenses` технический код может существовать, но дистрибутив будет
  неполным с точки зрения обязательной поставки лицензий и notices.

Итак, `Ghidra/*` содержит основную платформу и Java-интеграцию, а `GPL` закрывает
отдельные внешние backends, native-сборку, сторонний код и лицензионную упаковку.
Они не взаимозаменяемы: Ghidra может работать частично без некоторых GPL-модулей,
но для полного набора функций и корректной сборки/поставки нужны оба слоя.

## Рекомендуемый маршрут чтения исходников

1. [`settings.gradle`](../settings.gradle) — увидеть, как `GPL` подключается к
   корневой сборке.
2. [`GPL/DemanglerGnu/build.gradle`](../GPL/DemanglerGnu/build.gradle) и
   [`GnuDemangler.java`](../Ghidra/Features/GnuDemangler/src/main/java/ghidra/app/util/demangler/gnu/GnuDemangler.java)
   — понять связку Java и native demangler.
3. [`GPL/GnuDisassembler/README.md`](../GPL/GnuDisassembler/README.md) и
   [`GNUExternalDisassembler.java`](../Ghidra/Extensions/SleighDevTools/src/main/java/ghidra/app/util/disassemble/GNUExternalDisassembler.java)
   — увидеть роль binutils как внешней проверки Sleigh.
4. [`GPL/DMG/build.gradle`](../GPL/DMG/build.gradle),
   [`DmgServer.java`](../GPL/DMG/src/dmg/java/mobiledevices/dmg/server/DmgServer.java)
   и [`DmgServerProcessManager.java`](../Ghidra/Features/FileFormats/src/main/java/ghidra/file/formats/ios/dmg/DmgServerProcessManager.java)
   — проследить IPC-контракт DMG.
5. [`gradle/root/distribution.gradle`](../gradle/root/distribution.gradle) и
   [`gradle/support/ip.gradle`](../gradle/support/ip.gradle) — понять, почему
   лицензии и support-файлы попадают в отдельный каталог поставки.
