# Phase 3 TDD Plan Review — Pushback & Anomalies

**Status:** ✅ **COMPLETED** — all items resolved in TDD v0.2  
**Document reviewed:** `tdd/phase3_ranking_query_engine_tdd.md` (v0.1 → **v0.2**)  
**Spec dependency:** `specs/phase3_ranking_query_engine.md` (v0.2)  
**Spec pushback (closed):** `pushback/specs/phase3_ranking_query_engine_review.md`  
**Review date:** 2026-06-28  
**Resolved date:** 2026-06-28  
**Reviewer:** AI code review (cross-checked against spec v0.2, codebase, and existing tests)

---

## Summary

The TDD implementation record was a **solid template** but not ready to execute as-is.
All **13 pushback items** (plus follow-up §3.0 ex7) were addressed in
**`tdd/phase3_ranking_query_engine_tdd.md` v0.2** (2026-06-28). The TDD plan is
**approved for implementation**; next step is coding E1 (reference fixture), not
further plan revision.

---

## Resolution (TDD v0.2)

| # | Item | Resolved in TDD v0.2 |
|---|------|----------------------|
| 1 | Orphan TF test must not be wired as-is | E3: delete TF case; rewrite + wire |
| 2 | Wrong E1 / missing implementation order | §1b order; E1 = reference fixture |
| 3 | Matrix understates green tests | Pre-marked 🟢/🟡 in §2 |
| 4 | §3.0 examples need E2E tests | Rows §3.0 ex1–ex7 + E3 |
| 5 | Runtime `/search` + `k` underspecified | `test_runtime_search_ranking.cpp` + E4 |
| 6 | §2.1.5 untestable via public API | ⏭️ deferred in matrix + §4 |
| 7 | `test_bm25` vs `test_ranking` ownership | §2a table |
| 8 | Duplicate coverage policy | §2b smoke vs score-level |
| 9 | Fixtures / Python / CI gaps | §1a Prerequisites |
| 10 | Epsilon tie corpus strategy | `tie_break` in fixture; E2 |
| 11 | Redundant §1.1 rows | Merged + `[phase2_5]` row |
| 12 | Characterization vs strict RED | §1 + entry template |
| 13 | §6.2 deferral | §4 row 3 |
| — | §3.0 `alpha OR beta -gamma` (follow-up) | §3.0 ex7 + `or_with_not_example` in E3 |

---

## Folder convention (why `pushback/`)

| Folder | Purpose |
|--------|---------|
| `specs/` | Normative product requirements |
| `tdd/` | Implementation record (traceability, RED/GREEN log, sign-off) |
| `pushback/specs/` | Review findings on **spec** documents |
| `pushback/tdd/` | Review findings on **TDD / implementation plans** |

When TDD pushback is addressed, update this file to **COMPLETED** (same pattern as
`phase3_ranking_query_engine_review.md`).

---

## What Looks Good

- Clear structure: traceability → TDD log → deviations → sign-off
- Correctly references spec v0.2 and closed spec pushback
- Maps most normative requirements to test locations
- Acknowledges orphan `test_ranking.cpp` housekeeping
- Final verification mirrors spec §8 acceptance criteria
- Status legend and entry template are practical for ongoing work

---

## Recommended implementation order (missing from TDD)

The TDD starts with **E1: wire orphan `test_ranking.cpp`**, which is the wrong first
step. Most Phase 3 behavior already exists; work should follow this sequence:

1. Create `tests/fixtures/bm25_reference.json` + `bm25_reference.py` + loader test
   (pins expected scores — source of truth for all formula tests)
2. **RED → GREEN:** epsilon tie fix in `search_service.cpp` (§4.1 — only known code change)
3. **Rewrite** `test_ranking.cpp` (delete TF-order test; add BM25 + §3.0 cases); wire into `CMakeLists.txt`
4. Add `tests/test_runtime_search_ranking.cpp` for `/search` ordering + `k` truncation
5. Mark pre-existing passing tests as 🟢 in the traceability matrix

---

## Anomalies / Pushback

### 1. E1 "wire orphan" will fail immediately — existing test is wrong for BM25 — **HIGH**

The orphan `tests/test_ranking.cpp` contains:

```cpp
TEST_CASE("Results are ranked by term frequency")
{
    ...
    auto results = ss.search("banana");
    REQUIRE(results == std::vector<int>{1, 2});
}
```

