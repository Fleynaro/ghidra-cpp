# Что происходит с `GTA5.exe` в Ghidra

Этот текст -- практическая карта для новичка, который загрузил Windows PE-файл
`GTA5.exe` в Ghidra и нажал **Auto Analyze**. Он объясняет не «магический
анализ исходников», а последовательное построение всё более полезной модели из
байтов. Общая карта репозитория находится в
[`ARCHITECTURE_OVERVIEW_RU.md`](ARCHITECTURE_OVERVIEW_RU.md), а подробные нижние
уровни разобраны в [`FEATURES_BASE_OVERVIEW_RU.md`](FEATURES_BASE_OVERVIEW_RU.md),
[`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md) и
[`X86_SLEIGH_PCODE_RU.md`](X86_SLEIGH_PCODE_RU.md).

## Самая короткая mental model

```text
GTA5.exe на диске
  -> PE-loader: заголовки, секции и адреса
  -> Program: память, символы, импорты, entry point
  -> дизассемблер: байты -> Instruction
  -> анализаторы: ссылки, функции, данные, типы
  -> Program CFG и граф вызовов
  -> декомпилятор отдельной функции:
       Instruction -> p-code -> его CFG -> SSA -> типы/переменные -> C-подобный текст
```

Это не один проход от entry point до C. Это реактивная система: каждый найденный
объект сообщает об изменении, и подходящие анализаторы получают адреса для
дальнейшей работы. Поэтому после обнаружения одной инструкции могут появиться
новая ссылка, функция, тип и новые задачи анализа.

## 1. Что именно открывает Ghidra

### Файл, проект и `Program` -- разные вещи

Проект Ghidra -- это контейнер базы данных. Загруженный `GTA5.exe` -- исходный
поток байтов. Рабочий результат импорта -- объект `Program`: адресное
пространство, Memory blocks, Listing, инструкции, функции, символы, типы,
references и выбранные `Language`/`CompilerSpec`. Эти данные сохраняются через
модель `Program` и database, а не перечитываются из PE при каждом клике. См.
[`ProgramDB.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/ProgramDB.java)
и [`GHIDRA_DATABASE_RU.md`](GHIDRA_DATABASE_RU.md).

### Loader не дизассемблирует весь файл

