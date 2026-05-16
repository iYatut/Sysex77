# Формат Parameter Change SysEx (Yamaha SY99) — по материалам репозитория Sysex77

Официальный документ Yamaha **MIDI Data Format**:  
https://data.yamaha.com/files/download/other_assets/2/317162/SY99E2.PDF  

Локальная копия (если скачана): `_agent_context/SY99E2.pdf`. Подробности извлечения текста см. `_agent_context/SY99E2_reference.txt`.

Ниже по-прежнему — разбор **реализации Sysex77** и журнала `Source/MIDI_MAP_OBSERVATIONS.md`; таблицы адресов из PDF сюда не переписывались (у дистрибутива PDF без текстового слоя нужен OCR или ручной перенос).

---

## 1. Каноническая структура (как задаёт пользователь и как это ложится на код)

### 1.1 Полное сообщение по шине MIDI

Пользовательская форма:

```text
F0 43 1n 34 gg hh ll pp 0V vv F7
```

Смысл полей (как в запросе):

| Поле | Описание |
|------|----------|
| `F0` … `F7` | Обычное обрамление SysEx. |
| `43` | Идентификатор Yamaha. |
| `1n` | `n = MIDI channel − 1`; в проекте фактически кладут **`device ID`** в байт с индексом **1** массива данных SysEx (часто **`0x10`** для первого устройства/канала — см. `DeviceModel::getSysExDeviceID()`, `sysexEngine`). |
| `34` | Подтип «голосовых» параметров SY77/SY99 в этом приложении (везде **`0x34`**). |
| `gg` | MSB группы / старшая часть адреса. |
| `hh` | LSB группы. |
| `ll` | MSB смещения параметра. |
| `pp` | LSB смещения параметра. |
| `0V` | MSB значения; **младшие 4 бита** (`V`). |
| `vv` | LSB значения, **7 бит**. |

### 1.2 Как JUCE хранит payload (без `F0`/`F7`)

В коде используется **`MidiMessage::createSysExMessage(buf, 9)`**: в `buf` передаются **ровно 9 байт** между `F0` и `F7`:

```text
Индекс:  0    1    2    3    4    5    6    7    8
Байты:  43   1n   34   ?    ?    ?    ?   MSB  LSB
```

Сопоставление с буквами из запроса (естественное слияние 16-битных полей в 4 байта адреса):

| Индекс в коде | Поле пользователя | Замечание |
|---------------|-------------------|-----------|
| `[0]` | `43` | Константа Yamaha. |
| `[1]` | `1n` | Device/channel byte (`sysexEngine`, `getSysExDeviceID()`). |
| `[2]` | `34` | Константа модели сообщения в этом проекте. |
| `[3]` | часть адреса (**часто трактуется как «block»**) | В `MIDI_MAP_OBSERVATIONS.md` — **`byte[3]`**. |
| `[4]` | часть адреса (**часто «element offset»**: `00/20/40/60`) | **`byte[4]`**. |
| `[5]` | часть адреса (**часто `0x00`**) | **`byte[5]`**, «sub». |
| `[6]` | часть адреса (**параметр внутри блока**) | **`byte[6]`**. |
| `[7]` | `V` (MSB значения; в логах часто **`0x00`**) | Использование старшего байта **не выведено в отдельную функцию**; вручную меняют в `Oscillator.h` (`sysexdata[7]`). |
| `[8]` | `vv` / младшее значение | Основное значение в большинстве слайдеров. |

**Важно:** явного разбиения «`gg hh ll pp` как два 16-битных слова» в C++ **нет** — задают четыре последовательных байта **`[3]…[6]`**. Пользовательское `(gg, hh, ll, pp)` удобно считать **то же самое**, что **`sysexdata[3] sysexdata[4] sysexdata[5] sysexdata[6]`**.

### 1.3 Значение параметра: 11 бит vs практика проекта

Формально пара **`0V` + `vv`** даёт до **11 значимых бит** (4 + 7). В коде и в журнале наблюдений доминирует упрощение:

- **`[7] = 0x00`**, значение только в **`[8]`** (7-бит или «сырой» байт смысла параметра).
- Исключение: **`Oscillator.h`** выставляет **`sysexdata[7]`** для отдельных параметров (phase и др.) — см. исходник.

Подробные правила шкал (rate/level, bias −0x20 для level и т.д.) задокументированы в **`Source/MIDI_MAP_OBSERVATIONS.md`**, а не в C++.

