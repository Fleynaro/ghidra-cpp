# `DecompilerDependent`: что находится поверх декомпилятора

Этот документ объясняет новичку, зачем в Ghidra есть отдельный модуль
[`Ghidra/Features/DecompilerDependent`](../Ghidra/Features/DecompilerDependent),
если уже существует [`Ghidra/Features/Decompiler`](../Ghidra/Features/Decompiler).
Общую карту репозитория см. в
[`ARCHITECTURE_OVERVIEW_RU.md`](ARCHITECTURE_OVERVIEW_RU.md), а полный путь от
байтов до C-подобного текста -- в
[`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md).

## Короткий ответ

`Decompiler` -- это сам базовый механизм декомпиляции и стандартное окно
Decompiler. Он запускает native-декомпилятор, получает p-code и строит
`HighFunction`/AST, из которого показывается C-подобный текст. Его Java-часть,
UI и native-исходники находятся в
[`Ghidra/Features/Decompiler/src`](../Ghidra/Features/Decompiler/src).

`DecompilerDependent` -- отдельный слой **функций, которым нужен результат или
сервисы декомпилятора**. Он не заменяет и не дублирует алгоритм декомпиляции:
его классы вызывают API `Decompiler`, читают `HighFunction` и p-code, добавляют
плагины Ghidra или экспортируют результат во внешний анализатор.

Простая аналогия:

```text
Decompiler          = двигатель и приборная панель, показывающая C-подобный код
DecompilerDependent = инструменты, которые используют показания двигателя
                      для поиска, экспорта, taint-анализа и уточнения типов
```

Зависимость видна непосредственно в
[`build.gradle`](../Ghidra/Features/DecompilerDependent/build.gradle): модуль
подключает `:Base`, `:Decompiler` и `:Sarif` через `api project(...)`. Сам
`Decompiler` подключает `:Base` и `:SoftwareModeling`, а также собирает native
часть через [`buildNatives.gradle`](../Ghidra/Features/Decompiler/buildNatives.gradle).

## Зачем разделять модули

Разделение решает несколько практических задач:

1. Базовый декомпилятор остаётся самостоятельным и не обязан знать о taint-
   движках, поиске текста, CTADL или специальных эвристиках сигнатур.
2. Прикладные возможности получают готовый стабильный API: например,
   `DecompileResults`, `HighFunction`, `PcodeOpAST`, `ClangToken` и
   `ParallelDecompiler`.
3. Дополнительные плагины можно тестировать, собирать и изменять отдельно от
   native C++-движка.
4. Внешние движки могут получать p-code в удобном формате, не становясь частью
   основного алгоритма декомпиляции.

Это обычная направленность зависимостей:

```text
Base / SoftwareModeling
          ↓
     Decompiler
          ↓
  DecompilerDependent
          ↓
 внешний taint/индексатор (например, CTADL или SARIF-потребитель)
```

Обратной зависимости `Decompiler` от `DecompilerDependent` быть не должно:
иначе базовый модуль стал бы зависеть от специализированных сценариев.

## Что лежит в каталоге

| Путь | Назначение |
|---|---|
| [`src/main/java/ghidra/app/plugin/core/decompiler/taint`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/decompiler/taint) | Taint/data-flow: источники, приёмники, gates, срезы, подсветка и SARIF. |
| [`src/main/java/ghidra/app/plugin/core/decompiler/export`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/decompiler/export) | Экспорт p-code и сведений о программе в fact-файлы для внешнего движка. |
| [`src/main/java/ghidra/app/plugin/core/search`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/search) | Поиск строки или регулярного выражения в декомпилированном тексте функций. |
| [`src/main/java/ghidra/app/plugin/core/string/variadic`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic) | Разбор `printf`/`scanf`-подобных format strings и уточнение аргументов вызовов. |
| [`src/main/java/ghidra/app/extension/datatype/finder`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/extension/datatype/finder) | Поиск использований data type и полей через результат декомпиляции. |
| [`src/main/java/ghidra/app/plugin/core/decompiler/absint`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/decompiler/absint) | Extension point-контракт для результатов внешней abstract interpretation. |
| [`ghidra_scripts`](../Ghidra/Features/DecompilerDependent/ghidra_scripts) | Скрипты экспорта p-code/facts, включая экспорт функции и её callees. |
| [`src/main/help`](../Ghidra/Features/DecompilerDependent/src/main/help) | Help для пользовательских возможностей, например Text Finder и Taint. |
| [`src/test`](../Ghidra/Features/DecompilerDependent/src/test) | Обычные тесты модуля; `src/test.slow` содержит более медленные тесты. |

`README.md` модуля сейчас содержит только заголовок, поэтому реальное
назначение следует читать по `build.gradle`, Java-классам, help и тестам.

## 1. Поиск по декомпилированному тексту

### Что добавляется

[`DecompilerTextFinderPlugin.java`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/search/DecompilerTextFinderPlugin.java)
регистрирует действие **Search -> Decompiled Text...**. Оно ищет текст в
результате декомпиляции каждой функции, умеет обычный поиск и regex, а найденные
строки показывает в таблице. Это не поиск байтов и не поиск исходной строки в
памяти: сначала функция декомпилируется, затем анализируется её текст.

Help подтверждает сценарий и ограничение выбора:
[`Decompiler_Text_Finder.html`](../Ghidra/Features/DecompilerDependent/src/main/help/help/topics/DecompilerTextFinderPlugin/Decompiler_Text_Finder.html).
При `Search Selection` проверяются функции, чьи **entry point** входят в
выделение. Выделение середины тела функции само по себе её не добавляет.

### Пример

Если нужно найти функции, которые очищают поле, можно искать:

```text
ptr->data[1] = '\\0'
```

Для вызовов с меняющимся именем подходит regex, например:

```text
set_string\(.*->.*\)
```

После выбора строк кнопка `Select Functions` создаёт обычное выделение entry
points. В исходнике это видно в `selectFunctions()` плагина: он берёт
`TextMatch.getFunction()` и добавляет `f.getEntryPoint()`.

## 2. Taint-анализ и data-flow

`DecompilerTaint` отвечает не за построение C-кода, а за вопрос: «может ли
значение из источника дойти до интересующего приёмника?»

Главный класс --
[`TaintPlugin.java`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/decompiler/taint/TaintPlugin.java).
Он требует `DecompilerHighlightService` и `DecompilerMarginService`, поэтому
может подсвечивать результаты прямо в окне Decompiler. Статус плагина в
`@PluginInfo` -- `UNSTABLE`: это важный сигнал, что интерфейс и поведение могут
меняться.

### Модель source/sink/gate

- **source** -- откуда начинается отслеживание, например параметр функции;
- **sink** -- конечная точка, например вызов опасной функции;
- **gate** -- узел, через который taint не должен проходить.

Эти типы определены в
[`TaintState.java`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/decompiler/taint/TaintState.java).
Там же описаны query-типы `SRCSINK`, `DEFAULT`, `CUSTOM`, загрузка SARIF,
построение индекса и экспортный скрипт. `ExtensionPoint.manifest` регистрирует
контракты `TaintState` и `DataTypeReferenceFinder`, поэтому реализация может
выбираться через механизм extension points, а не жёстко зашиваться в базовый
Decompiler.

### Реальный конвейер

Help-файл
[`DecompilerTaint.html`](../Ghidra/Features/DecompilerDependent/src/main/help/help/topics/DecompilerTaint/DecompilerTaint.html)
описывает четыре шага:

```text
Program -> экспорт PCode facts -> внешний индекс -> query source/sink
        -> SARIF/результаты -> подсветка и slice tree в Decompiler
```

Например, для поиска пути от пользовательского параметра до `memcpy` нужно:

1. Экспортировать p-code программы.
2. Построить индекс внешним движком.
3. Отметить параметр как source, а аргумент `memcpy` как sink.
4. Запустить query и применить результат к окну Decompiler.

`Decompiler` предоставляет SSA-представление p-code, а `DecompilerDependent`
сохраняет метки, вызывает внешний engine и переводит ответ обратно в подсветку.

## 3. Экспорт p-code для внешних инструментов

Пакет `.../decompiler/export` превращает не только p-code функций, но и
сопутствующую модель программы в набор фактов. Основные точки:

- [`ExportDecompilationTask.java`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/decompiler/export/ExportDecompilationTask.java)
  выбирает одну или все функции, запускает `ParallelDecompiler` и в конце
  записывает facts.
- [`ExportDecompilerConfigurer.java`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/decompiler/export/ExportDecompilerConfigurer.java)
  выбирает стиль `decompile`, потому что экспорту нужны `HighVariable`.
- [`PcodeExporter.java`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/decompiler/export/PcodeExporter.java)
  экспортирует регистры, address spaces, mnemonic names, строки, параметры,
  типы и vtables.
- [`ExportFunctionPcodeViaDecoderScript.java`](../Ghidra/Features/DecompilerDependent/ghidra_scripts/ExportFunctionPcodeViaDecoderScript.java)
  запускает экспорт функции под курсором и принимает каталог результата.

Упрощённый пример использования скрипта:

```text
1. Открыть функцию и поставить курсор внутрь неё.
2. Запустить ExportFunctionPcodeViaDecoderScript.java.
3. Передать каталог facts или выбрать его в диалоге.
4. Проанализировать facts внешним индексатором.
```

Важно отличать этот экспорт от обычного показа p-code в Ghidra. Обычный
Decompiler выдаёт результат для пользователя или Java-клиента; экспортёр
сериализует его вместе с типами и метаданными в формат, который ожидает другой
инструмент.

## 4. Уточнение `printf`/`scanf` сигнатур

[`FormatStringAnalyzer.java`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/variadic/FormatStringAnalyzer.java)
-- это анализатор Ghidra, а не новая стадия native-декомпиляции. Он находит
variadic-функции с char-pointer format string, выбирает функции, которые их
вызывают, декомпилирует их параллельно и читает `HighFunction.getPcodeOps()`.

Например, у вызова:

```c
printf("id=%d name=%s", x, name);
```

из format string можно вывести, что после обязательного format-параметра идут
`int` и указатель на строку. Для `scanf` направление обратное: аргументы -- это
адреса, куда записываются значения. Исходник различает эти случаи через
`isOutputType` и создаёт `DataType` для найденных спецификаторов.

Затем анализатор переопределяет параметры вызова. Здесь видна польза слоя
поверх декомпилятора: p-code нужен как источник фактических аргументов вызова,
а изменение сигнатуры улучшает последующие результаты анализа и отображение
Decompiler.

## 5. Поиск использований типов через AST

[`DecompilerDataTypeReferenceFinder.java`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/extension/datatype/finder/DecompilerDataTypeReferenceFinder.java)
реализует `DataTypeReferenceFinder`. Он:

1. строит lineage типа, включая связанные базовые типы;
2. предварительно отбирает функции, где тип может использоваться;
3. запускает `ParallelDecompiler`;
4. разбирает `ClangToken` и переменные результата;
5. возвращает ссылки на переменные, поля, параметры и return type.

Это отличается от простого поиска адресных ссылок. Например, обращение
`obj->field` может быть понятно только после восстановления высокоуровневой
переменной и типа. Классы `DecompilerReference`, `VariableAccessDR` и
`DecompilerFieldAccess` в том же пакете представляют найденные варианты.

## 6. Abstract interpretation и extension points

[`AbstractInterpretationService.java`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/decompiler/absint/AbstractInterpretationService.java)
содержит небольшой контракт сервиса для результатов внешнего движка
abstract interpretation. Сам по себе интерфейс не делает анализ: он задаёт
точку интеграции, чтобы специализированный плагин мог сообщить имя активного
query.

Это ещё один пример правильного разделения: базовый Decompiler знает, как
получить `HighFunction`, но не обязан знать формат внешнего solver/indexer.

## Чем `DecompilerDependent` не является

- Это не копия `src/decompile/cpp` и не второй native-декомпилятор.
- Это не место для описаний CPU и Sleigh: они находятся в
  [`Ghidra/Processors`](../Ghidra/Processors).
- Это не общий p-code framework: базовые модели находятся в
  [`Ghidra/Framework/SoftwareModeling`](../Ghidra/Framework/SoftwareModeling).
- Это не просто набор скриптов: значительная часть функциональности -- обычные
  плагины, анализаторы, сервисы, extension points и тесты.

Если нужно изменить алгоритм восстановления CFG, SSA, типов или C-печати,
начинать следует с `Decompiler`, особенно с его native-исходников и
`DecompInterface`. Если нужно добавить операцию над уже полученным результатом,
поиск по нему, экспорт или интеграцию с внешним анализатором, естественное место
-- `DecompilerDependent`.

## Практический маршрут изучения

1. Прочитать [`build.gradle`](../Ghidra/Features/DecompilerDependent/build.gradle),
   чтобы увидеть зависимости.
2. Посмотреть [`DecompilerTextFinderPlugin.java`](../Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/search/DecompilerTextFinderPlugin.java)
   как самый простой пример UI-плагина.
3. Изучить `FormatStringAnalyzer.java`, чтобы увидеть вызов
   `ParallelDecompiler` и обработку `HighFunction`.
4. Затем перейти к `ExportDecompilationTask.java` и `PcodeExporter.java`.
5. Для data-flow читать `TaintPlugin.java`, `TaintState.java` и help
   [`DecompilerTaint.html`](../Ghidra/Features/DecompilerDependent/src/main/help/help/topics/DecompilerTaint/DecompilerTaint.html).
6. Сопоставлять каждый вызов с базовым API в
   [`Ghidra/Features/Decompiler/src/main/java`](../Ghidra/Features/Decompiler/src/main/java)
   и с конвейером из [`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md).

Итоговая формула: `Decompiler` отвечает на вопрос «что означает машинный код и
как показать его как C», а `DecompilerDependent` -- «что ещё полезного можно
сделать, имея это восстановленное представление».
