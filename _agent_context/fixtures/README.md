# Эталонные LIVEREAD / 07:1 Voice fixtures

Бинарные снимки **одного** internal voice с SY99 (режим bulk **07: 1 Voice**). Формат совпадает с файлами `LIVEREAD-*.syx`, которые пишет Sysex77 при **Stop read**.

## Файлы

| Файл | Сообщений | Размер | Имя @ offset 33 |
|------|-----------|--------|-----------------|
| `01_init_anonim_07x1_voice.syx` | 2 | 701 B | `ANONIM` |
| `02_ap_crsrock_07x1_voice.syx` | 2 | 700 B | `AP\|CrsRock` |
| `03_*_07x1_voice.syx` | 2 | ~700 B | **добавить вы** (голос с известным ELMODE на LCD) |

Таблица значений с дисплея SY99: [`lcd_reference.csv`](lcd_reference.csv) или, если CSV в Cursor не открывается, [`lcd_reference.md`](lcd_reference.md).

**Cursor:** открывайте **файл** (`lcd_reference.csv`), не папку `fixtures` (иначе «Unable to open fixtures»). Альтернатива: Блокнот → `c:\projects\sy_99\_agent_context\fixtures\lcd_reference.csv`.

## Структура одного `.syx`

```
[кадр 1: F0 … LM … 8101VC … ~585 B … F7][кадр 2: F0 … LM … 0040VC … ~113 B … F7]
```

Разбор заголовка и карта полей: [`../lm_8101_offsets.md`](../lm_8101_offsets.md).

### Быстрая проверка (Python)

```bash
python -c "
from pathlib import Path
b = Path('01_init_anonim_07x1_voice.syx').read_bytes()
f0 = b.index(0xF0)
f7 = b.index(0xF7)
frame = b[f0:f7+1]
assert frame[6:16] == b'LM  8101VC'
print('name:', frame[33:43])
print('len:', len(frame))
"
```

## Как добавить третий эталон (этап 0 — ваша часть)

1. На SY99 выставить голос с **известным** Element Mode на LCD (например «2 AFM POLY»).
2. Записать в `lcd_reference.csv` строку `03_…`.
3. Sysex77: **Read** → **07: 1 Voice** → **Stop**.
4. Скопировать `LIVEREAD-*.syx` из  
   `%AppData%\Application Support\Sysex77\`  
   сюда как `03_elmode_known_07x1_voice.syx` (или осмысленное имя).
5. Убедиться: ровно **2** кадра, теги `8101VC` и `0040VC`.

## Происхождение fixtures 01–02

Собраны из полного hex monitor (апрель–май 2026, hardware capture). Пересборка:

```bash
python _build_captured_fixtures.py
```

Скрипт `_build_captured_fixtures.py` — только для регенерации; в git можно не коммитить после проверки.

## Не путать с

| Источник | Кадров | Использование |
|----------|--------|----------------|
| **07: 1 Voice** | 2 | Парсер LM, live read, эти fixtures |
| **05: 64 Voice** | ~128 | Весь банк; **не** использовать для offset-карты одного голоса |