---

## 2. Адреса `(gg hh ll pp)` = `(byte[3] byte[4] byte[5] byte[6])` из проекта

В вашем сообщении список адресов из логов **не был приложён**; ниже используются адреса из **`Source/MIDI_MAP_OBSERVATIONS.md`** (постфикс монитора Sysex77) и из **констант в `.h`**.

Ниже — **объединение**: типовые префиксы из **`MidiSysex.h`**, **`MidiDemo.h`** и фрагментов UI + расширенные примеры из **`MIDI_MAP_OBSERVATIONS.md`**.

### 2.1 Таблица префиксов из кода (OSC → SysEx и прямые вызовы)

| `gg hh ll pp` (hex) | Источник | Назначение (по комментариям/контексту в коде) |
|---------------------|----------|-----------------------------------------------|
| `0f 00 00 2d` | `MidiSysex.h`, `MidiDemo::comboBoxChanged` | Foot controller assignment. |
| `0f 00 00 2c` | то же | Mod wheel assignment. |
| `0f 00 00 34` | `MidiDemo::buttonClicked` (Bulk) | Bulk protect / связанный системный параметр. |
| `02 00 00 3f` | `MidiSysex.h` `oscTotalVoiceVolume`, `Voice.h` master | Общая громкость голоса. |
| `02 00 00 00` | `MidiSysex.h` `adresseOpMode` | Режим операторов / голоса (значение в `[8]`). |
| `02 00 00 01` | `Voice.h` (имя по символам) | Шаблон для посимвольной отправки имени (экспериментально). |
| `03 00/20/40/60 00 08` | `MidiSysex.h` | Voice group по элементам. |
| `03 .. 00 02` | `MidiSysex.h` | Pitch coarse по элементам. |
| `03 .. 00 01` | `MidiSysex.h` | Fine по элементам. |
| `07 00/20/40/60 00 02` | `MidiSysex.h` | «Fixed»/AWM-связанный параметр по элементам. |
| `07 .. 00 …` | `AWMVue.h`, `Volume.h` | AWM EG и др. (`byte[6]` варьируется: `01`, `04`, `50`–`56`, `4f` и т.д.). |
| `09 .. 00 …` | `CommonFilter.h`, `FilterVue copie.h` | Общий блок фильтра. |
| `05 00 00 00` | `Operator.h` | Алгоритм AFM + смещения элементов в `[4]`. |
| `46`–`56` / шаг `0x10` в `[3]` | `Oscillator.h` | Нумерация операторов через смену «старого» байта блока. |
| `0d 00 00 32` | `Voice.h` (`op1`) | Переключатель/параметр оператора 1. |

Это **не исчерпывающая карта SY99** — только то, что явно прошито в репозитории.

### 2.2 Дополнительные адреса из журнала логов (`MIDI_MAP_OBSERVATIONS.md`)

В том файле зафиксированы, в частности:

- **Pan EG (канон вкладки Rate/Level):** `0A [elem] 00 [param]` с **`byte[6]`** от `02` (HT) до `10` (SLP) и уровни `09`–`0F` для L-сегментов — см. таблицы в исходном markdown.
- **AFM Operator EG:** блок **`56`** — например HT `… 56 … 0D 00`, R1 `… 00 00`, R2 `… 01 00`.
- **AWM EG R2:** `07 40 00 51` (E3), `07 00 00 50` (E1) и др.
- **Legacy-пути** с другими `byte[3]` для «похожих» имён параметров (описаны как предупреждения при смешении дампов).

Для полного текста наблюдений открывайте **`Sysex77-master/Source/MIDI_MAP_OBSERVATIONS.md`** (в этот файл дублировать все 400 строк не требуется).

---

## 3. Парсер входящего Parameter Change SysEx

### 3.1 Единственное место осмысленного разбора 9-байтового `43 … 34`

Реализовано в **`MidiDemo::handleAsyncUpdate`** (`MidiDemo.h`): копирование сырых байтов и фильтр по префиксу, затем публикация в `valueSysexIn` для UI.

**Полный фрагмент (как в исходнике):**

