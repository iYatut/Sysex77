# Карта кода Sysex77 (репозиторий `sy_99`)

Документ составлен по состоянию файлов в `Sysex77-master/` и шаблону `NewProject/`. Строки указаны по текущим версиям исходников (ориентир для поиска в IDE).

---

## Файлы `.js`, `.ts`, `.jsx`, `.tsx`, `.cpp`, `.h`, `.py`

Файлов `.js`, `.ts`, `.jsx`, `.tsx`, `.py` в рабочей копии **не найдено**. Далее — все `.cpp` и `.h` (пути от корня `sy_99`).

`Sysex77-master/Source/ADSR.h` — сборка сегментов ADSR/EG через `Segment`/`Hook` и привязка SysEx к каждому сегменту.

`Sysex77-master/Source/AlgoDraw.h` — отрисовка схемы алгоритма FM.

`Sysex77-master/Source/AfmVue.h` — вкладочный контейнер экрана AFM (OP, OSC, EG).

`Sysex77-master/Source/AWMVue.h` — AWM: таблица волн, слайдеры и OSC `/SYSEX`.

`Sysex77-master/Source/BankTableModel.h` — модель списка банков для библиотеки.

`Sysex77-master/Source/CommonFilter.h` — общие параметры фильтра (частоты, резонанс, MIDI к фильтру).

`Sysex77-master/Source/Config.h` — вкладка настроек, сохранение профиля в XML, OSC repaint.

`Sysex77-master/Source/Constructeur.h` — вспомогательные/конструкторские объявления проекта.

`Sysex77-master/Source/Controller.h` — страница MIDI-контроллеров (Foot/Mod) через OSC.

`Sysex77-master/Source/Declarations.h` — общие глобальные объявления UI/MIDI монитора (legacy).

`Sysex77-master/Source/DeviceModel.h` — модель выбранного синтезатора SY77/TG77/SY99 и SysEx device ID.

`Sysex77-master/Source/Element.h` — UI-карточка элемента голоса и команды редактирования.

`Sysex77-master/Source/Filter1.h` — первая конфигурация фильтра и её EG-слайдеры.

`Sysex77-master/Source/Filter2.h` — вторая конфигурация фильтра и её EG-слайдеры.

`Sysex77-master/Source/FilterControls.h` — стили и хелперы для MidiSlider/MidiButton.

`Sysex77-master/Source/FilterVue copie.h` — черновик/копия экрана фильтра с комбобоксами и EG.

`Sysex77-master/Source/FilterVue.h` — вкладочная оболочка фильтра (Common / Filter1 / Filter2).

`Sysex77-master/Source/Hook.h` — перетаскиваемые узлы EG и отправка OSC `/SYSEX` из `Segment`.

`Sysex77-master/Source/labo.h` — экспериментальный заголовок.

`Sysex77-master/Source/Librairie.h` — библиотека банков: SEND/RECEIVE через OSC и сохранение SysEx дампа.

`Sysex77-master/Source/LookAndFeel.h` — кастомное оформление интерфейса.

`Sysex77-master/Source/Main.cpp` — `JUCEApplication`, создание окна с `MidiDemo`.

`Sysex77-master/Source/MidiDemo.h` — главный экран: вкладки, MIDI устройства, монитор, OSC↔SysEx мост.

`Sysex77-master/Source/MidiObjects.h` — `MidiSlider`, `MidiButton`, `MidiCombo` и прочие MIDI-связанные виджеты.

`Sysex77-master/Source/MidiSysex.h` — тело методов OSC/`sendSysex`/`sendRaw`, включается внутрь `MidiDemo`.

`Sysex77-master/Source/Operator.h` — панель оператора AFM (алгоритм и параметры).

`Sysex77-master/Source/Oscillator.h` — шесть AFM-операторов: осцилляторы, fine, detune, phase, фикс/фаза-кнопки.

`Sysex77-master/Source/Pan.h` — Pan EG: слайдеры уровней/скоростей и SysEx-шаблоны.

`Sysex77-master/Source/Pitch.h` — сдвиг высоты и fine для элемента.

`Sysex77-master/Source/PitchEG.h` — огибающая высоты (pitch EG).

`Sysex77-master/Source/Redraw.h` — вспомогательная перерисовка.

`Sysex77-master/Source/ToggleBar.h` — UI-переключатель/панель.

`Sysex77-master/Source/Values.h` — цвета и общие константы приложения.

