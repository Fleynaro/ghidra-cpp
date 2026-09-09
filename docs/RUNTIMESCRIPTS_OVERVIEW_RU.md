# `Ghidra/RuntimeScripts`: что это и зачем нужно

Этот документ продолжает [обзор архитектуры Ghidra](ARCHITECTURE_OVERVIEW_RU.md)
и объясняет новичку каталог [`Ghidra/RuntimeScripts`](../Ghidra/RuntimeScripts).
Здесь разобраны именно скрипты, которые запускают и обслуживают установленную
Ghidra. Это **не** каталог пользовательских скриптов анализа из
`ghidra_scripts`.

## Короткий ответ

`RuntimeScripts` -- это внешний слой запуска Ghidra. Он находится между командой
пользователя и Java-классами приложения:

```text
команда пользователя
  -> ghidraRun / analyzeHeadless / pyghidraRun / server/ghidraSvr
  -> общий launch.sh или launch.bat
  -> поиск подходящего JDK и чтение launch.properties
  -> Java-класс Ghidra
```

Например, `support/analyzeHeadless` сам не анализирует PE-файл. Он подготавливает
JVM и вызывает `ghidra.app.util.headless.AnalyzeHeadless`; основная логика
находится в [`AnalyzeHeadless.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/headless/AnalyzeHeadless.java).

Каталог нужен для четырех задач:

- запуск GUI и вспомогательных Java-инструментов;
- запуск анализа без GUI и пакетная обработка файлов;
- запуск серверной части Ghidra и ее администрирование;
- перенос одинакового способа запуска между Windows, Linux и macOS.

## Как каталог попадает в дистрибутив

`RuntimeScripts` подключен к общей Gradle-сборке как проект
[`RuntimeScripts`](../settings.gradle#L42-L44). Его `build.gradle` не компилирует
большой Java-модуль: он копирует `support`, `server`, `ghidraRun` и
`ghidraRun.bat` в итоговый дистрибутив. Это видно в
[`Ghidra/RuntimeScripts/build.gradle`](../Ghidra/RuntimeScripts/build.gradle#L20-L38).

В режиме разработки скрипты находят корень репозитория и классы из `bin/main` или
Gradle JAR. В установленной Ghidra они используют готовые JAR и каталоги
дистрибутива. Такое различие реализовано в
[`support/launch.sh`](../Ghidra/RuntimeScripts/support/launch.sh#L103-L125) и
[`support/launch.bat`](../Ghidra/RuntimeScripts/support/launch.bat#L99-L124).

## Структура каталога

```text
Ghidra/RuntimeScripts/
  ghidraRun, ghidraRun.bat       обычный GUI-запуск
  build.gradle                   упаковка файлов в дистрибутив
  certification.manifest         метаданные сертификации
  support/                       клиентские и разработческие утилиты
    launch.sh, launch.bat        общий запуск Java
    launch.properties            JVM, кодировка, TLS, каталоги и UI
    analyzeHeadless              пакетный анализ без GUI
    ghidraDebug                  запуск GUI с JDWP-отладкой
    pyghidraRun                  Python + Java API
    jshellRun                    интерактивный Java JShell
    sleigh                      компилятор описаний процессора Sleigh
    bsim                        командная строка BSim
    ghidraClean                 поиск и очистка артефактов
    ...                         остальные служебные команды
  server/                       Ghidra Server и его обслуживание
    ghidraSvr                   запуск/остановка серверной службы
    server.conf                 конфигурация YAJSW и сервера
    svrInstall, svrUninstall    установка и удаление службы
    svrAdmin                    пользователи и репозитории
    certTool                    сертификаты и keystore
```

Почти для каждой Unix-команды существует Windows-обертка `.bat`: например,
`analyzeHeadless` и `analyzeHeadless.bat`, `ghidraRun` и `ghidraRun.bat`.
Это не две разные реализации Ghidra, а две версии оболочки с одинаковой целью.

## Общий запуск Java

### `ghidraRun`

Корневой [`ghidraRun`](../Ghidra/RuntimeScripts/ghidraRun) выбирает память из
`GHIDRA_MAXMEM` и `GHIDRA_GUI_MAXMEM`, собирает пользовательские JVM-опции и
вызывает:

```bash
support/launch.sh bg jdk Ghidra "" "..." ghidra.GhidraRun [аргументы]
```

`bg` означает запуск в фоне, `jdk` -- требование JDK, а
`ghidra.GhidraRun` -- Java-класс GUI. Поэтому обычный запуск из терминала выглядит
так:

```bash
./Ghidra/RuntimeScripts/ghidraRun
```

На Windows используется тот же контракт через
[`ghidraRun.bat`](../Ghidra/RuntimeScripts/ghidraRun.bat#L26-L35).

### `launch.sh` и `launch.bat`

Это центральный диспетчер. Его параметры имеют вид:

```text
launch <режим> <jdk|jre> <имя> <макс-память> "<JVM-опции>" <Java-класс> <аргументы>
```

Поддерживаются режимы `fg` (текущий процесс), `bg` (фон), `debug` и
`debug-suspend`. В debug-режиме добавляется JDWP, по умолчанию на
`127.0.0.1:18001`; адрес можно изменить через `DEBUG_ADDRESS`.

Далее скрипт:

1. определяет production или development installation;
2. находит `java` через `PATH` или `JAVA_HOME`;
3. запускает `LaunchSupport` для выбора поддерживаемого JDK;
4. читает переменные и JVM-аргументы из `launch.properties`;
5. вызывает `ghidra.Ghidra` с именем Java-приложения и его аргументами.

Java-часть выбора окружения находится в модуле
[`GhidraBuild/LaunchSupport`](../GhidraBuild/LaunchSupport), а путь к
`launch.properties` вычисляется в
[`AppConfig.java`](../GhidraBuild/LaunchSupport/src/main/java/ghidra/launch/AppConfig.java#L361-L383).

### `launch.properties`

Файл [`launch.properties`](../Ghidra/RuntimeScripts/support/launch.properties)
задает базовые настройки для всех запусков. Примеры:

- `-Djava.system.class.loader=ghidra.GhidraClassLoader` -- загрузчик классов;
- UTF-8 и английская локаль;
- параметры Java2D для Windows, Linux и macOS;
- TLS 1.2/1.3 и настройки доверенных сертификатов;
- ограничения числа ядер через `cpu.core.limit` или `cpu.core.override`;
- каталоги настроек, кэша и временных файлов;
- диагностические флаги PDB и PyGhidra.

Большинство строк начинается с `VMARGS=` и превращается в аргумент JVM. Не стоит
без необходимости менять этот файл: он влияет не только на GUI, но и на серверные
клиентские соединения и headless-запуски.

## Пакетный анализ: `support/analyzeHeadless`

Headless Analyzer -- это Ghidra без окон. Обертка
[`analyzeHeadless`](../Ghidra/RuntimeScripts/support/analyzeHeadless) по умолчанию
задает `2G`, включает `java.awt.headless=true`, ограничивает GC/JIT-потоки и
вызывает `AnalyzeHeadless`.

Подробная спецификация аргументов находится в
[`analyzeHeadlessREADME.md`](../Ghidra/RuntimeScripts/support/analyzeHeadlessREADME.md).
Утилита умеет:

- импортировать один файл или каталог;
- рекурсивно обрабатывать много файлов;
- запускать `-preScript` до анализа и `-postScript` после него;
- выбирать `-processor` и `-cspec`;
- писать отдельные application/script logs;
- работать read-only, удалять временный проект или коммитить shared-проект.

Пример: создать проект `DemoProject` и импортировать ELF/PE-файлы из каталога:

```bash
./Ghidra/RuntimeScripts/support/analyzeHeadless \
  /work/projects DemoProject \
  -import /work/binaries -recursive
```

Пример пакетного запуска пользовательского Java-скрипта после автоанализа:

```bash
./Ghidra/RuntimeScripts/support/analyzeHeadless \
  /work/projects DemoProject \
  -process '*.exe' \
  -scriptPath /work/scripts \
  -postScript ExportSymbols.java
```

Важно: имя в `-postScript` -- это имя скрипта, а каталог передается через
`-scriptPath`. По умолчанию Ghidra ищет скрипты в `$USER_HOME/ghidra_scripts` и
в `ghidra_scripts` внутри дистрибутива. Это описано в README на строках
[`204-224`](../Ghidra/RuntimeScripts/support/analyzeHeadlessREADME.md#L204-L224).

### Параметры процессора

Для x86 можно явно указать язык и compiler spec:

```bash
./Ghidra/RuntimeScripts/support/analyzeHeadless \
  /work/projects X86Project \
  -import program.exe \
  -processor x86:LE:32:default -cspec windows
```

Допустимые идентификаторы следует брать из `*.ldefs`, например из
каталога [`Processors/x86/data/languages`](../Ghidra/Processors/x86/data/languages).

## Остальные команды `support`

| Команда | Назначение и Java-точка входа |
|---|---|
| `ghidraDebug` | GUI с JDWP; вызывает `ghidra.GhidraRun`. |
| `ghidraClean` | Интерактивный поиск артефактов Ghidra; вызывает `utility.application.AppCleaner`. |
| `jshellRun` | JShell с классами Ghidra; вызывает `ghidra.JShellRun`. |
| `pyghidraRun` | Запуск PyGhidra через Python 3 и JPype; передает управление `Features/PyGhidra/support/pyghidra_launcher.py`. |
| `sleigh` | Компиляция языка Sleigh в `.sla`; вызывает `SleighCompileLauncher`. |
| `bsim` | CLI для загрузки/запросов BSim; вызывает `BSimLaunchable`. |
| `convertStorage` | Конвертация файлового хранилища Ghidra; вызывает `ConvertFileSystem`. |
| `buildGhidraJar` | Создание единого `ghidra.jar`; вызывает `GhidraJarBuilder`. |
| `updateServerAllowList` | Просмотр/изменение списка разрешенных серверов; вызывает `UpdateServerAllowList`. |
| `createPdbXmlFiles.bat` | Windows-конвертация `.pdb` в `.pdb.xml` для импорта символов на не-Windows системах. |
| `GhidraGo/ghidraGo` | Передача `ghidra://` URL уже запущенной или новой Ghidra. |

Например, для запуска PyGhidra нужны Python 3 и его зависимости, а сам runtime
скрипт находится в [`pyghidraRun`](../Ghidra/RuntimeScripts/support/pyghidraRun#L19-L35).
Его Python-логика находится в
[`Features/PyGhidra`](../Ghidra/Features/PyGhidra), поэтому исправлять поведение
Python API нужно там, а не в тонкой оболочке.

### Единый JAR

`buildGhidraJar` создает в текущем каталоге минимальный `ghidra.jar`:

```bash
./Ghidra/RuntimeScripts/support/buildGhidraJar
java -Xmx1G -jar ghidra.jar -gui
```

Дополнительный `-srczip имя.zip` добавляет архив исходников для отладки. У такого
режима есть ограничение: README прямо указывает, что другие языки скриптов,
например Python, в single-JAR конфигурации не поддерживаются. Подробности и
Java API-сценарий см. в
[`buildGhidraJarREADME.txt`](../Ghidra/RuntimeScripts/support/buildGhidraJarREADME.txt).

### PDB XML

На Windows команда:

```bat
Ghidra\RuntimeScripts\support\createPdbXmlFiles.bat C:\Symbols
```

рекурсивно находит `.pdb` и создает рядом `.pdb.xml`. Это позволяет перенести
символьную информацию Microsoft-программ на Linux или macOS; исходный алгоритм
обхода и вызова `pdb.exe` виден в
[`createPdbXmlFiles.bat`](../Ghidra/RuntimeScripts/support/createPdbXmlFiles.bat#L60-L97).

### GhidraGo

GhidraGo связывает `ghidra://` URL с проектом. Например:

```text
ghidra://hostname/Repo/notepad.exe#main
```

Команду передают так:

```bash
ghidraGo ghidra://hostname/Repo/notepad.exe#main
```

Механизм может запустить Ghidra, открыть DomainFile и поставить курсор на символ.
Он использует файловый обмен, а не открытый порт. Полное описание форматов URL и
настройки protocol handler находится в
[`ghidraGoREADME.html`](../Ghidra/RuntimeScripts/support/GhidraGo/ghidraGoREADME.html),
а запуск GUI-сервиса -- в [`GhidraGo.java`](../Ghidra/Features/GhidraGo/src/main/java/ghidra/GhidraGo.java#L150-L170).

## Серверный каталог

`RuntimeScripts/server` -- это не серверная реализация целиком. Это операционные
обертки и конфигурация для модуля
[`Features/GhidraServer`](../Ghidra/Features/GhidraServer). Главный скрипт
[`ghidraSvr`](../Ghidra/RuntimeScripts/server/ghidraSvr) использует YAJSW и запускает
класс `ghidra.server.remote.GhidraServer`, указанный в
[`server.conf`](../Ghidra/RuntimeScripts/server/server.conf#L128-L144).

Перед установкой нужно настроить `server.conf`, прежде всего:

- `ghidra.repositories.dir` -- отдельный каталог репозиториев;
- TLS/keystore и сертификаты;
- параметры аутентификации;
- `wrapper.java.initmemory` и `wrapper.java.maxmemory`;
- последовательные `wrapper.app.parameter.N` для аргументов сервера.

Минимальный жизненный цикл на Linux/macOS:

```bash
sudo Ghidra/RuntimeScripts/server/svrInstall
Ghidra/RuntimeScripts/server/ghidraSvr status
sudo Ghidra/RuntimeScripts/server/ghidraSvr stop
sudo Ghidra/RuntimeScripts/server/svrUninstall
```

Для Windows используются `.bat`-варианты. Административные операции выполняются
через `svrAdmin`, например:

```bash
Ghidra/RuntimeScripts/server/svrAdmin -list
Ghidra/RuntimeScripts/server/svrAdmin -users
Ghidra/RuntimeScripts/server/svrAdmin -add analyst --p
```

`certTool` создает CSR, PKCS#12 или self-signed keystore. Например, схема из
серверного README:

```bash
Ghidra/RuntimeScripts/server/certTool request -outkey server.key
Ghidra/RuntimeScripts/server/certTool pkcs12 \
  -inkey server.key -cert server.crt -out server.p12
```

Для реального сетевого сервера нужен доверенный сертификат и корректные SAN;
self-signed сертификат ограничивает безопасную проверку клиента. Практические
предупреждения находятся в [`server/svrREADME.md`](../Ghidra/RuntimeScripts/server/svrREADME.md#L40-L67).

## Частые ошибки новичков

- Запуск `ghidraRun` из исходного репозитория без сборки: `launch.*` ожидает классы
  Eclipse или Gradle и сообщает, что Ghidra не скомпилирована.
- Попытка использовать JRE для команды, требующей JDK: у `ghidraRun`, headless,
  Sleigh и PyGhidra в оболочках указан `jdk`.
- Запуск headless поверх уже открытого в GUI проекта: README предупреждает, что
  это может не сработать.
- Указание `-cspec` без `-processor`: headless README запрещает такую комбинацию.
- Изменение `server.conf` без переустановки службы: параметры wrapper-сервиса
  могли быть сгенерированы при установке.
- Принятие `RuntimeScripts` за место для Java/Python-логики: здесь в основном
  маршрутизация и окружение; настоящие точки входа находятся в модулях, на которые
  ссылается таблица выше.

## Куда идти дальше

1. Для общей модели `Program`, памяти и транзакций --
   [`FRAMEWORK_OVERVIEW_RU.md`](FRAMEWORK_OVERVIEW_RU.md).
2. Для PE и x86 -- разделы маршрута в
   [`ARCHITECTURE_OVERVIEW_RU.md`](ARCHITECTURE_OVERVIEW_RU.md).
3. Для автоматизации импорта и анализа --
   [`analyzeHeadlessREADME.md`](../Ghidra/RuntimeScripts/support/analyzeHeadlessREADME.md).
4. Для Python API -- [`Features/PyGhidra`](../Ghidra/Features/PyGhidra).
5. Для совместной работы -- [`server/svrREADME.md`](../Ghidra/RuntimeScripts/server/svrREADME.md).