```cpp
        for (auto& message : batch)
        {
            // SysEx parameter extraction — always, independent of display filters
            if (message.getSysExDataSize() == 9)
            {
                memcpy(&data, message.getSysExData(), message.getSysExDataSize());
                if (data[0] == 0x43 && data[2] == 0x34)
                    valueSysexIn = make_var_array(data[3], data[4], data[5],
                                                  data[6], data[7], data[8]);
            }

            // "SysEx only" display filter
            if (showSysExOnly && !message.isSysEx())
                continue;

            // Format: SysEx gets a sequential index and full hex bytes;
            // other messages use JUCE's compact getDescription()
            if (message.isSysEx())
            {
                const uint8* d   = message.getSysExData();
                const int    len = message.getSysExDataSize();
                String hexStr = "F0";
                for (int i = 0; i < len; ++i)
                    hexStr << " " << String::toHexString(d[i]).toUpperCase();
                hexStr << " F7";
                messageText << "[#" << String (++sysExMsgCounter) << "] SysEx: "
                            << hexStr << "\n";
            }
            else
            {
                messageText << message.getDescription() << "\n";
            }
        }
```

Отдельной функции вида `parseYamahaParameterChange()` **нет**. Декодирование **`gg hh ll pp` → имя параметра** и сборка **11-битного значения из `0V` и `vv`** в коде **NOT IMPLEMENTED**.

### 3.2 Приём дампов библиотеки (не разбор полей)

- **`MidiDemo::handleIncomingMidiMessage`**: при `requestSysex` складывает **любые** входящие SysEx в `arraySysex`.
- **`LibrairiePage::saveDump`** (`Librairie.h`): записывает **сырой поток** `getRawData()` подряд в файл `.syx` — без разбора формата Parameter Change.

### 3.3 Разбор имён голосов из `.syx` в `BankTableModel.h`

В **`BankTableModel.h`** при загрузке файла банка ищется **`0xF0`**, затем эвристика **`i += 33`** и побайтовое чтение **10 символов имени** — это **не** парсер Parameter Change и **не** общий SysEx-декодер.

---

## 4. Сборка исходящего SysEx (builders)

### 4.1 Общие функции в `MidiSysex.h` (включены в класс `MidiDemo`)

Ниже — **полный файл** `MidiSysex.h` как единственное централизованное место OSC→SysEx для многих адресов:

