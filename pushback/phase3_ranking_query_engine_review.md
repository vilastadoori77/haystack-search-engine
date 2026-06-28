# Phase 3 Spec Review — Pushback & Anomalies

**Status:** ✅ **COMPLETED** — all items resolved in spec v0.2  
**Spec reviewed:** `specs/phase3_ranking_query_engine.md` (Draft v0.1 → **v0.2**)  
**Review date:** 2026-06-28  
**Resolved date:** 2026-06-28  
**Reviewer:** AI code review (cross-checked against codebase, Phase 2 specs, and tests)

---

## Summary

The Phase 3 draft was **directionally correct** and §9 was unusually honest about what already exists. All 10 pushback items were addressed in **`specs/phase3_ranking_query_engine.md` v0.2** (2026-06-28). The spec is **ready for approval**; remaining work is Phase 3 **implementation** (see spec §9 gaps), not further spec revision.

---

## Resolution (v0.2)

| # | Item | Resolved in v0.2 |
|---|------|------------------|
| 1 | Tolerance `1e-6` vs `1e-9` | §4.2, §5.4 → **`1e-9`** |
| 2 | Tie-breaking inconsistency | §4.1 → **`1e-9` epsilon tie rule** (+ implementation note) |
| 3 | TF-IDF messaging | Non-Goals §50–53; README + `COMMERCIAL_ROADMAP.md` updated |
| 4 | Query syntax underspecified | **§3.0 Query Language** (table + examples) |
| 5 | Dual source of truth with Phase 2.1 | Header: Phase 3 **authoritative**, reuses Phase 2.1 tolerance |
| 6 | Reference validation vague | §5.2–5.3: `bm25_reference.json`, script, CI gating |
| 7 | Test plan vs repo reality | §7 housekeeping: orphan `test_ranking.cpp` |
| 8 | GUI debug view scope creep | Non-Goals + §6.3 + §8: **out of scope for completion** |
| 9 | Commercial roadmap overclaim | `COMMERCIAL_ROADMAP.md`: formalization in progress |
| 10 | `k` parameter not mentioned | §6.1.1: rank then truncate top-k |

---

## What Looks Good

- **Accurate baseline (§9):** BM25, AND/OR/NOT, `N`/`avgdl` persistence, score in `/search`, and `ResultCard` score display all match the current code.
- **Backward-compat framing (§1):** Additive-only changes to API/GUI; no ingestion/persistence/CLI/`/health` breakage — aligned with Phase 2.5.
- **Formula (§2):** Matches `search_service.cpp` (`k1=1.2`, `b=0.75`, IDF with `+1` smoothing, length-normalization guard).
- **Test plan (§7):** Reasonable gaps list — IDF, OR, NOT, tie-break, save/load parity are mostly missing from dedicated Phase 3 tests today.

---

## Anomalies / Pushback

### 1. Tolerance conflict: `1e-6` vs `1e-9` — ✅ RESOLVED

Phase 3 §4.2 / §5.2 specify **`1e-6`**.

But Phase 2.1 (`specs/phase2_persistence.md`) and existing tests already use **`1e-9`**:

- `specs/phase2_persistence.md`: `Scores SHALL match within tolerance: abs(score_before - score_after) < 1e-9`
- `tests/test_persistence_determinism.cpp`: `const double tolerance = 1e-9`

**Fix:** Pick one tolerance project-wide. Recommend keeping **`1e-9`** since it's already normative in Phase 2.1 and in tests.

**Resolution (v0.2):** §4.2 and §5.4 now use **`1e-9`**, aligned with Phase 2.1.

---

### 2. Tie-breaking definition is inconsistent — ✅ RESOLVED

Phase 2.1 says ties are when `|score_a - score_b| < 1e-9`.

Phase 3 §4.1 says ties broken by ascending `docId`, but the implementation uses **exact** float equality in `search_service.cpp`:

```cpp
if (a.score != b.score) return a.score > b.score;
return a.doc_id < b.doc_id;
```

**Fix:** Phase 3 should either:

- adopt the Phase 2.1 epsilon tie rule explicitly, **or**
- state it supersedes Phase 2.1 and update Phase 2.1 + tests accordingly.

Right now there are three different tie rules across spec, Phase 2.1, and code.

**Resolution (v0.2):** §4.1 adopts **`1e-9` epsilon tie rule**; implementation change tracked in spec §9.

---

### 3. TF-IDF is promised elsewhere but absent from Phase 3 — ✅ RESOLVED

`README.md` and `COMMERCIAL_ROADMAP.md` both say **"BM25/TF-IDF"**. Phase 3 only specifies BM25 and never mentions TF-IDF in goals or non-goals.

**Fix:** Either:

- add TF-IDF to **Non-Goals** explicitly ("deferred; BM25 only in Phase 3"), **or**
- remove TF-IDF from README/commercial docs if it's not planned.

Otherwise buyers/readers will expect two ranking modes that don't exist.

**Resolution (v0.2):** Non-Goals §50–53; README and `COMMERCIAL_ROADMAP.md` updated to **BM25 only**.

---

### 4. Query syntax is underspecified (important) — ✅ RESOLVED

§3 references `ParsedQuery.is_or` and `not_terms` but **never defines user-facing syntax**. Actual behavior in `query_parser.cpp`:

- `OR` / `or` anywhere in the query flips **the entire query** to OR mode (global union, not grouped boolean logic).
- `-term` marks NOT exclusion.
- Default is AND.

Examples **not** covered in the spec:

- `"a b OR c"` → union of all three terms (not `(a AND b) OR c`)
- `"OR"` alone → empty results
- `"-foo"` only → empty results (§3.5 mentions this; good)

**Fix:** Add a **Query Language** section with concrete examples and edge cases. Without it, §3 is normative about internals but not about what users can type.

**Resolution (v0.2):** **§3.0 Query Language** added with syntax table and worked examples.

---

### 5. Dual source of truth with Phase 2.1 — ✅ RESOLVED

`phase2_persistence.md` already normatively defines BM25 ordering, score stability, and `1e-9` tolerance. Phase 3 largely re-states the same rules.

**Risk:** Future edits to one spec drift from the other.

**Fix:** Phase 3 should say explicitly:

- "This spec is the authoritative definition of ranking/query semantics"
- and which Phase 2.1 sections it **supersedes** (or cross-reference them as "see Phase 3 §2").

**Resolution (v0.2):** Spec header declares Phase 3 **authoritative**; reuses Phase 2.1 `1e-9` tolerance (does not supersede).

---

### 6. §5 Reference validation is vague — ✅ RESOLVED

"Independent reference (hand-computed or small reference implementation)" doesn't say:

- where it lives (e.g. `tests/fixtures/bm25_reference.json`, Python script, inline vectors)
- who maintains it when the formula changes
- whether it's CI-gated

Phase 2.5 was much tighter on acceptance criteria.

**Fix:** Tighten §5 with a concrete artifact format.

**Resolution (v0.2):** §5.2–5.3 specify `tests/fixtures/bm25_reference.json`, reference script, and CI gating.

---

### 7. Test plan vs repo reality — ✅ RESOLVED

| Spec item | Status |
|-----------|--------|
| `test_bm25.cpp` length-normalization case | ✅ Exists (only 1 case) |
| IDF, OR, NOT, tie-break tests | ⚠️ Partially elsewhere (`test_search_service.cpp`, `test_query_parser.cpp`) — not score/reference-level |
| `test_ranking.cpp` | ❌ **Orphan** — file exists but is **not in `CMakeLists.txt`**, never runs |
| Runtime `/search` score ordering test (§7.10) | ❌ Not present in `runtime_tests` |
| GUI score display (§7.11) | ✅ Already covered in `ResultCard.test.tsx` |

**Fix:** Note `test_ranking.cpp` should be wired in or deleted. The spec's §7 gap list is fair but should call out the orphan file.

**Resolution (v0.2):** §7 test-file housekeeping requires wire-or-delete for orphan `test_ranking.cpp`.

---

### 8. §6.3 GUI "debug view" is scope creep risk — ✅ RESOLVED

§6.3 says GUI **MAY** provide a debug view for why a result ranked where it did. That's fine as optional, but if someone treats it as required for "Phase 3 complete," scope balloons.

**Fix:** Keep it in Non-Goals or mark clearly as **optional / post-Phase-3** in acceptance criteria (§8 currently doesn't require it — good, but §6.3 wording could be clearer).

**Resolution (v0.2):** Non-Goals, §6.3, and §8 mark debug view **out of scope for Phase 3 completion**.

---

### 9. Commercial roadmap vs spec status mismatch — ✅ RESOLVED

`COMMERCIAL_ROADMAP.md` lists BM25 as "✅ (Phase 3)" but Phase 3 is still **Draft v0.1**. Technically BM25 works today, but Phase 3 completion criteria (reference validation, formal test suite) are not done.

**Fix:** In commercial doc, say **"BM25 implemented; Phase 3 formalization in progress"** to avoid overclaiming.

**Resolution (v0.2):** `COMMERCIAL_ROADMAP.md` updated to **"implemented; formalization in progress"**.

---

### 10. Minor: `k` parameter not mentioned — ✅ RESOLVED

`/search` supports `k` (top-k truncation in `searchd`). Phase 3 doesn't mention whether ranking happens before or after truncation. Code ranks full result set then truncates — worth one sentence so API behavior is specified.

**Resolution (v0.2):** §6.1.1 documents rank-full-set-then-truncate top-k behavior.

---

## Recommended Actions Before Approval — ✅ ALL COMPLETE

1. ~~Align tolerance and tie-breaking with Phase 2.1 (`1e-9` + epsilon ties, or explicitly supersede).~~ ✅
2. ~~Add a **Query Language** section (syntax + examples).~~ ✅
3. ~~Resolve **TF-IDF** messaging across docs.~~ ✅
4. ~~Clarify **supersedes / references** vs `phase2_persistence.md`.~~ ✅
5. ~~Make §5 reference validation concrete (artifact + CI).~~ ✅
6. ~~Clean up **orphan `test_ranking.cpp`**.~~ ✅ (specified in spec §7/§8; implementation pending)

---

## Verdict

~~**Approve with revisions.**~~ **CLOSED — pushback resolved in spec v0.2 (2026-06-28).** Ready for spec approval. Phase 3 implementation gaps remain per `specs/phase3_ranking_query_engine.md` §9.
