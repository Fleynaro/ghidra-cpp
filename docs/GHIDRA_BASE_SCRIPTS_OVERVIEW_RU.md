# Скрипты `Ghidra/Features/Base/ghidra_scripts`

Этот документ продолжает [обзор архитектуры Ghidra](ARCHITECTURE_OVERVIEW_RU.md)
и относится к прикладному модулю [`Ghidra/Features/Base`](../Ghidra/Features/Base).
Общая модель `Program`, память, listing, функции, типы и ссылки описаны в
[`FEATURES_BASE_OVERVIEW_RU.md`](FEATURES_BASE_OVERVIEW_RU.md) и
[`FRAMEWORK_OVERVIEW_RU.md`](FRAMEWORK_OVERVIEW_RU.md).

## Как читать каталог

Скрипт Ghidra обычно является классом Java, унаследованным от
[`GhidraScript`](../Ghidra/Features/Base/src/main/java/ghidra/app/script/GhidraScript.java),
с методом `run()`. В нём доступны часто используемые контексты:

| Объект | Простое объяснение |
|---|---|
| `currentProgram` | Открытая программа: память, инструкции, функции, типы и символы. |
| `currentAddress` | Адрес под курсором. |
| `currentSelection` | Выделенный диапазон адресов; может быть `null`. |
| `currentLocation` | Более подробное положение курсора в Listing. |
| `monitor` | Прогресс и отмена длительной операции. |

Часть файлов является не готовым инструментом, а учебным примером API или
внутренним тестом разработчика. Перед скриптами, которые меняют байты, память,
функции или символы, сохраните проект или работайте с копией.

## Полная таблица

Ссылки в таблице ведут непосредственно в исходники каталога
[`Ghidra/Features/Base/ghidra_scripts`](../Ghidra/Features/Base/ghidra_scripts).
Названия и описание сверены с комментариями и реализацией файлов.

### Add--Create

