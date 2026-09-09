# `Ghidra/Features/Base`: что это и как проходит анализ

Этот документ объясняет исходники [`Ghidra/Features/Base`](../Ghidra/Features/Base)
простыми словами. Он рассчитан на новичка, который видит в Ghidra слово
«анализ» и хочет понять, что именно происходит после импорта бинарного файла.

Для общей карты репозитория сначала используйте
[`ARCHITECTURE_OVERVIEW_RU.md`](ARCHITECTURE_OVERVIEW_RU.md). Для нижнего слоя
`Program`, памяти, database и транзакций полезен
[`FRAMEWORK_OVERVIEW_RU.md`](FRAMEWORK_OVERVIEW_RU.md), а для пути от готовой
функции к C-подобному тексту --
[`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md).

## Короткий ответ

`Features/Base` -- это не только «базовые анализаторы» и не один построитель CFG.
Это большой обязательный Java-модуль, который связывает базовую модель `Program`
с пользовательскими возможностями Ghidra:

```text
загрузчик -> Program/Memory -> дизассемблер -> Instruction/References
          -> AutoAnalysisManager -> функции, данные, типы, имена
          -> CodeBrowser, графы, поиск, скрипты, экспорт
```

В частности, здесь находятся:

| Область | Что искать |
|---|---|
| Автоанализ | [`app/plugin/core/analysis`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis) |
| Анализ формата | [`app/analyzers`](../Ghidra/Features/Base/src/main/java/ghidra/app/analyzers) и [`app/cmd/formats`](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/formats) |
| Загрузчики | [`app/util/opinion`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion) |
| Разбор бинарных структур | [`app/util/bin/format`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format) |
| Дизассемблирование | [`app/cmd/disassemble`](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/disassemble) |
| Создание и границы функций | [`app/cmd/function`](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function) |
| Модель блоков и графовый UI | [`Framework/SoftwareModeling/.../block`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block) и [`app/plugin/core/graph`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/graph) |
| Основной CodeBrowser/UI | [`app/plugin/core`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core) |
| Файловые системы и импорт | [`formats/gfilesystem`](../Ghidra/Features/Base/src/main/java/ghidra/formats/gfilesystem) и [`plugins/importer`](../Ghidra/Features/Base/src/main/java/ghidra/plugins/importer) |
| Парсеры C/C++ и типы | [`app/util/cparser`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/cparser) и [`data/typeinfo`](../Ghidra/Features/Base/data/typeinfo) |
| Тесты | [`src/test`](../Ghidra/Features/Base/src/test) |

Название `Base` означает базовый пользовательский feature, а не то, что весь
код является фундаментом Framework. Например, `Program`, `Memory` и основной
`BasicBlockModel` принадлежат `Framework/SoftwareModeling`; `Base` использует их
и добавляет команды, анализаторы и интерфейс.

## 1. Из чего состоит модуль

### `src/main/java/ghidra/app`

Это основной прикладной слой. Важные части:

- `analyzers` -- адаптеры анализа бинарных форматов. Например,
  [`PortableExecutableAnalyzer.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/analyzers/PortableExecutableAnalyzer.java)
  запускает команду PE-разметки.
- `cmd` -- операции над `Program`: дизассемблировать, создать функцию,
  создать данные, добавить ссылку, разметить формат.
- `plugin/core/analysis` -- планировщик автоанализа и встроенные анализаторы.
- `plugin/core/codebrowser`, `function`, `graph`, `blockmodel`, `references` --
  окна и сервисы, показывающие результат анализа.
