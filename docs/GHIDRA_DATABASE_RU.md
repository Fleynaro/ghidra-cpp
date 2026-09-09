# База данных Ghidra: транзакции, хранение и rollback

Этот документ объясняет внутреннюю database Ghidra простым языком. Речь идет
о модуле [`Ghidra/Framework/DB`](../Ghidra/Framework/DB), а не о SQL-сервере.
Здесь нет SQL, таблицы не являются таблицами SQLite, а транзакция не означает
немедленную запись каждого изменения на диск.

Для общей карты Framework полезно сначала прочитать
[`FRAMEWORK_OVERVIEW_RU.md`](FRAMEWORK_OVERVIEW_RU.md), а для общей карты всего
репозитория -- [`ARCHITECTURE_OVERVIEW_RU.md`](ARCHITECTURE_OVERVIEW_RU.md).
Практический пользовательский слой находится в
[`SoftwareModeling`](../Ghidra/Framework/SoftwareModeling): `ProgramDB` строит
модель программы поверх `DBHandle`.

## 1. Главная идея

Упрощенная цепочка выглядит так:

```text
Program / DomainObject
        |
        v
ProgramDB и менеджеры (Memory, Listing, Symbol, Function, DataType...)
        |
        v
DBHandle -> Table -> B-tree узлы -> DataBuffer
        |
        v
BufferMgr -> кэш и версии буферов -> *.gbf
```

`Program` знает, что по адресу находится функция. `DB` знает только о записях,
ключах, полях и байтовых буферах. Например, менеджер символов может хранить
символ как запись в таблице, но низкоуровневый DB-слой не знает, что такое
«символ».