This expects **raw term-frequency order** (doc 1 before doc 2). Under BM25, doc 2
(three occurrences of `banana`) should outrank doc 1 (one occurrence). Wiring this
file in **as-is will RED** on behavior the spec does not require.

**Fix:** E1 should say **delete the TF test and rewrite** `test_ranking.cpp` from
scratch (or delete the file and consolidate into `test_bm25.cpp`). Do not treat
"wire" as the default action.

---

### 2. Recommended work order is missing (and E1 is in the wrong place) — **HIGH**

Strict Red→Green for every row is misleading when most code already passes. Starting
with E1 before the reference fixture exists will cause rework.

**Fix:** Add § "Recommended implementation order" to the TDD doc (see above). Reorder
E1–En to match.

---

### 3. Traceability matrix understates what is already green — **MEDIUM**

These already pass today but are marked ⬜:

| Spec req | Existing test | Suggested status |
|----------|---------------|------------------|
| §2.2.1 | `test_persistence.cpp` — "save/load preserves BM25 corpus stats" | 🟢 |
| §4.3 | `test_persistence_determinism.cpp` — save/load scores within `1e-9` | 🟢 |
| §6.3 | `ResultCard.test.tsx` — score rendering | 🟢 |
| §3.4 (partial) | `test_search_service.cpp` — NOT filter | 🟢 (order only, no scores) |
| §3.3 (partial) | `test_search_service.cpp` — OR | 🟢 (order only, no scores) |
| §6.1 (partial) | `test_phase2_5_pdf_ocr_ingestion.cpp` — curl checks `score` field exists | 🟡 (not ordering) |

**Fix:** Pre-populate matrix with 🟢/🟡 so real gaps are visible before coding.

---

### 4. §3.0 Query Language — no dedicated end-to-end tests planned — **HIGH**

The matrix splits §3.0.1 between `test_query_parser.cpp` (parser internals) and
`test_ranking.cpp` (semantics). Parser tests alone **cannot** verify spec §3.0 worked
examples:

| Query | Expected behavior |
|-------|-------------------|
| `alpha beta OR gamma` | global union — **not** `(alpha AND beta) OR gamma` |
| `-foo` only | empty result set |
| `OR` alone | empty result set |
| `""` (empty) | empty result set, success |

**Fix:** Add one matrix row (and one TDD cycle entry) per §3.0 table example, each
as an integration test in `test_ranking.cpp` using `search_scored()` with expected
docIds and optionally scores.

---

### 5. §6.1 and §6.1.1 runtime tests are named but not designed — **HIGH**

Matrix says `runtime_tests /search` and `k-truncation` but:

- No test file name proposed
- No RED scenario described
- Phase 2.5 only checks `"score"` substring in JSON — **not** descending order

**Fix:** Add `tests/test_runtime_search_ranking.cpp` with concrete scenarios:

- Index known corpus via `searchd --index`; curl `/search?q=...`; parse JSON; assert
  `results[i].score >= results[i+1].score`
- Index ≥10 docs; `k=3`; assert 3 results in same relative order as full search

---

### 6. §2.1.5 "missing doc length" is hard to test through public API — **MEDIUM**

`add_document()` always sets `doc_len_`. The skip path in `search_scored` is
defensive and not reachable via normal ingestion.

**Fix:** Mark as **⏭️ deferred** in matrix (white-box only), or add a test-only hook
(document in TDD §4 deviations if added). Do not leave as a normal RED test without
a plan to trigger the branch.

---

### 7. File ownership split between `test_bm25.cpp` and `test_ranking.cpp` is ambiguous — **MEDIUM**

Spec §5.2 allows either file for the reference loader. The TDD puts reference in
`test_bm25.cpp` and semantics in `test_ranking.cpp`, while spec §9 says "extend
`test_bm25.cpp`" for IDF/OR/NOT/tie-break.

**Fix:** Adopt one convention in the TDD doc:

| File | Owns |
|------|------|
| `test_bm25.cpp` | Formula, reference fixture loader, IDF, length normalization, tie-break |
| `test_ranking.cpp` | Query semantics (AND/OR/NOT) and §3.0 examples at `search_scored` level |

---

### 8. Duplicate / overlapping coverage not called out — **MEDIUM**