| Файл | Что делает |
|---|---|
| [AddCommentToProgramScript.java](../Ghidra/Features/Base/ghidra_scripts/AddCommentToProgramScript.java) | Добавляет комментарий к текущей программе. |
| [AddReferencesInSwitchTable.java](../Ghidra/Features/Base/ghidra_scripts/AddReferencesInSwitchTable.java) | Добавляет ARM-ссылки из таблицы `switch` к обработчикам. |
| [AddSingleReferenceInSwitchTable.java](../Ghidra/Features/Base/ghidra_scripts/AddSingleReferenceInSwitchTable.java) | Добавляет одну ссылку из записи таблицы переходов. |
| [AddSourceFileScript.java](../Ghidra/Features/Base/ghidra_scripts/AddSourceFileScript.java) | Добавляет сведения об исходном файле для source mapping. |
| [AddSourceMapEntryScript.java](../Ghidra/Features/Base/ghidra_scripts/AddSourceMapEntryScript.java) | Добавляет сопоставление адреса с исходным кодом. |
| [AppleSingleDoubleScript.java](../Ghidra/Features/Base/ghidra_scripts/AppleSingleDoubleScript.java) | Разбирает контейнеры AppleSingle/AppleDouble. |
| [ArmThumbFunctionTableScript.java](../Ghidra/Features/Base/ghidra_scripts/ArmThumbFunctionTableScript.java) | Обрабатывает таблицы функций ARM Thumb. |
| [AsciiToBinaryScript.java](../Ghidra/Features/Base/ghidra_scripts/AsciiToBinaryScript.java) | Преобразует ASCII-представление байтов в бинарные данные. |
| [AssembleBlockScript.java](../Ghidra/Features/Base/ghidra_scripts/AssembleBlockScript.java) | Демонстрирует сборку блока ассемблерных инструкций. |
| [AssembleCheckDevScript.java](../Ghidra/Features/Base/ghidra_scripts/AssembleCheckDevScript.java) | Разработческий тест assembler API. |
| [AssembleScript.java](../Ghidra/Features/Base/ghidra_scripts/AssembleScript.java) | Ассемблирует одну инструкцию по адресу курсора. |
| [AssemblyThrasherDevScript.java](../Ghidra/Features/Base/ghidra_scripts/AssemblyThrasherDevScript.java) | Стресс-тест assembler и выбора вариантов инструкций. |
| [AskScript.java](../Ghidra/Features/Base/ghidra_scripts/AskScript.java) | Показывает разные методы ввода `ask*()`. |
| [AskScript.properties](../Ghidra/Features/Base/ghidra_scripts/AskScript.properties) | Значения по умолчанию для `AskScript`. |
| [AskValuesExampleScript.java](../Ghidra/Features/Base/ghidra_scripts/AskValuesExampleScript.java) | Пример типизированного ввода чисел, адресов, файлов и строк. |
| [AssociateExternalPELibrariesScript.java](../Ghidra/Features/Base/ghidra_scripts/AssociateExternalPELibrariesScript.java) | Связывает импортированные PE-библиотеки с внешними ссылками. |
| [AutoRenameLabelsScript.java](../Ghidra/Features/Base/ghidra_scripts/AutoRenameLabelsScript.java) | Переименовывает метки, созданные анализаторами. |
| [AutoRenameSimpleLabels.java](../Ghidra/Features/Base/ghidra_scripts/AutoRenameSimpleLabels.java) | Переименовывает простые метки для `RET` и ветвлений. |
| [BatchRename.java](../Ghidra/Features/Base/ghidra_scripts/BatchRename.java) | Пакетно переименовывает программы проекта. |
| [BatchSegregate64bit.java](../Ghidra/Features/Base/ghidra_scripts/BatchSegregate64bit.java) | Пакетно отделяет 64-битные программы. |
| [BinaryToAsciiScript.java](../Ghidra/Features/Base/ghidra_scripts/BinaryToAsciiScript.java) | Преобразует бинарные данные в ASCII-вид. |
| [BuildGhidraJarScript.java](../Ghidra/Features/Base/ghidra_scripts/BuildGhidraJarScript.java) | Создаёт облегчённый JAR с основными модулями Ghidra. |
| [CallAnotherScript.java](../Ghidra/Features/Base/ghidra_scripts/CallAnotherScript.java) | Показывает запуск другого скрипта. |
| [CallAnotherScriptForAllPrograms.java](../Ghidra/Features/Base/ghidra_scripts/CallAnotherScriptForAllPrograms.java) | Запускает выбранный скрипт для всех программ проекта. |
| [CallotherCensusScript.java](../Ghidra/Features/Base/ghidra_scripts/CallotherCensusScript.java) | Собирает статистику операций Sleigh `CALLOTHER`. |
| [ChangeDataSettingsScript.java](../Ghidra/Features/Base/ghidra_scripts/ChangeDataSettingsScript.java) | Показывает изменение настроек объекта данных. |
| [ChooseDataTypeScript.java](../Ghidra/Features/Base/ghidra_scripts/ChooseDataTypeScript.java) | Даёт выбрать тип данных через GUI. |
| [ClearOrphanFunctions.java](../Ghidra/Features/Base/ghidra_scripts/ClearOrphanFunctions.java) | Ищет и удаляет или исправляет функции-сироты. |
| [COFF_ArchiveScript.java](../Ghidra/Features/Base/ghidra_scripts/COFF_ArchiveScript.java) | Разбирает COFF-архив. |
| [COFF_Script.java](../Ghidra/Features/Base/ghidra_scripts/COFF_Script.java) | Выводит сведения о COFF-файле. |
| [CompareAnalysisScript.java](../Ghidra/Features/Base/ghidra_scripts/CompareAnalysisScript.java) | Сравнивает результаты анализа двух программ. |
| [CompareGDTs.java](../Ghidra/Features/Base/ghidra_scripts/CompareGDTs.java) | Сравнивает два GDT-архива типов. |
| [ComputeCyclomaticComplexity.java](../Ghidra/Features/Base/ghidra_scripts/ComputeCyclomaticComplexity.java) | Вычисляет цикломатическую сложность текущей функции. |
| [CondenseAllRepeatingBytes.java](../Ghidra/Features/Base/ghidra_scripts/CondenseAllRepeatingBytes.java) | Сворачивает длинные последовательности одинаковых байтов. |
| [CondenseFillerBytes.java](../Ghidra/Features/Base/ghidra_scripts/CondenseFillerBytes.java) | Сворачивает заполняющие байты между функциями. |
| [CondenseRepeatingBytes.java](../Ghidra/Features/Base/ghidra_scripts/CondenseRepeatingBytes.java) | Превращает повторяющиеся байты в массив данных. |
| [CondenseRepeatingBytesAtEndOfMemory.java](../Ghidra/Features/Base/ghidra_scripts/CondenseRepeatingBytesAtEndOfMemory.java) | Обрабатывает повторяющиеся неопределённые байты в конце памяти. |
| [ConvertDotDotDotScript.java](../Ghidra/Features/Base/ghidra_scripts/ConvertDotDotDotScript.java) | Преобразует устаревшие `...` в комментариях или метках. |
| [ConvertDotToDashInAutoAnalysisLabels.java](../Ghidra/Features/Base/ghidra_scripts/ConvertDotToDashInAutoAnalysisLabels.java) | Заменяет точки дефисами в метках автоанализа. |
| [CountAndSaveStrings.java](../Ghidra/Features/Base/ghidra_scripts/CountAndSaveStrings.java) | Считает строки и сохраняет результат. |
| [CountSymbolsScript.java](../Ghidra/Features/Base/ghidra_scripts/CountSymbolsScript.java) | Считает символы разных типов. |
| [CreateDefaultGDTArchivesScript.java](../Ghidra/Features/Base/ghidra_scripts/CreateDefaultGDTArchivesScript.java) | Создаёт стандартные GDT-архивы. |
| [CreateEmptyProgramScript.java](../Ghidra/Features/Base/ghidra_scripts/CreateEmptyProgramScript.java) | Создаёт пустую программу. |
| [CreateExampleGDTArchiveScript.java](../Ghidra/Features/Base/ghidra_scripts/CreateExampleGDTArchiveScript.java) | Создаёт демонстрационный GDT-архив. |
| [CreateExportFileForDLL.java](../Ghidra/Features/Base/ghidra_scripts/CreateExportFileForDLL.java) | Создаёт export-файл для DLL. |
| [CreateFunctionAfterTerminals.java](../Ghidra/Features/Base/ghidra_scripts/CreateFunctionAfterTerminals.java) | Создаёт функцию после терминальных инструкций. |
| [CreateFunctionsFromSelection.java](../Ghidra/Features/Base/ghidra_scripts/CreateFunctionsFromSelection.java) | Создаёт функции в текущем выделении. |
| [CreateHelpTemplateScript.java](../Ghidra/Features/Base/ghidra_scripts/CreateHelpTemplateScript.java) | Генерирует шаблон справочной документации. |
| [CreateOperandReferencesInSelectionScript.java](../Ghidra/Features/Base/ghidra_scripts/CreateOperandReferencesInSelectionScript.java) | Создаёт ссылки операндов в выделении. |
| [CreatePdbXmlFilesScript.java](../Ghidra/Features/Base/ghidra_scripts/CreatePdbXmlFilesScript.java) | Преобразует PDB в XML-представление. |
| [CreateRelocationBasedOperandReferences.java](../Ghidra/Features/Base/ghidra_scripts/CreateRelocationBasedOperandReferences.java) | Создаёт ссылки операндов по relocation. |
| [CreateStringScript.java](../Ghidra/Features/Base/ghidra_scripts/CreateStringScript.java) | Находит строки с `\n` и создаёт типы данных. |
| [CreateUEFIGDTArchivesScript.java](../Ghidra/Features/Base/ghidra_scripts/CreateUEFIGDTArchivesScript.java) | Создаёт GDT-архивы типов UEFI. |

### Debug--Languages

