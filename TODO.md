3) high pcode
5) p-code emulator
10) >>> причесывать проект, убирать битые/внешние ссылки, насытить readme.md, унифицировать (те же тесты + test data)
    1) в исходниках тоже могут быть ссылки на всякие файлы из Ghidra (например analyzers)
    2) в тестах нужно сделать обертки! особенно в тестах декомпиляции, там где мы указывали огромные классы провайдеры. нужно кароче
    3) имена бы попдправтиь местами
12) >>> юзая прошлые сессии kilo code нужно сделать подробные README.md разработки со всеми нюансами и т.д. То есть контекст сессий хранит важную операционную инфу - ее надо сохранить отдельно


1) const prop почему то странно робит
2) по моему порт вообще неверный был: function body - это ваще откуда?
3) куча СВОИХ тестов допом
5) по одному делать анализатор, чтобы он лучше фокусировался
6) начать делать TTD модуль с интерфейсом своим и т.д. для анализа
7) analyeers
    1) реализовать остальные и усилить максимально тесты
    2) обязательно запустить тест на ВСЕ анализаторы. Нужно новую папку сделать, туда же - analyzer_builtin
    3) на GTA5.exe провериь, причем можно либо агенту дать доступ к гидре, а лучше все таки заранее сформировать .md отчет и сделать выборку по функциям и т.д.
        1) Хотя эту задачу можно оставить на ПОТОМ. Когда сделаем все остальное. Можно подключить сильного агента (terra/sol). Пока не зацикливаемся. Щас пускай тестовые .exe файлы умеет делать

8) свой SoftwareModling на event-sourcing моделе с сохранением для объединения всеъ этих модулей воедино! Причем сохранялка тут отдельно работает.
    1) Что важно, тут придется хочешь не хочешь а переносить в software modeling все shared классы, т.е инверсия зависимостей. Иначе эта хуйня не будет расширяться.
    2) окерарстратор модель из микросервисов? 
    3) 
9) начать делать API для нашего VS Code расширения и уже запихивать. Далее по GTA5 будем уже смотреть че где не так. Искать расхождения и давать агенту исправлять, ужесточяя тесты и т.д.
10) фокус на GUI и на TTD + Debugger также

11) скорость анализаторов долгая - 3 минуты
12) свой векторный FUNCTION ID (QWEN). С нуля сделать либу свою и бд функций с векторами и сигнатурами






Задача #1
1) resize колонок в сторону уменьшения должен переносить контент по возможности на новую строку, это касается например hex bytes
2) заголовок функции в листинге ПЕРЕКРЫВАЕТ первую инструкцию функции - нужно исправить это. Заголовок должен быть НАД инструкцией первой а не на ней самой
3)





Задача #2
Optimize the current Listing View scrolling performance. The Listing View already works, but scrolling/jumping to a random part of a large image currently causes about ~1 second of latency. This is not acceptable.

Your goal is to make random scrolling/jumping effectively instant, especially on very large binaries.

Use Ghidra’s architecture as the conceptual reference. Do NOT blindly copy its implementation, but follow the same core idea:

* Do not rebuild/render the whole listing for the entire image.
* Treat the listing as a large logical indexed document.
* Keep an efficient mapping between address ↔ logical instruction/listing index so jumping to an arbitrary address does not require scanning from the beginning.
* Render/decode only the current viewport (plus a small nearby buffer/prefetch window).
* Cache decoded/generated listing data and reuse it while scrolling.
* Keep expensive work out of the critical scroll path whenever possible.
* Flow/JMP arrows should also be viewport-based: query only references relevant to currently visible instructions rather than recomputing the entire image.
* Separate persistent indexes/data structures from the UI viewport/rendering layer.

First profile the current implementation and identify the actual source of the ~1 second delay. Do not guess. Determine whether the bottleneck is address→position lookup, backend scanning, instruction decoding, listing generation, arrow/reference calculation, serialization/IPC, frontend rendering, or something else.

Then implement the necessary optimization(s). You are expected to make architectural changes where needed (for example, adding an index/cache) rather than applying superficial micro-optimizations.

Important requirements:

* Random jump/scroll must not scale linearly with the total image size.
* Large images must remain responsive.
* Preserve current Listing View behavior and correctness.
* Avoid loading the entire image/listing into the frontend just to make scrolling fast.
* Keep memory usage reasonable.
* Reuse existing project architecture where possible instead of introducing unnecessary new infrastructure.

Add end-to-end performance tests for this exact problem. Create a sufficiently large realistic test image and measure random jumps/scrolls to many different locations, including locations near the beginning, middle, and end. The tests should verify both correctness and latency/regression characteristics.

Before changing code, inspect the current Listing View implementation and trace the full request path from a frontend scroll/jump to the backend and back. Compare that path conceptually with Ghidra’s indexed + viewport-based FieldPanel/Listing architecture.

Do not stop after identifying the bottleneck. Implement the optimization, run the tests, benchmark before/after, and leave the project in a measurably faster state.















Задача #3
Fix and redesign the Listing View JMP/flow arrow rendering. The current implementation is incorrect: arrows can appear from instructions that are not actually JMP/branch-like instructions, and arrows sometimes overlap unrelated listing elements.

