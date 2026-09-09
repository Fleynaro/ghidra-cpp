# Конфигурация `Public_Release` в Ghidra

Этот документ объясняет простыми словами, что находится в
[`Ghidra/Configurations/Public_Release`](../Ghidra/Configurations/Public_Release), зачем
эта папка нужна и как ее файлы попадают в готовую поставку Ghidra.

Это специализированное продолжение
[`ARCHITECTURE_OVERVIEW_RU.md`](ARCHITECTURE_OVERVIEW_RU.md). В общем обзоре папка
`Ghidra/Configurations` описана как место для конфигураций сборки и состава поставки;
здесь разобрана конкретная конфигурация `Public_Release`.

## Короткий ответ

`Public_Release` -- это не отдельная функция анализа бинарников и не проект
пользователя. Это Gradle-модуль, который участвует в сборке публичного дистрибутива
Ghidra.

Он собирает вокруг основного кода поставочные материалы:

- текст пользовательского соглашения;
- текст лицензии на стартовом экране;
- первоначальную раскладку инструмента CodeBrowser;
- `What's New` и историю изменений;
- список известных серверов PDB-символов;
- список серверов DWARF/debuginfod;
- служебную карту лицензирования файлов для внутренней проверки сборки.

Сам декомпилятор, загрузчики, процессоры и анализаторы находятся в других модулях.
Например, базовый конвейер `файл -> Program -> дизассемблирование -> p-code ->
декомпиляция` описан в [архитектурном обзоре](ARCHITECTURE_OVERVIEW_RU.md), а исходники
декомпилятора находятся в [`Ghidra/Features/Decompiler`](../Ghidra/Features/Decompiler).

## Как модуль подключается к сборке