| Файл | Что делает |
|---|---|
| [DebugSleighInstructionParse.java](../Ghidra/Features/Base/ghidra_scripts/DebugSleighInstructionParse.java) | Отлаживает разбор инструкции Sleigh. |
| [DeleteDeadDefaultPlatesScript.java](../Ghidra/Features/Base/ghidra_scripts/DeleteDeadDefaultPlatesScript.java) | Удаляет неиспользуемые стандартные plate-комментарии. |
| [DeleteEmptyPlateCommentsScript.java](../Ghidra/Features/Base/ghidra_scripts/DeleteEmptyPlateCommentsScript.java) | Удаляет пустые plate-комментарии. |
| [DeleteExitCommentsScript.java](../Ghidra/Features/Base/ghidra_scripts/DeleteExitCommentsScript.java) | Удаляет exit-комментарии. |
| [DeleteFunctionDefaultPlatesScript.java](../Ghidra/Features/Base/ghidra_scripts/DeleteFunctionDefaultPlatesScript.java) | Удаляет стандартные plate-комментарии функций. |
| [DeleteSpacePropertyScript.java](../Ghidra/Features/Base/ghidra_scripts/DeleteSpacePropertyScript.java) | Удаляет свойство address space. |
| [DemangleAllScript.java](../Ghidra/Features/Base/ghidra_scripts/DemangleAllScript.java) | Деманглирует подходящие символы всей программы. |
| [DemangleSymbolScript.java](../Ghidra/Features/Base/ghidra_scripts/DemangleSymbolScript.java) | Деманглирует выбранный символ. |
| [DoARMDisassemble.java](../Ghidra/Features/Base/ghidra_scripts/DoARMDisassemble.java) | Принудительно дизассемблирует код в ARM-режиме. |
| [DoThumbDisassemble.java](../Ghidra/Features/Base/ghidra_scripts/DoThumbDisassemble.java) | Принудительно дизассемблирует код в Thumb-режиме. |
| [DWARFLineInfoCommentScript.java](../Ghidra/Features/Base/ghidra_scripts/DWARFLineInfoCommentScript.java) | Переносит DWARF line information в комментарии. |
| [DWARFLineInfoSourceMapScript.java](../Ghidra/Features/Base/ghidra_scripts/DWARFLineInfoSourceMapScript.java) | Создаёт source-map записи из DWARF line information. |
| [DWARFMacroScript.java](../Ghidra/Features/Base/ghidra_scripts/DWARFMacroScript.java) | Исследует и выводит макросы DWARF. |
| [DWARFSetExternalDebugFilesLocationPrescript.java](../Ghidra/Features/Base/ghidra_scripts/DWARFSetExternalDebugFilesLocationPrescript.java) | Задаёт каталог внешних DWARF-файлов перед анализом. |
| [EditBytesScript.java](../Ghidra/Features/Base/ghidra_scripts/EditBytesScript.java) | Изменяет байты программы через GUI. |
| [EmbeddedFinderScript.java](../Ghidra/Features/Base/ghidra_scripts/EmbeddedFinderScript.java) | Ищет встроенные PE-файлы по DOS/NT-заголовкам. |
| [EmuX86DeobfuscateExampleScript.java](../Ghidra/Features/Base/ghidra_scripts/EmuX86DeobfuscateExampleScript.java) | Показывает эмуляцию x86-функции для деобфускации. |
| [EmuX86GccDeobfuscateHookExampleScript.java](../Ghidra/Features/Base/ghidra_scripts/EmuX86GccDeobfuscateHookExampleScript.java) | Показывает x86-эмуляцию с hook для `malloc`, `free` и строк. |
| [ExampleColorScript.java](../Ghidra/Features/Base/ghidra_scripts/ExampleColorScript.java) | Демонстрирует изменение цветов отображения. |
| [ExampleGraphServiceScript.java](../Ghidra/Features/Base/ghidra_scripts/ExampleGraphServiceScript.java) | Демонстрирует создание графа. |
| [ExportFunctionInfoScript.java](../Ghidra/Features/Base/ghidra_scripts/ExportFunctionInfoScript.java) | Экспортирует адреса и характеристики функций. |
| [ExportImagesScript.java](../Ghidra/Features/Base/ghidra_scripts/ExportImagesScript.java) | Извлекает изображения из программы. |
| [ExportProgramScript.java](../Ghidra/Features/Base/ghidra_scripts/ExportProgramScript.java) | Демонстрирует экспорт программы. |
| [ExtractELFDebugFilesScript.java](../Ghidra/Features/Base/ghidra_scripts/ExtractELFDebugFilesScript.java) | Извлекает внешние debug-файлы из ELF. |
| [FFsBeGoneScript.java](../Ghidra/Features/Base/ghidra_scripts/FFsBeGoneScript.java) | Ищет и обрабатывает области, заполненные `0xff`. |
| [FindAndReplaceCommentScript.java](../Ghidra/Features/Base/ghidra_scripts/FindAndReplaceCommentScript.java) | Ищет и заменяет текст комментариев. |
| [FindAudioInProgramScript.java](../Ghidra/Features/Base/ghidra_scripts/FindAudioInProgramScript.java) | Ищет аудиоданные по известным признакам. |
| [FindDataTypeConflictCauseScript.java](../Ghidra/Features/Base/ghidra_scripts/FindDataTypeConflictCauseScript.java) | Исследует причину конфликта типов данных. |
| [FindDataTypeScript.java](../Ghidra/Features/Base/ghidra_scripts/FindDataTypeScript.java) | Ищет тип данных в менеджере и архивах типов. |
| [FindFunctionsUsingTOCinPEFScript.java](../Ghidra/Features/Base/ghidra_scripts/FindFunctionsUsingTOCinPEFScript.java) | Находит функции, использующие TOC в PEF. |
| [FindImagesScript.java](../Ghidra/Features/Base/ghidra_scripts/FindImagesScript.java) | Ищет изображения в памяти программы. |
| [FindInstructionsNotInsideFunctionScript.java](../Ghidra/Features/Base/ghidra_scripts/FindInstructionsNotInsideFunctionScript.java) | Находит инструкции вне функций. |
| [FindOverlappingCodeUnitsScript.java](../Ghidra/Features/Base/ghidra_scripts/FindOverlappingCodeUnitsScript.java) | Обнаруживает пересекающиеся code units. |
| [FindRunsOfPointersScript.java](../Ghidra/Features/Base/ghidra_scripts/FindRunsOfPointersScript.java) | Ищет последовательности указателей в 32-битной программе. |
| [FindSharedReturnFunctionsScript.java](../Ghidra/Features/Base/ghidra_scripts/FindSharedReturnFunctionsScript.java) | Находит функции с общими адресами возврата. |
| [FindTextScript.java](../Ghidra/Features/Base/ghidra_scripts/FindTextScript.java) | Ищет текст и переходит к первому совпадению. |
| [FindUndefinedFunctionsFollowUpScript.java](../Ghidra/Features/Base/ghidra_scripts/FindUndefinedFunctionsFollowUpScript.java) | Дополнительно обрабатывает найденные неопределённые функции. |
| [FindUndefinedFunctionsScript.java](../Ghidra/Features/Base/ghidra_scripts/FindUndefinedFunctionsScript.java) | Находит функции по известным байтовым прологам. |
| [FindUnrecoveredSwitchesScript.java](../Ghidra/Features/Base/ghidra_scripts/FindUnrecoveredSwitchesScript.java) | Ищет не восстановленные анализатором `switch`. |
| [FindX86RelativeCallsScript.java](../Ghidra/Features/Base/ghidra_scripts/FindX86RelativeCallsScript.java) | Ищет относительные вызовы x86. |
| [Fix_ARM_Call_JumpsScript.java](../Ghidra/Features/Base/ghidra_scripts/Fix_ARM_Call_JumpsScript.java) | Исправляет ARM call/jump-ссылки. |
| [FixArrayStructReferencesScript.java](../Ghidra/Features/Base/ghidra_scripts/FixArrayStructReferencesScript.java) | Исправляет ссылки на элементы массивов структур. |
| [FixElfExternalOffsetDataRelocationScript.java](../Ghidra/Features/Base/ghidra_scripts/FixElfExternalOffsetDataRelocationScript.java) | Исправляет relocation внешних ELF-данных. |
| [FixOffcutInstructionScript.java](../Ghidra/Features/Base/ghidra_scripts/FixOffcutInstructionScript.java) | Исправляет безопасные переходы в середину инструкций. |
| [FixOldSTVariableStorageScript.java](../Ghidra/Features/Base/ghidra_scripts/FixOldSTVariableStorageScript.java) | Исправляет устаревшее хранилище переменных. |
| [FixupCompositeDataTypesScript.java](../Ghidra/Features/Base/ghidra_scripts/FixupCompositeDataTypesScript.java) | Исправляет composite-типы данных. |
| [FixupGolangFuncParamStorageScript.java](../Ghidra/Features/Base/ghidra_scripts/FixupGolangFuncParamStorageScript.java) | Исправляет расположение параметров Go-функций. |
| [FixupNoReturnFunctionsNoRepairScript.java](../Ghidra/Features/Base/ghidra_scripts/FixupNoReturnFunctionsNoRepairScript.java) | Ищет no-return функции без автоматического ремонта. |
| [FixupNoReturnFunctionsScript.java](../Ghidra/Features/Base/ghidra_scripts/FixupNoReturnFunctionsScript.java) | Находит и исправляет no-return функции. |
| [FormatExampleScript.java](../Ghidra/Features/Base/ghidra_scripts/FormatExampleScript.java) | Демонстрирует форматирование вывода. |
| [GenerateLotsOfProgramsScript.java](../Ghidra/Features/Base/ghidra_scripts/GenerateLotsOfProgramsScript.java) | Генерирует много тестовых программ. |
| [GenerateMaskedBitStringScript.java](../Ghidra/Features/Base/ghidra_scripts/GenerateMaskedBitStringScript.java) | Создаёт маскированное битовое представление. |
| [GeneratePrototypeTestFileScript.java](../Ghidra/Features/Base/ghidra_scripts/GeneratePrototypeTestFileScript.java) | Создаёт тестовый файл для prototype/calling convention. |
| [GetAndSetAnalysisOptionsScript.java](../Ghidra/Features/Base/ghidra_scripts/GetAndSetAnalysisOptionsScript.java) | Демонстрирует чтение и изменение опций анализа. |
| [GraphClassesScript.java](../Ghidra/Features/Base/ghidra_scripts/GraphClassesScript.java) | Строит граф иерархии C++-классов. |
| [HelloWorldPopupScript.java](../Ghidra/Features/Base/ghidra_scripts/HelloWorldPopupScript.java) | Минимальный пример `popup()`. |
| [HelloWorldScript.java](../Ghidra/Features/Base/ghidra_scripts/HelloWorldScript.java) | Минимальный пример `GhidraScript`. |
| [ImportAllProgramsFromADirectoryScript.java](../Ghidra/Features/Base/ghidra_scripts/ImportAllProgramsFromADirectoryScript.java) | Импортирует все поддерживаемые файлы каталога. |
| [ImportProgramScript.java](../Ghidra/Features/Base/ghidra_scripts/ImportProgramScript.java) | Импортирует один файл и открывает `Program`. |
| [InnerClassScript.java](../Ghidra/Features/Base/ghidra_scripts/InnerClassScript.java) | Показывает внутренний класс в скрипте. |
| [InstructionSearchScript.java](../Ghidra/Features/Base/ghidra_scripts/InstructionSearchScript.java) | Ищет инструкции по маскам и операндам. |
| [IterateDataScript.java](../Ghidra/Features/Base/ghidra_scripts/IterateDataScript.java) | Демонстрирует обход объектов данных. |
| [IterateFunctionsByAddressScript.java](../Ghidra/Features/Base/ghidra_scripts/IterateFunctionsByAddressScript.java) | Обходит функции в адресном порядке. |
| [IterateFunctionsScript.java](../Ghidra/Features/Base/ghidra_scripts/IterateFunctionsScript.java) | Демонстрирует общий обход функций. |
| [IterateInstructionsScript.java](../Ghidra/Features/Base/ghidra_scripts/IterateInstructionsScript.java) | Демонстрирует обход инструкций. |
| [LabelDataScript.java](../Ghidra/Features/Base/ghidra_scripts/LabelDataScript.java) | Создаёт метки для данных без перезаписи пользовательских. |
| [LabelDirectFunctionReferencesScript.java](../Ghidra/Features/Base/ghidra_scripts/LabelDirectFunctionReferencesScript.java) | Помечает прямые ссылки на функции. |
| [LabelIndirectReferencesScript.java](../Ghidra/Features/Base/ghidra_scripts/LabelIndirectReferencesScript.java) | Помечает косвенные ссылки. |
| [LabelIndirectStringReferencesScript.java](../Ghidra/Features/Base/ghidra_scripts/LabelIndirectStringReferencesScript.java) | Помечает косвенные ссылки на строки. |
| [LanguagesAPIDemoScript.java](../Ghidra/Features/Base/ghidra_scripts/LanguagesAPIDemoScript.java) | Демонстрирует API языков Sleigh и p-code. |
| [LinuxSystemMapImportScript.java](../Ghidra/Features/Base/ghidra_scripts/LinuxSystemMapImportScript.java) | Импортирует символы Linux `System.map`. |
| [LocateMemoryAddressesForFileOffset.java](../Ghidra/Features/Base/ghidra_scripts/LocateMemoryAddressesForFileOffset.java) | Находит адреса памяти для file offset. |
| [LocateMemoryAddressesForFileOffset.py](../Ghidra/Features/Base/ghidra_scripts/LocateMemoryAddressesForFileOffset.py) | Python/Jython-версия поиска адресов по file offset. |

