# Library review reports (agent handoff)

Sysex77 writes review JSON to `%AppData%/Sysex77/reviews/{uuid}.json` and optionally mirrors `{uuid}.md` here.

## One-time mirror setup

Create `%AppData%/Sysex77/agent_reviews_mirror.txt` with a single line:

```
c:\projects\sy_99\_agent_context\library_reviews
```

After submitting a review from the Library UI, open:

- React: `http://localhost:5173/library/sy99/{page}/{mm}?review={uuid}`
- API: `GET http://127.0.0.1:8765/api/library/reviews/{uuid}`

## Agent workflow

1. User marks LCD mismatches in Library voice table and clicks **Отправить отчёт**.
2. Read `{uuid}.md` or fetch JSON from API.
3. Fix decode/offsets in `Sy99ParamRegistry.cpp` / `YamahaLmVoiceDump.h`.
4. Run regression: `python _agent_context/fixtures/_validate_bulk_parse.py`
5. User reloads page with `?review=` — resolved items show ✓.
6. `DELETE http://127.0.0.1:8765/api/library/reviews/{uuid}` when done.

## Report priority

1. **Manual** — user-confirmed LCD values (source of truth)
2. **Auto** — decode/source/fixture suspects not confirmed manually