```cpp
/*
 ==============================================================================
 
 MidiSysex.h
 Created: 13 Nov 2018 8:45:55pm
 Author:  Sébastien Portrait
 
 ==============================================================================
 */

#pragma once
void addOscListener ()
{
    
    addListener (this, adresseOscFoot); // [4]
    addListener (this, adresseOscMod);
    addListener(this, adresseOpMode);
    addListener(this, adresseOscSendBank);
    addListener(this,adresseOscRepaint);
    addListener(this, oscTotalVoiceVolume);

    

    addListener(this, oscVoicePan1);
    addListener(this, oscVoicePan2);
        addListener(this, oscVoicePan3);
        addListener(this, oscVoicePan4);
    addListener(this,oscVoiceGrp1);
     addListener(this,oscVoiceGrp2);
        addListener(this,oscVoiceGrp3);
        addListener(this,oscVoiceGrp4);
    addListener(this, oscVoicePitch1);
        addListener(this, oscVoicePitch2);
        addListener(this, oscVoicePitch3);
        addListener(this, oscVoicePitch4);
    addListener(this, oscVoiceFine1);
        addListener(this, oscVoiceFine2);
        addListener(this, oscVoiceFine3);
        addListener(this, oscVoiceFine4);
    addListener(this, oscVoiceFixe1);
    addListener(this, oscVoiceFixe2);
        addListener(this, oscVoiceFixe3);
        addListener(this, oscVoiceFixe4);
    addListener(this, oscSendMidiMessage);

    

    addListener(this, "/SYSEX");
}





void oscMessageReceived (const OSCMessage& message) override
{
    auto address = message.getAddressPattern();

    if (address.matches("/SYSEX"))
    {
        if (message.size() > 0 )
        {
            uint8 sysexdata[9];
            //     Logger::writeToLog("Foot " + String( message[0].getInt32()));
            for(int i =0; i < message.size() ;i++)
                sysexdata[i] = message[i].getInt32();
            
            MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
      //      m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs (m);
        }
    }
    if (address.matches(adresseOscRepaint))
        repaint();
    
    if (address.matches(adresseOscFoot))
    {
        uint8 sysexdata[9] = { 0x43, sysexEngine, 0x34, 0x0f, 0x00, 0x00, 0x2d, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    
    if (address.matches(adresseOscMod))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x0f, 0x00, 0x00, 0x2c, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscTotalVoiceVolume))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x02, 0x00, 0x00, 0x3f, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    
   
     if (address.matches(oscVoiceGrp1))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x00, 0x00, 0x08, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceGrp2))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x20, 0x00, 0x08, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceGrp3))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x40, 0x00, 0x08, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceGrp4))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x60, 0x00, 0x08, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoicePitch1))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x00, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoicePitch2))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x20, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoicePitch3))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x40, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoicePitch4))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x60, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFine1))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x00, 0x00, 0x01, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFine2))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x20, 0x00, 0x01, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFine3))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x40, 0x00, 0x01, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFine4))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x03, 0x60, 0x00, 0x01, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFixe1))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x07, 0x00, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFixe2))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x07, 0x20, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFixe3))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x07, 0x40, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(oscVoiceFixe4))
    {
        uint8 sysexdata[9] = { 0x43, 0x10, 0x34, 0x07, 0x60, 0x00, 0x02, 0x00, 0x00 };
        sendSysex(message, sysexdata);
    }
    if (address.matches(adresseOpMode))
    {
        Logger::writeToLog("Operateur mode envoi");
        uint8 sysexdata[9] = { 0x43, 0X10, 0x34, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 };
        sendSysex(message, sysexdata);
        
    }
    if (address.matches ( oscSendMidiMessage))
    {
        if (message.size() == 1 && message[0].isInt32())
        {
            for(auto i = 0 ; 0 == message[0].getInt32(); i++)
                 sendToOutputs (oscMidiMessage[i]);
        }
    }
    if (address.matches(adresseOscSendBank))
    {
        File file {(appDirPath.getFullPathName() + "/" + message[0].getString())};
        MemoryBlock mb;
        if(file.loadFileAsData(mb))
        {
            const uint8* const data = (const uint8*) mb.getData();
            sendRaw(data, mb.getSize());
        }
        
    }
    //        rotaryKnob.setValue (jlimit (0.0f, 10.0f, message[0].getFloat32())); // [6]
}
void sendSysex(const OSCMessage& message, uint8 sysexdata[0])
{
    if (message.size() == 1 && message[0].isInt32())
    {
        //     Logger::writeToLog("Foot " + String( message[0].getInt32()));
        
        sysexdata[8] = message[0].getInt32();
        MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }
    
}
void sendRaw(const void* sysexData, const long dataSize)
{
    if (dataSize >1)
    {
        Logger::writeToLog(String(dataSize));
        //    Logger::writeToLog(String(dataSize));
        //Envoi midi brut depuis la mémoire
        MidiMessage m = MidiMessage (sysexData, dataSize);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }
    else
    {
        Logger::writeToLog("Error Send RAW");
    }
}
```

**Замечание:** `sendSysex` записывает OSC-аргумент только в **`sysexdata[8]`** — т.е. строитель **не использует полный 11-битный (`0V`,`vv`) закодированный алгоритм**; старший байт значения задаётся только шаблоном до вызова.

### 4.2 Прямые `createSysExMessage` в `MidiDemo`

Полный фрагмент **bulk / foot / mod** (как в исходнике):

```cpp
    void buttonClicked (Button* button) override
    {
        if(button == &btBulk)
        {
            Logger::writeToLog("Bulk Protect");
            uint8 sysexdata[9] = { 0x43, DeviceModel::getInstance().getSysExDeviceID(), 0x34, 0x0f, 0x00, 0x00, 0x34, 0x00, 0x00 };
            sysexdata[8] = btBulk.getToggleState();
            MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
            m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs (m);
        }
    }
```

```cpp
    void comboBoxChanged	(	ComboBox * 	comboBoxThatHasChanged	) override
    {
        if(comboBoxThatHasChanged == &comboFoot)
        {
        uint8 sysexdata[9] = { 0x43, sysexEngine, 0x34, 0x0f, 0x00, 0x00, 0x2d, 0x00, 0x00 };
        sysexdata[8] = comboFoot.getSelectedItemIndex()+1;
        MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
        }
        if(comboBoxThatHasChanged == &comboMod)
        {
            uint8 sysexdata[9] = { 0x43, DeviceModel::getInstance().getSysExDeviceID(), 0x34, 0x0f, 0x00, 0x00, 0x2c, 0x00, 0x00 };
            sysexdata[8] = comboMod.getSelectedItemIndex()+1;
            MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
            m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs (m);
        }
        ...
    }
```