### M--R

| Файл | Что делает |
|---|---|
| [MachO_Script.java](../Ghidra/Features/Base/ghidra_scripts/MachO_Script.java) | Разбирает Mach-O-файл. |
| [MakeFunctionsInlineVoidScript.java](../Ghidra/Features/Base/ghidra_scripts/MakeFunctionsInlineVoidScript.java) | Создаёт inline void-функции. |
| [MakeFunctionsScript.java](../Ghidra/Features/Base/ghidra_scripts/MakeFunctionsScript.java) | Ищет байтовый шаблон и создаёт функции. |
| [MakeStackRefs.java](../Ghidra/Features/Base/ghidra_scripts/MakeStackRefs.java) | Создаёт ссылки на стековые переменные. |
| [MarkCallOtherPcode.java](../Ghidra/Features/Base/ghidra_scripts/MarkCallOtherPcode.java) | Отмечает инструкции с `CALLOTHER`. |
| [MarkUnimplementedPcode.java](../Ghidra/Features/Base/ghidra_scripts/MarkUnimplementedPcode.java) | Создаёт bookmarks для `UNIMPLEMENTED` p-code. |
| [MarkupWallaceSrcScript.java](../Ghidra/Features/Base/ghidra_scripts/MarkupWallaceSrcScript.java) | Делает специализированную разметку Wallace-примера. |
| [Mips_Fix_T9_PositionIndependentCode.java](../Ghidra/Features/Base/ghidra_scripts/Mips_Fix_T9_PositionIndependentCode.java) | Исправляет обработку `t9` в MIPS PIC-коде. |
| [MultiInstructionMemReference.java](../Ghidra/Features/Base/ghidra_scripts/MultiInstructionMemReference.java) | Создаёт ссылку на адрес, вычисленный несколькими инструкциями. |
| [NameStringPointersPlus.java](../Ghidra/Features/Base/ghidra_scripts/NameStringPointersPlus.java) | Именует указатели на строки. |
| [OpenSourceFileAtLineInEclipseScript.java](../Ghidra/Features/Base/ghidra_scripts/OpenSourceFileAtLineInEclipseScript.java) | Открывает исходник в Eclipse по source mapping. |
| [OpenSourceFileAtLineInVSCodeScript.java](../Ghidra/Features/Base/ghidra_scripts/OpenSourceFileAtLineInVSCodeScript.java) | Открывает исходник в VS Code по адресу и строке. |
| [Override_ARM_Call_JumpsScript.java](../Ghidra/Features/Base/ghidra_scripts/Override_ARM_Call_JumpsScript.java) | Переопределяет ARM call/jump-переходы. |
| [PE_script.java](../Ghidra/Features/Base/ghidra_scripts/PE_script.java) | Размечает структуры PE-заголовка. |
| [PEF_script.java](../Ghidra/Features/Base/ghidra_scripts/PEF_script.java) | Разбирает формат PEF. |
| [PasteCopiedListingBytesScript.java](../Ghidra/Features/Base/ghidra_scripts/PasteCopiedListingBytesScript.java) | Вставляет байты из скопированного Listing. |
| [PortableExecutableRichPrintScript.java](../Ghidra/Features/Base/ghidra_scripts/PortableExecutableRichPrintScript.java) | Печатает сведения Rich Header. |
| [PrintFunctionCallTreesScript.java](../Ghidra/Features/Base/ghidra_scripts/PrintFunctionCallTreesScript.java) | Печатает деревья вызовов функций. |
| [PrintStructureScript.java](../Ghidra/Features/Base/ghidra_scripts/PrintStructureScript.java) | Печатает структуру и поля типа данных. |
| [ProgressExampleScript.java](../Ghidra/Features/Base/ghidra_scripts/ProgressExampleScript.java) | Показывает progress monitor и отмену. |
| [PropagateConstantReferences.java](../Ghidra/Features/Base/ghidra_scripts/PropagateConstantReferences.java) | Распространяет константы и создаёт вычисленные ссылки. |
| [PropagateExternalParametersScript.java](../Ghidra/Features/Base/ghidra_scripts/PropagateExternalParametersScript.java) | Переносит имена и типы параметров внешних функций. |
| [PropagateX86ConstantReferences.java](../Ghidra/Features/Base/ghidra_scripts/PropagateX86ConstantReferences.java) | Выполняет x86-специализированное распространение ссылок. |
| [RecursiveStringFinder.py](../Ghidra/Features/Base/ghidra_scripts/RecursiveStringFinder.py) | Строит дерево вызовов и ищет строки в вызываемых функциях. |
| [RegisterTouchesPerFunction.java](../Ghidra/Features/Base/ghidra_scripts/RegisterTouchesPerFunction.java) | Собирает статистику использования регистров. |
| [ReloadSleighLanguage.java](../Ghidra/Features/Base/ghidra_scripts/ReloadSleighLanguage.java) | Перезагружает язык Sleigh. |
| [RemoveDeletedOverlayReferences.java](../Ghidra/Features/Base/ghidra_scripts/RemoveDeletedOverlayReferences.java) | Удаляет ссылки на удалённые overlay-пространства. |
| [RemoveSourceLanguage.java](../Ghidra/Features/Base/ghidra_scripts/RemoveSourceLanguage.java) | Удаляет сведения о языке исходного кода. |
| [RemoveSourceMapEntryScript.java](../Ghidra/Features/Base/ghidra_scripts/RemoveSourceMapEntryScript.java) | Удаляет source-map запись. |
| [RemoveSymbolQuotesScript.java](../Ghidra/Features/Base/ghidra_scripts/RemoveSymbolQuotesScript.java) | Убирает кавычки из имён символов. |
| [RemoveUserCheckoutsScript.java](../Ghidra/Features/Base/ghidra_scripts/RemoveUserCheckoutsScript.java) | Завершает checkout-файлов пользователя. |
| [RenameProgramsInProjectScript.java](../Ghidra/Features/Base/ghidra_scripts/RenameProgramsInProjectScript.java) | Переименовывает программы в проекте. |
| [RenameStructMembers.java](../Ghidra/Features/Base/ghidra_scripts/RenameStructMembers.java) | Массово переименовывает поля структур. |
| [RenameVariable.java](../Ghidra/Features/Base/ghidra_scripts/RenameVariable.java) | Переименовывает переменную по правилу. |
| [RepairDisassemblyScript.java](../Ghidra/Features/Base/ghidra_scripts/RepairDisassemblyScript.java) | Помогает исправлять ошибки дизассемблирования. |
| [RepairFuncDefinitionUsageScript.java](../Ghidra/Features/Base/ghidra_scripts/RepairFuncDefinitionUsageScript.java) | Исправляет использование определений функций. |
| [ReplaceInComments.java](../Ghidra/Features/Base/ghidra_scripts/ReplaceInComments.java) | Заменяет текст в комментариях. |
| [ReportDisassemblyErrors.java](../Ghidra/Features/Base/ghidra_scripts/ReportDisassemblyErrors.java) | Выводит ошибки дизассемблирования. |
| [ReportPercentDisassembled.java](../Ghidra/Features/Base/ghidra_scripts/ReportPercentDisassembled.java) | Считает процент дизассемблированной памяти. |
| [RepositoryFileUpgradeScript.java](../Ghidra/Features/Base/ghidra_scripts/RepositoryFileUpgradeScript.java) | Обновляет repository-файлы. |
| [ResolveExternalReferences.java](../Ghidra/Features/Base/ghidra_scripts/ResolveExternalReferences.java) | Разрешает внешние ссылки. |
| [ResolveX86orX64LinuxSyscallsScript.java](../Ghidra/Features/Base/ghidra_scripts/ResolveX86orX64LinuxSyscallsScript.java) | Распознаёт Linux syscall x86/x64. |