`Sysex77-master/Source/ValueTrees.h` — логика ValueTree для данных голоса (подключается из `Voice.h`).

`Sysex77-master/Source/VoicesTableModel.h` — таблицы голосов по банковым слотам.

`Sysex77-master/Source/Voice.h` — страница Voice: режим, элементы, master volume, правки имён.

`Sysex77-master/Source/VoiceMode.h` — перечисление режимов полифонии/AFM/AWM.

`Sysex77-master/Source/Volume.h` — amplitude EG громкости элемента.

`Sysex77-master/Source/WaveEg.h` — volume EG для волнового элемента.

`Sysex77-master/Source/WaveVue.h` — вкладки WAVE + связанные EG.

`Sysex77-master/JuceLibraryCode/AppConfig.h` — конфиг модулей JUCE для Sysex77.

`Sysex77-master/JuceLibraryCode/BinaryData.cpp` — определения встроенных бинарных ресурсов.

`Sysex77-master/JuceLibraryCode/BinaryData.h` — декларации BinaryData.

`Sysex77-master/JuceLibraryCode/include_juce_audio_basics.cpp` — include translation unit JUCE audio_basics.

`Sysex77-master/JuceLibraryCode/include_juce_audio_devices.cpp` — include translation unit JUCE audio_devices.

`Sysex77-master/JuceLibraryCode/include_juce_audio_formats.cpp` — include translation unit JUCE audio_formats.

`Sysex77-master/JuceLibraryCode/include_juce_audio_processors.cpp` — include translation unit JUCE audio_processors.

`Sysex77-master/JuceLibraryCode/include_juce_audio_processors_headless.cpp` — headless audio_processors.

`Sysex77-master/JuceLibraryCode/include_juce_audio_processors_headless_ara.cpp` — ARA headless.

`Sysex77-master/JuceLibraryCode/include_juce_audio_processors_headless_lv2_libs.cpp` — LV2 libs headless.

`Sysex77-master/JuceLibraryCode/include_juce_audio_utils.cpp` — include translation unit JUCE audio_utils.

`Sysex77-master/JuceLibraryCode/include_juce_core.cpp` — include translation unit JUCE core.

`Sysex77-master/JuceLibraryCode/include_juce_core_CompilationTime.cpp` — время компиляции JUCE.

`Sysex77-master/JuceLibraryCode/include_juce_data_structures.cpp` — include translation unit JUCE data_structures.

`Sysex77-master/JuceLibraryCode/include_juce_events.cpp` — include translation unit JUCE events.

`Sysex77-master/JuceLibraryCode/include_juce_graphics.cpp` — include translation unit JUCE graphics.

`Sysex77-master/JuceLibraryCode/include_juce_graphics_Harfbuzz.cpp` — HarfBuzz для graphics.

`Sysex77-master/JuceLibraryCode/include_juce_gui_basics.cpp` — include translation unit JUCE gui_basics.

`Sysex77-master/JuceLibraryCode/include_juce_gui_extra.cpp` — include translation unit JUCE gui_extra.

`Sysex77-master/JuceLibraryCode/include_juce_osc.cpp` — include translation unit JUCE OSC.

`Sysex77-master/JuceLibraryCode/JuceHeader.h` — общий заголовок JUCE проекта Sysex77.

`NewProject/Source/Main.cpp` — точка входа шаблонного приложения NewProject.

`NewProject/Source/MainComponent.cpp` — реализация пустого главного компонента.

`NewProject/Source/MainComponent.h` — заголовок главного компонента NewProject.

`NewProject/JuceLibraryCode/JuceHeader.h` — заголовок JUCE для NewProject.

`NewProject/JuceLibraryCode/include_juce_core.cpp` — JUCE core для NewProject.

`NewProject/JuceLibraryCode/include_juce_core_CompilationTime.cpp` — compilation time для NewProject.

`NewProject/JuceLibraryCode/include_juce_data_structures.cpp` — data_structures для NewProject.

`NewProject/JuceLibraryCode/include_juce_events.cpp` — events для NewProject.

`NewProject/JuceLibraryCode/include_juce_graphics.cpp` — graphics для NewProject.

`NewProject/JuceLibraryCode/include_juce_graphics_Harfbuzz.cpp` — HarfBuzz для NewProject.

`NewProject/JuceLibraryCode/include_juce_gui_basics.cpp` — gui_basics для NewProject.