### 4.3 Косвенный «builder» через OSC от слайдеров (`MidiSlider`)

Полный метод **`MidiSlider::sliderValueChanged`** (`MidiObjects.h`): он **не** вызывает `createSysExMessage` напрямую, а шлёт **9 целых** на `/SYSEX`, которые на стороне `MidiDemo` превращаются в SysEx (см. §4.1).

```cpp
    void sliderValueChanged (Slider* slider) override
    {
     
        int value = slider->getValue();
        Logger::writeToLog ("Slider Value changed " + String(value));
        if(value<0)
        {
            if(!boolInvert)
            value = ~value;

            value += intNegativeDelta;
        Logger::writeToLog ("Signed minus " + String(value));
        }
        
       // uint8 data = slider->getValue();
        sysexData[8] = value;
        
        sender.send(oscAddressPatern, (uint8) sysexData[0], sysexData[1], sysexData[2], sysexData[3], sysexData[4], sysexData[5], sysexData[6], sysexData[7], sysexData[8]);
        
    }
```

Аналогично **`MidiButton::buttonClicked`** и **`MidiCombo::comboBoxChanged`** формируют тот же OSC-пакет (см. тот же файл).

### 4.4 Сегменты EG (`Hook.h`)

Полный метод **`Segment::sendOsc`** (косвенный builder: считает числа для `[8]` и шлёт два OSC-сообщения `/SYSEX`):

```cpp
    void sendOsc()
    {
        int rate;
        int level;
        //20 * 100 /5 = 400
        if(intRate == 0)
        {
            rate = intRangeRate * 100;
        }
        else
        {
        rate = intRangeRate * 100 / intRate;
        }
        
        if(intLevel == 0)
        {
            level = intRangeLevel * 100;
        }
        else
        {
        level = intRangeLevel * 100 / intLevel;
        }
        sysexRate[8] = rate;
        sysexLevel[8] = level;
        sender.send(oscAddressPatern, (uint8) sysexRate[0], sysexRate[1], sysexRate[2], sysexRate[3], sysexRate[4], sysexRate[5], sysexRate[6], sysexRate[7], sysexRate[8]);
        sender.send(oscAddressPatern, (uint8) sysexLevel[0], sysexLevel[1], sysexLevel[2], sysexLevel[3], sysexLevel[4], sysexLevel[5], sysexLevel[6], sysexLevel[7], sysexLevel[8]);
        
    }
```

---

## 5. NOT IMPLEMENTED (в явном виде отсутствует в коде)

| Возможность | Статус |
|-------------|--------|
| Функция сборки сообщения из полей `(n, gg, hh, ll, pp, value11bit)` с корректным разбиением старших 4 бит и младших 7 бит по канону Yamaha | **NOT IMPLEMENTED** (значение задаётся в основном одним байтом `[8]`). |
| Функция разбора входящего SysEx в структуру «адрес + значение» независимо от UI | **NOT IMPLEMENTED** (только проброс в `valueSysexIn`). |
| Валидация длины, checksum, не-SY99 сообщений | **NOT IMPLEMENTED**. |
| Единая таблица всех параметров SY99 в коде | **NOT IMPLEMENTED** (разбросано по `.h` и markdown-журналу). |

---

## 6. Связанные файлы в репозитории

| Файл | Роль |
|------|------|
| `Source/MIDI_MAP_OBSERVATIONS.md` | Журнал реальных логов SY99: адреса, шкалы, противоречия legacy. |
| `Source/MidiDemo.h` | Парсер для UI + MIDI монитор + прямые SysEx (bulk/foot/mod). |
| `Source/MidiSysex.h` | OSC→SysEx шаблоны и `sendRaw`. |
| `Source/MidiObjects.h` | OSC-пакетирование 9 байт из виджетов. |
| `Source/DeviceModel.h` | Байт устройства для `[1]`. |
| Панели `Pan.h`, `Oscillator.h`, `Filter*.h`, … | Локальные шаблоны `sysexdata[9]`. |

---

*Файл создан для агента/разработчика; при расхождении с железом приоритет у проверенных дампов в `MIDI_MAP_OBSERVATIONS.md`.*