### Search--Z

| Файл | Что делает |
|---|---|
| [SearchBaseExtended.java](../Ghidra/Features/Base/ghidra_scripts/SearchBaseExtended.java) | Общая основа поиска инструкций с масками. |
| [SearchForImageBaseOffsets.java](../Ghidra/Features/Base/ghidra_scripts/SearchForImageBaseOffsets.java) | Ищет байтовые представления image-base offset. |
| [SearchForImageBaseOffsetsScript.java](../Ghidra/Features/Base/ghidra_scripts/SearchForImageBaseOffsetsScript.java) | Расширяет поиск ссылок на image base для 32/64 бит. |
| [SearchGuiMulti.java](../Ghidra/Features/Base/ghidra_scripts/SearchGuiMulti.java) | GUI-поиск нескольких instruction-шаблонов. |
| [SearchGuiSingle.java](../Ghidra/Features/Base/ghidra_scripts/SearchGuiSingle.java) | GUI-поиск одного instruction-шаблона. |
| [SearchMemoryForStringsRegExScript.java](../Ghidra/Features/Base/ghidra_scripts/SearchMemoryForStringsRegExScript.java) | Ищет байты в памяти регулярным выражением. |
| [SearchMnemonicsNoOpsNoConstScript.java](../Ghidra/Features/Base/ghidra_scripts/SearchMnemonicsNoOpsNoConstScript.java) | Ищет мнемоники без операндов и констант. |
| [SearchMnemonicsOpsConstScript.java](../Ghidra/Features/Base/ghidra_scripts/SearchMnemonicsOpsConstScript.java) | Ищет мнемоники с операндами и константами. |
| [SearchMnemonicsOpsNoConstScript.java](../Ghidra/Features/Base/ghidra_scripts/SearchMnemonicsOpsNoConstScript.java) | Ищет мнемоники с операндами без констант. |
| [SelectAddressesMappedToSourceFileScript.java](../Ghidra/Features/Base/ghidra_scripts/SelectAddressesMappedToSourceFileScript.java) | Выделяет адреса, связанные с исходным файлом. |
| [SelectFunctionsScript.java](../Ghidra/Features/Base/ghidra_scripts/SelectFunctionsScript.java) | Выделяет функции по условиям. |
| [SetEquateScript.java](../Ghidra/Features/Base/ghidra_scripts/SetEquateScript.java) | Создаёт или задаёт equate. |
| [SetHeadlessContinuationOptionScript.java](../Ghidra/Features/Base/ghidra_scripts/SetHeadlessContinuationOptionScript.java) | Показывает настройку продолжения headless-анализа. |
| [ShowEquatesInSelectionScript.java](../Ghidra/Features/Base/ghidra_scripts/ShowEquatesInSelectionScript.java) | Выводит equate в выделении. |
| [ShowSourceMapEntryStartsScript.java](../Ghidra/Features/Base/ghidra_scripts/ShowSourceMapEntryStartsScript.java) | Показывает начала source-map записей. |
| [SplitMultiplePefContainersScript.java](../Ghidra/Features/Base/ghidra_scripts/SplitMultiplePefContainersScript.java) | Разделяет несколько PEF-контейнеров. |
| [SplitUniversalBinariesScript.java](../Ghidra/Features/Base/ghidra_scripts/SplitUniversalBinariesScript.java) | Разделяет Apple Universal Binary по архитектурам. |
| [SubsToFuncsScript.java](../Ghidra/Features/Base/ghidra_scripts/SubsToFuncsScript.java) | Преобразует подпрограммы в функции. |
| [SynchronizeGDTCategoryPaths.java](../Ghidra/Features/Base/ghidra_scripts/SynchronizeGDTCategoryPaths.java) | Синхронизирует пути категорий GDT. |
| [TestPrototypeScript.java](../Ghidra/Features/Base/ghidra_scripts/TestPrototypeScript.java) | Тестирует prototype/calling convention через эмулятор. |
| [TranslateStringsScript.java](../Ghidra/Features/Base/ghidra_scripts/TranslateStringsScript.java) | Обрабатывает или переводит строки. |
| [TurnOffStackAnalysis.java](../Ghidra/Features/Base/ghidra_scripts/TurnOffStackAnalysis.java) | Отключает stack-анализатор в опциях анализа. |
| [VersionControl_AddAll.java](../Ghidra/Features/Base/ghidra_scripts/VersionControl_AddAll.java) | Добавляет подходящие файлы в version control. |
| [VersionControl_ResetAll.java](../Ghidra/Features/Base/ghidra_scripts/VersionControl_ResetAll.java) | Сбрасывает изменения version control. |
| [VersionControl_UndoAllCheckout.java](../Ghidra/Features/Base/ghidra_scripts/VersionControl_UndoAllCheckout.java) | Отменяет checkout-операции. |
| [VersionControl_VersionSummary.java](../Ghidra/Features/Base/ghidra_scripts/VersionControl_VersionSummary.java) | Печатает сводку версий и состояния файлов. |
| [XorMemoryScript.java](../Ghidra/Features/Base/ghidra_scripts/XorMemoryScript.java) | Выполняет XOR над байтами памяти. |
| [YaraGhidraGUIScript.java](../Ghidra/Features/Base/ghidra_scripts/YaraGhidraGUIScript.java) | Генерирует YARA-правило по выбранным инструкциям. |
| [ZapBCTRScript.java](../Ghidra/Features/Base/ghidra_scripts/ZapBCTRScript.java) | Специализированно обрабатывает PowerPC-переходы через `BCTR`. |
| [RunYARAFromGhidra.py](../Ghidra/Features/Base/ghidra_scripts/RunYARAFromGhidra.py) | Запускает внешний YARA и пишет совпавшие правила в комментарии. |
| [mark_in_out.py](../Ghidra/Features/Base/ghidra_scripts/mark_in_out.py) | Создаёт I/O-ссылки для overlay-пространства `IOMEM`. |
| [world.png](../Ghidra/Features/Base/ghidra_scripts/world.png) | Графический ресурс демонстрационного примера. |