Корневой [`settings.gradle`](../settings.gradle#L23-L30) подключает всю группу
`Ghidra/Configurations`. Внутри нее Gradle находит `Public_Release` как обычный
подпроект.

Главный файл модуля --
[`Public_Release/build.gradle`](../Ghidra/Configurations/Public_Release/build.gradle#L16-L28).
Он подключает общие скрипты Java, тестов, JaCoCo, справки и поставочного модуля:

```groovy
apply from: "$rootProject.projectDir/gradle/distributableGhidraModule.gradle"
apply from: "$rootProject.projectDir/gradle/javaProject.gradle"
apply from: "$rootProject.projectDir/gradle/jacocoProject.gradle"
apply from: "$rootProject.projectDir/gradle/javaTestProject.gradle"
apply from: "$rootProject.projectDir/gradle/helpProject.gradle"
```

Существенная часть здесь -- `distributableGhidraModule.gradle`. Этот общий скрипт
добавляет в задачу `assembleDistribution` содержимое `data`, JAR модуля,
лицензию, скрипты и файлы из `src/global`.
Подробности видны в
[`gradle/distributableGhidraModule.gradle`](../gradle/distributableGhidraModule.gradle#L27-L49)
и [`gradle/distributableGhidraModule.gradle`](../gradle/distributableGhidraModule.gradle#L81-L111).

Упрощенная схема:

```text
Public_Release/
  build.gradle
       |
       v
assembleDistribution
       |
       +--> data/*
       +--> src/global/docs/*  -> docs/*
       +--> src/main/resources/* (обычные ресурсы модуля)
       +--> UserAgreement.html -> docs/UserAgreement.html
```

Корневая задача `assembleDistribution` копирует платформонезависимые файлы в
staging-каталог будущего дистрибутива. Это видно в
[`gradle/root/distribution.gradle`](../gradle/root/distribution.gradle#L343-L494).
Файл `certification.manifest` при этом исключается из готовой поставки
([там же](../gradle/root/distribution.gradle#L359-L369)).

Пример команды, которую используют другие Gradle-файлы проекта для локальной
поставки:

```text
gradle buildLocalPublic_Release
```

Название задачи формируется механизмом сборки; конкретные подзадачи зависят от
версии проекта и включенных платформ. Важно не само имя команды, а то, что
`Public_Release` добавляет свои ресурсы в общий distribution, а не создает
самостоятельную программу.

## Содержимое папки

### `build.gradle`

[`build.gradle`](../Ghidra/Configurations/Public_Release/build.gradle) объявляет
модуль поставки. Строка `eclipse.project.name = 'Z Public Release'` задает ему имя
в Eclipse и намеренно помещает его в конец списка проектов.

Особое правило находится в строках 24--28:

```groovy
rootProject.assembleDistribution {
    from ("${this.projectDir}/src/main/resources/UserAgreement.html") {
        into "docs"
    }
}
```

То есть `UserAgreement.html` не остается только Java-ресурсом внутри JAR: при
сборке он явно копируется в каталог `docs` готовой Ghidra.

### `Module.manifest`

[`Module.manifest`](../Ghidra/Configurations/Public_Release/Module.manifest)
сейчас пуст. Это нормально: модуль не содержит Java-классов и не требует
дополнительных метаданных модуля, но общий поставочный скрипт ожидает такой файл
у distributable-модулей и пытается скопировать его в соответствующий каталог
([`distributableGhidraModule.gradle`](../gradle/distributableGhidraModule.gradle#L29-L35)).

Пустой файл не означает, что папка не работает. Ее поведение задается Gradle и
ресурсами, а не содержимым `Module.manifest`.

### `README.md`

[`README.md`](../Ghidra/Configurations/Public_Release/README.md) содержит только
заголовок `Public_Release`. Общий скрипт distributable-модуля подключает README к
задаче преобразования Markdown в HTML
([`distributableGhidraModule.gradle`](../gradle/distributableGhidraModule.gradle#L214-L221)).
В текущей версии это фактически минимальная служебная страница, а не руководство
пользователя.

### `data/PDB_SYMBOL_SERVER_URLS.pdburl`

[`PDB_SYMBOL_SERVER_URLS.pdburl`](../Ghidra/Configurations/Public_Release/data/PDB_SYMBOL_SERVER_URLS.pdburl)
содержит три заранее известных источника Microsoft PDB-символов:

```text
Internet|https://msdl.microsoft.com/download/symbols/|WARNING: ...
Internet|https://chromium-browser-symsrv.commondatastorage.googleapis.com|WARNING: ...
Internet|https://symbols.mozilla.org/|WARNING: ...
```

Формат строки:

```text
категория|URL|предупреждение
```

Ghidra ищет все файлы с расширением `.pdburl`, разбирает поля по символу `|` и
создает объекты известных серверов в
[`WellKnownSymbolServerLocation.java`](../Ghidra/Features/PDB/src/main/java/pdb/symbolserver/ui/WellKnownSymbolServerLocation.java#L42-L69).
В окне настройки PDB эти URL появляются как готовые пункты меню
([`ConfigPdbDialog.java`](../Ghidra/Features/PDB/src/main/java/pdb/symbolserver/ui/ConfigPdbDialog.java#L535-L562)).

Пример для новичка: если в Windows-бинарнике нет локального PDB, пользователь
может открыть настройку загрузки PDB, выбрать сервер Microsoft и попробовать
найти отладочные имена, типы и исходные строки. Это не гарантирует наличие
символов для любого файла.

Предупреждение в третьем поле не декоративно. Для недоверенных серверов оно
показывается пользователю в диалоге загрузки PDB через
[`WellKnownSymbolServerLocation.java`](../Ghidra/Features/PDB/src/main/java/pdb/symbolserver/ui/WellKnownSymbolServerLocation.java#L72-L95).
Документация PDB также прямо указывает, что список берется из этого файла:
[`LoadPDB.html`](../Ghidra/Features/PDB/src/main/help/help/topics/Pdb/LoadPDB.html#L137-L140).

### `data/DWARF.debuginfod_urls`

[`DWARF.debuginfod_urls`](../Ghidra/Configurations/Public_Release/data/DWARF.debuginfod_urls)
содержит URL серверов debuginfod для внешних DWARF-файлов. DWARF -- это формат
отладочной информации, обычно встречающийся у ELF/Linux-программ. Если бинарник
отделен от своих debug-файлов, Ghidra может искать их по build ID на таких
серверах.

Строки имеют тот же общий вид:

```text
Internet|https://debuginfod.elfutils.org/|WARNING: ...
```

Диалог настройки внешних DWARF-файлов загружает все ресурсы с расширением
`.debuginfod_urls`:
[`ExternalDebugFilesConfigDialog.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/dwarf/external/gui/ExternalDebugFilesConfigDialog.java#L53-L60).
Разбор строк и удаление дубликатов выполняет
[`WellKnownDebugProvider.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/dwarf/external/gui/WellKnownDebugProvider.java#L37-L69).

Пример сценария:

1. Аналитик импортирует ELF без встроенных debug-секций.
2. В программе есть идентификатор, по которому можно найти внешний debug-файл.
3. В настройках внешних DWARF-файлов выбирается debuginfod-сервер.
4. После загрузки Ghidra получает больше имен функций, типов и исходных привязок.

Как и в случае PDB, наличие URL не означает автоматическую загрузку при каждом
импорте. Серверы добавляются как известные варианты в настройках, а доступ и
поиск зависят от конфигурации пользователя и сети.

### `src/main/resources/UserAgreement.html`

[`UserAgreement.html`](../Ghidra/Configurations/Public_Release/src/main/resources/UserAgreement.html)
содержит Apache 2.0 и отдельное соглашение об ответственном использовании Ghidra
как SRE-инструмента. В тексте отдельно сказано, что пользователь сам отвечает за
законность reverse engineering и распространения.

Файл читает
[`UserAgreementDialog.java`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/main/UserAgreementDialog.java#L39-L53)
через `ResourceManager` в строках 87--103
([`UserAgreementDialog.java`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/main/UserAgreementDialog.java#L87-L103)).
При отмене диалога приложение может завершиться, если диалог запущен с
`exitOnCancel=true` ([там же](../Ghidra/Framework/Project/src/main/java/ghidra/framework/main/UserAgreementDialog.java#L111-L117)).

Иными словами, это runtime-ресурс, который становится доступен приложению из
classpath, и одновременно отдельный HTML-файл в каталоге `docs` поставки благодаря
правилу в `build.gradle`.

### `src/main/resources/splash.txt`

[`splash.txt`](../Ghidra/Configurations/Public_Release/src/main/resources/splash.txt)
содержит небольшой HTML-фрагмент с Apache 2.0 и указанием на `LICENSE.txt` для
сторонних компонентов.

Класс информационной панели ищет ресурс с именем `splash.txt` и выводит его как
HTML в окне информации о Ghidra:
[`InfoPanel.java`](../Ghidra/Features/Base/src/main/java/ghidra/framework/main/InfoPanel.java#L37-L49)
и обработка HTML в
[`InfoPanel.java`](../Ghidra/Features/Base/src/main/java/ghidra/framework/main/InfoPanel.java#L96-L124).
Это объясняет, почему файл находится именно в `src/main/resources`, а не в
`src/global/docs`: он нужен приложению во время работы, а не только как документ
поставки.

### `src/main/resources/defaultTools/CodeBrowser.tool`

[`CodeBrowser.tool`](../Ghidra/Configurations/Public_Release/src/main/resources/defaultTools/CodeBrowser.tool)
-- XML-шаблон первоначальной конфигурации CodeBrowser. Он описывает размещение и
активность окон Listing, Decompiler, Program Tree, Symbol Tree, Data Type Manager,
Console и других компонентов.

Например, в файле явно включены Listing и Decompiler:

```xml
<COMPONENT_INFO NAME="Listing" OWNER="CodeBrowserPlugin" ACTIVE="true" />
<COMPONENT_INFO NAME="Decompiler" OWNER="DecompilePlugin" ACTIVE="true" />
```

Ghidra находит `.tool` в classpath под каталогом `defaultTools` через
[`ToolUtils.java`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/ToolUtils.java#L61-L89).
Затем эти шаблоны используются при создании/импорте стандартных инструментов;
например, `ToolActionManager` отдельно добавляет default tools
([`ToolActionManager.java`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/main/ToolActionManager.java#L200-L204)).

Практический смысл: после первого запуска новичок получает не пустое окно, а
рабочее расположение CodeBrowser с Listing слева/в центре и Decompiler рядом.
Это только стартовый шаблон. Пользовательские изменения сохраняются отдельно и
не должны редактировать этот файл в исходном репозитории.

### `src/global/docs/WhatsNew.md`

[`WhatsNew.md`](../Ghidra/Configurations/Public_Release/src/global/docs/WhatsNew.md)
-- заметки о текущем выпуске, его назначении, требованиях и важных ограничениях.
Например, в текущем файле указаны требования JDK, особенности Jython, native
компонентов и совместимости проектов.

Каталог `src/global` имеет специальную семантику: общий Gradle-скрипт удаляет
префикс `src/global` и копирует содержимое в корень дистрибутива. Поэтому этот
файл становится `docs/WhatsNew.md` (а Markdown дополнительно участвует в задаче
преобразования Markdown в HTML) -- это прямо реализовано в
[`distributableGhidraModule.gradle`](../gradle/distributableGhidraModule.gradle#L81-L111).

### `src/global/docs/ChangeHistory.md`

[`ChangeHistory.md`](../Ghidra/Configurations/Public_Release/src/global/docs/ChangeHistory.md)
содержит подробную историю исправлений и улучшений по версиям. Это не код
анализатора: файл нужен пользователю готовой поставки, чтобы понимать изменения,
совместимость и исправленные ошибки.

Как и `WhatsNew.md`, после сборки он размещается в `docs/ChangeHistory.md`, а
помощник справки умеет находить Markdown-файлы глобальной документации и строить
для них HTML-версию ([`HelpBuildUtils.java`](../Ghidra/Framework/Help/src/main/java/help/HelpBuildUtils.java#L205-L216)).

### `certification.manifest`

[`certification.manifest`](../Ghidra/Configurations/Public_Release/certification.manifest)
перечисляет файлы модуля и связывает их с маркировкой `GHIDRA`:

```text
data/PDB_SYMBOL_SERVER_URLS.pdburl||GHIDRA||||END|
```

Это не пользовательская настройка и не Java-описание модуля. Внутренний скрипт
[`gradle/support/ip.gradle`](../gradle/support/ip.gradle#L125-L166) читает manifest,
проверяет, что файлы получили допустимую IP-маркировку, и отвергает файл без
маркировки или с неразрешенной маркировкой. Для не-исходников запись берется из
manifest в [`ip.gradle`](../gradle/support/ip.gradle#L279-L294).

При создании дистрибутива этот служебный файл исключается и конечному пользователю
не нужен ([`distribution.gradle`](../gradle/root/distribution.gradle#L359-L369)).

## Почему здесь нет исходников анализаторов

Название `Configurations` может ввести в заблуждение. Эта папка не конфигурирует
поведение каждого анализатора и не содержит «настроек декомпиляции». Она собирает
ресурсы, которые нужны именно публичной упаковке Ghidra.

Например:

- алгоритмы декомпиляции находятся в [`Ghidra/Features/Decompiler`](../Ghidra/Features/Decompiler);
- базовая модель программы находится в [`Ghidra/Framework/SoftwareModeling`](../Ghidra/Framework/SoftwareModeling);
- процессорные описания находятся в [`Ghidra/Processors`](../Ghidra/Processors);
- PDB-логика находится в [`Ghidra/Features/PDB`](../Ghidra/Features/PDB);
- DWARF-логика находится в базовых форматах и компонентах `Features/Base`.

`Public_Release` связывает эти возможности с поставкой через ресурсы: например,
PDB-модуль читает `.pdburl`, а базовый DWARF UI читает `.debuginfod_urls`.

## Что можно менять, а что не следует менять

Для обычного пользователя исходного дерева:

- не нужно менять `Public_Release`, чтобы анализировать файл;
- не нужно редактировать `CodeBrowser.tool` для настройки своего окна;
- не следует удалять предупреждения из URL-файлов: они информируют о сетевой загрузке;
- `certification.manifest` не является местом для пользовательских настроек.

Для разработчика, который собирает собственный дистрибутив:

- новый `.pdburl` добавляют, если нужно предложить известный PDB-сервер;
- новый `.debuginfod_urls` добавляют, если нужно предложить известный DWARF-сервер;
- файл в `src/global/docs` добавляют, если он должен попасть в корень документации поставки;
- ресурс в `src/main/resources` добавляют, если его должен читать код приложения через classpath;
- для нового не-исходникового файла нужно учитывать проверку `certification.manifest`.

## Итоговая модель для новичка

Представляйте Ghidra как большой набор модулей и отдельный конвейер упаковки:

```text
модули Ghidra (анализаторы, процессоры, UI, декомпилятор)
                         +
Public_Release (ресурсы публичной поставки)
                         |
                         v
                 готовая папка Ghidra
```

`Public_Release` отвечает не за то, **как** Ghidra анализирует машинный код, а за
то, **с каким стартовым интерфейсом, юридическими текстами, changelog и известными
источниками отладочной информации** этот код будет выдан пользователю.