This is a complex task. Treat Ghidra’s existing Flow Arrow architecture as the conceptual reference — this problem is already solved there. Do not invent a fundamentally different approach unless the current architecture requires it.

Core requirements:

1. Only draw legitimate control-flow arrows

* An arrow must originate from a real control-flow instruction/reference.
* Do not create arrows merely because two visible addresses happen to be related.
* Distinguish unconditional jumps, conditional jumps, fallthroughs, calls/returns, etc. according to the project’s actual instruction/reference model.
* The source instruction must itself be the correct instruction that owns the flow reference.
* Do not infer arrows from arbitrary address proximity.
* Verify the backend/reference data and fix the backend mock/test data if necessary.

2. Use the sliding viewport model
   The Listing View already uses a sliding window / viewport. Arrows must follow the same model:

* Never build arrows for the entire binary.
* For every viewport update, consider only flow references relevant to the currently visible instructions and a small necessary off-screen range.
* Query already-indexed backend reference information rather than rescanning or decoding the entire image.
* Arrow generation/layout must be fast enough to happen interactively while scrolling.
* Reuse/cache stable reference information where useful.

Conceptually:

```
indexed control-flow references
             ↓
   visible instruction window
             ↓
      relevant arrows only
             ↓
    viewport arrow layout
             ↓
           paint
```

3. Correct off-screen behavior
Handle arrows whose source or destination is outside the current viewport correctly.

An arrow may connect:

* visible source → visible destination
* visible source → off-screen destination
* off-screen source → visible destination

But do not draw meaningless arrows simply because an address is nearby. Follow Ghidra’s concept of determining flow arrows from actual references and then deciding what portion of those flows should be represented in the current viewport.

4. Arrow layout must not overlap listing content
   Fix the geometry/layout system, not individual pixel offsets.

The arrows should have their own dedicated margin/track area and must not run through unrelated listing text, fields, instructions, or other UI elements.

Multiple arrows must be laid out into separate tracks/columns when necessary. Use a deterministic collision/overlap strategy similar in spirit to Ghidra’s grouping of arrows by shared endpoints and allocation of overlapping flow paths into different columns.

Requirements:

* arrows sharing endpoints should be grouped sensibly;
* overlapping paths should be assigned different tracks/columns;
* routing should avoid crossing unrelated listing fields;
* the result must remain stable while scrolling;
* extremely complicated flow must have sensible limits so rendering cannot explode in cost.

5. Backend/API correctness
   Inspect the complete data path:

instruction
→ control-flow/reference information
→ backend API
→ viewport
→ arrow model
→ geometry/layout
→ frontend paint

Determine where the current false arrows are being created.

If the current backend mock/API does not expose enough information to reproduce Ghidra-like behavior, extend it. Do not weaken the UI logic by guessing from incomplete data.

The ideal backend representation should conceptually provide something like:

```
source instruction/address
destination address
reference/flow type
source instruction type
optionally fallthrough information
```

The UI should consume these references instead of re-decoding or heuristically inferring jumps.

6. Performance
   This must remain compatible with the optimized sliding-window Listing View.

Scrolling to a random location in a huge image must not require:

* scanning the entire image;
* rebuilding all references;
* decoding all instructions;
* generating arrows for the whole binary.

Only the current viewport/relevant nearby range should participate in arrow generation/layout.

7. Tests
   Add proper tests, not just a visual/manual fix.

Create end-to-end/integration tests covering at least:

* unconditional JMP;
* conditional JMP/Jcc;
* fallthrough;
* calls if supported by the current model;
* multiple branches targeting the same destination;
* branches crossing the viewport boundary;
* source outside viewport → destination inside viewport;
* source inside viewport → destination outside viewport;
* instructions that are NOT control-flow instructions but happen to have related addresses;
* dense/overlapping control flow;
* several simultaneous arrows that require multiple tracks/columns;
* scrolling through multiple viewport positions and ensuring arrows update correctly.

Explicitly include regression cases for the current bugs:

* “arrow appears from nowhere”;
* “arrow originates from a non-JMP-like instruction”;
* “arrow overlaps unrelated listing elements”.

Also add a sufficiently large synthetic backend mock/test image so arrow generation is exercised under realistic viewport scrolling rather than only tiny examples.

8. Use Ghidra as the behavioral reference
   Before implementing, inspect the relevant Ghidra concepts/classes, especially:

* FlowArrowMarginProvider
* FlowArrowPanel
* FieldPanel / viewport layout handling
* Ghidra’s Reference/flow information

The important architectural idea is:

```
persistent indexed references
            +
sliding viewport
            +
visible-flow extraction
            +
viewport-only arrow layout
            +
dedicated arrow tracks/margin
```

Do not simply copy superficial drawing logic. Reproduce the underlying behavior and invariants.

Finally:

* identify the root cause of the current incorrect arrows;
* implement the proper backend/frontend changes;
* update mocks/fixtures where needed;
* add regression + E2E tests;
* run the full relevant test suite;
* verify that arrows are both correct and fast during random scrolling.

Do not consider the task complete because the arrows “look better”. They must be semantically correct, geometrically isolated from listing content, stable across viewport changes, and cheap to recompute for the current sliding window.