## Самые важные скрипты подробно

### 1. Импорт: `ImportProgramScript`

Источник: [`ImportProgramScript.java`](../Ghidra/Features/Base/ghidra_scripts/ImportProgramScript.java).
Скрипт сначала вызывает `importFile(file)`, то есть даёт зарегистрированным
загрузчикам определить формат. Если это не получилось, он использует
`importFileAsBinary` с языком x86 и CompilerSpec по умолчанию, а затем открывает
созданный объект через `openProgram(program)`.

Практический сценарий: неизвестный файл можно быстро открыть как raw x86:

```text
1. Запустить ImportProgramScript.
2. Выбрать файл.
3. Если формат не распознан, получить бинарную программу x86.
4. Проверить правильность языка и адреса загрузки вручную.
```

Важно: fallback не универсален. Для ARM или MIPS нужно импортировать файл с
правильным языком, иначе байты будут интерпретированы неверно.

### 2. Создание функций по выделению

Источник: [`CreateFunctionsFromSelection.java`](../Ghidra/Features/Base/ghidra_scripts/CreateFunctionsFromSelection.java).
Скрипт проходит адреса через `currentSelection.getAddresses(true)`, пропускает
адреса, уже принадлежащие функции, и вызывает `createFunction(addrStart, null)`.

Пример: после поиска мёртвых подпрограмм выделить найденную область и создать
функции. Выделять нужно именно возможные entry points, а не весь большой блок:
скрипт проверяет каждый адрес, поэтому широкое выделение может создать ложные
функции или дать первой функции слишком большое тело.

