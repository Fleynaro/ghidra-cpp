# Как Ghidra превращает x86 в C: путь через p-code

Этот документ объясняет путь, который проходит **одна функция**: от её байтов и
инструкций x86/x86-64 до текста, похожего на C. Это не восстановление исходника:
в машинном коде нет исходных имён, большинства типов, границ локальных переменных
и конструкций `if`/`while`. Декомпилятор строит наиболее обоснованную гипотезу.
Общую карту модулей проекта см. в [ARCHITECTURE_OVERVIEW_RU.md](ARCHITECTURE_OVERVIEW_RU.md).

## 1. Короткая карта пути

```text
байты функции
  -> Java SLEIGH: декодирует x86 и исполняет семантику из .slaspec/.sinc
  -> raw p-code: простые операции над регистрами, памятью и потоком управления
  -> packed callback: p-code по инструкции передаётся нативному процессу по запросу
  -> CFG: базовые блоки и рёбра переходов
  -> SSA: у каждого промежуточного значения одна точка определения
  -> правила упрощения, вывод типов, восстановление прототипа и структуры потока
  -> объединение SSA-значений в переменные
  -> PrintC: размеченные токены C, затем строки с отступами
  -> Java UI: C-текст, подсветка, навигация к инструкции и HighFunction
```

Основные места в дереве исходников:

| Слой | Где читать | Роль |
|---|---|---|
| Семантика x86 | [x86.slaspec](../Ghidra/Processors/x86/data/languages/x86.slaspec), [ia.sinc](../Ghidra/Processors/x86/data/languages/ia.sinc) | SLEIGH-описание байтов, операндов и эффекта инструкций. |
| Java SLEIGH и callback | [DecompileCallback.java](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompileCallback.java) | Берёт p-code у `Instruction` и отвечает на запросы нативного процесса. |
| ABI | [x86-64-win.cspec](../Ghidra/Processors/x86/data/languages/x86-64-win.cspec) | Правила аргументов, возврата и стека Windows x64. |
| Java-вход | [DecompInterface.java](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompInterface.java) | Создаёт процесс, посылает запросы и разбирает ответ. |
| Нативный вход | [ghidra_process.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/ghidra_process.cc) | Команда `decompileAt` запускает обработку `Funcdata`. |
| Внутренняя модель | [op.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/op.hh), [varnode.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/varnode.hh), [block.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/block.hh) | `PcodeOp`, `Varnode` и блоки графа потока. |
| Пайплайн действий | [coreaction.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/coreaction.cc) | Состав и порядок стандартного набора действий `decompile`. |
| SSA | [heritage.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/heritage.hh) | Построение SSA, `MULTIEQUAL` и обработка стека. |
| Вывод C | [printc.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/printc.hh), [prettyprint.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/prettyprint.hh) | Превращает структуру и выражения в C-токены и форматирует их. |

Самое полное встроенное описание стадий расположено в комментарии
[docmain.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/docmain.hh). Дальше оно
разобрано более простым языком и с одним сквозным примером.

## 2. Что подготовлено до вызова декомпилятора

Декомпилятор не стартует с «голых байтов файла». Перед этим Ghidra импортирует
файл в объект `Program`, а дизассемблер создаёт `Instruction`, память, функции,
символы, ссылки и типы. `Program` также содержит выбранные `Language` и
`CompilerSpec`.

Для x86 язык задан в [каталоге описаний x86](../Ghidra/Processors/x86/data/languages).
В частности, [x86-64.slaspec](../Ghidra/Processors/x86/data/languages/x86-64.slaspec)
подключает базовое описание, а `x86-64-win.cspec` говорит, что первые целочисленные
аргументы Windows x64 находятся в `RCX`, `RDX`, `R8`, `R9`, результат - в `RAX`.
Без этих правил `RCX` был бы лишь регистром, а не параметром `a`.