---

## MIDI точки входа

Формат Yamaha parameter change в UI обычно: 9 байт `F0 43 <dev> 34 aa bb cc dd pp F7` (в коде без обрамления F0/F7 задаётся через `MidiMessage::createSysExMessage`). Ниже — функции по ролям.

### (a) Отправка MIDI / SysEx

| Функция | Файл | Строка (пример) | Назначение |
|---------|------|-----------------|------------|
| `MidiDemo::sendToOutputs` | `MidiDemo.h` | ~808 | Рассылает `MidiMessage` на все открытые MIDI outputs (`sendMessageNow`). |
| `MidiDemo::handleNoteOn` | `MidiDemo.h` | ~436 | Ноты On → MIDI out. |
| `MidiDemo::handleNoteOff` | `MidiDemo.h` | ~443 | Ноты Off → MIDI out. |
| `MidiDemo::buttonClicked` (`btBulk`) | `MidiDemo.h` | ~397 | SysEx bulk protect (байты `0x34 0x0f … 0x34`, значение из тумблера). |
| `MidiDemo::comboBoxChanged` | `MidiDemo.h` | ~923–940 | SysEx для назначений Foot (`0x2d`) и Modulation (`0x2c`) из комбобоксов. |
| `MidiDemo::oscMessageReceived` (ветки) | `MidiSysex.h` | ~55–199 | Приём OSC: `/SYSEX` → сборка SysEx; адреса панелей → `sendSysex`; банк файла → `sendRaw`; repaint и др. |
| `sendSysex` | `MidiSysex.h` | ~202 | Подставляет байт значения в шаблон SysEx и вызывает `sendToOutputs`. |
| `sendRaw` | `MidiSysex.h` | ~215 | Отправка сырого блока как MIDI (`MidiMessage(sysexData, dataSize)`). |
| `ControllerPage::comboBoxChanged` | `Controller.h` | ~66 | OSC на `adresseOscFoot` / `adresseOscMod` (не прямой MIDI; см. приём в `MidiSysex.h`). |
| `MidiSlider::sliderValueChanged` | `MidiObjects.h` | ~184 | Отправка OSC `/SYSEX` с 9 байтами (payload параметра). |
| `MidiButton::buttonClicked` | `MidiObjects.h` | ~325 | То же для переключателя. |
| `MidiCombo::comboBoxChanged` | `MidiObjects.h` | ~487 | То же для выпадающего списка. |
| `Segment::sendOsc` | `Hook.h` | ~154 | Два сообщения OSC `/SYSEX` для rate и level сегмента EG. |
| `TableTutorialComponent::selectedRowsChanged` | `AWMVue.h` | ~218 | Выбор строки таблицы волн → OSC `/SYSEX` (номер волны в байте [8]). |
| `VoicePage::comboBoxChanged` | `Voice.h` | ~364 | OSC `adresseOpMode` при смене режима голоса → SysEx в `MidiSysex.h`. |
| `VoicePage::textEditorReturnKeyPressed` | `Voice.h` | ~336 | Строит до 10 SysEx из символов имени и дергает OSC `oscSendMidiMessage` (логика массива сообщений — см. раздел stub). |
| `LibrairiePage::buttonClicked` (`btSend`) | `Librairie.h` | ~150 | OSC `adresseOscSendBank` → выбор файла банка для `sendRaw`. |
| `ConfigPage` (сохранение и OSC) | `Config.h` | ~98–101 | Запись XML пути приложения; OSC repaint (не SysEx). |

### (b) Приём входящих MIDI данных

| Функция | Файл | Строка | Назначение |
|---------|------|--------|------------|
| `MidiDemo::handleIncomingMidiMessage` | `MidiDemo.h` | ~666 | Колбэк `MidiInput`: фильтры Active Sense, сбор SysEx в `arraySysex` при `requestSysex`, очередь для монитора и async-обработки. |
| `MidiDemo::handleAsyncUpdate` | `MidiDemo.h` | ~700 | Разбор очереди: форматирование в монитор, извлечение параметров из 9-байтовых SysEx. |

### (c) Парсинг / интерпретация SysEx

