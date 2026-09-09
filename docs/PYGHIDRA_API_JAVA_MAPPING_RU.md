# Сопоставление API PyGhidra с Java-реализацией

Этот документ отвечает на вопрос: **какой код выполняется после вызова функции
PyGhidra**. Он является навигацией по текущему репозиторию, а не отдельной
реализацией API. Общий маршрут изучения начинается в
[`ARCHITECTURE_OVERVIEW_RU.md`](ARCHITECTURE_OVERVIEW_RU.md), а запуск runtime
описан в [`RUNTIMESCRIPTS_OVERVIEW_RU.md`](RUNTIMESCRIPTS_OVERVIEW_RU.md).

Главные исходники Python API находятся в
[`Ghidra/Features/PyGhidra/src/main/py/src/pyghidra`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra),
а Java-мосты PyGhidra -- в
[`Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra`](../Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra).

## 1. Как читать сопоставление

У PyGhidra есть три разных уровня:

```text
Python convenience API
  -> JPype: вызов Java-метода или конструктора
  -> Java API Ghidra / Java-мост PyGhidra
  -> database, loader, analysis, plugin или UI
```

Важно не смешивать их:

- `pyghidra.open_project()` действительно создает `PyGhidraProjectManager`, но
  тот почти пустой и наследует поведение `DefaultProjectManager`;
- `pyghidra.analyze()` не имеет собственного анализатора в PyGhidra: он получает
  `AutoAnalysisManager` и запускает обычный pipeline Base;
- `program.getListing()`, `program.save()` и большинство методов объектов
  `Program` -- прямые Java-вызовы через JPype, а не функции из Python-файлов
  PyGhidra;
- специальные Java-классы PyGhidra нужны там, где Python должен реализовать
  callback Java, получить доступ к protected-полям скрипта или предоставить
  timed `TaskMonitor`.

## 2. Публичные функции `pyghidra`

