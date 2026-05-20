# LCD-эталон (этап 0)

Заполнение из чата / с SY99. CSV: `lcd_reference.csv`.

---

## 01 — ANONIM (`01_init_anonim_07x1_voice.syx`) — **заполнено**

| Экран SY99 | Параметр | Значение |
|------------|----------|----------|
| #200 | Element Mode | **1 AFM and 1 AWM** |
| #212 | Total voice volume (WOL) | **127** |
| — | Element 1 Level | **110** |
| — | Element 2 Level | **127** |
| #203 | Element detune El.1 / El.2 | **0** / **0** |
| #204 | Note shift El.1 / El.2 | **0** / **0** |
| #205 | Note limit El.1 / El.2 (default) | **C2–G8** / **C2–G8** |
| #206 | Velocity limit low / high | **1** / **127** |
| #208 | Output Group El.1 / El.2 | **both** / **both** |

---

## 02 — AP|CrsRock (`02_ap_crsrock_07x1_voice.syx`) — **заполнено**

| Экран SY99 | Параметр | Значение |
|------------|----------|----------|
| #200 | Element Mode | **1 AFM 1 AWM** |
| #212 | Total voice volume (WOL) | **107** |
| — | Element 1 / 2 Level | **127** / **127** |
| #203 | Element detune El.1 / El.2 | **−1** / **+2** |
| #208 | Output Group El.1 / El.2 | **both** / **both** |

---

## 03 — EP:Classic (`03_ep_classic_07x1_voice.syx`) — **заполнено**

| Экран SY99 | Параметр | Значение |
|------------|----------|----------|
| #200 | Element Mode | **2 AFM POLY** |
| #212 | Total voice volume (WOL) | **110** |
| — | Element 1 / 2 Level | **127** / **127** |
| #203 | Element detune El.1 / El.2 | **+2** / **−2** |
| #208 | Output Group El.1 / El.2 | **both** / **both** |

**Примечание:** кадр 8101VC укорочен (475 B); ELMODE в bulk @+73 не совпал с LCD — нужен diff «только ELMODE» (этап 1).
