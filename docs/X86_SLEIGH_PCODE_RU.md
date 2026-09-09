# Как Ghidra переводит x86 в P-Code

Этот документ объясняет устройство поддержки x86 в репозитории и путь от байтов
машинной инструкции до P-Code. Цель -- дать новичку маршрут по исходникам, а не
описать весь синтаксис Intel. Общая карта модулей находится в
[`ARCHITECTURE_OVERVIEW_RU.md`](ARCHITECTURE_OVERVIEW_RU.md), а следующий уровень
изучения полного конвейера декомпиляции -- в
[`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md).

## 1. Короткий ответ

Преобразование не зашито в отдельный Java-метод `mov`. Оно описано декларативно
на языке SLEIGH:

```text
байты инструкции
  -> выбранный Language из x86.ldefs
  -> таблица конструкторов, скомпилированная из .slaspec/.sinc в .sla
  -> распознанные операнды и их Varnode
  -> тело конструктора (семантика SLEIGH)
  -> raw P-Code
```

Для инструкции

```asm
mov dword ptr [rcx + 0x10], edx
```

смысл можно записать так:

```text
tmp = RCX + 0x10
STORE ram, tmp, EDX
```

В реальном выводе могут присутствовать дополнительные временные Varnode,
операция сегмента, адрес инструкции и служебная информация. Но существенный
эффект именно такой: вычислить адрес и записать 4 байта из `EDX` в память. P-Code
не является текстовым ассемблером и не обязан выглядеть как ровно две строки.

## 2. Что находится в `Processors/x86`

Главная папка поддержки процессора --
[`Ghidra/Processors/x86`](../Ghidra/Processors/x86). В ней есть несколько
разных слоев:

| Путь | Назначение |
|---|---|
| [`data/languages`](../Ghidra/Processors/x86/data/languages) | Языки процессора, регистры, декодирование инструкций и их семантика. |
| [`data/patterns`](../Ghidra/Processors/x86/data/patterns) | Шаблоны прологов и характерных последовательностей для поиска функций. |
| [`data/manuals`](../Ghidra/Processors/x86/data/manuals) | Индекс справочных материалов по инструкциям. |
| [`src/main/java/ghidra/app/plugin/core/analysis/X86Analyzer.java`](../Ghidra/Processors/x86/src/main/java/ghidra/app/plugin/core/analysis/X86Analyzer.java) | Анализ констант и ссылок, специфичный для x86. |
| [`src/main/java/ghidra/program/emulation/X86PcodeUseropLibraryFactory.java`](../Ghidra/Processors/x86/src/main/java/ghidra/program/emulation/X86PcodeUseropLibraryFactory.java) | Реализация пользовательских P-Code-операций для эмуляции, например `LOCK` и `UNLOCK`. |
| [`src/test.processors`](../Ghidra/Processors/x86/src/test.processors) | Эмуляционные и CompilerSpec-тесты x86/x64. |

Важно не смешивать эти слои. `X86Analyzer` использует уже созданные инструкции и
их P-Code, но не является местом, где описано значение `MOV`. Семантика обычных
инструкций находится в SLEIGH-файлах.

## 3. Как выбирается описание языка

Точкой входа для выбора языка служит
[`x86.ldefs`](../Ghidra/Processors/x86/data/languages/x86.ldefs). В нем описаны
идентификаторы языков, размер адреса, endian, файл процессора и CompilerSpec.
Например, запись `x86:LE:64:default` указывает:

```xml
slafile="x86-64.sla"
processorspec="x86-64.pspec"
```

и связывает язык с `x86-64-win.cspec` для Windows или
`x86-64-gcc.cspec` для GCC. Там же находятся 16-, 32-битный и compat32-варианты.

Файл [`x86-64.slaspec`](../Ghidra/Processors/x86/data/languages/x86-64.slaspec)
задает `IA64` и подключает базовый
[`x86.slaspec`](../Ghidra/Processors/x86/data/languages/x86.slaspec). Поэтому
основной маршрут для long mode такой:

```text
x86-64.slaspec
  -> x86.slaspec
  -> ia.sinc
  -> avx*.sinc, bmi*.sinc, adx.sinc и другие расширения
```

`.slaspec` -- композиция языка, а `.sinc` -- подключаемые фрагменты с
конструкторами. Для обычного `MOV` нужен прежде всего
[`ia.sinc`](../Ghidra/Processors/x86/data/languages/ia.sinc).

CompilerSpec не декодирует байты. Он описывает ABI и организацию типов: например,
в [`x86-64-win.cspec`](../Ghidra/Processors/x86/data/languages/x86-64-win.cspec)
заданы размер указателя 8, `RSP` как stack pointer, аргументы в `RCX`, `RDX`,
`R8`, `R9` и результат в `RAX`. Это влияет на последующее понимание функции,
но не меняет базовый эффект `mov [rcx+10], edx`.

## 4. Что такое правило SLEIGH

Упрощенная форма конструктора выглядит так:

```text
:MNEMONIC operands is битовые_условия_распознавания {
    семантика;
}
```

Левая часть после `is` отвечает на вопрос «какие байты и поля ModR/M подходят»,
а тело в фигурных скобках -- «что инструкция делает». Например, для чтения из
`r/m32` в регистр есть правило:

```text
:MOV Reg32,rm32 is ... byte=0x8b; rm32 & Reg32 ... {
    Reg32 = rm32;
    build check_Reg32_dest;
}
```

Оно находится в [`ia.sinc`](../Ghidra/Processors/x86/data/languages/ia.sinc#L4000-L4004).
Для записи регистра в память используется парное правило `MOV^xrelease m32,Reg32`,
в котором операция выбора памяти и присваивание разделены через `build`:

```text
:MOV^xrelease m32,Reg32 is ... byte=0x89; m32 & Reg32 ... {
    build xrelease;
    build m32;
    m32=Reg32;
}
```

Оно находится в [`ia.sinc`](../Ghidra/Processors/x86/data/languages/ia.sinc#L3980-L3987).
`xrelease` нужен для вариантов с соответствующим префиксом/семантикой; для
обычного `MOV` выбирается обычная ветка этого конструктора. `build m32` раскрывает
вложенный конструктор операнда памяти.

Размер памяти задается не названием инструкции, а экспортом операнда:

```text
m8  -> export *:1 Mem
m16 -> export *:2 Mem
m32 -> export *:4 Mem
m64 -> export *:8 Mem
```

Это видно в [`ia.sinc`](../Ghidra/Processors/x86/data/languages/ia.sinc#L1155-L1158).
Звездочка означает обращение по адресу в экспортированном Varnode, а число --
размер доступа в байтах.

## 5. Как разбирается адрес `[rcx + 0x10]`

В x86 адресация кодируется полями ModR/M, а иногда еще SIB. Эти токены описаны
в [`ia.sinc`](../Ghidra/Processors/x86/data/languages/ia.sinc#L535-L657).

Для примера возьмем байты:

```text
89 51 10
```

Это `mov dword ptr [rcx + 0x10], edx` в 64-битном режиме:

| Байты/поле | Значение |
|---|---|
| `0x89` | Код `MOV r/m32, r32`, то есть направление регистр -> r/m. |
| `0x51` | ModR/M: `mod=01`, `reg=010`, `r/m=001`. |
| `mod=01` | База плюс знаково расширенное 8-битное смещение. |
| `reg=010` | Источник `EDX`. |
| `r/m=001` | База `RCX` в long mode без REX.B. |
| `0x10` | `disp8`, равный `+0x10`. |

Правило `addr64` для `mod=1` и обычной базы выбирает именно альтернативу:

```text
addr64: [Base64 + simm8_64]
    is mod=1 & r_m=4; ... { local tmp=...; export tmp; }
```

Для `r/m=001` фактическая таблица регистров подставляет `RCX`, а для `disp8`
экспортируется 64-битная знаково расширенная константа. Набор альтернатив адресации
находится в [`ia.sinc`](../Ghidra/Processors/x86/data/languages/ia.sinc#L1085-L1105).
В данном случае SLEIGH получает эффективный адрес:

```text
tmp_addr = RCX + 0x10
```

Если был бы `mod=0`, адрес мог бы быть просто `RCX`; если присутствовал бы `r/m=4`,
разбиралось бы SIB-выражение вроде `Base + Index*Scale + displacement`.

Регистры и их перекрывающиеся представления также заданы в `ia.sinc`: в 64-битном
режиме `RCX` имеет размер 8, а `ECX` -- размер 4; аналогично `EDX` является
четырехбайтовым представлением `RDX`. Начать изучение этой таблицы можно с
[`ia.sinc`](../Ghidra/Processors/x86/data/languages/ia.sinc#L13-L31).

## 6. Пошаговое построение P-Code для примера

### Шаг 1. Выбирается конструктор

Для байтов `89 51 10` выбран long-mode язык из `x86.ldefs`. Условие с opcode
`0x89`, `opsize=1`, `mod!=3` и операндами `m32,Reg32` соответствует записи
`MOV^xrelease m32,Reg32` в `ia.sinc`.

Здесь `mod!=3` существенно: `mod=3` означал бы регистровый пункт назначения,
например `mov ecx, edx`, а не память.

### Шаг 2. Раскрывается `m32`

`m32` ссылается на `Mem`, а `Mem` в long mode выбирает `addr64`. `addr64`
вычисляет адресную часть `RCX + 0x10` и экспортирует ее как адресный Varnode.
Затем `m32` добавляет размер `4` байта.

### Шаг 3. Выполняется присваивание SLEIGH

Строка `m32=Reg32` означает запись значения источника в memory Varnode. На
уровне общего P-Code это выражается операцией `STORE`, а не `COPY`:

```text
tmp_addr = INT_ADD(RCX, 0x10)
STORE ram, tmp_addr, EDX
```

Более близкая к форме P-Code запись с явными выходами может выглядеть так:

```text
unique_addr:8 = INT_ADD RCX, 0x0000000000000010
STORE ram, unique_addr:8, EDX:4
```

Имена `unique_addr` и точная нумерация временных значений условны: Ghidra может
назвать Varnode иначе или встроить константу в конкретное место. Не меняется
главное:

```text
операция   = STORE
пространство = ram
адрес      = RCX + 0x10
размер     = 4
значение   = EDX
```

Упрощенная C-подобная интерпретация этого P-Code:

```c
*(uint32_t *)(RCX + 0x10) = EDX;
```

Это еще не декомпилированный C-код: типы `uint32_t`, указатель и имена переменных
появятся только на последующих стадиях анализа.

### Шаг 4. Что будет с `RDX`

Запись через `EDX` имеет размер 4. Это не запись 8 байт через `RDX`. На настоящем
x86 запись в `EDX` также обнуляет старшую половину `RDX`, но данная инструкция не
записывает `EDX`, поэтому ее P-Code не должен добавлять операцию изменения `RDX`.
Правило `MOV m32,Reg32` использует именно четырехбайтовый источник и четырехбайтовый
доступ к памяти.

Для контраста:

```asm
mov qword ptr [rcx + 0x10], rdx
```

будет иметь размер `m64` и в P-Code записывать 8 байт из `RDX`. А

```asm
mov ecx, edx
```

будет использовать регистровую ветку с `mod=3` и даст `COPY`/частичный регистровый
эффект, а не `STORE`.

## 7. Где Java превращает шаблон в P-Code

Файлы SLEIGH сначала компилируются инструментами Ghidra в `.sla`; при работе
загруженный `SleighLanguage` строит прототип инструкции и хранит выбранное дерево
конструкторов. Упрощенный Java-путь такой:

1. [`SleighLanguage.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighLanguage.java) разбирает байты и выбирает `SleighInstructionPrototype`.
2. [`SleighInstructionPrototype.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighInstructionPrototype.java#L1040-L1069) создает `ParserWalker`, `PcodeEmitPacked` и вызывает `emit.build(...)` для семантического шаблона.
3. [`ParserWalker.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/ParserWalker.java#L20-L33) описывает обход дерева конструкторов и разрешение их экспортов.
4. [`PcodeEmit.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/PcodeEmit.java) и [`PcodeEmitPacked.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/PcodeEmitPacked.java) превращают операции шаблона в P-Code и упаковывают их для передачи.

Есть два полезных режима получения результата. `PcodeEmitObjects` материализует
массив объектов `PcodeOp[]` для Java API, а `PcodeEmitPacked` сериализует результат
компактно. Декомпилятор обычно использует packed-вариант, чтобы передавать данные
между JVM и отдельным нативным процессом без лишнего копирования объектов.

Запрос декомпилятора проходит через
[`DecompileCallback.java`](../Ghidra/Features/Decompiler/src/main/java/ghidra/app/decompiler/DecompileCallback.java#L211-L232): callback находит `Instruction` и вызывает
`instr.getPrototype().getPcodePacked(...)`. Поэтому C++-часть декомпилятора не
разбирает `89 51 10` заново. Она получает уже построенные операции через callback.
Подробный маршрут Java/C++ описан в
[`GHIDRA_DECOMPILER_FLOW_RU.md`](GHIDRA_DECOMPILER_FLOW_RU.md#3-sleigh-из-x86-инструкции-в-raw-p-code).

## 8. Где смотреть результат в Ghidra

В интерфейсе можно выбрать инструкцию и открыть окно Listing с P-Code через
контекстное меню `P-code`/`Instruction Pcode` (название зависит от версии и
контекста). Для программной проверки полезны Java API `Instruction.getPcode()` и
скрипт-пример
[`ExportFunctionPcodeViaDecoderScript.java`](../Ghidra/Features/DecompilerDependent/ghidra_scripts/ExportFunctionPcodeViaDecoderScript.java).

Полезно различать три представления:

| Представление | Что в нем видно |
|---|---|
| Дизассемблер | Мнемоника и операнды, например `MOV dword ptr [RCX + 0x10],EDX`. |
| Raw P-Code | `INT_ADD`, `STORE`, регистровые Varnode и временные значения. |
| Декомпилированный C | Гипотеза о типах, переменных и структуре функции. |

P-Code из Listing и P-Code, который нативный декомпилятор получает по packed
протоколу, имеют одну семантическую основу, но формат отображения и набор
служебных деталей могут отличаться.

## 9. Что читать дальше

Практический порядок изучения:

1. [`x86.ldefs`](../Ghidra/Processors/x86/data/languages/x86.ldefs) -- какой язык выбран для 16/32/64 бит.
2. [`x86-64.slaspec`](../Ghidra/Processors/x86/data/languages/x86-64.slaspec) и [`x86.slaspec`](../Ghidra/Processors/x86/data/languages/x86.slaspec) -- как собираются фрагменты языка.
3. [`ia.sinc`](../Ghidra/Processors/x86/data/languages/ia.sinc) -- регистры, токены ModR/M и SIB, адресация, размеры операндов и семантика инструкций.
4. [`SleighInstructionPrototype.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/SleighInstructionPrototype.java) -- как выбранный конструктор исполняется для конкретных байтов.
5. [`PcodeEmitPacked.java`](../Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/app/plugin/processors/sleigh/PcodeEmitPacked.java) -- как P-Code упаковывается.
6. [`op.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/op.hh) и [`varnode.hh`](../Ghidra/Features/Decompiler/src/decompile/cpp/varnode.hh) -- как эти операции представлены внутри нативного декомпилятора.

Главный ответ на вопрос «где описано преобразование x86 в P-Code?» состоит из
двух частей: **семантика** инструкции описана в `ia.sinc` и подключаемых `.sinc`,
а **исполнение этого описания** реализовано общим SLEIGH-кодом в
`SleighInstructionPrototype` и `PcodeEmit*`. Для `mov [rcx+0x10], edx` начинать
нужно с правил `MOV^xrelease m32,Reg32`, затем перейти к `m32`, `Mem` и
`addr64` в том же `ia.sinc`.