Пользователь открывает функцию в [DecompilePlugin.java](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/decompile/DecompilePlugin.java).
Провайдер UI вызывает `DecompInterface`. Его `openProgram()` проверяет, что язык
поддерживает p-code, приводит язык к `SleighLanguage`, создаёт
`PcodeDataTypeManager` и `DecompileCallback` ([DecompInterface.java](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompInterface.java#L373-L415)).

Важно: Java не вызывает C++ как библиотеку в той же JVM. Она запускает и держит
отдельный нативный процесс декомпилятора. `decompileFunction()` устанавливает
текущую функцию в callback, кодирует её entry address и отправляет команду
`decompileAt` ([DecompInterface.java](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompInterface.java#L769-L833)).
Нативная сторона читает этот адрес через `PackedDecode`, получает `Funcdata` из
таблицы символов и запускает выбранный набор действий
([ghidra_process.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/ghidra_process.cc#L277-L325)).

Нативный процесс не получает целиком Listing и не обязан сам повторно декодировать
x86-байты. Когда ему нужен p-code очередной инструкции, `GhidraTranslate` делает
обратный запрос к Java; `DecompileCallback.getPcode()` получает его от
`Instruction` через `getPrototype().getPcodePacked(...)`. См.
[DecompileCallback.java](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompileCallback.java)
и [ghidra_translate.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/ghidra_translate.cc).

Такое разделение объясняет две практические детали:

- Тайм-аут или отмена останавливают отдельный процесс, а не JVM.
- После изменения имени, типа или сигнатуры уже декомпилированной функции клиенту
  полезно вызвать `flushCache()`: нативная сторона кэширует символы, типы, строки
  и комментарии ([DecompInterface.java](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompInterface.java#L699-L725), [ghidra_process.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/ghidra_process.cc#L255-L265)).

## 3. SLEIGH: из x86-инструкции в raw p-code

### 3.1. Где фактически выполняется SLEIGH

SLEIGH-семантика x86 выполняется на Java-стороне Ghidra при создании p-code для
`Instruction`. C++-декомпилятор не заново разбирает x86-инструкции из образа
памяти: он запрашивает у Java готовый packed p-code по адресу. Поэтому путь
выглядит так:

```text
SleighLanguage + Instruction (Java): x86 bytes -> raw p-code
DecompileCallback / packed protocol: raw p-code -> native process
GhidraTranslate + FlowInfo (C++): raw p-code -> CFG и дальнейший анализ
```

Java использует выбранный `SleighLanguage`, поэтому `x86.slaspec` и `ia.sinc`
остаются источником семантики. Нативная сторона декодирует уже сериализованные
`<inst>`/`<op>` в собственные `PcodeOp` и `Varnode`.

На Java путь от скомпилированной Sleigh-спецификации к этому потоку проходит через
[SleighLanguage.java](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighLanguage.java),
[SleighInstructionPrototype.java](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighInstructionPrototype.java)
и [PcodeEmitPacked.java](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/PcodeEmitPacked.java).
Для Listing/API тот же шаблон может материализоваться в Java-массив `PcodeOp[]`
через [PcodeEmitObjects.java](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/PcodeEmitObjects.java),
но декомпилятор использует packed-вариант, чтобы не пересылать объекты JVM.

### 3.2. Почему нужен p-code

Машинная команда x86 одновременно кодирует адресацию, размер операнда, неявные
регистры и флаги. Анализировать сотни таких вариантов напрямую неудобно. SLEIGH
переводит их в небольшой, архитектурно-независимый язык переноса регистров
(RTL), где эффект полностью явен.

В нативной модели `PcodeOp` имеет упорядоченный список входов и максимум один
явный выход; его комментарий прямо фиксирует эту модель
([op.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/op.hh#L47-L62)). Значение
представляет `Varnode`: это конкретный диапазон байтов в пространстве регистров,
памяти, констант или временных значений.

Часто встречающиеся операции:

| p-code | Значение |
|---|---|
| `COPY out, in` | копирование регистра/константы/временного значения |
| `INT_ADD out, a, b` | целочисленное сложение |
| `INT_EQUAL out, a, b` | булево `a == b` |
| `LOAD out, space, address` | чтение памяти |
| `STORE space, address, value` | запись памяти |
| `CBRANCH target, condition` | условный переход |
| `BRANCH target` | безусловный переход |
| `CALL target, ...` / `RETURN ...` | вызов / возврат |
| `MULTIEQUAL out, a, b, ...` | phi-узел SSA, добавляется декомпилятором, не процессором |

Операции привязаны к адресу инструкции через `SeqNum`; это видно по полю `start`
в `PcodeOp` и методу `getAddr()` ([op.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/op.hh#L123-L164)).
Именно эта связь позднее позволяет подсветить в листинге инструкцию, породившую
фрагмент C-кода.

### 3.3. Где лежит семантика x86

SLEIGH-правило имеет две части: условие распознавания байтов после `is` и блок
в фигурных скобках с семантикой. Например, вариант `MOV Reg32,rm32` описан в
[ia.sinc](../Ghidra/Processors/x86/data/languages/ia.sinc#L4002) как присваивание
`Reg32 = rm32`; его эффект - p-code `COPY` или `LOAD` плюс `COPY` в зависимости
от того, был ли `rm32` памятью.

Для `ADD Reg32,rm32` тот же файл вызывает `addflags`, складывает операнды и
вычисляет флаги через `resultflags`
([ia.sinc](../Ghidra/Processors/x86/data/languages/ia.sinc#L2557)). Поэтому одна
машинная инструкция `add` создаёт не только результат, но и p-code для `ZF`,
`CF`, `SF`, `OF` и других флагов.

`CMP` не сохраняет разность, но его Sleigh-семантика всё равно вычисляет временную
разность и флаги: см. `CMP Reg32,rm32` в
[ia.sinc](../Ghidra/Processors/x86/data/languages/ia.sinc#L3046-L3049).
Следующий условный переход использует нужный флаг, а правила упрощения затем
восстанавливают удобное сравнение C.

`RET` также раскрывается: в long mode правило извлекает адрес из стека в `RIP` и
выполняет `return [RIP]` ([ia.sinc](../Ghidra/Processors/x86/data/languages/ia.sinc#L4600)).
Позже декомпилятор скрывает технический адрес возврата и добавляет явное
возвращаемое значение.

### 3.4. Raw p-code - ещё не C

Raw p-code сохраняет машинную точность. Он знает, что записан `ZF`, но не обязан
сразу знать, что это была проверка `a == 0`; он видит адрес стека, но ещё не
обязан назвать его локальной переменной. Сначала декомпилятор проходит поток
инструкций от entry point, ставит в очередь адреса новых ветвей и останавливается,
когда новых адресов не остаётся. Это описано в
[docmain.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/docmain.hh#L132-L158).

## 4. Протокол между Java и нативным декомпилятором

Процесс `decompile` использует stdin/stdout и двунаправленный packed-протокол.
При `openProgram()` Java регистрирует processor/compiler specs, пространства
адресов, core types и настройки. Команда `decompileAt` переносит entry address,
а C++ затем лениво запрашивает Java p-code, байты памяти, символы, references,
типы и комментарии. Команды расположены в
[ghidra_process.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/ghidra_process.cc),
а Java-backed architecture и протокол - в
[ghidra_arch.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/ghidra_arch.cc).

Логические элементы `<inst>`, `<op>` и `<varnode>` не передаются текстовым XML:
pipe использует компактное двоичное кодирование `PackedEncode`/`PackedDecode`.
Его Java-реализации находятся в
[PackedEncode.java](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/PackedEncode.java)
и [PackedDecode.java](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/pcode/PackedDecode.java).
Это также объясняет необходимость `flushCache()` после изменений в `Program`.

## 5. Сквозной пример

Возьмём специально маленькую функцию Windows x64. Предположим, исходная идея
была такой:

```c
int add_if_positive(int a, int b)
{
    int sum = a + b;
    if (sum > 0)
        return sum;
    return 0;
}
```

Один из реалистичных вариантов машинного кода, без пролога, выглядит так:

```asm
; RCX = a, RDX = b, RAX = return value
140001000  mov eax, ecx
140001002  add eax, edx
140001004  test eax, eax
140001006  jle 14000100c
140001008  ret
14000100c  xor eax, eax
14000100e  ret
```

Дальше показан **упрощённый, учебный** raw p-code. Реальный вывод может содержать
больше `SUBPIECE`, `INT_ZEXT` и операций флагов из-за точного размера регистров.
Смысл и ключевые зависимости остаются теми же.

```text
; 140001000: mov eax, ecx
EAX:4 = COPY RCX:4

; 140001002: add eax, edx
t0:4  = INT_ADD(EAX:4, RDX:4)
EAX:4 = COPY t0:4
ZF:1  = INT_EQUAL(t0:4, 0:4)
SF:1  = INT_SLESS(t0:4, 0:4)
OF:1  = INT_SCARRY(EAX_before:4, RDX:4)

; 140001004: test eax,eax
t_test:4 = INT_AND(t0:4, t0:4)
ZF:1     = INT_EQUAL(t_test:4, 0:4)
SF:1     = INT_SLESS(t_test:4, 0:4)
OF:1     = COPY 0:1

; 140001006: jle negative_or_zero
t1:1 = BOOL_OR(ZF:1, BOOL_XOR(SF:1, OF:1))
CBRANCH 14000100c, t1:1

; 140001008
RETURN t0:4

; 14000100c: xor eax,eax
EAX:4 = COPY 0:4
; 14000100e
RETURN 0:4
```

В этом фрагменте видно важное различие: `EAX` - физический регистр, `t0` -
временное p-code значение, а `sum` появится существенно позднее как
высокоуровневая переменная. Знание ABI связывает входной `RCX:4` с первым
параметром `a`, `RDX:4` - с `b`, а `EAX:4` перед `RETURN` - с `int`-результатом.
После распространения константы `OF = 0` и свёртки флаговых проверок выражение
`BOOL_OR(ZF, SF != OF)` становится понятным условием `t0 <= 0`.

## 6. CFG: из переходов в граф базовых блоков

После построения raw p-code декомпилятор режет его на **базовые блоки**. Блок -
это последовательность p-code операций с одним входом и без перехода внутри;
последняя операция определяет исходящие рёбра. Это блоки именно p-code, не
машинных инструкций, потому что одна инструкция создаёт несколько `PcodeOp`.
Документация реализации прямо отмечает это различие и нормализует единственный
стартовый блок ([docmain.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/docmain.hh#L160-L170)).

Для примера CFG такой:

```text
          [B0: mov, add, test, jle]
             /                 \
   true: <= 0 /                   \ false: > 0
           v                       v
 [B1: eax=0; return]       [B2: return sum]
```

У базовых блоков есть специальные подтипы для восстановленных конструкций
(`BlockIf`, `BlockWhileDo`, `BlockSwitch` и т.д.); их семейство определено в
[block.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/block.hh). Пока это
лишь низкоуровневый граф: направление рёбер ещё не является `if`.

Перед основными преобразованиями декомпилятор также изучает вызовы: ищет прямую
вызываемую функцию в базе, использует её сигнатуру, а для неизвестной цели
применяет default prototype. Эта стадия описана в
[docmain.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/docmain.hh#L172-L183).

## 7. SSA: почему один регистр превращается в много значений

Один `EAX` в машинном коде многократно перезаписывается. Для анализа это
неудобно: непонятно, какая запись читает конкретное использование. SSA
(*Static Single Assignment*) создаёт отдельную версию для каждой записи:

```text
eax_1 = a
eax_2 = eax_1 + b
if (eax_2 <= 0) goto B1
return eax_2
B1:
eax_3 = 0
return eax_3
```

Если две ветви потом сходятся, SSA добавляет `MULTIEQUAL` (обычно называемый
phi-узлом). Например, если бы функция после ветки возвращала `eax`, граф
содержал бы:

```text
eax_4 = MULTIEQUAL(eax_3 from B1, eax_2 from B2)
RETURN eax_4
```

Это не исполняемая CPU-инструкция и не печатается как `phi` в C. Это запись о том,
какое значение выбрать в зависимости от вошедшего ребра.

Класс `Heritage` строит SSA: размещает `MULTIEQUAL` через доминаторы и выполняет
переименование. Его комментарий также объясняет, почему построение может идти
несколькими проходами: сначала понятнее поток регистров, затем обнаруженные
ссылки на стек повышаются из `LOAD`/`STORE` до обычных `Varnode`
([heritage.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/heritage.hh#L172-L205)).

## 8. Основной цикл: не один алгоритм, а набор повторяемых действий

Профиль по умолчанию называется `decompile`. В Java его выбирают через
`setSimplificationStyle("decompile")`; также есть `normalize`, `firstpass` и
`paramid` ([DecompInterface.java](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompInterface.java#L447-L503)).
Состав групп профиля задан в
[coreaction.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/coreaction.cc#L5792-L5830),
а реальный порядок действий - в `ActionDatabase::universalAction()`
([coreaction.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/coreaction.cc#L5833-L6123)).

Главный и полный циклы повторяют работу, пока преобразования ещё меняют функцию.
На практике наиболее важны такие части:

| Действие | Что делает | Результат в примере |
|---|---|---|
| `ActionHeritage` | строит/достраивает SSA | `EAX` разделён на `eax_1`, `eax_2`, `eax_3`. |
| `ActionDeadCode` | выкидывает неиспользуемые эффекты | промежуточные флаги x86 исчезают, если нужны лишь для уже восстановленного условия. |
| `ActionInferTypes` | распространяет типовые ограничения | 32-битные значения становятся кандидатами на `int`; ABI закрепляет тип параметров и результата. |
| `Rule*` в `ActionPool` | локально переписывает дерево выражений | цепочка вычислений флагов превращается в `sum <= 0`. |
| `ActionRedundBranch`, `ActionUnreachable` | удаляют лишние ветви/блоки | убирают доказанно недостижимый поток. |
| `ActionBlockStructure` | распознаёт структуры потока | пара блоков становится `if`, а обратное ребро может стать `while`. |
| `ActionSwitchNorm` | нормализует switch/jump table | косвенный переход получает шанс стать `switch`. |

Например, `ActionHeritage`, dead-code elimination и `ActionInferTypes` добавляются
в основную петлю в
[coreaction.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/coreaction.cc#L5860-L5881),
а пул правил содержит, среди прочего, правила устранения копий, арифметических и
булевых упрощений ([coreaction.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/coreaction.cc#L5882-L6023)).

Это отвечает на частый вопрос «куда делись `ZF` и `SF`?». Они не игнорируются:
SLEIGH создал точную семантику, затем правило распознало зависимость перехода,
а лишние промежуточные вычисления стали мёртвыми. Поэтому результат `if (sum > 0)`
сохраняет наблюдаемое поведение, но не копирует внутреннее состояние флагов CPU.

## 9. Из CFG в `if`, `while`, `switch` и иногда `goto`

Структурирование анализирует доминаторы, точки слияния и обратные рёбра CFG.
Оно не знает исходный текст, но может распознать типичные формы:

```text
ветка на один блок + общий выход        -> if / if-else
ребро назад к условию                   -> while / do-while / for
косвенный переход через таблицу         -> switch
переход, который нельзя структурировать -> goto + label
```

Для примера `jle B1` можно напечатать в прямом виде:

```c
if (sum <= 0) {
    return 0;
}
return sum;
```

Или, выбрав более удобный противоположный предикат, как исходная версия:

```c
if (sum > 0) {
    return sum;
}
return 0;
```

Оба варианта эквивалентны машинному коду. Финальная перестройка упорядочивает
компоненты и cases, помечает `break` и ставит метки только для оставшихся
неструктурируемых переходов
([docmain.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/docmain.hh#L401-L406)).
Если виден `goto`, это не автоматически ошибка: исходный код мог его содержать,
либо оптимизация компилятора создала CFG, который нельзя выразить без потери
точности обычными конструкциями C.

## 10. Выход из SSA и восстановление переменных

SSA удобна машине, но C-пользователь не хочет видеть `eax_17`. После основных
правил декомпилятор:

1. Устраняет `MULTIEQUAL` и `INDIRECT`, объединяя совместимые значения либо
   вставляя `COPY`.
2. Решает, что оставить отдельной локальной переменной, а что встроить в выражение.
3. Пытается дополнительно объединить совместимые низкоуровневые значения, не
   позволяя одной переменной хранить два живых значения одновременно.
4. Добавляет необходимые приведения типов.
5. Восстанавливает прототип, имена параметров, локальных и глобальных переменных.

Эти стадии подробно перечислены в
[docmain.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/docmain.hh#L345-L399).
В реализации после cleanup идут `ActionAssignHigh`, обязательные и эвристические
merge-действия, `ActionOutputPrototype`, `ActionInputPrototype`, `ActionNameVars`
и `ActionSetCasts` ([coreaction.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/coreaction.cc#L6094-L6121)).

В нашем примере `eax_2` получает роль `sum`, потому что значение переживает
вычисление условия и является значимым для `return`. Но при другой оптимизации
декомпилятор вправе напечатать `return a + b;` в одной ветви, вообще не создавая
`sum`. Это не потеря информации, а решение о читаемости.

## 11. Печать C и возврат результата в Java

После анализа `PrintC` создаёт не строку, а поток размеченных токенов: у него есть
правила для выражений, операторов, объявлений, прототипов, `if/else`, циклов и
`switch` ([printc.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/printc.hh#L56-L65)).
Токены знают синтаксическую роль, а большинство также связано с адресом исходной
машинной инструкции. Затем pretty-printer выбирает переносы и отступы; это
объясняет основная документация
([docmain.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/docmain.hh#L408-L420)).

`DecompileAt::rawAction()` сначала кодирует `Funcdata`, если включено дерево,
а затем вызывает `print->docFunction(fd)`, если нужен C-код
([ghidra_process.cc](../Ghidra/Features/Decompiler/src/decompile/cpp/ghidra_process.cc#L306-L325)).
Результат возвращается в packed XML-подобном документе, а не в виде одной строки.

На Java-стороне `DecompileResults` читает его в два представления:

- `HighFunction`: восстановленная высокоуровневая модель p-code, символов и
  переменных;
- `ClangTokenGroup`: дерево размеченных C-токенов.

Это видно в [DecompileResults.java](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompileResults.java#L26-L50)
и в его `decodeStream()` ([DecompileResults.java](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompileResults.java#L211-L260)).
`getDecompiledFunction()` прогоняет токены через `PrettyPrinter` и отдаёт обычный
текст C и отдельную сигнатуру
([DecompileResults.java](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompileResults.java#L195-L209)).

Итог для нашего примера может быть таким:

```c
int add_if_positive(int a,int b)

{
  int sum;

  sum = a + b;
  if (0 < sum) {
    return sum;
  }
  return 0;
}
```

Имена `add_if_positive`, `a`, `b` и `sum` показаны для обучения. В реальном
бинарнике при отсутствии символов Ghidra обычно предложит автоматически созданные
имена, например `FUN_140001000`, `param_1`, `param_2`, `iVar1`.

## 12. Как проверять каждую стадию в Ghidra

Для отладки не нужно верить C-тексту на слово. Полезный порядок проверки:

1. В Listing проверьте, что границы функции и инструкция перехода определены
   корректно.
2. Откройте p-code конкретной инструкции и убедитесь, что Sleigh-семантика
   действительно описывает её эффект. Для неизвестной инструкции ищите правило в
   [ia.sinc](../Ghidra/Processors/x86/data/languages/ia.sinc) или подключаемых
   `*.sinc` из [каталога x86](../Ghidra/Processors/x86/data/languages).
3. Сверьте CFG: оба исхода условного перехода должны соответствовать двум рёбрам.
4. В декомпиляторе используйте подсветку токена и переход к листингу: привязка
   токенов к адресам создана именно для этого.
5. Если тип или аргументы неверны, сначала исправьте signature/calling convention
   или тип переменной, затем декомпилируйте повторно. ABI и типы меняют последующие
   выводы сильнее, чем косметическое переименование.
6. Если C выглядит подозрительно, переключитесь на `normalize`: этот режим не
   выполняет часть восстановления типов и финальной подготовки C, зато полезен
   для более близкого к data-flow дерева
   ([DecompInterface.java](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompInterface.java#L455-L470)).

## 13. Границы точности

Декомпилятор хорошо сохраняет семантику наблюдаемого поведения, но не может
достоверно вернуть информацию, уничтоженную компилятором. Несколько типичных
следствий:

| В C-окне | Возможная причина |
|---|---|
| `undefined4`, `longlong`, странный cast | нет достаточных типов или конфликтуют способы использования значения |
| `local_res8`, `uVar3` | нет исходного имени; переменная выделена из временного регистра/стека |
| `goto` | CFG не удалось безопасно представить структурными конструкциями |
| много обращений через указатели | неизвестный alias, непрямой вызов или нераспознанная структура |
| неверные параметры функции | ошибочно выбрана функция, ABI, signature или прототип вызываемой функции |

Поэтому правильный рабочий цикл обратной разработки такой: проверить инструкции
и p-code, улучшить границы функции/типы/сигнатуры/имена, затем снова прочитать
результат. Нативный конвейер специально использует данные программы, типы и
прототипы на нескольких стадиях, а не только один раз в начале
([docmain.hh](../Ghidra/Features/Decompiler/src/decompile/cpp/docmain.hh#L269-L305)).