- `util/opinion` -- выбор подходящего загрузчика и создание `Program` из файла.
  Например, [`PeLoader.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/PeLoader.java)
  относится к загрузке PE, а не к последующей эвристике автоанализа.
- `util/bin/format` -- структуры форматов: PE, ELF, Mach-O, DWARF, PDB,
  Go runtime и другие. Они читают байты и предоставляют Java-представление
  заголовков и таблиц.

### Остальные верхнеуровневые пакеты

В корне `src/main/java/ghidra` есть также несколько самостоятельных подсистем:

- [`ghidra/program`](../Ghidra/Features/Base/src/main/java/ghidra/program) --
  database-реализации `Program`, Flat API и вспомогательная логика анализа;
- [`ghidra/formats/gfilesystem`](../Ghidra/Features/Base/src/main/java/ghidra/formats/gfilesystem) --
  виртуальные файловые системы для контейнеров и вложенных файлов;
- [`ghidra/framework`](../Ghidra/Features/Base/src/main/java/ghidra/framework) --
  прикладные команды, импорт и интеграция с PluginTool;
- [`ghidra/features/base`](../Ghidra/Features/Base/src/main/java/ghidra/features/base) --
  отдельные функции Base: поиск памяти, сравнение кода, replace и quick-fix;
- [`ghidra/util`](../Ghidra/Features/Base/src/main/java/ghidra/util) --
  поиск байтов, XML, строки, таблицы и прочие общие утилиты.

`src/main/resources` содержит help, изображения, DTD и ресурсы UI. В `data`
хранятся, среди прочего, номера системных вызовов и type archives. Это не
«входной код анализатора», но анализаторы и UI используют эти данные. Сборка
также генерирует Java-код из JavaCC-грамматик `C.jj` и `CPP.jj`; это явно задано
задачами [`build.gradle`](../Ghidra/Features/Base/build.gradle#L78-L124).

## 2. Что такое Analyzer в Ghidra

Анализатор -- это объект, который реагирует на изменение конкретного вида
данных в `Program`. Контракт находится в
[`Analyzer.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/services/Analyzer.java):
у него есть `canAnalyze`, тип, приоритет, настройки и методы `added`/`removed`.
Большинство реализаций расширяет
[`AbstractAnalyzer.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/services/AbstractAnalyzer.java).

Типы событий определены в
[`AnalyzerType.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/services/AnalyzerType.java):

| Тип | Событие | Пример результата |
|---|---|---|
| `BYTE_ANALYZER` | добавлены блоки байтов/памяти | разбор PE, DWARF, relocation |
| `INSTRUCTION_ANALYZER` | созданы инструкции | ссылки, constant propagation, no-return |
| `FUNCTION_ANALYZER` | создана функция | анализ тела, подписи, calling convention |
| `FUNCTION_MODIFIERS_ANALYZER` | изменился thunk/inline/no-return и т. п. | исправление зависимых свойств |
| `FUNCTION_SIGNATURES_ANALYZER` | изменилась сигнатура | распространение типов |
| `DATA_ANALYZER` | создан объект данных | ссылки от данных и дополнительные типы |

Существенная деталь: анализатор обычно не запускается один раз линейно на весь
файл. Он получает `AddressSet` изменившихся адресов. Поэтому появление новой
инструкции может породить ссылку, ссылка -- функцию, а новая функция -- ещё
несколько событий для следующих анализаторов.

## 3. Как устроен Auto Analysis Pipeline

### Регистрация

