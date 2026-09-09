# Ghidra Framework: понятное введение по исходникам

Этот документ объясняет, что находится в [`Ghidra/Framework`](../Ghidra/Framework),
как его части связаны между собой и где искать реализацию. Он рассчитан на
новичка, который хочет перейти от использования Ghidra к чтению Java-исходников
и написанию плагинов, анализаторов или скриптов.

Для общей карты репозитория сначала полезно прочитать
[`ARCHITECTURE_OVERVIEW_RU.md`](ARCHITECTURE_OVERVIEW_RU.md). Для следующего шага
по декомпиляции см. [`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md).

## 1. Что означает Framework

`Framework` -- это не один «движок анализа», а нижний слой Ghidra. Он дает
остальным модулям общие строительные блоки:

* модель программы: память, адреса, инструкции, данные, функции, типы, символы
  и ссылки;
* постоянное хранилище и транзакции;
* проектное файловое хранилище и открытие domain objects;
* оконный docking-интерфейс, задачи и диалоги;
* p-code и исполнение/эмуляцию инструкций;
* графы, XML, общие коллекции, работу с файлами и системный PTY.

Подробное самостоятельное изучение слоя DB продолжает
[`GHIDRA_DATABASE_RU.md`](GHIDRA_DATABASE_RU.md): там разобраны физические
буферы, B-tree, checkpoint, rollback, undo/redo и recovery.

Модуль `Features` обычно отвечает за конкретную возможность, например PE,
декомпилятор или анализатор. `Framework` предоставляет API, на котором эта
возможность строится. Например, загрузчик создает блоки в `Memory`, а анализатор
создает `Function`, `Data` и `Reference` внутри `Program`.

## 2. Карта модулей

| Модуль | Назначение | С чего начать |
|---|---|---|
| [`SoftwareModeling`](../Ghidra/Framework/SoftwareModeling) | Основная модель программы и адресов | [`Program.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Program.java) |
| [`DB`](../Ghidra/Framework/DB) | Табличная database API, буферы, B-tree, транзакции | [`DBHandle.java`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java) |
| [`Project`](../Ghidra/Framework/Project) | Проекты, domain objects, блокировки, команды | [`DomainObject.java`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/model/DomainObject.java) |
| [`FileSystem`](../Ghidra/Framework/FileSystem) | Локальное/удаленное файловое хранилище проекта | [`FileSystem.java`](../Ghidra/Framework/FileSystem/src/main/java/ghidra/framework/store/FileSystem.java) |
| [`Emulation`](../Ghidra/Framework/Emulation) | P-code emulator и состояния памяти/регистров | [`PcodeEmulator.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/emu/PcodeEmulator.java) |
| [`Docking`](../Ghidra/Framework/Docking) | Окна, панели, действия и контекст UI | [`DockingWindowManager.java`](../Ghidra/Framework/Docking/src/main/java/docking/DockingWindowManager.java) |
| [`Gui`](../Ghidra/Framework/Gui) | Общие Swing-компоненты и UI-инфраструктура | [`Ghidra/Framework/Gui/src/main/java`](../Ghidra/Framework/Gui/src/main/java) |
| [`Generic`](../Ghidra/Framework/Generic) | Общие утилиты и структуры данных | [`Ghidra/Framework/Generic/src/main/java`](../Ghidra/Framework/Generic/src/main/java) |
| [`Utility`](../Ghidra/Framework/Utility) | Базовые утилиты, XML, задачи и приложение | [`Ghidra/Framework/Utility/src/main/java`](../Ghidra/Framework/Utility/src/main/java) |
| [`Graph`](../Ghidra/Framework/Graph) | Абстракции графов и их отображение | [`Ghidra/Framework/Graph/src/main/java`](../Ghidra/Framework/Graph/src/main/java) |
| [`Help`](../Ghidra/Framework/Help) | Модель и проверка help-документации | [`Ghidra/Framework/Help/src/main/java`](../Ghidra/Framework/Help/src/main/java) |
| [`Pty`](../Ghidra/Framework/Pty) | Псевдотерминалы для запуска процессов | [`Pty.java`](../Ghidra/Framework/Pty/src/main/java/ghidra/pty/Pty.java) |

`SoftwareModeling`, `DB`, `Project` и `FileSystem` образуют путь хранения
программы. `Docking`, `Gui` и `Help` обслуживают интерфейс. `Emulation` --
отдельный путь исполнения p-code; он может читать модель программы, но его
изменяемое состояние не является автоматически сохраненной базой `Program`.

## 3. Два разных значения слова «база»

Новички часто смешивают два уровня.

### 3.1. Модель программы

`Program` -- это объект, с которым работают анализаторы и плагины. В его
интерфейсе прямо перечислены основные менеджеры: `getMemory()`, `getListing()`,
`getFunctionManager()`, `getSymbolTable()`, `getReferenceManager()`,
`getDataTypeManager()` и другие ([`Program.java#L42-L55`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Program.java#L42-L55),
[`Program.java#L90-L152`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Program.java#L90-L152)).

Это логическая модель: «по адресу `0x401000` находится инструкция, у нее есть
ссылка на `FUN_402000`, а аргумент имеет тип `int`».

### 3.2. Низкоуровневая database

`DB` -- это механизм хранения записей, а не понятия «функция» или «инструкция».
`DBHandle` держит buffer manager, master table и набор таблиц
([`DBHandle.java#L31-L49`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java#L31-L49)).
`Table` хранит схему, дерево узлов, число записей и индексы
([`Table.java#L27-L48`](../Ghidra/Framework/DB/src/main/java/db/Table.java#L27-L48)).

Поверх этого низкого уровня `ProgramDB` собирает менеджеры памяти, кода, типов,
символов, ссылок и функций. Это видно по импортам и объявлению класса
([`ProgramDB.java#L25-L71`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/ProgramDB.java#L25-L71)).
То есть `ProgramDB` -- реализация предметной модели, а `DBHandle` -- ее
персистентный фундамент.

## 4. DomainObject и транзакции

`Program` является `DomainObject`. Правило простое: изменения domain object
делаются внутри транзакции. Интерфейс предлагает безопасную форму:

```java
program.withTransaction("Rename function", () -> {
    Function function = program.getFunctionManager().getFunctionAt(entry);
    if (function != null) {
        function.setName("main_loop", SourceType.USER_DEFINED);
    }
});
```

`withTransaction` стартует транзакцию и завершает ее с commit; вариант с
возвращаемым значением откатывает изменения при исключении
([`DomainObject.java#L409-L469`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/model/DomainObject.java#L409-L469)).
В ручной форме используются `startTransaction(description)` и
`endTransaction(id, commit)`, причем все изменения должны быть внутри такой
области ([`DomainObject.java#L472-L515`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/model/DomainObject.java#L472-L515)).

Конкретно для database `DomainObjectAdapterDB` превращает этот ID в объект
`Transaction`, а завершение передает его менеджеру транзакций
([`DomainObjectAdapterDB.java#L313-L382`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/data/DomainObjectAdapterDB.java#L313-L382)).
Практическое следствие: чтение обычно можно выполнять напрямую, но создание
метки, функции, блока памяти или типа должно быть транзакционным.

## 5. Адреса и address spaces

Адрес в Ghidra -- не обязательно просто `long`. Он состоит из address space и
offset. Поэтому существуют обычные адреса памяти, регистры, stack space,
unique space и специальные адреса анализа. Это определение находится в
[`Address.java#L23-L30`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/address/Address.java#L23-L30).

Например, строка `ram:00401000` означает offset `0x00401000` в пространстве
`ram`, а `register:8` -- другое пространство с другим смыслом. Разбор строки
делает `Address.getAddress()`, а создание следующего адреса --
`getNewAddress()` ([`Address.java#L45-L67`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/address/Address.java#L45-L67)).

Пример поиска адреса в скрипте:

```java
Address entry = currentProgram.getAddressFactory()
    .getAddress("ram:00401000");
if (entry != null && currentProgram.getMemory().contains(entry)) {
    println(currentProgram.getListing().getCodeUnitAt(entry).toString());
}
```

Используйте `AddressFactory`, а не конструируйте адреса вручную: factory знает
пространства конкретного processor language. Наборы адресов представлены
`AddressSetView`/`AddressSet`; они позволяют анализатору работать с диапазонами,
а не перебирать каждый байт.

## 6. Memory: байты и блоки

`Memory` представляет адресуемые области программы. В ней бывают initialized,
uninitialized, byte-mapped, bit-mapped и overlay blocks. Полное описание типов
находится в [`Memory.java#L30-L78`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/mem/Memory.java#L30-L78).

Важно различать:

* initialized block содержит известные байты из файла, stream или нули;
* uninitialized block обозначает область, содержимое которой неизвестно;
* overlay дает альтернативное содержимое и собственное overlay address space;
* `OTHER_SPACE` удобен для данных файла, которые не загружаются в runtime-память.

Реализация базы -- [`MemoryMapDB`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/mem/MemoryMapDB.java),
а отдельный блок реализует [`MemoryBlockDB`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/mem/MemoryBlockDB.java).
Добавление и изменение блоков требует exclusive access и обычно должно быть
сделано до основного анализа, поскольку уже найденные ссылки зависят от карты
памяти ([`Memory.java#L36-L41`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/mem/Memory.java#L36-L41)).

## 7. Listing: что найдено по адресам

`Listing` -- слой code-level объектов: инструкции, определенные данные,
неопределенные участки, комментарии, fragments и program trees. Он умеет взять
объект, начинающийся по адресу, или объект, содержащий адрес
([`Listing.java#L34-L84`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Listing.java#L34-L84)).

Упрощенная проверка:

```java
Listing listing = currentProgram.getListing();
CodeUnit unit = listing.getCodeUnitContaining(entry);
if (unit instanceof Instruction instruction) {
    println(instruction.getMnemonicString());
}
```

Не следует считать listing просто текстом окна Listing: это структурированная
модель. Реализация на database-уровне -- [`ListingDB`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/ListingDB.java).
Она согласует code units с памятью: согласно контракту `Program`, создание
code unit невозможно там, где память еще undefined
([`Program.java#L45-L54`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Program.java#L45-L54)).

## 8. Функции, символы и ссылки

Функция -- не только диапазон инструкций. У нее есть entry point, тело,
параметры, return type, calling convention, локальные переменные и namespace.
Контракт функции находится в [`Function.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Function.java),
а database-менеджер -- [`FunctionManagerDB.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/function/FunctionManagerDB.java).

Символ связывает адрес с именем; у адреса могут быть несколько символов, но один
из них primary. Reference описывается как source address, destination address,
тип и operand/mnemonic index ([`SymbolTable.java#L26-L49`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/symbol/SymbolTable.java#L26-L49)).

Пример создания пользовательской метки:

```java
try (Transaction tx = currentProgram.openTransaction("Add label")) {
    currentProgram.getSymbolTable().createLabel(
        entry, "important_entry", SourceType.USER_DEFINED);
}
```

Менеджер символов хранится в [`SymbolManager`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/symbol/SymbolManager.java),
а отдельная база символа -- в [`SymbolDB`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/symbol/SymbolDB.java).
При переименовании функции меняются связанные части модели, поэтому не следует
обходить эти database-классы напрямую из плагина: используйте интерфейсы модели.

## 9. Типы данных

`DataTypeManager` хранит встроенные типы, категории, структуры, массивы,
typedef, указатели и пользовательские типы. Важная операция `resolve()` делает
так, чтобы тип принадлежал именно этому менеджеру и разрешает конфликт имен
([`DataTypeManager.java#L34-L37`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/data/DataTypeManager.java#L34-L37),
[`DataTypeManager.java#L104-L112`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/data/DataTypeManager.java#L104-L112)).

Для `Program` используется [`ProgramDataTypeManager`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/data/ProgramDataTypeManager.java),
а базовая database-реализация -- [`DataTypeManagerDB`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/data/DataTypeManagerDB.java).
Тип влияет на декомпиляцию: указатель определяет разыменование, структура
помогает назвать поля, а сигнатура функции задает типы параметров.

## 10. Язык процессора и p-code

`SoftwareModeling` хранит абстракции `Language`, `CompilerSpec`, регистров,
calling convention и p-code. Описание конкретных инструкций находится не в
`Framework`, а в [`Ghidra/Processors`](../Ghidra/Processors); это важно для
навигации: Framework задает модель, Processor задает семантику CPU.

Упрощенная цепочка такова:

```text
байты -> Language/Sleigh -> Instruction -> p-code -> анализ/декомпилятор
```

Одна машинная инструкция может породить несколько простых p-code операций.
Например, условное сложение может читать регистр, складывать его с константой,
писать результат и обновлять флаги. Такой общий язык позволяет анализу не
дублировать всю логику для каждого CPU.

## 11. Emulation: исполнение p-code

`Emulation` не изменяет listing сам по себе. Он создает состояние, где есть
регистры, память, потоки и program counter, и исполняет p-code. Базовый
`PcodeEmulator` получает `Language`, создает byte arithmetic, thread и shared/
local state ([`PcodeEmulator.java#L122-L169`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/emu/PcodeEmulator.java#L122-L169)).

Идея использования:

```java
Language language = currentProgram.getLanguage();
PcodeEmulator emulator = new PcodeEmulator(language);
// Дальше конфигурация состояния и запуск зависят от нужного сценария.
```

Для системных вызовов и импортов часто нужны stubs: документация самого
эмулятора описывает внедрение p-code через `PcodeMachine.inject()`
([`PcodeEmulator.java#L110-L120`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/emu/PcodeEmulator.java#L110-L120)).
Поэтому эмуляция -- это контролируемая модель поведения, а не гарантия запуска
реального бинарника со всеми ОС-зависимостями.

## 12. Проект и файловое хранилище

`Project` управляет domain files, открытыми объектами, блокировками, событиями
и задачами. `FileSystem` дает абстракцию папок и элементов данных; реализации
находятся в `store/local`, `store/remote` и `store/db`. Версионное и packed
хранилища представлены [`VersionedDatabase`](../Ghidra/Framework/FileSystem/src/main/java/ghidra/framework/store/db/VersionedDatabase.java)
и [`PackedDatabase`](../Ghidra/Framework/FileSystem/src/main/java/ghidra/framework/store/db/PackedDatabase.java).

Итого путь сохранения выглядит примерно так:

```text
ProgramDB
  -> DomainObjectAdapterDB
  -> DBHandle / таблицы
  -> FileSystem domain item
  -> локальный или удаленный проект
```

Конкретный формат каталога проекта не следует путать с API `Program`: плагину
обычно достаточно работать через `Program`, `DomainObject` и `ProjectData`, не
зная внутренних файлов.

## 13. UI, задачи и события

`Docking` предоставляет оконную модель Ghidra. `ComponentProvider` -- базовый
класс панели/провайдера, `DockingAction` -- действие меню или toolbar, а
`DockingWindowManager` управляет размещением окон. Исходники находятся в
[`Docking`](../Ghidra/Framework/Docking/src/main/java/docking).

Долгие операции должны выполняться как задачи с `TaskMonitor`, чтобы UI мог
показывать прогресс и отмену. Реализации task API находятся в
[`Project/task`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/task)
и общие типы монитора в [`Utility/task`](../Ghidra/Framework/Utility/src/main/java/ghidra/util/task).
Практическая схема плагина: действие получает текущий `Program`, запускает
background command, команда открывает транзакцию и регулярно проверяет monitor.

## 14. Минимальный маршрут чтения исходников

1. [`Program.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Program.java) -- публичный фасад модели.
2. [`Address.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/address/Address.java) и каталог `model/address` -- пространства и диапазоны.
3. [`Memory.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/mem/Memory.java) и `database/mem` -- карта памяти.
4. [`Listing.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Listing.java) и `ListingDB.java` -- code units.
5. `Function`, `SymbolTable`, `DataTypeManager` -- семантика анализа.
6. [`ProgramDB.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/ProgramDB.java) -- сборка менеджеров в реализацию.
7. [`DomainObject.java`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/model/DomainObject.java) и [`DBHandle.java`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java) -- изменения, commit и хранение.
8. [`PcodeEmulator.java`](../Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/emu/PcodeEmulator.java) -- отдельная модель исполнения.

Главная практическая граница такая: публичные интерфейсы `ghidra.program.model`
и `ghidra.framework.model` предназначены для использования расширениями, а
классы `database.*` объясняют внутреннее хранение и полезны прежде всего при
изучении реализации или разработке самого Framework.
