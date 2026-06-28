# Phase 3 Implementation Review — Pushback & Anomalies

**Status:** ✅ **COMPLETED** — all items resolved (2026-06-28)
**Reviewed:** `tests/phase3_ranking/` test implementation + `src/core/search_service.cpp`
**Spec:** `specs/phase3_ranking_query_engine.md` (v1.0, Finalized & Approved)
**TDD plan:** `tdd/phase3_ranking_query_engine_tdd.md` (v0.3)
**Prior pushback:** `pushback/specs/...` ✅ CLOSED · `pushback/tdd/...` ✅ CLOSED
**Review date:** 2026-06-28
**Reviewer:** AI code review (built + ran the suite, cross-checked against spec v1.0)

---

## Folder convention

| Folder | Reviews |
|--------|---------|
| `pushback/specs/` | spec documents |
| `pushback/tdd/` | TDD / implementation **plans** |
| `pushback/implementation/` | the **implemented** tests + code (this file) |

When resolved, flip Status to **COMPLETED** (same pattern as the spec/TDD pushbacks).

---

## Verification performed

- Built `unit_tests` + `runtime_tests` clean.
- `./build/unit_tests "[phase3]"` → **16 cases / 63 assertions, all pass**.
- `./build/runtime_tests "[phase3]"` → **2 cases / 24 assertions, all pass**.
- Full regression: `unit_tests` **294 assertions / 109 cases**; `runtime_tests`
  **462 / 64** — all green, no Phase 2.x regressions (§1.1 satisfied).

---

## What Looks Good

- **Coverage maps to the spec.** 17 Phase 3 cases across the matrix: reference
  fixture, IDF, length normalization, all 10 §3.0 query-semantics examples (incl.
  `or_with_not_example`), and runtime `/search` descending-order + top-`k`.
- **Reference fixture exists and is real** (`tests/phase3_ranking/fixtures/`):
  `bm25_reference.json` (k1=1.2, b=0.75, tolerance, cases, tie_break) + generator
  `bm25_reference.py`. Scores are asserted against the engine — satisfies spec §5
  intent (concrete, checked-in artifact).
- **File ownership** follows TDD §2a (`test_bm25_reference.cpp` = formula/IDF/length-
  norm/tie; `test_ranking.cpp` = semantics; `test_runtime_search_ranking.cpp` = E2E).
- **Orphan resolved:** `test_ranking.cpp` rewritten (the old non-BM25 TF-order
  assertion is gone) and wired into `CMakeLists.txt` under `tests/phase3_ranking/`.

---

## Anomalies / Pushback

### 1. §4.1 epsilon tie-break is a FALSE GREEN — **HIGH** — ✅ RESOLVED

**Spec §4.1** defines ties as `|Δscore| < 1e-9`, broken by ascending `docId`.

**Was:** `search_service.cpp` used exact float inequality; `tie_break` fixture used
identical docs (Δ = 0), so the epsilon path was never exercised.

**Resolution (2026-06-28):**
- Extracted `haystack::compare_scored_desc` / `scores_tied` to `src/core/score_order.h`
  (shared `kScoreTieEpsilon = 1e-9`).
- `search_service.cpp` sort comparator now uses epsilon tie rule.
- Added `test_bm25_reference.cpp::epsilon tie comparator prefers ascending docId on
  near-equal scores` — feeds crafted scores differing by `1e-10` (would fail with exact
  `!=`).
- Kept identical-doc `tie_break` fixture case (exact tie, still passes).

---

### 2. §2.1.5 "missing doc length" branch — confirm deferral is recorded — **LOW** — ✅ RESOLVED

TDD §4 marks §2.1.5 (skip a candidate with no recorded length) as ⏭️ deferred
(defensive branch, unreachable via `add_document()`). Confirmed not covered by a
test, which is acceptable per that decision. Deferral remains documented in TDD §4.

---

### 3. TDD record not yet reconciled with results — **LOW** — ✅ RESOLVED

TDD v0.3 reconciled: traceability matrix rows flipped to 🟢, E1–E5 cycle log filled,
§6 test counts and acceptance checkboxes updated.

---

## Severity summary

| # | Issue | Severity | Status |
|---|-------|----------|--------|
| 1 | §4.1 epsilon tie-break false green | **High** | ✅ Resolved |
| 2 | §2.1.5 deferral — confirm documented | Low | ✅ Resolved |
| 3 | TDD matrix/counts not reconciled | Low | ✅ Resolved |

---

## Verdict

**Approved for sign-off.** Coverage and structure are excellent; the §4.1 epsilon
tie-break gap is closed in code and tested. TDD record reconciled.
