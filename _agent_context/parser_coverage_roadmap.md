# Parser Coverage Roadmap — статус

> **Обновлено:** 2026-05-23 — exit gate Phase 1 **HW PASS** (`TEST_STATUS.md`). Следующий фокус: **Фаза 4–5 AFM OP** (экран Voice → AFM OP).

## Завершено (фаза 1 — mixer tail)

| Param | Bulk offset E1 | Bulk offset E2+ | bindStatus |
|-------|----------------|-----------------|------------|
| ENLL | outsel+2 | anchor+8 | `confirmed_bulk8101` |
| ENLH | outsel+3 | anchor+9 | `confirmed_bulk8101` |
| EVLH | outsel+**5** (was +3) | anchor+**11** | `confirmed_bulk8101` |
| EVLL | outsel+4 | anchor+10 | `confirmed_bulk8101` |

## Завершено (фаза 2 — EFSD + 0040 tail)

| Param | Bulk source | Offset | bindStatus | Evidence |
|-------|-------------|--------|------------|----------|
| EFSDLV E1 | 8101VC efln+12 | `confirmed_bulk8101` | **elmode 4 only** (EP:Classic fixture 03) |
| EFSDLV E1/E2 elmode 8 | **0040VC @100 / @104** | `confirmed_bulk0040` | hardware diff EP\|GrnDual step A/B′ |
| ~~AFTMD~~ | ~~0040 @100~~ | — | **отвергнуто** — @100 = EFSDLV E1 |
| EFSDVSNS / EFSDSCL | — | not in 8101 | `manual_only` | live param9 only; efln+2/+3 falsified on fixture 02 |
| ATPBR, PMRNG, PMASN, mod ranges… | 0040VC | @36–56, @90 tail | `confirmed_bulk0040` | `_validate_0040_bulk.py` fixtures 01–03 |
| AFTMD | live `02 00 00 42` only | `manual_only` | bulk @100 falsified by EFSDLV diff |

Regression:

- [`fixtures/_validate_bulk_parse.py`](fixtures/_validate_bulk_parse.py) fixtures 01–03 PASS (+ EFSDLV/EFLN on 03)
- [`fixtures/_validate_0040_bulk.py`](fixtures/_validate_0040_bulk.py) fixtures 01–03 + EFMODE 06–08 PASS
- [`fixtures/_validate_efmode_bulk.py`](fixtures/_validate_efmode_bulk.py) PASS

## Следующие фазы

| Фаза | Цель | Статус |
|------|------|--------|
| 0 | `catalog-coverage-report.py` + CSV coverage table | pending |
| 3 | Generic binding reader для Library | **done** |
| 4 | AFM Element Common group 05 | pending |
| 5 | AFM OP groups 46–56 | pending |
| 6–7 | AWM 07, Filter/Pan/Effect | pending |
| 8 | `confirmed_live` promotion | pending |

См. также [`lm_8101_offsets.md`](lm_8101_offsets.md), [`sy99_param_bindings.json`](fixtures/sy99_param_bindings.json).