| Behavior | Already in | TDD also assigns |
|----------|------------|------------------|
| NOT exclusion | `test_search_service.cpp` | `test_ranking.cpp` NOT |
| OR union | `test_search_service.cpp` | `test_ranking.cpp` OR |
| save/load parity | `test_persistence_determinism.cpp` | reference fixture + §4.3 |

**Fix:** State whether existing tests remain as smoke tests and new tests add
**score-level** assertions, or consolidate and remove duplicates.

---

### 9. §5 reference fixture — infrastructure gaps not noted — **MEDIUM**

- `tests/fixtures/` directory **does not exist yet**
- No JSON loader helper planned (need C++ test utility or inline parsing)
- Python script for regen — **Python 3 not listed** in `BUILD.md` prerequisites
- **No GitHub Actions / CI config** in repo — "CI gating" currently means local `ctest` only

**Fix:** Add TDD "Prerequisites" subsection: Python 3 for fixture regen; clarify CI =
local `ctest` until GitHub Actions is added.

---

### 10. §4.1 epsilon tie — no test construction strategy — **MEDIUM**

The code change (replace `a.score != b.score` with `|a−b| < 1e-9`) needs a corpus
where two docs produce scores within `1e-9` but have different `docId`s. Non-trivial
to construct by hand.

**Fix:** Derive tie corpus from the reference script, or add a dedicated `"tie_break"`
case in `bm25_reference.json`.

---

### 11. §1.1.1 and §1.1.2 are redundant rows — **LOW**

Both map to "full suites stay green." One row is enough.

**Fix:** Merge into one regression row; use freed row for e.g. Phase 2.5 `[phase2_5]`
tags still pass.

---

### 12. Strict TDD wording vs characterization-test reality — **LOW**

The TDD says every test is written **before** the code. For Phase 3, most code
**already exists** — characterization tests may GREEN immediately; only epsilon tie
and reference drift are strict RED cycles.

**Fix:** Add note in TDD §1:

> *Pre-existing behavior: write characterization tests first (may GREEN immediately).
> Net-new behavior (§4.1 epsilon tie, reference fixture): strict RED→GREEN.*

---

### 13. §6.2 optional ranking detail not noted — **LOW**

§6.2 (optional per-result ranking detail fields) is correctly out of scope. One line
in TDD §4: *"§6.2 deferred post-Phase-3"* avoids reviewer questions.

---

## Severity summary

| # | Issue | Severity |
|---|--------|----------|
| 1 | Orphan `test_ranking.cpp` asserts TF order, not BM25 | **High** |
| 2 | Wrong E1 order; missing implementation sequence | **High** |
| 3 | Matrix doesn't mark already-green tests | Medium |
| 4 | §3.0 examples lack dedicated E2E tests | **High** |
| 5 | Runtime `/search` + `k` tests underspecified | **High** |
| 6 | §2.1.5 missing-len untestable via public API | Medium |
| 7 | `test_bm25` vs `test_ranking` ownership unclear | Medium |
| 8 | Duplicate coverage vs existing tests | Medium |
| 9 | Fixtures dir, Python, CI gaps | Medium |
| 10 | Epsilon tie test corpus strategy missing | Medium |
| 11 | Redundant §1.1 rows | Low |
| 12 | TDD wording vs characterization reality | Low |
| 13 | §6.2 deferral not noted | Low |

---

## Recommended actions before implementation — ✅ ALL COMPLETE

1. ~~Revise `tdd/phase3_ranking_query_engine_tdd.md` with implementation order.~~ ✅
2. ~~Rewrite E1: delete TF test; define `test_bm25` vs `test_ranking` ownership.~~ ✅
3. ~~Add §3.0 example rows and runtime test file design to traceability matrix.~~ ✅
4. ~~Pre-mark 🟢/🟡 for existing passing tests.~~ ✅
5. ~~Add Prerequisites (Python 3, `tests/fixtures/`, CI = `ctest`).~~ ✅
6. ~~Note characterization vs strict RED in §1.~~ ✅
7. ~~Close this pushback doc when TDD is updated.~~ ✅

---

## Verdict

~~**Revise TDD plan before coding.**~~ **CLOSED — pushback resolved in TDD v0.2 (2026-06-28).**
Approved for implementation. Proceed with E1 (reference fixture) per `tdd/phase3_ranking_query_engine_tdd.md` §1b.