| Функция | Файл | Строка | Назначение |
|---------|------|--------|------------|
| `MidiDemo::handleAsyncUpdate` | `MidiDemo.h` | ~761–767 | Если размер SysEx == 9 и `data[0]==0x43`, `data[2]==0x34`, выставляет `valueSysexIn` как массив `(data[3]…data[8])` для синхронизации UI. |
| `MidiDemo::handleIncomingMidiMessage` | `MidiDemo.h` | ~674–679 | При `requestSysex` накапливает входящие SysEx в `arraySysex` для дампа библиотеки. |
| `LibrairiePage::saveDump` | `Librairie.h` | ~118–145 | Склеивает сырые байты сообщений из `arraySysex` в `.syx` (без высокоуровневого разбора параметров). |
| `MidiSlider::valueChanged` | `MidiObjects.h` | ~110 | Сопоставляет `valueSysexIn` с локальным шаблоном `sysexData[3…6]` и обновляет позицию слайдера. |
| `MidiButton::valueChanged` | `MidiObjects.h` | ~288 | Аналогично для toggle-текста. |
| `MidiCombo::valueChanged` | `MidiObjects.h` | ~495 | Обновляет выбранный id из `valueSysexIn`. |
| `TableTutorialComponent::valueChanged` | `AWMVue.h` | ~52 | Подсветка/выбор строки таблицы по входящему SysEx. |

---

## UI привязки (элементы ↔ логика ↔ адрес параметра)

Общая схема: **`MidiSlider` / `MidiButton` / `MidiCombo`** задают шаблон через **`setMidiSysex(int[9])`**; при действии пользователя вызываются **`sliderValueChanged` / `buttonClicked` / `comboBoxChanged`** → **OSC `/SYSEX`** на `127.0.0.1:9001` → в **`MidiDemo::oscMessageReceived`** (`MidiSysex.h`) сообщение превращается в **`MidiMessage::createSysExMessage`** и уходит в **`sendToOutputs`**. Обратный путь: входящий MIDI SysEx → **`valueSysexIn`** → **`valueChanged`** у тех же виджетов.

Адрес параметра в таблице ниже: байты **`[3][4][5][6]`** шаблона (как в коде), значение — **`[8]`** (иногда с промежуточным **`[7]`** = 0 в шаблонах). Префикс **`43 <dev> 34`** задаётся в массивах; `<dev>` синхронизируется с `DeviceModel::getSysExDeviceID()` / `sysexEngine` где явно не переопределено.

### Комбобоксы и клавиатура (не через MidiSlider)

| UI элемент | Обработчик | Адрес / примечание |
|------------|------------|--------------------|
| `comboFoot` | `MidiDemo::comboBoxChanged` | `… 0x0f 00 00 0x2d 00` + CC тип в [8]. |
| `comboMod` | `MidiDemo::comboBoxChanged` | `… 0x0f 00 00 0x2c 00` + тип в [8]. |
| `btBulk` | `MidiDemo::buttonClicked` | `… 0x0f 00 00 0x34 00` + флаг в [8]. |
| `MidiKeyboardComponent` | `handleNoteOn` / `handleNoteOff` | Обычные note MIDI, не SysEx. |
| `comboMode` (Voice) | `VoicePage::comboBoxChanged` + OSC | SysEx шаблон в `MidiSysex.h` для `adresseOpMode`: `02 00 00 00` + режим в [8]. |
| Таблица волн `TableTutorialComponent` | `selectedRowsChanged` | База `07 00 00 01`; смещение оператора в [4] (`00/20/40/60`); индекс волны в [8]. |

### Слайдеры/кнопки с `setMidiSysex` (по модулям)

Формат: **`компонент → метод отправки → типичный адрес ([3][4][5][6])`**. Полный перечень полей см. в конструкторах соответствующих `.h` (там после `applyElem`/`sysexdata[6]=…` задаются байты).

