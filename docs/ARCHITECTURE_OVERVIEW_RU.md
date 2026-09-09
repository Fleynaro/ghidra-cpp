# Обзор архитектуры Ghidra для новичка

Этот файл описывает исходный код репозитория простыми словами. Основной акцент
сделан на x86/x86-64 и формате Windows PE.

Для углубленного изучения конвейера декомпиляции используйте
[`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md). Этот документ
ссылается обратно на текущий обзор и продолжает его маршрут от архитектуры к
исходникам декомпилятора.

Для подробного изучения базового слоя Ghidra используйте
[`FRAMEWORK_OVERVIEW_RU.md`](FRAMEWORK_OVERVIEW_RU.md): там разобраны модули
`Ghidra/Framework`, модель `Program`, database, транзакции, память, listing,
типы, символы, p-code и эмуляция.

Для отдельного подробного разбора внутреннего устройства database, формата
буферов, checkpoint-транзакций, rollback, undo/redo и recovery используйте
[`GHIDRA_DATABASE_RU.md`](GHIDRA_DATABASE_RU.md).

Для подробного изучения обязательного прикладного модуля `Ghidra/Features/Base`
используйте [`FEATURES_BASE_OVERVIEW_RU.md`](FEATURES_BASE_OVERVIEW_RU.md): там
показаны загрузчики, auto-analysis pipeline, дизассемблирование, создание
функций и различие между CFG `Program` и CFG декомпилятора.

Для навигации по встроенным примерам и рабочим автоматизациям Base используйте
[`GHIDRA_BASE_SCRIPTS_OVERVIEW_RU.md`](GHIDRA_BASE_SCRIPTS_OVERVIEW_RU.md): там
есть таблица каждого файла `Ghidra/Features/Base/ghidra_scripts` и подробный
разбор наиболее полезных скриптов.

Для понимания запуска установленной Ghidra, headless-анализа, PyGhidra и
Ghidra Server используйте
[`RUNTIMESCRIPTS_OVERVIEW_RU.md`](RUNTIMESCRIPTS_OVERVIEW_RU.md). Документ
связывает оболочки `Ghidra/RuntimeScripts` с реальными Java-точками входа и
показывает практические команды.

Для понимания конфигурации публичной поставки используйте
[`PUBLIC_RELEASE_CONFIGURATION_RU.md`](PUBLIC_RELEASE_CONFIGURATION_RU.md): там
разобраны ресурсы `Ghidra/Configurations/Public_Release`, их упаковка Gradle и
использование PDB/DWARF URL, стартовой раскладки и юридических текстов.

Для понимания отдельного лицензионного и интеграционного контура используйте
[`GPL_OVERVIEW_RU.md`](GPL_OVERVIEW_RU.md): там разобраны `GPL/DemanglerGnu`,
`GPL/GnuDisassembler`, `GPL/DMG`, native-сборка, лицензии и связи с
`Ghidra/*`.

Для понимания границы между базовым декомпилятором и функциями поверх его API
используйте [`DECOMPILER_DEPENDENT_RU.md`](DECOMPILER_DEPENDENT_RU.md): там
разобраны `Ghidra/Features/DecompilerDependent`, поиск по декомпилированному
тексту, экспорт p-code, taint-анализ, уточнение variadic-сигнатур и поиск
использований типов.

Для понимания контейнеров, виртуальных файловых систем, APK, Android boot image,
архивов и связи вложенных файлов с обычными загрузчиками используйте
[`FILE_FORMATS_OVERVIEW_RU.md`](FILE_FORMATS_OVERVIEW_RU.md).

## 1. Что такое Ghidra

Ghidra получает бинарный файл и строит из него модель программы: память,
инструкции, функции, данные, символы и связи между ними. После этого анализаторы
уточняют модель, а декомпилятор показывает приблизительный C-код.

Упрощенный конвейер выглядит так:

```text
файл (PE/ELF/...) 
  -> загрузчик и разбор формата
  -> Program: память, адреса, данные и символы
  -> язык процессора (x86 через Sleigh)
  -> дизассемблирование и p-code
  -> анализ функций, типов и ссылок
  -> декомпиляция в C-подобный код
```

Главное правило навигации по коду: формат файла отвечает на вопрос «где что
лежит», а процессор отвечает на вопрос «что означают байты инструкций».

## 2. Карта верхнего уровня

Корневой `settings.gradle` подключает основные группы проектов:
[`settings.gradle`](../settings.gradle#L18-L31).

| Папка | За что отвечает |
|---|---|
| [`Ghidra/Framework`](../Ghidra/Framework) | Базовые сервисы и модель программы, которыми пользуются остальные модули. |
| [`Ghidra/Features`](../Ghidra/Features) | Пользовательские возможности: базовый анализ, загрузчики форматов, декомпилятор, UI-плагины и т. д. |
| [`Ghidra/Processors`](../Ghidra/Processors) | Поддержка конкретных CPU: инструкции, регистры, адресные пространства, calling convention и правила анализа. |
| [`Ghidra/Debug`](../Ghidra/Debug) | Отладчик, трассы, эмуляция и интеграция с живым процессом. |
| [`Ghidra/Extensions`](../Ghidra/Extensions) | Дополнительные, необязательные расширения. |
| [`Ghidra/Configurations`](../Ghidra/Configurations) | Конфигурации сборки и состав поставляемых компонентов. |
| [`Ghidra/RuntimeScripts`](../Ghidra/RuntimeScripts) | Скрипты запуска и поддержки установленной Ghidra. |
| [`Ghidra/Test`](../Ghidra/Test) | Общая инфраструктура тестирования. |
| [`GPL`](../GPL) | Компоненты с отдельными GPL-лицензиями, включая некоторые нативные части. |
| [`GhidraBuild`](../GhidraBuild) | Инструменты сборки, шаблоны и интеграции с IDE. |

Внутри большинства модулей структура похожа на обычный Gradle-проект:

```text
src/main/java       исходный Java-код
src/main/resources  XML, описания, картинки и прочие ресурсы
src/test             тесты
data                описания и данные для runtime/анализаторов
build.gradle        зависимости и правила сборки модуля
Module.manifest     метаданные модуля
```

## 3. Базовая модель программы

После импорта Ghidra не работает напрямую с исходным файлом на каждом шаге.
Она создает объект `Program`, в котором хранятся адреса, память, listing,
функции, типы и символы. Этот слой находится преимущественно в
[`Framework/SoftwareModeling`](../Ghidra/Framework/SoftwareModeling).

Там же находятся ключевые понятия Sleigh и p-code. Например, пакет
[`ghidra.sleigh`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/sleigh)
разбирает язык описания процессоров, а пакет
[`ghidra.pcodeCPort`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/pcodeCPort)
содержит низкоуровневую модель p-code.

`p-code` -- это промежуточное представление инструкций. Одна x86-инструкция
может быть сложной, поэтому Ghidra сначала переводит ее в более простой набор
операций чтения регистров, арифметики, памяти и переходов. На этом общем
представлении работают анализ и декомпилятор.

## 4. Как поддерживается x86

Вся поддержка x86 находится в
[`Ghidra/Processors/x86`](../Ghidra/Processors/x86).

### 4.1. Описание инструкций

Главный файл --
[`x86.slaspec`](../Ghidra/Processors/x86/data/languages/x86.slaspec).
Он описывает регистры, операнды, декодирование байтов и семантику инструкций
в Sleigh. Для 64-битного режима используется
[`x86-64.slaspec`](../Ghidra/Processors/x86/data/languages/x86-64.slaspec),
который подключает базовый x86 и дополнительные расширения вроде SGX и FMA.

Файлы `*.sinc` рядом -- подключаемые куски описаний отдельных расширений:
AVX, AVX2, AVX-512, BMI, FMA, SHA и другие.

Подробный разбор структуры `Processors/x86` и пошаговый пример преобразования
`mov [rcx + 0x10], edx` в P-Code см. в
[`X86_SLEIGH_PCODE_RU.md`](../X86_SLEIGH_PCODE_RU.md).

### 4.2. Выбор языка

Файлы `*.ldefs`, `*.pspec`, `*.opinion` и `*.dwarf` связывают имя языка,
режим процессора, регистры и поддержку отладочной информации. Начать изучение
можно с каталога
[`Processors/x86/data/languages`](../Ghidra/Processors/x86/data/languages).

Обычно важны такие варианты:

- `x86` / `x86-16` -- 16- и 32-битные режимы;
- `x86-64` -- общий 64-битный режим;
- `x86-64-win` -- x86-64 с соглашениями Windows;
- `x86-64-gcc` -- x86-64 с соглашениями GCC.

### 4.3. Calling convention и типы

Файл [`x86-64-win.cspec`](../Ghidra/Processors/x86/data/languages/x86-64-win.cspec)
описывает ABI Windows x64: размер указателя 8 байт, stack pointer `RSP`,
передачу аргументов через `RCX`, `RDX`, `R8`, `R9` и возврат через `RAX`.
Именно эти правила помогают декомпилятору понять, где аргумент функции, где
локальная переменная и где результат.

Анализатор, специфичный для x86, находится в
[`X86Analyzer.java`](../Ghidra/Processors/x86/src/main/java/ghidra/app/plugin/core/analysis/X86Analyzer.java).
Реализация специальных p-code user-op находится в
[`X86PcodeUseropLibraryFactory.java`](../Ghidra/Processors/x86/src/main/java/ghidra/program/emulation/X86PcodeUseropLibraryFactory.java).

Файлы `data/patterns/*x86*` содержат шаблоны распознавания прологов и других
характерных последовательностей инструкций, которые помогают находить функции.

## 5. Как разбирается PE

Основная реализация PE находится в пакете
[`ghidra.app.util.bin.format.pe`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe).

### 5.1. Проверка, что файл является PE

Точка входа анализа:
[`PortableExecutableAnalyzer.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/analyzers/PortableExecutableAnalyzer.java).
Он делегирует работу команде
[`PortableExecutableBinaryAnalysisCommand.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/formats/PortableExecutableBinaryAnalysisCommand.java).

Метод `canApply()` в этой команде делает минимальную проверку:

1. читает DOS-заголовок;
2. проверяет сигнатуру `MZ`;
3. переходит по `e_lfanew`;
4. проверяет сигнатуру `PE\0\0`.

Это видно в [`PortableExecutableBinaryAnalysisCommand.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/formats/PortableExecutableBinaryAnalysisCommand.java#L48-L70).

### 5.2. Объект-разборщик PE

[`PortableExecutable.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/PortableExecutable.java)
собирает основные части образа:

- `DOSHeader` -- старый DOS-заголовок и DOS stub;
- `RichHeader` -- необязательная информация, оставленная инструментами Microsoft;
- `NTHeader` -- заголовок PE/NT;
- `SectionLayout.FILE` или `MEMORY` -- лежат ли секции как в файле или уже в памяти.

Конструктор читает DOS-заголовок, берет `e_lfanew` и создает `NTHeader`.
Если включен расширенный разбор, через optional header обрабатываются data
directories: импорты, экспорты, ресурсы, relocations, TLS, debug и другие.

### 5.3. NT header и RVA

[`NTHeader.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/NTHeader.java)
содержит:

- сигнатуру `PE\0\0`;
- `FileHeader` (`IMAGE_FILE_HEADER`);
- `OptionalHeader` (`IMAGE_OPTIONAL_HEADER32/64`);
- список секций;
- перевод RVA в смещение файла.

Метод `rvaToPointer()` особенно важен: RVA -- это адрес относительно image base,
а при чтении файла нужно найти секцию и посчитать соответствующее файловое
смещение через `PointerToRawData` и `VirtualAddress`.

### 5.4. Optional header и data directories

[`OptionalHeader.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/OptionalHeader.java)
описывает различия PE32 и PE32+, entry point, image base, выравнивание,
характеристики DLL и массив из 16 data directories.

Конкретные структуры директорий находятся в том же пакете:

- `ImportDataDirectory`, `ImportDescriptor`, `ImportInfo` -- импорты DLL и функций;
- `ExportDataDirectory`, `ExportInfo` -- экспортируемые функции;
- `ResourceDataDirectory` и каталог `resource/` -- иконки, строки, версии и другие ресурсы;
- `ExceptionDataDirectory`, `PEx64UnwindInfo` -- unwind/exception-информация x64;
- `Base Relocation`-классы -- перемещения при ASLR;
- `TLSDataDirectory` и `TLSDirectory` -- thread-local storage;
- `DebugDataDirectory` и каталог `debug/` -- отладочные записи и символы;
- `SecurityDataDirectory` -- сертификат и подпись;
- `LoadConfigDataDirectory` -- CFG, security cookie и другие параметры загрузки.

## 6. Как PE превращается в аннотированную программу

В `analysisWorkerCallback()` команда создает `ByteProvider`, затем
`PortableExecutable` с `SectionLayout.FILE`. После проверки заголовков вызывается
`createDataTypes()`:

```text
DOS header
  -> NT header
  -> section headers
  -> data directories
  -> COFF symbols
  -> Data/Fragments/Comments внутри Program
```

Исходный код этого процесса находится в
[`PortableExecutableBinaryAnalysisCommand.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/formats/PortableExecutableBinaryAnalysisCommand.java#L72-L136).
Для каждой data directory вызывается `markup()`, после чего создаются структуры
данных и фрагменты в памяти Ghidra. Поэтому PE-классы не просто читают байты:
они умеют представить найденные поля в UI и модели `Program`.

## 7. Где искать декомпилятор

Пользовательская часть декомпилятора находится в
[`Features/Decompiler/src/main/java`](../Ghidra/Features/Decompiler/src/main/java).
Основные точки входа для UI -- `DecompilePlugin` и `DecompilerProvider`.
Нативная часть подключается как отдельный проект `decompile`, что явно видно в
[`settings.gradle`](../settings.gradle#L42-L43).

Связка выглядит так:

```text
x86 bytes
  -> Sleigh decoder
  -> p-code
  -> анализ Program и функций
  -> decompiler
  -> C-подобный текст
```

Декомпилятор не восстанавливает исходный C-код дословно. Он строит наиболее
правдоподобное представление на основе машинных инструкций, p-code, типов,
calling convention и найденных связей.

## 8. Практический маршрут изучения

Если нужно разобраться с x86 PE-файлом, читайте исходники в таком порядке:

1. [`README.md`](../README.md) -- назначение и сборка проекта.
2. [`settings.gradle`](../settings.gradle) -- какие модули входят в сборку.
3. [`PortableExecutableAnalyzer.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/analyzers/PortableExecutableAnalyzer.java) -- как запускается PE-анализ.
4. [`PortableExecutableBinaryAnalysisCommand.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/formats/PortableExecutableBinaryAnalysisCommand.java) -- проверка PE и markup в `Program`.
5. [`PortableExecutable.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/PortableExecutable.java) -- общий объект PE.
6. [`NTHeader.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/NTHeader.java) и [`OptionalHeader.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/OptionalHeader.java) -- заголовки, секции и RVA.
7. [`x86.slaspec`](../Ghidra/Processors/x86/data/languages/x86.slaspec) -- инструкции и семантика x86.
8. [`x86-64.slaspec`](../Ghidra/Processors/x86/data/languages/x86-64.slaspec) -- 64-битное расширение.
9. [`x86-64-win.cspec`](../Ghidra/Processors/x86/data/languages/x86-64-win.cspec) -- Windows x64 ABI.
10. [`Features/Decompiler`](../Ghidra/Features/Decompiler) -- путь от p-code к C-подобному результату.
11. [`FEATURES_BASE_OVERVIEW_RU.md`](../FEATURES_BASE_OVERVIEW_RU.md) -- прикладной pipeline Base: Loader, автоанализ, функции и CFG.

## 9. Коротко: «какая папка за что»

- Нужен разбор PE-заголовков или импортов: `Ghidra/Features/Base/.../format/pe`.
- Нужен запуск и аннотирование PE в проекте: `Ghidra/Features/Base/.../analyzers` и `.../cmd/formats`.
- Нужны x86-инструкции и регистры: `Ghidra/Processors/x86/data/languages`.
- Нужен x86-специфичный анализ: `Ghidra/Processors/x86/src/main/java`.
- Нужно понять p-code и модель адресов: `Ghidra/Framework/SoftwareModeling`.
- Нужно понять UI декомпилятора: `Ghidra/Features/Decompiler/src/main/java`.
- Нужна отладка живого x86-процесса: `Ghidra/Debug`.
- Нужен запуск GUI, headless или Ghidra Server: [`RUNTIMESCRIPTS_OVERVIEW_RU.md`](../RUNTIMESCRIPTS_OVERVIEW_RU.md).
- Нужно изменить сборку или зависимости: `build.gradle`, `settings.gradle` и `gradle/`.