Сначала Ghidra выбирает реализацию `Loader`. Для PE это
[`PeLoader.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/PeLoader.java).
`findSupportedLoadSpecs()` читает PE, определяет machine, image base и подходящий
язык/ABI. Затем `load()` делает примерно следующее:

1. Создаёт `FileBytes`, чтобы связать модель памяти с исходным файлом.
2. Превращает секции PE в Memory blocks через `processMemoryBlocks()`.
3. Разбирает data directories: imports, exports, relocations, debug, TLS и т. д.
4. Создаёт в Listing структуры, комментарии, символы и внешние библиотеки.
5. Помечает entry point и выбирает compiler/ABI.

Последовательность видна в [`PeLoader.java#L102-L175`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/PeLoader.java#L102-L175).
Низкоуровневые классы PE находятся в
[`format/pe`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe).

Для `GTA5.exe` это означает: Ghidra знает, где `.text`, `.rdata`, `.data`,
ресурсы и другие секции, какие диапазоны исполняемые, где image base и entry
point. Но «исполняемый Memory block» ещё не означает «каждый байт уже является
Instruction»: это отдельная операция дизассемблирования.

## 2. Почему появляются окна с множеством опций

Кнопка **Auto Analyze** запускает не один анализатор. `AutoAnalysisManager`
находит классы, реализующие `Analyzer`, проверяет `canAnalyze(program)` и
раскладывает их по очередям: byte, instruction, function, modifiers, signatures
и data. Это видно в [`AutoAnalysisManager.java#L149-L200`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisManager.java#L149-L200).

Каждый Analyzer имеет тип, приоритет, enablement и собственные параметры.
Настройки в диалоге -- это выбор применяемых эвристик, а не обещание, что Ghidra
найдёт истину. Приоритет обычно движется от разметки формата и блоков к
дизассемблированию, ссылкам, функциям, Function ID и распространению типов; это
описано в [`AnalysisPriority.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/services/AnalysisPriority.java)
и [`FEATURES_BASE_OVERVIEW_RU.md`](FEATURES_BASE_OVERVIEW_RU.md#3-как-устроен-auto-analysis-pipeline).

Поток событий примерно такой:

```text
PE-разметка добавила bytes
  -> BYTE_ANALYZER
дизассемблер создал Instruction
  -> INSTRUCTION_ANALYZER
найдены call/reference
  -> FUNCTION_ANALYZER
создана функция или изменена сигнатура
  -> FUNCTION_* и DATA_ANALYZER
```

Менеджер планирует не обязательно весь файл целиком, а `AddressSet` изменённых
адресов. Поэтому анализ может возвращаться к уже обработанной области после
новой информации. UI-старт этого процесса находится в
[`AutoAnalysisPlugin.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisPlugin.java),
а очередь -- в [`AnalysisScheduler.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AnalysisScheduler.java).

## 3. От entry point к инструкциям

Обычно надёжные начальные адреса -- PE entry point, экспорт, импортный thunk,
известный символ или адрес, на который ссылается инструкция. Дизассемблер не
переводит произвольный `.text` вслепую: он начинает с seed-адресов и может
следовать потокам переходов.

[`DisassembleCommand.java#L325-L340`](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/disassemble/DisassembleCommand.java#L325-L340)
вызывает `disassembler.disassemble(..., followFlow)`. После успеха он сообщает
`AutoAnalysisManager.codeDefined(...)`. В Listing появляются mnemonic, операнды,
`FlowType` и ссылки на цели переходов, данные, функции и внешние адреса.

Важная поправка: Ghidra не «исполняет GTA5.exe». Она статически декодирует байты
и выводит возможный поток. Непрямой `jmp [таблица]`, packer,
самомодификация или данные, похожие на код, могут привести к неполному или
неверному результату.

## 4. Откуда берутся функции

Функция в `Program` -- это entry point плюс набор адресов тела. Когда Ghidra
видит прямой `CALL target`, анализатор может создать функцию по target. Именно
это делает [`FunctionAnalyzer.java#L44-L143`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/FunctionAnalyzer.java#L44-L143):
он просматривает call references, исключает уже известные функции и передаёт
адреса в `AutoAnalysisManager.createFunction()`.

Тело функции вычисляется flow-анализом от entry point. В
[`CreateFunctionCmd.java#L590-L627`](../Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateFunctionCmd.java#L590-L627)
`FollowFlow` идёт по переходам внутри функции, но обычный `CALL` не включает
вызываемую функцию в тело текущей. Tail call, shared return, непрямой вызов и
обфускация требуют проверки человеком.

## 5. CFG: какой именно граф строится

### CFG `Program`

`BasicBlockModel` строит базовые блоки из созданных инструкций и flow references.
Блок заканчивается на jump/terminal instruction или соответствующей ссылке;
реализация находится в
[`BasicBlockModel.java#L64-L77`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/BasicBlockModel.java#L64-L77).

```text
B0: cmp eax, 0; je  B2
 |\
 | `-> B2: sub ecx, 1; ret
 `--> B1: add ecx, 1; jmp B3
       `-> B3: ret
```

Это граф инструкций/переходов, доступный Java API и Function Graph. Он может
существовать ещё до открытия окна Decompiler.

### CFG декомпилятора

При открытии Decompiler для конкретной функции native-декомпилятор строит другой
граф: из p-code операций. Затем он меняет его во время SSA, удаления мёртвого
кода и структурирования `if`, циклов и `switch`. Function Graph и Decompiler CFG
похожи, но не являются одной структурой. См.
[`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md).

## 6. Как получается Call Graph

Call graph -- это представление отношений между уже известными функциями: узел =
функция, ребро `A -> B` = найденный вызов из A в B. Основа -- функции и call
references в `Program`; UI-тип называется `CallGraphType` и описан как “Shows
relationships between functions” в
[`CallGraphType.java`](../Ghidra/Features/Base/src/main/java/ghidra/graph/CallGraphType.java).

Прямой `call 0x...` даёт хорошее ребро. Непрямой вызов через указатель может не
иметь одной доказуемой цели; constant propagation, imports, Function ID и ручные
типы могут уточнить его, но не гарантируют результат. Импорт DLL обычно
представлен внешней функцией или thunk, поэтому граф может вести в Windows DLL.

## 7. Как x86-64 становится p-code

Для GTA5 выбирается x86-64 `Language`, а Windows ABI задаётся
[`x86-64-win.cspec`](../Ghidra/Processors/x86/data/languages/x86-64-win.cspec):
первые целочисленные аргументы обычно идут через `RCX`, `RDX`, `R8`, `R9`,
возврат -- через `RAX`, stack pointer -- `RSP`.

Sleigh-описания в [`x86-64.slaspec`](../Ghidra/Processors/x86/data/languages/x86-64.slaspec),
[`x86.slaspec`](../Ghidra/Processors/x86/data/languages/x86.slaspec) и
[`ia.sinc`](../Ghidra/Processors/x86/data/languages/ia.sinc) определяют, какие
байты означают инструкцию и какой у неё эффект над регистрами, памятью, флагами
и потоком.

Например, `mov [rcx+0x10], edx` концептуально становится:

```text
tmp = RCX + 0x10
STORE ram, tmp, EDX
```

Одна x86-команда может дать несколько p-code операций: `add` меняет результат и
флаги, а `cmp` вычисляет флаги для последующего условного перехода. P-code
нужен, чтобы единый анализатор работал с x86, ARM и другими CPU через `COPY`,
`INT_ADD`, `LOAD`, `STORE`, `CBRANCH`, `CALL`, `RETURN`. См.
[`X86_SLEIGH_PCODE_RU.md`](X86_SLEIGH_PCODE_RU.md),
[`op.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/op.hh) и
[`varnode.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/varnode.hh).

## 8. Почему Decompiler не работает во время Auto Analyze

Auto Analyze готовит `Program`: Listing, функции, ссылки, типы и символы.
Decompiler обычно запускается лениво, когда пользователь открывает конкретную
функцию. Java `DecompInterface` держит отдельный native-процесс, отправляет ему
адрес функции, а native-часть получает p-code, память, символы, references и
типы через callback. Точки входа:

- Java: [`DecompInterface.java`](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompInterface.java);
- callback: [`DecompileCallback.java`](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompileCallback.java);
- native command: [`ghidra_process.cc`](../Ghidra/Features/Decompiler/src/decompile/cpp/ghidra_process.cc);
- порядок действий: [`coreaction.cc`](../Ghidra/Features/Decompiler/src/decompile/cpp/coreaction.cc).

P-code может существовать как представление инструкции в Java Listing, но
глубокая работа `Funcdata`/SSA/PrintC начинается при декомпиляции выбранной
функции, а не обязана выполняться для всего огромного `GTA5.exe`.

## 9. Как появляются параметры и локальные переменные

Машинный код не хранит исходное объявление вроде `int playerCount`. Сначала
декомпилятор создаёт низкоуровневые `Varnode`: диапазон регистра, stack offset,
адрес памяти или временное значение p-code. Затем он:

1. Связывает записи и чтения через data flow.
2. Строит SSA: повторные записи в `EAX` становятся разными версиями, а слияние
   ветвей получает `MULTIEQUAL` (аналог phi).
3. Распространяет ограничения типов: размер, знаковость, указатель, арифметика,
   сравнения и способы передачи в вызовы.
4. Связывает входные места с параметрами через ABI. `RCX` может стать первым
   параметром, если это подтверждают calling convention и поток данных.
5. Выходит из SSA, объединяет совместимые значения и решает, что оставить
   локальной переменной, а что встроить в выражение.

Последовательность описана в [`docmain.hh#L185-L240`](../Ghidra/Features/Decompiler/src/decompile/cpp/docmain.hh#L185-L240)
и в разделах SSA и переменных
[`GHIDRA_DECOMPILER_FLOW_RU.md#7-ssa-почему-один-регистр-превращается-в-много-значений`](GHIDRA_DECOMPILER_FLOW_RU.md#7-ssa-почему-один-регистр-превращается-в-много-значений),
[`GHIDRA_DECOMPILER_FLOW_RU.md#10-выход-из-ssa-и-восстановление-переменных`](GHIDRA_DECOMPILER_FLOW_RU.md#10-выход-из-ssa-и-восстановление-переменных).

`local_18`, `uVar3` или `local_res8` -- чаще всего имя, придуманное Ghidra для
восстановленного значения стека/регистра. Это не означает, что в PE была строка
с таким именем. PDB/DWARF, настоящие символы, импортированные типы и ручная
разметка могут дать более точное имя.

Стековая локальная переменная считается определённой там, где p-code впервые
записывает соответствующий stack offset. Чтение до доказанной записи может
остаться `undefined`, параметром, полем указателя или ошибочно объединённым
значением. Register reuse, частичные регистры, aliasing и оптимизация часто
мешают точному выводу. Поэтому сначала проверяйте инструкции и p-code, а не имя
`local_XX`.

Источники типов: debug information, imports/exports, Windows x64 `CompilerSpec`,
способы использования значения, Function ID, type propagation и эвристика
декомпилятора. Если объект используется и как число, и как указатель, результат
может быть `undefined4`, `longlong` или множеством cast-ов: информация могла
быть уничтожена компилятором.

## 10. Как читать результат

```text
адрес функции в Program
  -> запрос Instruction и packed p-code у Java
  -> p-code blocks и CFG
  -> вызовы и прототипы
  -> SSA / MULTIEQUAL
  -> dead code, constant propagation, type inference
  -> структурирование if/while/switch
  -> выход из SSA и высокоуровневые переменные
  -> PrintC: токены и C-подобный текст
```

`PrintC` не пишет восстановленный оригинальный C-файл. Он печатает наиболее
понятное выражение известной data-flow модели. Поэтому `goto`, странный cast,
неверный prototype или `local_XX` могут честно отражать недостаток информации.

## 11. Практический порядок для функции GTA5

1. Проверьте исполняемую секцию, начало дизассемблирования и границу функции.
2. Проверьте `FlowType`, call/jump references и тело функции.
3. Сравните Function Graph (CFG `Program`) с инструкциями.
4. В Decompiler проверьте calling convention, параметры и типы.
5. Для подозрительного значения откройте p-code и проследите его определения,
   особенно записи в `RSP`-offset и повторное использование регистров.
6. Исправьте имя, тип, signature или границу функции и декомпилируйте заново.

Нельзя ожидать, что Ghidra статически восстановит поведение всей игры одной
кнопкой: большие PE содержат middleware, оптимизированный C++, обфускацию и
непрямые вызовы. Сила Ghidra в связной проверяемой модели:

```text
байт -> Instruction -> reference -> функция -> p-code -> переменная -> токен C
```

Итого: Ghidra сначала выясняет, **где** что находится в PE, затем постепенно
выясняет, **какие байты могут быть кодом и как они связаны**, а при открытии
функции строит осторожную C-подобную гипотезу. Локальные переменные и типы в
основном выводятся, а не читаются готовыми из машинного кода.
