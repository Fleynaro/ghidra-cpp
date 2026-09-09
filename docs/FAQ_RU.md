# FAQ по архитектуре и исходникам Ghidra

Этот FAQ фиксирует ответы на часто возникающие вопросы о коде репозитория. Он
рассчитан на чтение вместе с [обзором архитектуры](ARCHITECTURE_OVERVIEW_RU.md),
[обзором Framework](FRAMEWORK_OVERVIEW_RU.md) и [потоком декомпиляции](GHIDRA_DECOMPILER_FLOW_RU.md).
Каждый ответ содержит ссылки на конкретные исходники, чтобы его можно было
проверить по текущей реализации.

## 1. `FunctionDB`, `Function`, `AddressMapDB` и адаптеры: это ORM?

### Короткий ответ

Это похоже на ORM по внешнему ощущению: код работает с объектом `Function`, а
объект связан с записью database. Но это **не классический ORM над SQL**. В Ghidra
есть предметная модель `Program`, DB-backed реализации этой модели и собственный
табличный storage с `DBRecord`, `Table`, B-tree и буферами. SQL-таблиц здесь нет:
см. [описание DB](GHIDRA_DATABASE_RU.md#1-главная-идея) и
[`DBHandle.java`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java).

### Что означает `Function extends ... implements Function`

[`Function.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Function.java)
-- это интерфейс публичной модели: контракт функции с entry point, телом,
параметрами, типом результата, calling convention и локальными переменными.
Плагин обычно получает именно этот интерфейс через
`program.getFunctionManager()`.

[`FunctionDB.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/function/FunctionDB.java#L45-L83)
-- database-реализация интерфейса. Она хранит ссылку на `ProgramDB`, менеджер,
`DBRecord`, символ функции и кэшированные/лениво загружаемые части. Поэтому ее
методы действительно читают и изменяют DB-записи, но одновременно поддерживают
связи с другими менеджерами модели: символами, переменными, типами и thunk-ами.
Это не «голый объект-строка», а объект предметной модели, который инкапсулирует
доступ к storage.

То же правило применяется к другим парам:

| Публичная модель | DB-реализация | Смысл |
|---|---|---|
| `AddressMap` | [`AddressMapDB`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/map/AddressMapDB.java#L49-L99) | Преобразует адреса разных address spaces в устойчивые DB-ключи и обратно. |
| `MemoryBlock` | [`MemoryBlockDB`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/mem/MemoryBlockDB.java) | Представляет блок памяти в модели и хранит его состояние в database. |
| `Symbol` | [`SymbolDB`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/symbol/SymbolDB.java) | Представляет символ и его свойства поверх записи database. |

### Что такое `AddressMapDBAdapter`

`AddressMapDBAdapter` -- не адаптер Java API для пользователя и не DTO. Это
внутренний **адаптер формата хранения**. В database могли существовать разные
версии таблицы address map, поэтому фабрика выбирает `V1`, `V0` или вариант без
таблицы, а при необходимости выполняет upgrade
([`AddressMapDBAdapter.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/map/AddressMapDBAdapter.java#L28-L81)).

Иными словами, `AddressMapDB` отвечает за предметную логику адресов, а adapter
скрывает детали конкретной схемы и миграции. Такой слой позволяет открыть старую
database, прочитать ее read-only или преобразовать в текущий формат, не заставляя
весь код `AddressMapDB` знать каждую историческую схему.

### Где здесь транзакция

Да, изменения `Program` должны выполняться внутри транзакции. В типичном коде
меняется объект модели, например `function.setName(...)`, а реализация обновляет
связанные DB-записи и события. При commit изменения становятся частью нового
состояния domain object; при rollback восстанавливается прежнее состояние.
Контракт находится в [`DomainObject.java`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/model/DomainObject.java#L409-L515),
а DB-связка -- в [`DomainObjectAdapterDB.java`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/data/DomainObjectAdapterDB.java#L313-L382).

Важно не представлять это как «каждый setter немедленно пишет SQL». Ghidra
работает с кэшем буферов и checkpoint-ами; детали commit, rollback и undo описаны
в [разборе database](GHIDRA_DATABASE_RU.md#3-физическое-хранение-не-один-объект----один-файл).
Пользовательский код должен использовать интерфейсы модели и API транзакций, а не
напрямую вызывать database-классы.

## 2. Как x86-64 превращается в P-Code и что происходит со Sleigh-файлами?

### Короткий ответ

`.slaspec` и подключаемые `.sinc` -- исходные тексты языка Sleigh. При первом
использовании или после устаревания они компилируются в бинарный `.sla`. В runtime
Ghidra обычно читает именно `.sla`, а не каждый раз парсит текстовую спецификацию.
Компиляцию выполняет [`SleighCompile.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/pcodeCPort/slgh_compile/SleighCompile.java#L1832-L1888),
а проверку свежести, загрузку и декодирование --
[`SleighLanguageFile.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighLanguageFile.java)
и [`SleighLanguage.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighLanguage.java#L140-L190).

Путь для одной инструкции выглядит так:

```text
байты x86-64
  -> SleighLanguage выбирает конструктор инструкции из .sla
  -> SleighInstructionPrototype декодирует байты и контекст
  -> PcodeEmitObjects или PcodeEmitPacked исполняет шаблон семантики
  -> raw P-code
```

`SleighInstructionPrototype` строит обычный `PcodeOp[]` через
[`PcodeEmitObjects.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/PcodeEmitObjects.java)
или упаковывает операции для нативного декомпилятора через
[`PcodeEmitPacked.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/PcodeEmitPacked.java).
Семантика `MOV`, `ADD`, `CMP` и других инструкций остается заданной в
[`ia.sinc`](../Ghidra/Processors/x86/data/languages/ia.sinc); `.sla` -- это
скомпилированное представление этих правил, а не отдельная ручная таблица
соответствий.

Более подробный маршрут с примером `mov [rcx + 0x10], edx` находится в
[`X86_SLEIGH_PCODE_RU.md`](X86_SLEIGH_PCODE_RU.md), а взаимодействие Java и
нативного декомпилятора -- в [`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md#3-sleigh-из-x86-инструкции-в-raw-p-code).

## 3. Почему CFG декомпилятора отличается от CFG самой Ghidra?

### Короткий ответ

Потому что это графы разных слоев и с разными целями.

CFG `Program`/Listing -- часть сохраненной модели программы. Он строится вокруг
`Instruction`, адресов, flow references и границ функций; с ним работают
анализаторы и UI. Точки входа API видны в
[`FunctionManager.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/FunctionManager.java#L141-L158),
а практическая логика построения flow находится в анализаторах `Features/Base`,
например [`BasicBlockModel.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/BasicBlockModel.java).

CFG декомпилятора -- временный граф `Funcdata`, построенный нативным C++-слоем из
raw P-Code конкретной функции. `FlowInfo` декодирует инструкции, отслеживает
достижимые адреса, делит операции на базовые блоки и соединяет их
([`flow.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/flow.hh#L50-L58),
[`flow.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/flow.hh#L110-L125)).
После этого граф может изменяться во время анализа: восстанавливаются jump table,
обрабатываются косвенные переходы, удаляются/создаются технические блоки,
выполняется структурирование `if`/`while`, а при inline меняется вид функции.

Отсюда типичные расхождения:

- Program CFG отражает текущую Java-модель дизассемблирования и ее flow-ссылки;
- decompiler CFG может использовать другой набор достижимых блоков после анализа
  P-Code и восстановления косвенного потока;
- декомпилятор добавляет SSA- и структурные сущности, которые не являются
  отдельными машинными блоками;
- неуверенный или недоступный flow может быть представлен искусственным halt,
  усечен границами функции или обработан иначе;
- отображаемый C-граф после структурирования уже не обязан быть изоморфен
  исходному графу базовых блоков.

Практический маршрут от байтов к двум CFG разобран в
[`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md#4-протокол-между-java-и-нативным-декомпилятором)
и [`FEATURES_BASE_OVERVIEW_RU.md`](FEATURES_BASE_OVERVIEW_RU.md).

## 4. Как символы, параметры и локальные переменные привязаны к машинному коду?

### Короткий ответ

У привязки несколько уровней, и она не всегда является отношением «один символ
= один адрес инструкции».

1. **Глобальный или функциональный символ.** Символ обычно имеет адрес, имя,
   namespace и source type. Функция связана с entry point, а символ функции
   хранится через `SymbolTable`; базовые контракты находятся в
   [`SymbolTable.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/symbol/SymbolTable.java#L26-L49)
   и [`Function.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Function.java).

2. **Параметр или локальная переменная в Program.** Это объект переменной с
   типом, именем и `VariableStorage`. Storage может описывать регистр, диапазон
   стека или несколько varnode, то есть переменная привязана не обязательно к
   одной инструкции, а к месту хранения значения. См.
   [`VariableStorage.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/VariableStorage.java),
   [`ParameterDB.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/function/ParameterDB.java)
   и [`FunctionVariables.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/function/FunctionVariables.java).

3. **Переменная декомпилятора.** Нативный декомпилятор строит `Varnode` для
   регистра, памяти, константы или временного значения, а затем объединяет
   совместимые значения в `HighVariable`/`HighSymbol`. Эти сущности принадлежат
   временному представлению декомпиляции и описаны в
   [`varnode.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/varnode.hh),
   [`HighVariable.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/HighVariable.java)
   и [`HighSymbol.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/HighSymbol.java).

Связь с конкретной инструкцией сохраняется через P-Code: `PcodeOp` имеет
`SeqNum`/адрес исходной инструкции, поэтому фрагмент C можно подсветить в
Listing ([`op.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/op.hh#L123-L164)).
Но одна переменная может жить в разных регистрах и stack offsets в разные
моменты, а один storage может переиспользоваться разными логическими
переменными. Поэтому соответствие выводится анализом, а не всегда читается из
готовой таблицы символов.

Имена и типы могут прийти из debug-информации или быть восстановлены анализом.
Calling convention и `.cspec` помогают определить параметры и их storage, но
при отсутствии символов декомпилятор строит гипотезу. Подробное объяснение этого
перехода и ограничений восстановления исходника см. в
[`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md#1-короткая-карта-пути)
и [`FRAMEWORK_OVERVIEW_RU.md`](FRAMEWORK_OVERVIEW_RU.md#8-функции-символы-и-ссылки).

## Правило обновления FAQ

Если пользователь задает вопрос по архитектуре, зависимостям модулей или
конкретной реализации исходников, агент обязан не только дать понятный ответ в
чате, но и зафиксировать подтвержденный ответ в этом FAQ. Запись должна содержать
ссылки на актуальные исходники и связанные документы. Если существующий ответ
устарел, сначала исправляется FAQ, а затем тот же актуальный ответ сообщается в
чате.