### 3. Поиск неопределённых функций по прологу

Источник: [`FindUndefinedFunctionsScript.java`](../Ghidra/Features/Base/ghidra_scripts/FindUndefinedFunctionsScript.java).
Скрипт ищет только неопределённые данные внутри исполняемых memory blocks. Для
x86 Windows он знает пролог `55 8B EC`, для GCC -- `55 89 E5`; также есть
ограниченные шаблоны ARM и PowerPC. Совпадение дизассемблируется и превращается
в функцию.

Это полезно после неполного автоанализа, но не является доказательством функции:
байтовый пролог может встретиться в данных, а функция может не иметь ожидаемого
пролога. Неподдерживаемый processor приводит к ошибке.

### 4. Пользовательский шаблон: `MakeFunctionsScript`

Источник: [`MakeFunctionsScript.java`](../Ghidra/Features/Base/ghidra_scripts/MakeFunctionsScript.java).
Пользователь вводит байты через `askBytes`, после чего скрипт ищет шаблон в
исполняемых блоках и вызывает `disassemble` и `createFunction` для совпадений.
Если у программы один memory block, скрипт дополнительно делит его на код и
данные через `memory.split` и меняет флаг `execute`.

Пример: найти нестандартный пролог `F3 0F 1E FA` и проверить совпадения как
кандидаты функций. Перед запуском сохраните копию: деление единственного блока
меняет карту памяти, а поиск байтов не понимает границы инструкций.

### 5. Распространение констант

Источник: [`PropagateConstantReferences.java`](../Ghidra/Features/Base/ghidra_scripts/PropagateConstantReferences.java).
Область берётся из выделения, функции под курсором или одного текущего адреса.
Затем `SymbolicPropogator` проходит тело функции, а
`ConstantPropagationContextEvaluator` создаёт ссылки там, где вычисленный адрес
можно считать константным.

Например, цепочка `mov eax, 0x401000` может получить ссылку на `0x401000`.
Результат зависит от предположений constant propagation: изменяемая память,
сложные ветвления и неизвестные значения регистров могут дать ошибочную ссылку.

### 6. Исправление инструкции через assembler API

Источник: [`AssembleScript.java`](../Ghidra/Features/Base/ghidra_scripts/AssembleScript.java).
`Assemblers.getAssembler(currentProgram)` выбирает assembler для языка текущей
программы, `askString` получает текст, а `asm.assemble(currentAddress, text)`
записывает результат по адресу курсора.

