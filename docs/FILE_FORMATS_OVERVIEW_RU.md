# `Ghidra/Features/FileFormats`: зачем нужен этот модуль

Этот документ объясняет новичку, что находится в
[`Ghidra/Features/FileFormats`](../Ghidra/Features/FileFormats), почему там встречается
слово «файловая система» и как этот модуль связан с анализом PE, ELF, DEX и других
бинарников.

Для общей карты репозитория сначала полезен
[`ARCHITECTURE_OVERVIEW_RU.md`](ARCHITECTURE_OVERVIEW_RU.md). Для базовых API модели
`Program`, памяти и listing см. [`FRAMEWORK_OVERVIEW_RU.md`](FRAMEWORK_OVERVIEW_RU.md).
Разбор обычного PE находится в разделе 5 архитектурного обзора: его основной код
расположен в [`Ghidra/Features/Base/.../format/pe`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe),
а не в этом модуле.

## Короткий ответ

`FileFormats` нужен не для сканирования файловой системы компьютера. Он умеет
читать сложные бинарные контейнеры, архивы, образы дисков и прошивок, сжатые или
зашифрованные вложения, а также некоторые форматы метаданных. Задача обычно такая:

```text
один контейнер (APK, boot.img, DMG, ZIP, firmware image)
  -> показать его вложенные логические файлы
  -> извлечь ByteProvider для выбранного файла
  -> передать его соответствующему загрузчику/анализатору
  -> получить Program с памятью, функциями и дизассемблированием
```

Например, APK -- это ZIP-контейнер. Внутри могут находиться `classes.dex`,
нативная библиотека `lib/arm64-v8a/libfoo.so` и бинарный Android Manifest.
Чтобы анализировать `libfoo.so`, Ghidra сначала должна найти и прочитать его
внутри APK. Только после этого обычный ELF-загрузчик может создать `Program`.

## 1. Два разных значения слова «файл»

### 1.1. Файл как входной бинарник загрузчика