| Модуль | Элементы | Функция отправки | Примеры адресов (байты 3–6) |
|--------|----------|------------------|-----------------------------|
| `Voice.h` | `sliderMaster` | `MidiSlider::sliderValueChanged` | `02 00 00 3f` (общая громкость). |
| `Voice.h` | `op1` (`MidiButton`) | `MidiButton::buttonClicked` | `0d 00 00 32` и др. по инициализации. |
| `Oscillator.h` | `sliderOsc*`, `sliderFine*`, `sliderDetune*`, `sliderPhase*`, `btFix*`, `btPhase*` | MidiSlider/MidiButton | Множество адресов для элементов 1–6 (`06`/`07` и смещения в [4]). |
| `Operator.h` | `sliderAlgo` | MidiSlider | Задаётся в конструкторе `sysexdata`. |
| `Pitch.h` | `sliderFine`, `sliderPitch` | MidiSlider | См. массивы в `Pitch.h`. |
| `Volume.h` | `sliderR*`, `sliderL*`, `btHold` | MidiSlider/MidiButton | Один шаблон с разными `sysexdata[6]`. |
| `Pan.h` | `sliderHT`, `sliderSLP`, пары L/R сегментов | MidiSlider | Часто `0x0A` в [3], индексы сегментов в [6] (`02`…`0F` и уровни `09`…`0E`). |
| `WaveEg.h`, `PitchEG.h` | Слайдеры EG + `sliderSlope` | MidiSlider + `Segment::sendOsc` для хуков | Элемент-зависимые `elemOffset` в [4]. |
| `Filter1.h`, `Filter2.h` | Слайдеры EG, `sliderSlope` | MidiSlider / Segment | `sysexdata`/`sysexdata2` с разными [6]. |
| `CommonFilter.h` | `sliderFq1`, `sliderFq2`, `sliderResonnance`, `sliderVelocity`, `sliderLfoSens`, `btFilter2` | MidiSlider/MidiButton | См. конструктор `CommonFilter`. |
| `AWMVue.h` | `sliderWaveForm`, `sliderFine`, `btFixed` | MidiSlider/MidiButton | `07` + смещение элемента в [4]; подпараметры в [6] (`01`,`02`,`04` и т.д.). |
| `FilterVue copie.h` | Слайдеры + `MidiCombo` | MidiSlider/MidiCombo | Расширенный черновик фильтра. |
| `Hook.h` | Перетаскивание `Hook` внутри `Segment` | `Segment::sendOsc` | Два SysEx подряд для текущих `sysexRate`/`sysexLevel`. |

Отдельные **поворотные** слайдеры (`setSliderStyle(Rotary)`) — это всё те же `MidiSlider` после `setRangeAndRound` с отрицательным минимумом (pan и т.п.).

---

## Неизвестные / stub / места под доработку

| Что | Где | Комментарий |
|-----|-----|-------------|
| Отсутствуют `TODO`/`FIXME` в `Source/*.h`/`Main.cpp` | Поиск по проекту | Явных маркеров нет; ниже — логические заглушки и шаблонный код JUCE. |
| Шаблонный `paint()` «placeholder text» | `MidiObjects.h`, `FilterVue.h`, `WaveVue.h`, `AfmVue.h`, `VoicesTableModel.h`, `Oscillator.h`, `Operator.h`, `Volume.h`, `Pitch.h`, `Element.h`, `AWMVue.h` (частично), др. | Текст заготовки JUCE Component; фон без финальной графики. |
| `comboPreset` «Inactif pour le moment» | `FilterVue.h`, `AfmVue.h` | Пресеты фильтра/AFM не подключены. |
| `MidiSysex.h`: ветка `oscSendMidiMessage` | `MidiSysex.h` ~181–187 | Цикл `for (...; 0 == message[0].getInt32(); i++)` выглядит как ошибка/заглушка (условие всегда ложно при ненулевом аргументе). |
| `VoicePage::textEditorReturnKeyPressed` | `Voice.h` ~336–347 | Заполнение «сообщений для имени» и OSC `oscSendMidiMessage`; глобальный `oscMidiMessage` объявлен как `const` в `MidiDemo.h` — связка потенциально нерабочая без правки. |
| `VoicePage::sliderValueChanged` | `Voice.h` ~357 | Пустое тело — заглушка. |
| `ElementComponent::setElementNumber` | `MidiObjects.h` ~29 | Только лог; без логики привязки. |
| `ControllerPage` закомментирована во вкладках | `MidiDemo.h` (`DemoTabbedComponent`) | Отдельная MIDI-страница отключена; вместо неё комбобоксы на главном экране MIDI. |
| `LibrairiePage::saveDump` | `Librairie.h` | Комментарий «Make a better code later!» через флаг `loadBankRequest`; сохранение без валидации структуры дампа. |
| `CommonFilter.h` | ~208 | Комментарий о placeholder для АЧХ. |
| Документ с наблюдениями по карте MIDI | `Sysex77-master/Source/MIDI_MAP_OBSERVATIONS.md` | Markdown не входил в запрошенные расширения кода, но содержит внешние заметки по адресации (например Pan/SY99). |

---

*Конец карты.*