Пример: заменить условный переход `JZ` на `JNZ`. Безопаснее менять инструкцию
на такой же длины. Если новая команда длиннее, она может перезаписать соседние
байты, нарушить Listing и поток управления; исходная копия байтов скриптом не
сохраняется.

### 7. Экспорт функций

Источник: [`ExportFunctionInfoScript.java`](../Ghidra/Features/Base/ghidra_scripts/ExportFunctionInfoScript.java).
Скрипт ничего не меняет в `Program`. Он экспортирует простой JSON с именем и
entry point, подробные JSON/CSV с размером, числом инструкций, блоков,
параметров, callers/callees и цикломатической сложностью, либо DOT-граф вызовов.
Если есть выделение или подсветка, экспорт ограничивается пересекающими их
функциями.

Пример: выбрать `DOT (Function call graph)`, сохранить файл и открыть его в
Graphviz. Граф будет настолько точным, насколько точны существующие функции и
references.

### 8. Поиск байтов регулярным выражением

Источник: [`SearchMemoryForStringsRegExScript.java`](../Ghidra/Features/Base/ghidra_scripts/SearchMemoryForStringsRegExScript.java).
Скрипт читает каждый memory range, преобразует байты в `ISO-8859-1`, применяет
Java `Pattern` и показывает адреса совпадений. При наличии выделения поиск идёт
только в нём; иначе -- во всей памяти.

Пример regex для URL: `https?://[^\x00]+`. Unicode/UTF-16 автоматически не
распознаётся, каждый диапазон целиком помещается в массив, а после 500 совпадений
скрипт останавливается.

### 9. Деманглинг символов

Источник: [`DemangleAllScript.java`](../Ghidra/Features/Base/ghidra_scripts/DemangleAllScript.java).
Скрипт перебирает defined symbols, пропускает `SourceType.DEFAULT`, имена
`s_`, `u_`, `AddrTable` и применяет `DemanglerCmd`. Поддерживаются типичные
Microsoft и GNU mangled names; при успехе обновляются имя и доступная сигнатура.

Пример: имя C++ вроде `_ZN3Foo3barEi` становится читаемым именем пространства,
метода и параметра. Повреждённые имена не деманглируются, а уже существующая
ручная разметка может быть изменена, поэтому полезно сначала сохранить программу.

### 10. Исправление offcut-инструкций

Источник: [`FixOffcutInstructionScript.java`](../Ghidra/Features/Base/ghidra_scripts/FixOffcutInstructionScript.java).
Offcut -- это ситуация, когда поток управления входит в середину уже найденной
инструкции. Скрипт проверяет ссылки переходов, выравнивание и возможность
псевдодизассемблирования через `PseudoDisassembler`, затем устанавливает
`setLengthOverride`, дизассемблирует offcut и заменяет error bookmark на
информационный.

Сценарий подходит для статических полиглотных инструкций, некоторых оптимизаций
и обфускации. Динамически вычисляемые переходы не подтверждаются этим методом;
даже успешное исправление основано на локальном предположении о потоке.

### 11. Поиск нереализованного p-code

Источник: [`MarkUnimplementedPcode.java`](../Ghidra/Features/Base/ghidra_scripts/MarkUnimplementedPcode.java).
Обрабатывается выделение или всё исполняемое множество. Старые предупреждения
сначала очищаются, затем у инструкции с единственной операцией
`PcodeOp.UNIMPLEMENTED` создаётся `WARNING` bookmark.

Это удобно после изменения или обновления Sleigh-языка: bookmark показывает,
где декомпилятор не знает семантику инструкции. Скрипт не исправляет p-code и не
ловит частично неверную семантику, если массив p-code не состоит ровно из одной
операции `UNIMPLEMENTED`.

### 12. Генерация YARA-правила

Источник: [`YaraGhidraGUIScript.java`](../Ghidra/Features/Base/ghidra_scripts/YaraGhidraGUIScript.java).
Скрипт открывает диалог Instruction Pattern Search, загружает выбранные
инструкции и позволяет маскировать мнемоники или операнды. Метод
`generateYaraString` превращает битовую маску в hex-строку YARA, используя `?`
для неизвестных полубайтов.

Пример: выделить функцию, скрыть адреса и константы, получить правило для поиска
похожего кода в наборе образцов. Нужен установленный Instruction Pattern Search
plugin; YARA умеет байтовые и полубайтовые маски, но не полную семантику инструкции.

### 13. ARM switch table

Источник: [`AddReferencesInSwitchTable.java`](../Ghidra/Features/Base/ghidra_scripts/AddReferencesInSwitchTable.java).
До запуска таблица должна быть определена как `byte`, `word` или `dword`, а курсор
должен стоять на `add pc, ...`. Для каждой непрерывной записи скрипт вычисляет
`pc = currentAddress + 4`, затем адрес обработчика как `pc + 2 * value` и добавляет
`RefType.COMPUTED_JUMP`.

Это исправляет конкретный формат ARM switch table, а не все варианты ARM. Для
другой формулы, разреженной таблицы или другого типа данных адреса будут неверными.

## Быстрый выбор

| Задача | С чего начать |
|---|---|
| Научиться писать скрипты | `HelloWorldScript`, `AskScript`, `IterateInstructionsScript` |
| Найти текст или байты | `FindTextScript`, `SearchMemoryForStringsRegExScript`, `InstructionSearchScript` |
| Восстановить функции | `CreateFunctionsFromSelection`, `FindUndefinedFunctionsScript`, `MakeFunctionsScript` |
| Уточнить ссылки | `PropagateConstantReferences`, `CreateOperandReferencesInSelectionScript` |
| Исправить машинный код | `AssembleScript`, `EditBytesScript` |
| Разобраться с именами | `DemangleSymbolScript`, `DemangleAllScript` |
| Экспортировать анализ | `ExportFunctionInfoScript`, `PrintFunctionCallTreesScript` |
| Проверить проблемы дизассемблирования | `FixOffcutInstructionScript`, `MarkUnimplementedPcode`, `ReportDisassemblyErrors` |
| Сделать сигнатуру для YARA | `YaraGhidraGUIScript` или `RunYARAFromGhidra.py` |
