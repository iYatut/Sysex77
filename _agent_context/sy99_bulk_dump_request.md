# SY99 — Bulk Dump Request / Response (Deep Research, 2026-05-23)

> Источники: SY99 Service Manual / MIDI Data Format, SY77 SysEx articles (Sound On Sound),
> экстраполяция tail-байтов request из SY77 (не буквальная цитата SY99 PDF).
> Статус: **заголовки verified**, **tail request (mt/mm/checksum) — needs hardware log**.

---

## 1. Базовая схема байтов

| Байт | Значение |
|------|----------|
| `F0` | SysEx start |
| `43` | Yamaha |
| `1n` | Parameter change (уже используется в Sysex77) |
| `2n` | **Bulk Dump Request** |
| `0n` | **Bulk Dump Data** (ответ) |
| `n` | SysEx Device Number 0…F (Utility → MIDI на SY99) |

**Не путать:** parameter change = `F0 43 1n **34** … F7`  
Bulk = `F0 43 2n/**0n** **7A** … F7`

> **Исправление к старому CHANGELOG:** bulk request/response использует байт **`0x7A`**, не `0x34`.
> `0x34` — только parameter change.

---

## 2. LM-типы и заголовки (verified — из мануала)

### Voice — LM 0040VC

**Request:**

```text
F0 43 2n 7A 4C 4D 20 20 30 30 34 30 56 43 00 00 F7
```

ASCII: `"LM 0040VC"` + два нулевых байта (служебные, без явной расшифровки в request).

**Response:**

```text
F0 43 0n 7A 4C 4D 20 20 30 30 34 30 56 43 00 00 ... F7
```

### Multi — LM 0040MU

**Request:** `F0 43 2n 7A 4C 4D 20 20 30 30 34 30 4D 55 00 00 F7`  
**Response:** `F0 43 0n 7A … 4D 55 00 00 … F7`

### System Setup — LM 0040SY

**Request:** `F0 43 2n 7A 4C 4D 20 20 30 30 34 30 53 59 00 00 F7`  
**Response:** `F0 43 0n 7A … 53 59 00 00 … F7`

### Effect — LM 0040MS

**Request:** `F0 43 2n 7A 4C 4D 20 20 30 30 34 30 4D 53 00 00 F7`  
**Response:** `F0 43 0n 7A … 4D 53 00 00 … F7`

---

## 3. Memory_type / Memory# (voice & multi)

### Memory_type

| Value | Meaning |
|-------|---------|
| `00` | Internal (banks A–D) |
| `02` | Preset 1 |
| `03` | Preset 2 |
| `7F` | **Edit buffer** (текущий редактируемый voice/multi) |

### Memory# (voice, Internal mt=00)

| Range | Bank / Program |
|-------|----------------|
| `00–0F` | Bank A, programs 1–16 |
| `10–1F` | Bank B, programs 1–16 |
| `20–2F` | Bank C, programs 1–16 |
| `30–3F` | Bank D, programs 1–16 |

**Формула для Sysex77 LIBSYNC globalIdx 0…63:**

```text
memoryNumber = globalIdx   // 0..63 напрямую
bank         = globalIdx / 16   // 0=A, 1=B, 2=C, 3=D
program      = globalIdx % 16   // 0..15
```

### Multi Memory# (mt=00)

`00–0F` — 16 internal multi programs.

---

## 4. Нет «64 voice одним request»