Загрузчик программы отвечает на вопрос: «как эти байты становятся программой?»
Он определяет архитектуру, адреса загрузки, секции, entry point и создаёт объект
`Program`. Например, PE-разборщик находится в
[`PortableExecutable.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/pe/PortableExecutable.java),
а PE-анализатор -- в
[`PortableExecutableAnalyzer.java`](../Ghidra/Features/Base/src/main/java/ghidra/app/analyzers/PortableExecutableAnalyzer.java).

### 1.2. Файл как элемент контейнера

ZIP, APK, TAR, Android boot image или файловый образ могут содержать другие
файлы. Для них удобно использовать общий API виртуальной файловой системы
(`GFile`, `GFileSystem`, `ByteProvider`). Реализация этого API находится в
`Ghidra/Features/Base`, например:

- [`GFileSystem.java`](../Ghidra/Features/Base/src/main/java/ghidra/formats/gfilesystem/GFileSystem.java)
  задаёт операции списка, атрибутов и чтения;
- [`GFileSystemBase.java`](../Ghidra/Features/Base/src/main/java/ghidra/formats/gfilesystem/GFileSystemBase.java)
  даёт базовую реализацию простых файловых систем;
- [`FileSystemService.java`](../Ghidra/Features/Base/src/main/java/ghidra/formats/gfilesystem/FileSystemService.java)
  выбирает подходящую реализацию и монтирует контейнер.

Здесь «монтировать» означает не подключать диск к Windows. Это значит создать
объект, который умеет отвечать на запросы вроде `lookup("/classes.dex")` и
возвращать поток байтов нужного диапазона исходного контейнера.

## 2. Что находится в каталоге

Корневые файлы показывают, что это полноценный Gradle-модуль:

- [`build.gradle`](../Ghidra/Features/FileFormats/build.gradle) подключает `Base`,
  `Recognizers`, `PDB`, библиотеки dex2jar, smali, ZIP4J и AXMLPrinter2;
- [`Module.manifest`](../Ghidra/Features/FileFormats/Module.manifest) содержит
  метаданные модуля;
- [`src/main/java`](../Ghidra/Features/FileFormats/src/main/java) содержит
  Java-реализации загрузчиков, файловых систем, декодеров и анализаторов;
- [`src/main/help`](../Ghidra/Features/FileFormats/src/main/help) содержит help
  для действий File System Browser;
- [`src/test`](../Ghidra/Features/FileFormats/src/test) содержит тесты форматов и
  декодеров;
- [`src/lzfse`](../Ghidra/Features/FileFormats/src/lzfse) содержит native-код
  декодера Apple LZFSE, который включается в дистрибутив через `build.gradle`;
- [`ghidra_scripts`](../Ghidra/Features/FileFormats/ghidra_scripts) и
  [`developer_scripts`](../Ghidra/Features/FileFormats/developer_scripts) содержат
  специализированные скрипты, а не основной pipeline анализа.

В `src/main/java/ghidra/file/formats` сейчас есть группы для Android, iOS,
архивов и компрессии, файловых систем образов и старых объектных форматов:
[`formats`](../Ghidra/Features/FileFormats/src/main/java/ghidra/file/formats).
Это не означает, что каждый каталог превращает данные в `Program`. Часть классов
только извлекает байты или описывает структуру, а отдельный loader затем создаёт
`Program`.

## 3. Как работает виртуальная файловая система

Типичный формат состоит из двух частей:

1. `*FileSystemFactory` проверяет сигнатуру или структуру входных байтов и создаёт
   файловую систему.
2. `*FileSystem` читает заголовок, строит список `GFile` и реализует получение
   `ByteProvider` для вложенного объекта.

Простой пример -- ZIP. Аннотация в
[`ZipFileSystem.java`](../Ghidra/Features/FileFormats/src/main/java/ghidra/file/formats/zip/ZipFileSystem.java)
объявляет тип `zip`, расширения `zip` и `jar`, а метод `mount()` читает записи
архива и передаёт их в индекс. `getInputStream()` и `getByteProvider()` затем
отдают содержимое выбранной записи. Поддерживаются также ZIP-записи с паролем
через Zip4j.

Упрощённый сценарий выглядит так:

```java
try (ZipFileSystem zip = openAPK(provider, monitor)) {
    GFile nativeFile = zip.lookup("/lib/arm64-v8a/libfoo.so");
    try (ByteProvider bytes = zip.getByteProvider(nativeFile, monitor)) {
        // Эти байты можно передать ELF loader-у.
    }
}
```

В реальном коде APK loader использует именно эту схему: см.
[`ApkLoader.java#L52-L61`](../Ghidra/Features/FileFormats/src/main/java/ghidra/app/util/opinion/ApkLoader.java#L52-L61),
[`ApkLoader.java#L73-L96`](../Ghidra/Features/FileFormats/src/main/java/ghidra/app/util/opinion/ApkLoader.java#L73-L96)
и поиск через `FileSystemService` в
[`ApkLoader.java#L265-L285`](../Ghidra/Features/FileFormats/src/main/java/ghidra/app/util/opinion/ApkLoader.java#L265-L285).

## 4. Главный пример: APK

APK хорошо показывает, зачем анализировать «файловую систему» контейнера.
Физически это один ZIP-файл, но для анализа важны разные вложенные объекты:

```text
game.apk
|- AndroidManifest.xml  -- конфигурация приложения
|- classes.dex          -- байткод Dalvik/ART
|- lib/arm64-v8a/*.so   -- нативные ELF-библиотеки
|- res/*                -- ресурсы
```

`ApkLoader` сначала быстро проверяет ZIP-сигнатуру, открывает `ZipFileSystem`,
ищет Manifest и `classes.dex`. Manifest читается через
[`AndroidXmlFileSystem.java`](../Ghidra/Features/FileFormats/src/main/java/ghidra/file/formats/android/xml/AndroidXmlFileSystem.java),
который превращает бинарный Android XML в один логический текстовый файл.
Это нужно не для дизассемблирования XML, а чтобы определить версию Android и
выбрать подходящее описание языка/ABI.

Затем `ApkLoader` перебирает `classes.dex`, `classes2.dex` и следующие DEX-файлы
и делегирует собственно загрузку базовому
[`DexLoader.java`](../Ghidra/Features/FileFormats/src/main/java/ghidra/app/util/opinion/DexLoader.java).
`DexLoader` создаёт блоки памяти, раскладывает байткод методов, строит таблицы
поиска и символы методов, после чего получается отдельный `Program` для DEX.

Итого: виртуальная файловая система здесь -- подготовительный слой, а не конечная
цель анализа. Она позволяет добраться до DEX или ELF, не распаковывая APK вручную.

## 5. Android boot image и вложенные уровни

Android boot image -- уже не обычный архив. В его заголовке находятся размеры и
смещения kernel, ramdisk и иногда второго загрузчика. Реализация
[`BootImageFileSystem.java`](../Ghidra/Features/FileFormats/src/main/java/ghidra/file/formats/android/bootimg/BootImageFileSystem.java)
проверяет magic `ANDROID!`, создаёт логические файлы `kernel`, `ramdisk` и
`second`, а `getByteProvider()` возвращает соответствующий диапазон исходного
образа.

Практическая цепочка может быть многоуровневой:

```text
boot.img
  -> ramdisk
  -> GZIP
  -> CPIO
  -> файл init или другая программа/конфигурация
```

В коде boot image прямо поясняется, что ramdisk -- это GZIP, содержащий CPIO,
а kernel можно анализировать как отдельный бинарник. Значит, Ghidra не пытается
считать весь `boot.img` инструкциями: она сначала отделяет компоненты по их
формату, затем применяет подходящий следующий обработчик.

## 6. Какие группы форматов покрывает модуль

### Архивы и сжатие

`zip`, `tar`, `cpio`, `7z`, `gzip`, `bzip2`, `zstd`, `zlib`, LZSS, LZFSE и
похожие реализации нужны, когда код или данные упакованы. Например, GZIP может
быть внешним слоем ramdisk, а ZIP -- внешним слоем APK. Для некоторых форматов
используются внешние CLI-инструменты через
[`ghidra/file/cliwrapper`](../Ghidra/Features/FileFormats/src/main/java/ghidra/file/cliwrapper).

### Файловые системы и образы

`ext4`, `squashfs`, `cramfs`, `yaffs2`, sparse image, UBI и другие форматы
представляют не просто архив, а образ файловой системы с inode, блоками,
размерами и смещениями. Это особенно полезно при исследовании прошивок и дампов:
можно открыть образ, найти `system/bin`, `vendor/lib64` или конфигурационный
файл и анализировать выбранный объект отдельно.

### Android

Пакет `android` включает APK/DEX, ODEX/VDEX/OAT/ART, boot image, binary XML,
LZ4-слои и Android bootloader. Например, каталог
[`android/dex`](../Ghidra/Features/FileFormats/src/main/java/ghidra/file/formats/android/dex)
содержит не только чтение DEX, но и виртуальные представления «DEX to JAR» и
«DEX to SMALI». В `build.gradle` видны зависимости dex2jar и smali, поэтому это
интеграция с представлениями/декомпиляцией Android, а не поддержка CPU-инструкций.

### iOS/macOS и Apple-форматы

Пакеты `ios`, `bplist`, `dmg`, `img2`, `img3`, `xar`, LZFSE и dyld cache нужны
для прошивок, Mach-O file set, кэшей библиотек и plist-метаданных. Дополнительные
плагины и loader-ы находятся в пакетах
[`ghidra/macosx`](../Ghidra/Features/FileFormats/src/main/java/ghidra/macosx)
и [`ghidra/app/util/opinion`](../Ghidra/Features/FileFormats/src/main/java/ghidra/app/util/opinion).

### Дополнительные структуры

Есть также COFF/OMF-архивы, Java class/JAR с обработкой через JAD, изображения,
криптографические decryptor-ы и форматные анализаторы. Например, Java-файловая
система может представить декомпилированное содержимое JAR; действие описано в
[`FileFormats.html`](../Ghidra/Features/FileFormats/src/main/help/help/topics/FileFormatsPlugin/FileFormats.html).

## 7. Что этот модуль не делает

- Он не анализирует произвольные каталоги на диске пользователя.
- Он не является универсальным антивирусным сканером.
- Он не заменяет дизассемблер и декомпилятор.
- Он не определяет семантику x86-инструкции: это задача описаний процессоров в
  [`Ghidra/Processors`](../Ghidra/Processors) и последующих анализаторов.
- Он не обязан создавать `Program`: `ZipFileSystem`, `BootImageFileSystem` и
  `AndroidXmlFileSystem` в основном извлекают логические файлы/байты, тогда как
  `ApkLoader` и `DexLoader` создают программы.

Полезная мысленная модель:

```text
FileFormats: «найди и дай мне правильный кусок байтов»
Loader:      «разложи этот кусок как PE/ELF/DEX/Mach-O в Program»
Analyzer:     «найди структуры, функции, ссылки и метаданные»
Decompiler:   «покажи результат в C-подобном виде»
```

Для PE это означает: если у нас обычный `game.exe`, начинать нужно с PE loader-а
и кода из `Features/Base`, а не с `FileFormats`. Если же PE лежит внутри ZIP,
APK, прошивки или другого контейнера, `FileFormats` нужен как мост до этого PE.

## 8. Как читать исходники при практическом анализе

Рекомендуемый маршрут для новичка:

1. Определить внешний контейнер: APK, ZIP, boot image, ext4, DMG или просто PE.
2. Найти соответствующий `*FileSystemFactory` и `*FileSystem` в
   [`src/main/java/ghidra/file/formats`](../Ghidra/Features/FileFormats/src/main/java/ghidra/file/formats).
3. Посмотреть `isValid()`/probe: по magic или заголовку видно, как формат
   распознаётся.
4. Посмотреть `open()`/`mount()`: там строится список `GFile` и вычисляются
   размеры/смещения.
5. Посмотреть `getByteProvider()`: это граница между контейнером и вложенным
   бинарником.
6. Найти loader, который получает этот `ByteProvider`, например
   [`ApkLoader`](../Ghidra/Features/FileFormats/src/main/java/ghidra/app/util/opinion/ApkLoader.java),
   и затем перейти к обычному анализу `Program`.

Тесты полезны как исполняемые примеры предположений о формате. Например, в
модуле есть тесты ZLIB и LZSS:
[`src/test/java/ghidra/file/formats`](../Ghidra/Features/FileFormats/src/test/java/ghidra/file/formats),
а набор тестов CART показывает проверку заголовка, footer-а, расшифровки и
извлечения payload.

## Итог

Название `FileFormats` шире, чем «форматы исполняемых файлов». В реальной
форензике бинарник часто находится внутри нескольких слоёв: APK содержит DEX и
ELF, boot image содержит ramdisk, ramdisk содержит архив, а прошивка содержит
файловую систему. Ghidra анализирует не файловую систему компьютера, а структуру
исследуемого бинарного контейнера. `FileFormats` аккуратно раскрывает эти слои,
после чего стандартные загрузчики и анализаторы работают с нужным бинарником
так, будто он был импортирован отдельно.