Функции экспортируются из
[`pyghidra/__init__.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/__init__.py#L56-L74).
Ниже указаны точки входа по файлам `api.py` и `core.py`; номера строк следует
использовать вместе с именами функций, поскольку они могут измениться при
обновлении Ghidra.

### 2.1. Запуск JVM и состояние launcher

| Python API | Непосредственный переход | Java-реализация и результат |
|---|---|---|
| `pyghidra.start(verbose=False, install_dir=...)` | Создает `HeadlessPyGhidraLauncher`, затем вызывает `launcher.start()` | [`launcher.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/launcher.py#L669-L679): `jpype.startJVM(...)`, `GhidraApplicationLayout`, затем [`Application.initializeApplication`](../Ghidra/Framework/Generic/src/main/java/ghidra/framework/Application.java) с `HeadlessGhidraApplicationConfiguration`. |
| `pyghidra.started()` | `PyGhidraLauncher.has_launched()` | [`launcher.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/launcher.py#L634-L643): сначала проверяется `jpype.isJVMStarted()`, затем Java `Application.isInitialized()`. |
| `HeadlessPyGhidraLauncher.start()` | `_setup_java()` -> `_pre_launch_init()` -> `_launch()` | В `_setup_java()` регистрируется JPype domain `ghidra`; в `_pre_launch_init()` вызывается [`GhidraLauncher.initializeGhidraEnvironment`](../Ghidra/Framework/Utility/src/main/java/ghidra/GhidraLauncher.java), а в `_launch()` инициализируется headless Application. |
| `DeferredPyGhidraLauncher.initialize_ghidra(headless=True)` | `Application.initializeApplication(layout, config)` | [`launcher.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/launcher.py#L646-L667). При `headless=False` используется `GhidraRun.launch(...)`; до этого JVM и layout уже подготовлены. |
| `GuiPyGhidraLauncher.start()` / `pyghidra.gui.gui()` | Java-поток вызывает `Ghidra.main(["ghidra.GhidraRun", ...])` | [`launcher.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/launcher.py#L714-L759) и [`Ghidra.java`](../Ghidra/Framework/Utility/src/main/java/ghidra/Ghidra.java). GUI-режим дополнительно подключает `PyGhidraPlugin`. |

Перед инициализацией `launcher.py` устанавливает classpath, VM arguments,
`JAVA_HOME`, импорт-хуки и загружает внешние entry points. Это Python/JPype
инфраструктура, поэтому отдельного метода Java для каждого `add_vmargs()` или
`add_classpaths()` нет. Фактический JVM старт находится в
[`launcher.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/launcher.py#L428-L468).

### 2.2. Проекты и файлы

| Python API | Цепочка вызовов | Где продолжает выполняться код |
|---|---|---|
| `open_project(path, name, create=False)` | `ProjectLocator(...)` -> `PyGhidraProjectManager.openProject(...)` или `createProject(...)` | [`api.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/api.py#L60-L83); Java-класс [`PyGhidraProjectManager`](../Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra/PyGhidraProjectManager.java#L18-L24) только открывает protected-конструктор наследника [`DefaultProjectManager`](../Ghidra/Framework/Project/src/main/java/ghidra/framework/project/DefaultProjectManager.java#L45-L46). При создании `PyGhidraProject` устанавливает активный проект через `AppInfo.setActiveProject` ([`PyGhidraProject.java`](../Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra/PyGhidraProject.java#L23-L28)). |
| `open_filesystem(path)` | `FileSystemService.getInstance()` -> `getLocalFS().getLocalFSRL(File)` -> `openFileSystemContainer(fsrl, monitor)` | Основная реализация -- [`FileSystemService.java`](../Ghidra/Features/Base/src/main/java/ghidra/formats/gfilesystem/FileSystemService.java#L56-L56), метод открытия контейнера находится около [`openFileSystemContainer`](../Ghidra/Features/Base/src/main/java/ghidra/formats/gfilesystem/FileSystemService.java#L790-L792). `None` преобразуется Python-кодом в `ValueError`. |
| `consume_program(project, path, consumer=None)` | `project.getProjectData().getFile(path)` -> `DomainFile.getDomainObject(consumer, true, false, monitor)` | [`api.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/api.py#L105-L139). Java-код `DomainFile` открывает domain object; Python дополнительно проверяет `Program.class_.isAssignableFrom(...)` и освобождает объект при ошибке типа. |
| `program_context(project, path)` | Вход вызывает `consume_program`; выход вызывает `program.release(consumer)` | [`api.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/api.py#L141-L160). Это Python context manager, но освобождение выполняется методом Java `DomainObject.release`. |
| `walk_project(project, callback, start, file_filter)` | `project.projectData.getFolder(start)` -> `ProjectDataUtils.descendantFiles(start_folder)` | [`api.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/api.py#L323-L345). Обход выполняет Java `ProjectDataUtils`, а фильтр и callback остаются Python-функциями. |
| `walk_programs(project, callback, ...)` | Для каждого `DomainFile`: `program_context` -> `program_filter` -> callback | [`api.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/api.py#L347-L371). Непрограммы отбрасываются через Python `ProgramTypeError`; дополнительного Java-обходчика нет. |

`open_project()` не следует путать с legacy `open_program()`: первый работает с
уже существующим проектом, второй сам создает/открывает проект и импортирует
binary.

### 2.3. Импорт binary и legacy API

| Python API | Непосредственный Java-код |
|---|---|
| `program_loader()` | Возвращает `ProgramLoader.builder()` из [`ProgramLoader.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/importer/ProgramLoader.java#L40-L55). Все последующие `.source()`, `.project()`, `.load()`, `.save()` -- fluent Java API этого builder/result объекта. |
| `open_program(...)` | `_setup_project()` выбирает `GhidraProject.openProject/createProject`, затем вызывает один из overload-ов `project.importProgram(binary_path[, loader][, language, compiler])`; после этого `project.saveAs(...)`. Python-код находится в [`core.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/core.py#L54-L132). |
| `_get_language(lang_id)` | `DefaultLanguageService.getLanguageService().getLanguage(new LanguageID(lang_id))` | [`core.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/core.py#L29-L38). `LanguageNotFoundException` преобразуется в Python `ValueError`. |
| `_get_compiler_spec(lang, compiler)` | `lang.getDefaultCompilerSpec()` или `lang.getCompilerSpecByID(new CompilerSpecID(...))` | [`core.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/core.py#L41-L51). Проверка существования language/compiler выполняется Java API, форматирование ошибки -- Python. |
| `run_script(...)` | Legacy-обертка над `_flat_api(...)`; в конце вызывает `PyGhidraScript.run(script_path, script_args)` | [`core.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/core.py#L318-L371) и раздел ниже. Функция deprecated; новый маршрут -- `open_project()` + `ghidra_script()`. |

Важный нюанс `open_program()`: `FlatProgramAPI(program)` создается Python-кодом
через Java-конструктор, а анализ вызывается как `flat_api.analyzeAll(program)`.
Реальная реализация этого анализа находится в
[`FlatProgramAPI.java`](../Ghidra/Features/Base/src/main/java/ghidra/program/flatapi/FlatProgramAPI.java#L190-L240),
где используются `AutoAnalysisManager.initializeOptions()`,
`reAnalyzeAll(...)` и `startAnalysis(...)`.

### 2.4. Анализ, транзакции и параметры

| Python API | Java-цепочка |
|---|---|
| `analyze(program, monitor=None)` | `GhidraScriptUtil.acquireBundleHostReference()` -> `AutoAnalysisManager.getAnalysisManager(program)` -> `initializeOptions()` -> `reAnalyzeAll(null)` -> listener для `MessageLog` -> `startAnalysis(monitor, true)` -> `GhidraProgramUtilities.markProgramAnalyzed(program)` | Полная Python-обертка: [`api.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/api.py#L162-L196). Планировщик анализаторов -- [`AutoAnalysisManager.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisManager.java#L61-L65); метод `reAnalyzeAll` находится около [этой строки](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisManager.java#L325-L327). Bundle reference освобождается в `finally`. |
| `transaction(program, description)` | `program.startTransaction(description)` -> тело Python -> `program.endTransaction(transaction_id, success)` | [`api.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/api.py#L255-L275). При Python-исключении в Java передается `success=False`, поэтому изменения откатываются согласно контракту `DomainObject`. |
| `analysis_properties(program)` | `AutoAnalysisManager.getAnalysisManager(program).initializeOptions()` -> `program.getOptions(Program.ANALYSIS_PROPERTIES)` | [`api.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/api.py#L277-L286). Первое действие важно: без инициализации часть analysis options может быть недоступна. |
| `program_info(program)` | `program.getOptions(Program.PROGRAM_INFO)` | [`api.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/api.py#L288-L295). Это прямой getter модели `Program`; PyGhidra не хранит отдельную копию options. |
| `task_monitor(timeout=None)` | Без timeout возвращается `TaskMonitor.DUMMY`; с timeout создается `new PyGhidraTaskMonitor(JInt(timeout), null)` | [`api.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/api.py#L306-L321) и [`PyGhidraTaskMonitor.java`](../Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra/PyGhidraTaskMonitor.java#L33-L62). Таймер вызывает `cancel()`, а `checkCancelled()` бросает стандартный Java `CancelledException`. |

`analyze()` запускает анализ внутри транзакции. Это отличается от простого
вызова `program.startTransaction()`: сама Python-функция также управляет bundle
reference, слушателем лога и отметкой `Program` как проанализированной.

## 3. Выполнение GhidraScript

### 3.1. `ghidra_script()`

Вызов `pyghidra.ghidra_script(path, project, program, ...)` проходит так:

```text
ResourceFile(File(path))
  -> GhidraScriptUtil.getProvider(source_file)
  -> provider.getScriptInstance(source_file, writer)
  -> new GhidraState(...)
  -> new ScriptControls(stdout, stderr, monitor)
  -> script.setScriptArgs(args)
  -> script.execute(state, controls)
```

Python-код этого маршрута находится в
[`api.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/api.py#L198-L253).
Выбор конкретного провайдера выполняется общим Java `GhidraScriptUtil`, но для
нативного Python приоритетным провайдером является
[`PyGhidraScriptProvider`](../Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra/PyGhidraScriptProvider.java#L36-L80).

В `getScriptInstance()`:

- если Python runner не зарегистрирован, выбрасывается Java
  `GhidraScriptLoadException`;
- в headless создается `PyGhidraHeadlessScript`, иначе
  `PyGhidraGhidraScript`;
- оба объекта наследуют стандартный `GhidraScript`/`HeadlessScript` и получают
  `sourceFile`.

### 3.2. `PyGhidraScript.run()` и Python script body

`PyGhidraScript` -- Python wrapper над Java-объектом, возвращенным provider-ом.
Его [`run()`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/script.py#L215-L269)
не вызывает Java `run()` повторно. Он:

1. берет source file и аргументы у Java script;
2. временно меняет `sys.argv` и `sys.path`;
3. создает Python module через `_GhidraScriptLoader`;
4. исполняет файл CPython через `spec.loader.exec_module(m)`;
5. восстанавливает `sys.argv`, модули и `sys.path`.

Java вызывает Python runner через `PyGhidraScriptProvider.PyGhidra*Script.run()`:
[`PyGhidraScriptProvider.java`](../Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra/PyGhidraScriptProvider.java#L98-L146)
содержит `scriptRunner.accept(this)`. Регистрация этого callback выполняется в
[`plugin.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/internal/plugin/plugin.py#L343-L350):
`PyGhidraScriptProvider.setScriptRunner(Consumer @ _run_script)`.

### 3.3. `PyGhidraScript.set()` и exposed fields

Python `PyGhidraScript.set(state, monitor, writer, error_writer)` упаковывает
аргументы в Java `ScriptControls` и вызывает Java
`GhidraScript.set(...)` ([`script.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/script.py#L208-L213)).

Поля `currentProgram`, `currentAddress`, `monitor`, `state`, `writer` и другие
не являются обычными Python-полями. Схема доступа:

```text
@JImplementationFor(PythonFieldExposer)
  -> PythonFieldExposer.getProperties(Java class)
  -> ExposedField.fget/fset
  -> VarHandle.get/set(protected field)
```

Java-аннотации с именами и типами полей находятся в
[`PyGhidraScriptProvider.java`](../Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra/PyGhidraScriptProvider.java#L83-L157),
а отражение в `VarHandle` -- в
[`PythonFieldExposer.java`](../Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra/PythonFieldExposer.java#L39-L80)
и [`PythonFieldExposer.ExposedField`](../Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra/PythonFieldExposer.java#L124-L157).

## 4. GUI interpreter

`pyghidra.get_current_interpreter()` -- не запуск нового интерпретатора. В
headless режиме он возвращает singleton-like script, созданный через
`PyGhidraScriptProvider().getScriptInstance(...)`; в GUI он ищет активный Tool и
его plugin с именем `PyGhidraPlugin` ([`script.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/script.py#L271-L313)).

GUI-путь выглядит так:

```text
PyGhidraPlugin.init()
  -> new PyGhidraInterpreter(...)
  -> InterpreterPanelService.createInterpreterPanel(...)
  -> Python _init_plugin(plugin)
  -> new PyConsole(plugin)
  -> interpreter.init(console)
```

Соответствующие места:

- создание plugin и синхронизация текущей программы/location/selection:
  [`PyGhidraPlugin.java`](../Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra/PyGhidraPlugin.java#L49-L119);
- создание Java interpreter connection и completion bridge:
  [`PyGhidraInterpreter.java`](../Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra/interpreter/PyGhidraInterpreter.java#L34-L102);
- Python console, `InteractiveConsole`, Java thread attach/detach, interrupt и
  completion callback: [`plugin.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/internal/plugin/plugin.py#L148-L175)
  и [`plugin.py`](../Ghidra/Features/PyGhidra/src/main/py/src/pyghidra/internal/plugin/plugin.py#L254-L340);
- Java-side script state для console: [`InterpreterGhidraScript.java`](../Ghidra/Features/PyGhidra/src/main/java/ghidra/pyghidra/interpreter/InterpreterGhidraScript.java#L29-L85).

Например, смена активной программы в Tool вызывает Java
`PyGhidraPlugin.programActivated()`, который записывает значение в
`InterpreterGhidraScript.setCurrentProgram()`, а Python получает его через
JPype property/customization. Поэтому `currentProgram` в REPL отражает состояние
GUI, а не отдельную копию проекта.

## 5. Что искать дальше в Java

После PyGhidra-моста изучайте объект, который возвращен API:

| Получен из PyGhidra | Следующий слой |
|---|---|
| `Program` | модель listing/memory/function/database в `Ghidra/Framework/SoftwareModeling`; начальная навигация есть в [`FRAMEWORK_OVERVIEW_RU.md`](FRAMEWORK_OVERVIEW_RU.md). |
| `ProgramLoader.Builder` | импорт и выбор loader в [`ProgramLoader.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/importer/ProgramLoader.java). |
| `GFileSystem` | конкретный filesystem provider и [`FileSystemService.java`](../Ghidra/Features/Base/src/main/java/ghidra/formats/gfilesystem/FileSystemService.java). |
| `AutoAnalysisManager` | analyzer pipeline Base в [`AutoAnalysisManager.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisManager.java); прикладной маршрут описан в [`FEATURES_BASE_OVERVIEW_RU.md`](FEATURES_BASE_OVERVIEW_RU.md). |
| `GhidraScript` | общий script lifecycle в `Ghidra/Framework/Project` и `Ghidra/Features/Base`, после чего возвращайтесь к provider-у PyGhidra. |

## 6. Короткая карта вызовов

```text
pyghidra.start
  -> PyGhidraLauncher.start
  -> jpype.startJVM
  -> GhidraLauncher.initializeGhidraEnvironment
  -> Application.initializeApplication

pyghidra.open_project
  -> PyGhidraProjectManager
  -> DefaultProjectManager
  -> Project / ProjectData / DomainFile

pyghidra.open_program (legacy)
  -> GhidraProject.importProgram
  -> FlatProgramAPI
  -> AutoAnalysisManager

pyghidra.analyze
  -> AutoAnalysisManager
  -> analyzers Base
  -> Program marked analyzed

pyghidra.ghidra_script
  -> GhidraScriptUtil
  -> PyGhidraScriptProvider
  -> PyGhidraHeadlessScript or PyGhidraGhidraScript
  -> Python PyGhidraScript.run
  -> CPython script body

GUI PyGhidra
  -> PyGhidraPlugin
  -> PyGhidraInterpreter
  -> PyGhidraConsole / PyConsole
  -> InterpreterGhidraScript state
```

Для запуска из установленной Ghidra сначала сверяйте оболочку
[`pyghidraRun`](../Ghidra/RuntimeScripts/support/pyghidraRun) и описание runtime в
[`RUNTIMESCRIPTS_OVERVIEW_RU.md`](RUNTIMESCRIPTS_OVERVIEW_RU.md); для изучения
внутреннего API переходите непосредственно в файлы, перечисленные выше.