При создании `AutoAnalysisManager` для `Program` он создает отдельные списки
задач по типам анализаторов и через `ClassSearcher` находит все классы,
реализующие `Analyzer`. Затем анализатор попадает в список по своему
`AnalyzerType`; см. [`AutoAnalysisManager.java#L149-L200`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisManager.java#L149-L200).

Класс анализатора должен заканчиваться на `Analyzer`, иначе `ClassSearcher` его
не найдет -- это специально отмечено в контракте `Analyzer.java`.

### Приоритеты

Грубый порядок задает
[`AnalysisPriority.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/services/AnalysisPriority.java):

```text
FORMAT -> BLOCK -> DISASSEMBLY -> CODE -> FUNCTION
        -> REFERENCE -> DATA -> FUNCTION ID -> DATA TYPE PROPAGATION
```

Это не жесткая последовательность стадий без возвратов. «Ранний» анализатор
может добавить данные, которые запланируют более поздний анализ, а поздний
анализ может снова вызвать изменение, которое попадет в очередь. Но смысл
приоритетов именно такой: сначала более надежная информация, потом более
спекулятивная. Например, `FORMAT_ANALYSIS` предназначен для первичной разметки,
`DISASSEMBLY` -- для хорошего потока кода, а `FUNCTION_ANALYSIS` -- для
восстановления базовых функций.

[`AnalysisTaskList.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AnalysisTaskList.java#L35-L74)
вставляет анализаторы в список по числовому приоритету. При равенстве порядок
делается детерминированным по имени.

### Планирование и запуск

Когда появляется адрес, `AnalysisScheduler` складывает его в `addSet` и ставит
задачу в очередь; см.
[`AnalysisScheduler.java#L65-L112`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AnalysisScheduler.java#L65-L112).
Во время запуска набор извлекается, и вызываются `analyzer.added(...)` и
`analyzer.removed(...)` -- [`AnalysisScheduler.java#L171-L193`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AnalysisScheduler.java#L171-L193).
Обертка фоновой команды находится в
[`AnalysisTask.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AnalysisTask.java).

В UI цепочка начинается с
[`AutoAnalysisPlugin.java#L197-L223`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisPlugin.java#L197-L223):
Ghidra загружает настройки, запускает background-команду анализа и просит
менеджер повторно проанализировать всю память или выбранный диапазон.
Анализаторы можно включать и выключать в Analysis Options. Также язык может
переопределить enablement через свойства `Analyzers.<имя>` или
`DisableAllAnalyzers`; это видно в
[`AnalysisScheduler.java#L142-L160`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AnalysisScheduler.java#L142-L160).

## 4. Сквозной пример: PE/x86 от файла до CFG

Ниже не «единственная возможная последовательность», а полезная модель того,
что происходит с обычным x86 PE-файлом.

### Шаг 1. Loader создает Program

Загрузчик выбирается среди реализаций `Loader` в
[`app/util/opinion`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion).
Он создает память, блоки и выбранный `Language`/`CompilerSpec`. В этот момент
Ghidra уже знает, какие диапазоны исполняемые, но это еще не означает, что все
байты стали инструкциями.

Это различие важно:

```text
Memory block с правом Execute != Instruction в Listing
```

### Шаг 2. Разметка PE-заголовков

`PortableExecutableAnalyzer` делегирует работу
[`PortableExecutableBinaryAnalysisCommand.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/formats/PortableExecutableBinaryAnalysisCommand.java).
Сначала `canApply()` проверяет `MZ`, `e_lfanew` и `PE\0\0`
([строки 48-70](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/formats/PortableExecutableBinaryAnalysisCommand.java#L48-L70)).
Затем `analysisWorkerCallback()` создает `PortableExecutable`, проверяет DOS/NT
headers и вызывает `createDataTypes()`
([строки 72-135](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/formats/PortableExecutableBinaryAnalysisCommand.java#L72-L135)).

Упрощенный результат:

```text
PE bytes
  -> DOSHeader / NTHeader / sections
  -> imports, exports, resources, relocations, debug
  -> Data, comments, fragments, symbols в Program
```

Классы чтения формата находятся в
[`app/util/bin/format/pe`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe).
Это разбор контейнера и разметка, а не CFG: заголовок PE не рассказывает о
каждом внутреннем переходе функции.

### Шаг 3. Начальное дизассемблирование

Точка входа, экспорт, импортный stub или другая надежная ссылка становятся
seed-адресом. Команда
[`DisassembleCommand.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/disassemble/DisassembleCommand.java)
создает `Disassembler`, декодирует инструкции согласно языку `Program` и может
следовать потокам (`followFlow`). После успешного результата она сообщает новые
адреса `AutoAnalysisManager.codeDefined(...)`
([строки 325-340](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/disassemble/DisassembleCommand.java#L325-L340)).

Для каждой инструкции в Listing появляются mnemonic, операнды, `FlowType` и
references. Смысл байтов x86 задается не `Base`, а Sleigh-языком из
[`Processors/x86`](../Ghidra/Processors/x86); объяснение пути x86 -> p-code есть
в [`X86_SLEIGH_PCODE_RU.md`](X86_SLEIGH_PCODE_RU.md).

### Шаг 4. Вычисление тела функции

При создании функции
[`CreateFunctionCmd.java#L275-L337`](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateFunctionCmd.java#L275-L337)
проверяет существующую функцию, вычисляет body и записывает функцию в Listing.
Если body не передан, `getFunctionBody()` запускает `FollowFlow` от entry point
и не идет через обычные `CALL`-переходы
([строки 590-627](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateFunctionCmd.java#L590-L627)).

Пример машинного потока:

```text
00401000: cmp eax, 0
00401003: je  00401010
00401005: add ecx, 1
00401008: jmp 00401015
00401010: sub ecx, 1
00401015: ret
```

По этому потоку тело функции может включить все пять инструкций, но функция,
которую вызывает `CALL`, не становится частью тела вызывающей функции.

### Шаг 5. Где появляется CFG

В модели `Program` базовые блоки можно получить через
[`BasicBlockModel.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/BasicBlockModel.java).
Блок -- это непрерывная последовательность инструкций: label начинает блок,
а инструкция с локальным переходом или terminal flow заканчивает его
([строки 23-40](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/BasicBlockModel.java#L23-L40)).
Для примера выше получится примерно:

```text
B0  00401000 cmp; je 00401010
 |\
 | `-> B2 00401010 sub; ret
 `--> B1 00401005 add; jmp 00401015
       `-> B3 00401015 ret
```

Программный доступ к ребрам дает интерфейс
[`CodeBlockModel.java#L82-L112`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/CodeBlockModel.java#L82-L112):
`getDestinations()` возвращает исходящие потоки, `getSources()` -- входящие.
`Base` предоставляет UI и сервисы для показа таких моделей, но сам алгоритм
`BasicBlockModel` находится в Framework.

### Шаг 6. Что делают следующие анализаторы

После новых `Instruction` и `Function` подключаются анализаторы из `Base` и
других модулей. Типичная логика выглядит так:

```text
Instruction
  -> OperandReferenceAnalyzer / DataOperandReferenceAnalyzer
  -> ConstantPropagationAnalyzer
  -> новые адреса, данные и функции
  -> NoReturnFunctionAnalyzer и анализ модификаторов
  -> Function ID, demangler, типы и сигнатуры
```

Например, [`ConstantPropagationAnalyzer.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ConstantPropagationAnalyzer.java)
распространяет значения по потоку и может помочь интерпретировать вычисленный
адрес. Это не «построение CFG с нуля»: CFG опирается на уже дизассемблированные
инструкции и их flow references, а propagation уточняет смысл операндов и
адресов.

## 5. Два разных CFG

Слово CFG используется для двух связанных, но не одинаковых вещей.

### CFG модели Program

Это `BasicBlockModel` и связанные `CodeBlock`/`CodeBlockReference`. Он строится
из Listing, flow types и references и нужен для Function Graph, навигации,
поиска путей и команд анализа. Он живет в Java-модели Ghidra и доступен API.

### CFG внутри декомпилятора

Когда пользователь открывает Decompiler, нативная часть строит собственную
модель `Funcdata`, блоков p-code и затем SSA. Там CFG может быть преобразован
правилами структуризации и оптимизации. Это уже путь
`Features/Decompiler`, описанный в
[`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md), а не просто
класс из `Features/Base`.

Практическое правило:

```text
нужны инструкции, функции, ссылки, блоки и Function Graph -> Program/Framework/Base
нужен SSA, varnode, MULTIEQUAL и C-подобный вывод -> Features/Decompiler
```

## 6. Что Base не делает

- Не является описанием всех CPU: семантика x86/ARM/MIPS находится в
  `Ghidra/Processors`.
- Не является database-движком: хранение `Program` и транзакции находятся в
  `Ghidra/Framework`.
- Не является всем декомпилятором: нативная оптимизация p-code и печать C
  находятся в `Ghidra/Features/Decompiler`.
- Не гарантирует правильность эвристик. Дизассемблер и анализаторы работают с
  неполными сведениями, а непрямой jump, tail call, shared return или packed
  code могут потребовать ручной правки.

## 7. Как читать исходники по задаче

| Вопрос | Маршрут |
|---|---|
| Как файл стал `Program`? | `app/util/opinion/Loader.java` -> конкретный `*Loader.java` |
| Как разобрали PE? | `PortableExecutableAnalyzer` -> `PortableExecutableBinaryAnalysisCommand` -> `app/util/bin/format/pe` |
| Почему запустился анализатор? | `Analyzer` -> `AutoAnalysisManager` -> `AnalysisTaskList` -> `AnalysisScheduler` |
| Кто создал Instruction? | `app/cmd/disassemble/DisassembleCommand` -> `Disassembler` из SoftwareModeling |
| Как нашли тело функции? | `CreateFunctionCmd.getFunctionBody` -> `FollowFlow` |
| Как получить блоки и ребра? | `Framework/SoftwareModeling/.../block/BasicBlockModel` и `CodeBlockModel` |
| Почему появились ссылки/данные? | конкретные классы в `plugin/core/analysis`, затем `Program` managers |
| Как блоки показаны в UI? | `plugin/core/blockmodel`, `plugin/core/graph`, `plugin/core/codebrowser` |
| Как из функции получился C-код? | [`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md) |

## 8. Минимальный пример API

Ниже показан не самостоятельный скрипт, а схема того, как тот же pipeline
выглядит из Java-кода Ghidra:

```java
Address entry = toAddr("00401000");

// Декодировать инструкции от точки входа, следуя переходам.
new DisassembleCommand(entry, null, true).applyTo(currentProgram);

// Создать функцию; ее тело будет вычислено по flow.
new CreateFunctionCmd(entry).applyTo(currentProgram);

// Прочитать CFG функции/программы.
BasicBlockModel blocks = new BasicBlockModel(currentProgram);
CodeBlock block = blocks.getCodeBlockAt(entry, TaskMonitor.DUMMY);
CodeBlockReferenceIterator edges = blocks.getDestinations(block, TaskMonitor.DUMMY);
```

Реальные методы Flat API для первых двух операций также видны в
[`FlatProgramAPI.java`](../Ghidra/Features/Base/src/main/java/ghidra/program/flatapi/FlatProgramAPI.java#L170-L190)
и [`FlatProgramAPI.java#L1040-L1060`](../Ghidra/Features/Base/src/main/java/ghidra/program/flatapi/FlatProgramAPI.java#L1040-L1060).
Для изменения `Program` в полноценном коде нужно учитывать транзакции; см.
[`FRAMEWORK_OVERVIEW_RU.md`](FRAMEWORK_OVERVIEW_RU.md#4-domainobject-и-транзакции).

## Итоговая mental model

Думайте о `Features/Base` как о диспетчере и наборе инструментов вокруг модели
программы:

```text
Base не «угадывает исходник» сразу.
Base помогает превратить импортированный файл
в разметку Program, инструкции, ссылки, функции, данные и UI.
Decompiler берет подготовленную функцию и строит другой, более глубокий CFG
на уровне p-code/SSA, после чего печатает C-подобный результат.
```

Для изучения конкретного бинарника полезнее идти именно по цепочке
`Loader -> Program -> DisassembleCommand -> CreateFunctionCmd -> BasicBlockModel`
и только потом переходить к отдельным эвристическим анализаторам. Так проще
отличить причину ошибки разметки файла, ошибки дизассемблирования, неверной
границы функции и проблемы уже на уровне декомпиляции.