Это разделение видно в [`DBHandle.java`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java#L31-L49)
и [`ProgramDB.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/ProgramDB.java).

## 2. Что открывается при работе с базой

### `Database`

[`Database.java`](../Ghidra/Framework/DB/src/main/java/db/Database.java#L29-L41)
управляет каталогом database и создает `DBHandle`. Внутри используются имена
вроде `db.*.gbf`, `ver.*.gbf` и файлов изменений. Методы `open()` и
`openForUpdate()` отличаются режимом доступа:

```java
DBHandle readOnly = database.open(monitor);
DBHandle writable = database.openForUpdate(monitor);
```

В исходнике `open()` создает `LocalManagedBufferFile` с `false`, а
`openForUpdate()` -- с `true` ([`Database.java`](../Ghidra/Framework/DB/src/main/java/db/Database.java#L165-L194)).
То есть право на запись определяется еще до первой транзакции.

### `DBHandle`

`DBHandle` -- рабочий объект открытой базы. Он держит:

* `BufferMgr` -- буферный кэш, версии и checkpoint-ы;
* `DBParms` -- небольшие параметры базы;
* `MasterTable` -- описание всех пользовательских таблиц;
* коллекцию `Table`.

При открытии конструктор создает `BufferMgr`, читает параметры, создает master
table и загружает таблицы ([`DBHandle.java`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java#L96-L144)).

## 3. Физическое хранение: не «один объект -- один файл»

### Файл блоков

`LocalBufferFile` представляет файл как набор блоков фиксированного размера.
Первый блок является заголовком, а пользовательские buffer ID начинаются со
следующего блока. Формат заголовка и префикса пользовательского блока описан
прямо комментариями в [`LocalBufferFile.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/LocalBufferFile.java#L59-L87):

* magic number;
* ID файла;
* версия формата;
* размер блока;
* список свободных блоков;
* пользовательские параметры;
* для каждого блока -- флаг empty и ID `DataBuffer`.

У блока есть служебный префикс размером 5 байт, поэтому полезный размер buffer
меньше физического размера блока ([`LocalBufferFile.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/LocalBufferFile.java#L89-L99)).
Пустые блоки не обязательно удаляются из середины файла: они попадают в список
свободных и могут быть переиспользованы.

### Параметры базы

`DBParms` занимает buffer с ID `0`. В нем находятся маленькие целочисленные
параметры, в частности:

* ID корневого buffer master table;
* старшая и младшая части уникального ID базы.

Это не отдельный файл и не Java-конфигурация: параметры сериализуются в первый
buffer ([`DBParms.java`](../Ghidra/Framework/DB/src/main/java/db/DBParms.java#L27-L50)).
Поэтому восстановление корня master table фактически восстанавливает и ссылку
на описание таблиц.

### BufferMgr и DataBuffer

`BufferMgr` не выдает вызывающему прямую ссылку на файл. Он загружает
`DataBuffer` в кэш, помечает его locked и требует вызвать `releaseBuffer()`.
Для измененного buffer при release выполняется отдельная ветка
`releaseDirtyBuffer()` ([`BufferMgr.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/BufferMgr.java#L963-L984),
[`BufferMgr.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/BufferMgr.java#L1024-L1145)).

Принцип похож на copy-on-write:

1. читается старая версия buffer;
2. вызывающий меняет память;
3. при release старая версия сохраняется в цепочке версий;
4. новая версия связывается с текущим checkpoint;
5. только потом checkpoint можно зафиксировать или отменить.

`BufferNode` хранит сразу несколько связей: кэш, версии одного ID и список
buffer-ов одного checkpoint ([`BufferNode.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/BufferNode.java#L19-L49)).
Полезно различать два флага:

* `modified` -- buffer отличается от исходного файла;
* `isDirty` -- buffer изменился после последней записи в дисковый кэш.

Это не одно и то же ([`BufferNode.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/BufferNode.java#L72-L91)).

## 4. Таблицы и записи

### Master table

В database есть специальная master table. Она содержит записи о других таблицах:
имя, schema, номер таблицы, корневой buffer, число записей и максимальный ключ.
`MasterTable` читает ее корень из `DBParms`, затем создает `TableRecord` для
каждой таблицы ([`MasterTable.java`](../Ghidra/Framework/DB/src/main/java/db/MasterTable.java#L22-L69)).

Когда создается обычная таблица, сначала создается ее `TableRecord`, затем он
помещается в master table, а новый корень master table снова записывается в
`DBParms` ([`MasterTable.java`](../Ghidra/Framework/DB/src/main/java/db/MasterTable.java#L71-L101)).
Именно поэтому master table является каталогом базы, а не просто еще одной
независимой таблицей.

### Schema и DBRecord

`Schema` описывает тип ключа, поля, имена, версию и sparse-поля
([`Schema.java`](../Ghidra/Framework/DB/src/main/java/db/Schema.java#L25-L48)).
Поддерживаются типизированные поля вроде `ByteField`, `IntField`, `LongField`,
`StringField`, `BinaryField` и другие.

`DBRecord` содержит ключ и массив значений полей. При создании записи значения
строятся из типов schema, а sparse-поле может начинаться в null-состоянии
([`DBRecord.java`](../Ghidra/Framework/DB/src/main/java/db/DBRecord.java#L23-L64)).
Пример низкоуровневой записи:

```java
Schema schema = new Schema(
    1,
    "id",
    new Class<?>[] { StringField.class, IntField.class },
    new String[] { "name", "flags" });

Table table = db.createTable("items", schema);
try (Transaction tx = db.openTransaction(errorHandler)) {
    DBRecord record = schema.createRecord(42);
    record.setString(0, "entry");
    record.setIntValue(1, 3);
    table.putRecord(record);
    tx.commit();
}
```

Имена конкретных setter-ов следует сверять с текущим API `DBRecord`; смысл
примера -- показать, что пользователь работает с record/schema, а не вручную
вычисляет смещения байтов.

### B-tree

`Table` хранит корневой buffer, количество записей и `NodeMgr`. В зависимости от
schema выбирается тип B-tree узлов: long-key, fixed-key или variable-key; это
видно в [`Table.java`](../Ghidra/Framework/DB/src/main/java/db/Table.java#L32-L73)
и списке типов [`NodeMgr.java`](../Ghidra/Framework/DB/src/main/java/db/NodeMgr.java#L52-L119).

У дерева есть interior nodes и record nodes. Листья содержат ключи и записи.
Если запись не помещается в один buffer, для нее используется цепочка буферов
(`ChainedBuffer`). Поэтому крупное binary-поле не обязано занимать один блок.

При `putRecord()` таблица проверяет транзакцию, находит leaf, меняет дерево и
при необходимости обновляет root buffer ([`Table.java`](../Ghidra/Framework/DB/src/main/java/db/Table.java#L941-L1034)).
Удаление проходит тот же путь и также требует активной транзакции
([`Table.java`](../Ghidra/Framework/DB/src/main/java/db/Table.java#L1054-L1122)).

### Индексы

Вторичный индекс -- это отдельная index table. При добавлении, изменении или
удалении записи `Table` вызывает соответствующие методы для индекса
([`Table.java`](../Ghidra/Framework/DB/src/main/java/db/Table.java#L224-L280)).
Следствие: индекс является частью той же транзакции. Нельзя надежно изменить
основную таблицу и «потом когда-нибудь» вручную поправить индекс.

## 5. Что такое транзакция в Ghidra DB

Транзакция здесь -- граница набора изменений и checkpoint-а. Она дает:

* проверку, что запись выполняется в разрешенном контексте;
* атомарный commit набора измененных buffer-ов;
* возможность отбросить изменения до предыдущего checkpoint;
* основу для undo/redo верхнего уровня.

### Низкоуровневый API

```java
long id = db.startTransaction();
try {
    // Создание таблиц, putRecord(), изменение DBBuffer и т. п.
    db.endTransaction(id, true);  // commit
}
catch (RuntimeException | IOException e) {
    db.endTransaction(id, false); // rollback
    throw e;
}
```

На практике лучше использовать try-with-resources:

```java
try (Transaction tx = db.openTransaction(errorHandler)) {
    // Изменения
    tx.commit();
}
```

`Transaction` по умолчанию закрывается с commit. Для отката можно вызвать
`abort()` или `abortOnClose()` ([`Transaction.java`](../Ghidra/Framework/DB/src/main/java/db/Transaction.java#L43-L128)).
Идиоматичный вариант с исключением:

```java
try (Transaction tx = db.openTransaction(errorHandler)) {
    changeDatabase();
    if (!isValid()) {
        tx.abortOnClose();
    }
}
```

### Запуск

`DBHandle.startTransaction()` запрещает вторую активную транзакцию на том же
handle, сбрасывает состояние ожидания и запоминает текущий modification count
([`DBHandle.java`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java#L463-L478)).
Низкоуровневые транзакции не являются произвольно вложенными.

На уровне `DomainObject` могут существовать sub-transaction-ы и более удобный
API `withTransaction`; это уже адаптер Framework Project, а не новая физическая
транзакция DB. Смотрите [`DomainObject.java`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/model/DomainObject.java#L409-L515)
и [`DomainObjectAdapterDB.java`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/data/DomainObjectAdapterDB.java#L313-L382).

## 6. Commit: что происходит по шагам

Рассмотрим изменение записи.

1. `Table.putRecord()` получает или создает B-tree узлы.
2. Узлы получают буферы через `BufferMgr` и удерживаются locked.
3. При release измененные буферы попадают в текущий checkpoint.
4. При закрытии с commit `DBHandle.doEndTransaction()` сначала вызывает
   `masterTable.flush()`.
5. `BufferMgr.checkpoint()` закрывает текущий список изменений.
6. `DBHandle` увеличивает номер checkpoint и сбрасывает флаг активной
   транзакции ([`DBHandle.java`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java#L512-L538)).

`masterTable.flush()` важен: некоторые изменения `TableRecord` держатся в памяти
до конца транзакции. Если корень таблицы изменился, его нужно записать в master
table до checkpoint ([`MasterTable.java`](../Ghidra/Framework/DB/src/main/java/db/MasterTable.java#L194-L207)).

`BufferMgr.checkpoint()` не обязательно означает «файл уже сохранен». Он означает,
что изменения стали отдельным зафиксированным состоянием в памяти/кэше и могут
участвовать в undo. Физическое сохранение выполняется отдельным путем
`DBHandle.save()` и `BufferMgr.save()` ([`DBHandle.java`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java#L780-L802)).

## 7. Rollback текущей транзакции

Если закрыть текущую транзакцию с `commit == false`, `DBHandle` делает следующее:

```text
bufferMgr.undo(false)
    -> удаляется текущий checkpoint
    -> версии buffer-ов возвращаются к предыдущим
    -> новые buffer ID освобождаются/обрезаются
reloadTables()
    -> master table и TableRecord перечитываются
notifyDbRestored()
```

Это буквально видно в [`DBHandle.java`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java#L512-L533):
после `undo(false)` вызывается `reloadTables()`, а `DBRollbackException`
превращается в уведомление слушателей `dbRestored`.

### Как BufferMgr возвращает старую версию

У каждого измененного buffer ID может быть цепочка `BufferNode`, где версии
соответствуют checkpoint-ам. `undo(false)` идет по узлам удаленного checkpoint-а:

* если buffer был создан внутри транзакции, запись о нем удаляется;
* если buffer был удален, старый индекс снова выделяется;
* если buffer существовал, выбирается `oldVer`;
* список свободных индексов и размер файла корректируются;
* checkpoint уничтожается, redo-стек также очищается.

Реализация этих случаев находится в [`BufferMgr.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/BufferMgr.java#L1277-L1367).
Это не журнал SQL-команд «UPDATE обратно»: восстанавливаются предыдущие
байтовые версии буферов и связи дерева.

### Почему нужна перезагрузка таблиц

В памяти могли остаться старые `Table`, `TableRecord`, корни деревьев или кэши
узлов. Поэтому после undo/redo DBHandle вызывает `reloadTables()`, а master
table сопоставляет свежие записи со старыми объектами и инвалидирует исчезнувшие
([`MasterTable.java`](../Ghidra/Framework/DB/src/main/java/db/MasterTable.java#L145-L191)).
Это объясняет важное правило: объект верхнего уровня должен реагировать на
`dbRestored`, а не продолжать доверять кэшу после rollback.

## 8. Undo и redo уже завершенных транзакций

Текущий rollback и пользовательский Undo похожи, но не одинаковы:

* текущий rollback: `undo(false)`, откат без возможности redo;
* обычный Undo: `undo(true)`, откат с переносом checkpoint в redo-список;
* Redo: возврат checkpoint из redo-списка.

`DBHandle.undo()` разрешен только без активной транзакции, вызывает
`bufferMgr.undo(true)`, перезагружает таблицы и уведомляет слушателей
([`DBHandle.java`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java#L624-L652)).
`BufferMgr` хранит списки `checkpointHeads` и `redoCheckpointHeads`
([`BufferMgr.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/BufferMgr.java#L96-L116)).

Пример последовательности:

```text
baseline -> tx A -> tx B
                       ^ current

undo()  -> baseline -> tx A     + tx B в redo
redo()  -> baseline -> tx A -> tx B
```

После нового изменения в состоянии после undo redo-стек очищается: новая ветка
истории больше не совместима со старым будущим. Это происходит в
`startCheckpoint()` ([`BufferMgr.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/BufferMgr.java#L1223-L1245)).

Количество undo ограничено. При превышении лимита старый checkpoint упаковывается
в baseline; такие изменения уже нельзя откатить обычным `undo()`
([`BufferMgr.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/BufferMgr.java#L480-L510)).

## 9. Сохранение на диск и recovery

### Save -- не то же самое, что commit

Commit фиксирует логическую транзакцию и делает состояние доступным для undo.
`save()` переносит состояние в постоянный buffer-файл. Поэтому возможна база:

```text
сохраненное состояние на диске
        + несколько committed checkpoint-ов в памяти
```

Если приложение закрывается корректно, `save()` записывает измененные буферы,
переключает базовый файл и сбрасывает версии к baseline. Данные о change set
могут передаваться через [`DBChangeSet.java`](../Ghidra/Framework/DB/src/main/java/db/DBChangeSet.java).

### Recovery snapshot

Для защиты от падения во время работы используются recovery-снимки. Это не
rollback программиста, а копия несохраненных buffer-ов, чтобы после следующего
запуска восстановить потерянную работу.

`RecoveryMgr` использует два чередующихся snapshot-файла (`snapshotA.grf`,
`snapshotB.grf`) и соответствующие change-файлы
([`RecoveryMgr.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/RecoveryMgr.java#L31-L55)).
При открытии он проверяет валидность, выбирает более свежий снимок и передает
его `BufferMgr.recover()` ([`RecoveryMgr.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/RecoveryMgr.java#L63-L107)).

Снимок нельзя безопасно делать в любой момент: `DBHandle.takeRecoverySnapshot()`
отказывается работать при активной транзакции и проверяет, были ли изменения
после прошлого снимка ([`DBHandle.java`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java#L272-L300)).
Так избегается копирование противоречивого набора buffer-ов посреди изменения.

## 10. Синхронизация и ограничения

Большинство публичных операций таблицы синхронизируется на `DBHandle`; в
`Table` это прямо отмечено в комментарии класса
([`Table.java`](../Ghidra/Framework/DB/src/main/java/db/Table.java#L27-L31)).
`BufferMgr` дополнительно использует `snapshotLock`, чтобы snapshot не пересекался
с изменением buffer-а.

Практические правила:

* не менять запись или `DBBuffer` вне транзакции;
* не держать полученный buffer дольше необходимого;
* всегда вызывать `releaseBuffer()`/освобождать B-tree узлы;
* не выполнять undo/redo при locked buffer-ах;
* после rollback/undo не использовать старые кэши без обработки `dbRestored`;
* не считать commit эквивалентом физического save;
* не менять одновременно основную таблицу и индекс в обход API `Table`.

При checkpoint или undo наличие locked buffer считается ошибкой: `BufferMgr`
проверяет `lockCount` и выбрасывает assertion
([`BufferMgr.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/BufferMgr.java#L1180-L1201),
[`BufferMgr.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/BufferMgr.java#L1284-L1295)).

## 11. Что это означает для плагина

Обычному плагину не нужно создавать `Table` напрямую. Он работает с `Program`
и открывает транзакцию domain object:

```java
try (Transaction tx = currentProgram.openTransaction("Rename function")) {
    Function f = currentProgram.getFunctionManager().getFunctionAt(entry);
    if (f != null) {
        f.setName("main_loop", SourceType.USER_DEFINED);
    }
    // Если commit не вызвать или вызвать abort(), изменения откатятся.
    tx.commit();
}
```

Так менеджеры `ProgramDB` сами обновят связанные таблицы, индексы, события и
кэши. Прямой вызов DB API полезен для изучения Framework или разработки нового
database-backed менеджера, но опасен для обычного расширения: легко забыть
связанный индекс или уведомление верхнего уровня.

## 12. Краткая карта исходников

1. [`DBHandle.java`](../Ghidra/Framework/DB/src/main/java/db/DBHandle.java) -- публичная точка входа, транзакции, undo/redo, save.
2. [`Database.java`](../Ghidra/Framework/DB/src/main/java/db/Database.java) -- каталог и открытие buffer-файла.
3. [`BufferMgr.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/BufferMgr.java) -- кэш, dirty buffers, checkpoint-и и версии.
4. [`BufferNode.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/BufferNode.java) -- связи версий и состояние buffer-а.
5. [`LocalBufferFile.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/LocalBufferFile.java) -- физический блоковый формат `.gbf`.
6. [`MasterTable.java`](../Ghidra/Framework/DB/src/main/java/db/MasterTable.java) -- каталог таблиц.
7. [`Table.java`](../Ghidra/Framework/DB/src/main/java/db/Table.java) -- записи, B-tree и индексы.
8. [`Schema.java`](../Ghidra/Framework/DB/src/main/java/db/Schema.java) и [`DBRecord.java`](../Ghidra/Framework/DB/src/main/java/db/DBRecord.java) -- типизированные записи.
9. [`RecoveryMgr.java`](../Ghidra/Framework/DB/src/main/java/db/buffers/RecoveryMgr.java) -- восстановление после сбоя.
10. [`ProgramDB.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/database/ProgramDB.java) -- пример прикладного слоя над DB.

Если запомнить одну модель, пусть это будет такая: Ghidra изменяет не файл
напрямую, а версии буферов в checkpoint-е; commit закрывает checkpoint, rollback
возвращает предыдущие версии, undo сохраняет отмененное состояние для redo, а
save отдельно переносит актуальный baseline в постоянный файл.