Официально документируется **single voice bulk** (LM 0040VC + Memory_type + Memory#).

**Auto-dump 64 voices** = цикл 64 request'ов:

```text
for mm in 0x00..0x3F:
    requestSingleVoice(deviceId, memoryType=0x00, memoryNumber=mm)
```

По аналогии с SY77 (Sound On Sound), tail request может включать:

```text
… mt mm [length/checksum bytes] … F7
```

**⚠️ Tail не подтверждён в доступном SY99 PDF — verify на железе.**

---

## 5. Edit buffer vs single slot

| Функция | memoryType | memoryNumber | Confidence |
|---------|------------|--------------|------------|
| `requestCurrentEditVoice()` | `0x7F` | `0x00` | High (SY77-compatible extrapolation) |
| `requestSingleVoice(bank, slot)` | `0x00` | см. таблицу выше | High for mt/mm semantics |
| Tail bytes (length, checksum) | — | — | **Needs hardware log** |

---

## 6. Bulk Protect по SysEx — NOT VERIFIED

Кандидат в Sysex77 (`btBulk`):

```text
F0 43 1n 34 0F 00 00 34 00 VV F7   (VV=0 → off?)
```

**Не найдено в открытых SY99 MIDI Data List таблицах.**  
До подтверждения: снимать Bulk Protect **вручную** на SY99 Utility.

---

## 7. Universal Device Inquiry

**Request:** `F0 7E 7F 06 01 F7`  
**Expected reply:** `F0 7E <dev> 06 02 43 <family> <model> … F7`

SY99 family/model bytes — **capture locally**, не задокументированы в этом файле.

---

## 8. «ALL DATA» одним bulk — нет

Последовательный опрос: VC (×64) → MU → SY → MS → (samples отдельно).

---

## 9. Связь с парсером Sysex77

- Request header использует **`0040VC`** string.
- Ответ может содержать **`LM 8101VC`** + **`LM 0040VC`** блоки — см. `YamahaLmVoiceDump.h`, fixtures `_agent_context/fixtures/01..08_*.syx`.
- Ground truth для payload layout: **fixtures + hardware captures**, не MIDI Data List.

---

## 10. Implementation checklist (Sysex77)

- [ ] `Sy99DumpRequest.h` — builder с `makeDeviceByteRequest(0x20 | id)`
- [ ] `requestSingleVoice(id, mt, mm)` — header + tail (tail verify)
- [ ] `requestAllInternalVoices()` — loop mm=0..63, пауза между request
- [ ] Wire в `sendCurrentVoiceDumpRequestPendingVerification()` → mt=7F, mm=00
- [ ] Wire в `LibrairiePage::beginBulkCapture()` — auto-send перед `requestSysex=true`
- [ ] Hardware test: compare response vs manual DUMP OUT `.syx`
- [ ] Log SY Manager startup for tail-byte confirmation

---

## 11. Confidence matrix

| Item | Level | Source |
|------|-------|--------|
| `F0 43 2n 7A` bulk framing | ✅ Official | SY99 MIDI docs |
| LM type strings VC/MU/SY/MS | ✅ Official | SY99 MIDI docs |
| Memory_type / Memory# | ✅ Official | SY99 bulk data structure |
| mt/mm in **request** tail | ⚠️ SY77 extrapolation | SOS SY77 article |
| Request checksum/length | ⚠️ Unverified | SY77 only |
| Bulk Protect SysEx | ❌ Unverified | Sysex77 guess |
| 64-voice = 64 requests | ✅ Logical | No bank-64 type in docs |

---

## 12. C++ sketch (header verified; tail needs log)

```cpp
// deviceId = SysEx Device Number 0..0x0F from SY99 Utility
inline uint8 makeDeviceByteForRequest (uint8 deviceId) noexcept
{
    return uint8 (0x20 | (deviceId & 0x0F)); // 2n
}

inline uint8 makeDeviceByteForBulkData (uint8 deviceId) noexcept
{
    return uint8 (0x00 | (deviceId & 0x0F)); // 0n
}

// LM 0040VC voice bulk request header (official; tail mt/mm/cs — verify)
static constexpr uint8 kSy99Lm0040VcTypeString[] = {
    'L','M',' ',' ','0','0','4','0','V','C'
};
```

---

## 13. Hardware verify plan (minimal)

Отправить 3 request'а и сравнить ответы с ручным DUMP OUT:

1. `mt=0x7F, mm=0x00` — edit buffer (Sync from SY99)
2. `mt=0x00, mm=0x00` — A1
3. `mt=0x00, mm=0x10` — B1

Условия: Bulk Protect OFF на SY99, SysEx Device ID совпадает с `sysexEngine`.

---

## 14. Related project files

| File | Role |
|------|------|
| `Sysex77-master/Source/LiveSynthState.h` | `sendCurrentVoiceDumpRequestPendingVerification()` |
| `Sysex77-master/Source/Sy99BulkLibraryCapture.h` | Receive pipeline |
| `Sysex77-master/Source/YamahaLmVoiceDump.h` | Response parser |
| `Sysex77-master/Source/Librairie.h` | `beginBulkCapture()` |
| `_agent_context/sy99_sysex_complete.md` | Parameter change only |
| `_agent_context/lm_8101_offsets.md` | Payload offsets |
| `_agent_context/02_sysex_format.md` | Parameter change byte layout |
