# Phase 3: Ranking & Query Engine

## Spec Status

**Status:** Finalized and Approved
**Version:** 1.0
**Date:** 2026-06-28
**Approval Date:** 2026-06-28
**Approved By:** Spec Review (VT)
**Dependencies:** Phase 2.0 (Core indexing) — COMPLETE, Phase 2.1 (Persistence) — COMPLETE, Phase 2.2–2.4 (CLI/runtime lifecycle) — COMPLETE, Phase 2.5 (PDF + OCR ingestion) — COMPLETE

**Approval notes:**
- Specification is complete, internally consistent, and testable.
- Spec review pushback (`pushback/specs/phase3_ranking_query_engine_review.md`) — ✅ CLOSED (resolved in v0.2).
- TDD plan pushback (`pushback/tdd/phase3_ranking_query_engine_tdd_review.md`) — ✅ CLOSED (resolved in TDD v0.2).
- Approved for test generation and TDD implementation (`tdd/phase3_ranking_query_engine_tdd.md`).

**Revision history:**
- v1.0 (2026-06-28): Finalized and approved for implementation after both spec and TDD pushback reviews were closed.
- v0.2 (2026-06-28): Addressed review pushback (`pushback/specs/phase3_ranking_query_engine_review.md`) — aligned tolerance to `1e-9` and epsilon tie rule (Phase 2.1), added Query Language section, deferred TF-IDF in Non-Goals, declared authoritative relationship to Phase 2.1, made reference validation concrete, noted orphan `test_ranking.cpp`, clarified top-k and optional GUI debug view.
- v0.1 (2026-06-28): Initial draft.

**Relationship to Phase 2.1:** This spec is the **authoritative** definition of
ranking and query semantics for the project. `specs/phase2_persistence.md` also
states BM25 ordering, score-stability, and the `1e-9` tolerance rule; where they
overlap, this spec governs and intentionally **reuses** (does not supersede) Phase
2.1's `1e-9` tolerance and epsilon tie rule. Future changes to ranking/query
semantics SHALL be made here first.

---

## Overview

Phase 3 formalizes the **relevance ranking** and **query semantics** of the search
engine. A BM25 scorer and an AND/OR/NOT query parser already exist in the codebase
(`SearchService::search_scored`, `parse_query`); this phase makes their behavior a
**specified, deterministic, reference-validated contract** rather than an
undocumented implementation detail.

Phase 3 is strictly additive to the search contract and MUST NOT introduce
regressions to Phases 2.0 through 2.5. It does not change ingestion, persistence
formats, CLI flags, exit codes, or the `/health` contract. It MAY extend the
`/search` response to expose ranking detail (additive fields only).

## Goals

- Define the exact BM25 scoring model, formula, and parameters used for ranking
- Guarantee deterministic, reproducible result ordering for identical inputs
- Formalize query semantics (AND default, OR, NOT term exclusion)
- Validate scores against an independent reference implementation within tolerance
- Expose per-result ranking detail through the API and GUI (additive)
- Preserve backward compatibility with all existing phases

## Non-Goals

Phase 3 SHALL NOT include:

- **A separate TF-IDF ranking mode.** Phase 3 ships **BM25 only**. BM25 already
  incorporates TF and IDF components; there is no independent TF-IDF scorer or a
  user-selectable ranking mode. (README and commercial docs are corrected to say
  "BM25" rather than "BM25/TF-IDF".)
- Vector / semantic / embedding-based ranking (deferred; see commercial roadmap Phase 11)
- Learning-to-rank (LTR) or machine-learned scoring
- Phrase queries, proximity, or wildcard/fuzzy matching (deferred to a later phase)
- Field weighting / boosting (e.g., title vs body) beyond document-level scoring
- Per-query tunable BM25 parameters exposed to end users
- A GUI "ranking debug" view as a completion requirement — it is **optional /
  post-Phase-3** (see §6.3); Phase 3 acceptance (§8) does not require it
- Changes to ingestion, persistence schema, CLI flags, exit codes, or `/health`

## Terminology

- **TF (term frequency)**: Count of a term's occurrences within a single document.
- **DF (document frequency)**: Number of documents containing a term.
- **N**: Total number of indexed documents.
- **dl**: Document length in tokens (from the indexing tokenizer).
- **avgdl**: Average document length across the index.
- **IDF (inverse document frequency)**: Weight that rewards rarer terms.
- **k1, b**: BM25 tuning constants (term-frequency saturation; length normalization).
- **Candidate set**: Documents selected by the query parser before scoring.

---

## 1. Backward Compatibility

### 1.1 Non-Breaking Guarantee

**Requirement 1.1.1: No Regressions**

Phase 3 SHALL NOT:

- Change ingestion behavior or the persisted index format (`index_meta.json`,
  `docs.jsonl`, `postings.bin`) in a way that breaks Phase 2.x.
- Change CLI flags or exit code semantics (0, 2, 3).
- Change the `/health` contract.
- Remove or rename existing `/search` response fields (`query`, `results`,
  `docId`, `score`, `snippet`, `file_name`, `page_number`).

**Requirement 1.1.2: Test Compatibility**

All existing Phase 2.x unit and runtime tests SHALL continue to pass unchanged.

---

## 2. Scoring Model (BM25)

### 2.1 Ranking Function

**Requirement 2.1.1: BM25 Scoring**

The engine SHALL rank candidate documents using the Okapi BM25 scoring function.
For a query `Q` containing scoring terms `t ∈ T` and a document `d`, the score SHALL be:

```
score(d, Q) = Σ_{t ∈ T}  IDF(t) · ( tf(t,d) · (k1 + 1) )
                          ─────────────────────────────────────────
                          ( tf(t,d) + k1 · (1 − b + b · (dl_d / avgdl)) )
```

**Requirement 2.1.2: IDF Definition**

The IDF term SHALL use the BM25 probabilistic form with +1 smoothing to guarantee
non-negative weights:

```
IDF(t) = ln( ( (N − df(t) + 0.5) / (df(t) + 0.5) ) + 1 )
```

**Requirement 2.1.3: Parameters**

The BM25 parameters SHALL be fixed constants:

- `k1 = 1.2`
- `b = 0.75`

These values SHALL NOT be configurable via CLI or query parameters in Phase 3
(consistent with the project's determinism policy). Any future tunability is a
separate, explicitly-specified change.

**Requirement 2.1.4: Length Normalization Safety**

When `avgdl = 0` (empty index), the length-normalization factor
`(1 − b + b · (dl / avgdl))` SHALL default to `1.0` to avoid division by zero.
A query against an empty index SHALL return zero results, never an error.

**Requirement 2.1.5: Missing Statistics**

If a candidate document has no recorded length (`dl` unavailable), it SHALL be
skipped during scoring rather than producing an undefined score.

### 2.2 Index Statistics

**Requirement 2.2.1: Persisted Statistics**

`N` and `avgdl` SHALL be persisted with the index (`index_meta.json`) and restored
on load, so that scores after `save`→`load` are identical to scores before saving
(within the determinism tolerance of §4).

---

## 3. Query Semantics

### 3.0 Query Language (user-facing syntax)

The query string accepted by `/search?q=...` SHALL be interpreted as
whitespace-separated tokens with the following rules (matching `parse_query`):

| Token form | Meaning |
|---|---|
| `term` | A positive term to match and score |
| `-term` | Negation: exclude documents containing `term` (NOT) |
| `OR` or `or` (as a standalone token) | Switches the **entire** query to OR (union) mode |
| _(no `OR` token present)_ | Default AND (intersection) mode |

This is **global boolean mode**, not grouped/parenthesized boolean logic. A single
`OR` anywhere flips the whole query to union; there is no operator precedence and no
grouping in Phase 3.

**Worked examples** (corpus-independent semantics):

| Query | Interpretation |
|---|---|
| `alpha beta` | docs containing **alpha AND beta**, scored by BM25 |
| `alpha beta OR gamma` | docs containing **alpha OR beta OR gamma** (global union — *not* `(alpha AND beta) OR gamma`) |
| `alpha -beta` | docs containing **alpha** and **NOT beta** |
| `alpha OR beta -gamma` | union of **alpha, beta**, excluding any doc containing **gamma** |
| `-foo` (only NOT terms) | empty result set (no positive candidates) — see §3.5 |
| `OR` (alone) | empty result set (no terms) |
| `""` (empty) | empty result set, successful response |

**Requirement 3.0.1: Stable Interpretation**

The above interpretation SHALL be deterministic and SHALL NOT depend on token order
beyond what these rules specify (e.g., `OR` position does not change the result set,
only its presence does).

### 3.1 Matching and candidate selection

**Requirement 3.1: Tokenization Parity**

Query terms SHALL be tokenized with the same tokenizer used at indexing time, so
that matching is consistent between ingestion and query.

**Requirement 3.2: Default Conjunction (AND)**

A multi-term query SHALL default to AND semantics: the candidate set is the
**intersection** of the per-term posting lists.

**Requirement 3.3: Disjunction (OR)**

When OR mode is indicated by the parser (`ParsedQuery.is_or`), the candidate set
SHALL be the **union** of the per-term posting lists.

**Requirement 3.4: Negation (NOT)**

Terms marked as negated (`ParsedQuery.not_terms`) SHALL exclude any document
containing them from the candidate set, regardless of AND/OR mode. NOT terms SHALL
NOT contribute to the BM25 score.

**Requirement 3.5: Empty / Unmatched Queries**

- An empty query, or a query whose terms match no documents, SHALL return an empty
  result set with a successful response (no error).
- A query with only NOT terms SHALL return an empty result set (no positive
  candidates), deterministically.

---

## 4. Determinism

**Requirement 4.1: Deterministic Ordering**

Results SHALL be ordered by descending BM25 score. Two scores SHALL be considered
**tied** when their absolute difference is `< 1e-9` (consistent with Phase 2.1,
`specs/phase2_persistence.md`). Ties SHALL be broken by **ascending `docId`**. For
identical index content and an identical query, the ordered result list SHALL be
reproducible across runs, machines, and CI.

> Implementation note (informative): `haystack::compare_scored_desc` in
> `src/core/score_order.h` implements the `1e-9` epsilon tie rule; `search_service.cpp`
> uses it for result ordering (§4.1).

**Requirement 4.2: Score Reproducibility**

For identical inputs, computed scores SHALL be reproducible. Floating-point scores
SHALL be compared within an absolute tolerance of **`1e-9`** for test purposes
(matching Phase 2.1 and existing `tests/test_persistence_determinism.cpp`).

**Requirement 4.3: Stability Across Persistence**

Searching an in-memory index and searching the same index after `save`→`load` SHALL
produce identical ordering and scores within the `1e-9` tolerance of §4.2. This
reuses, and SHALL remain consistent with, Phase 2.1's persistence-determinism
guarantee.

---

## 5. Reference Validation

**Requirement 5.1: Independent Reference**

The implementation SHALL be validated against an independent BM25 reference for at
least:

- A single-term query over a small known corpus
- A multi-term AND query
- A multi-term OR query
- A query exercising NOT exclusion
- A case demonstrating length normalization (short relevant doc outranks long
  padded doc — see existing `tests/test_bm25.cpp`)

**Requirement 5.2: Reference Artifact (concrete)**

The reference SHALL be a **checked-in artifact**, not ad-hoc inline numbers:

- A fixture file `tests/fixtures/bm25_reference.json` containing, for each case: the
  corpus (docId → text), the query string, and the expected ordered
  `[{docId, score}]` computed from the §2 formula with `k1=1.2`, `b=0.75`.
- The expected scores SHALL be generated by a small, committed reference script
  (e.g. `tests/fixtures/bm25_reference.py`) so they can be regenerated when the
  formula or fixtures change. The script is the source of the numbers; the JSON is
  the committed expectation.
- A unit test (`test_bm25.cpp` / `test_ranking.cpp`) SHALL load the fixture and
  assert engine output against it.

**Requirement 5.3: CI Gating**

The reference-validation test SHALL run as part of the `unit_tests` target and
therefore in CI. A formula change that moves scores beyond tolerance SHALL fail CI
until the reference is regenerated and reviewed.

**Requirement 5.4: Tolerance**

Computed scores SHALL match reference values within an absolute tolerance of
**`1e-9`** (§4.2). Result ordering SHALL match the reference exactly.

---

## 6. API & GUI Exposure (Additive)

**Requirement 6.1: Score in `/search` Response**

Each result in the `/search` response SHALL include its BM25 `score` (already
present). The `score` SHALL be the value defined in §2, not a normalized or
rounded variant. Existing fields SHALL be preserved (§1.1.1).

**Requirement 6.1.1: Ranking Before Truncation (top-k)**

`/search` accepts an optional `k` parameter (top-k). Ranking SHALL be computed over
the **full** candidate set first (§2, §4), and the ordered list SHALL be truncated
to the top `k` **after** sorting. Truncation SHALL NOT change the relative order or
scores of the returned results. When `k` is absent, the server default applies
(currently 10).

**Requirement 6.2: Optional Ranking Detail**

The response MAY include additional, **additive** per-result ranking detail (e.g.,
matched terms or per-term contributions) under new field names. Such additions
SHALL NOT alter or remove existing fields and SHALL be safely ignorable by older
clients.

**Requirement 6.3: GUI Ranking Visibility**

The React GUI SHALL surface the relevance `score` for each result (e.g., a score
badge, already rendered by `ResultCard`). It MAY **optionally** provide a debug view
explaining why a result ranked where it did. The debug view is **out of scope for
Phase 3 completion** (see Non-Goals and §8) — it is a post-Phase-3 enhancement. Any
GUI change SHALL be additive and covered by tests, consistent with the Phase 2 GUI
methodology.

---

## 7. Test Plan

Unit tests (Catch2, `unit_tests`):

1. Single-term ranking matches reference within tolerance.
2. Length normalization: shorter relevant doc outranks longer padded doc
   (extends existing `test_bm25.cpp`).
3. IDF behavior: rarer term contributes more than a common term.
4. AND intersection candidate selection.
5. OR union candidate selection.
6. NOT exclusion removes documents and does not affect scoring of others.
7. Empty index and unmatched query return empty results, no error.
8. Deterministic tie-break by ascending `docId` on equal scores.
9. Score/order parity after `save`→`load`.

Runtime tests (`runtime_tests`) where end-to-end coverage adds value:

10. `/search` returns results ordered by descending score with the `score` field
    populated.

GUI tests (`vitest`):

11. `ResultCard` renders the score; any ranking-detail UI is covered by tests.

**Test-file housekeeping (required):** `tests/test_ranking.cpp` currently exists but
is **not registered in `CMakeLists.txt`**, so it never compiles or runs (orphan).
Phase 3 SHALL either wire it into the `unit_tests` target and use it for the cases
above, or delete it — the repository SHALL NOT contain orphaned test files that give
a false impression of coverage.

---

## 8. Acceptance Criteria

Phase 3 is complete when:

- [x] BM25 formula, IDF form, and parameters (k1=1.2, b=0.75) are implemented and
      documented exactly as in §2.
- [x] Query semantics (AND default, OR, NOT) behave as in §3.
- [x] Ordering is deterministic with a `1e-9` epsilon tie rule, tie-broken by
      ascending `docId` (§4.1).
- [x] Scores match the checked-in reference fixture within `1e-9` and ordering
      matches exactly; the test runs in CI (§5).
- [x] Score/order are stable across `save`→`load` within `1e-9` (§4.3).
- [x] Query Language (§3.0) behaves as documented, including the listed edge cases.
- [x] `/search` exposes `score` and ranks before top-k truncation (§6.1, §6.1.1);
      GUI surfaces the score (§6.3).
- [x] `tests/phase3_ranking/test_ranking.cpp` is wired into `CMakeLists.txt` (no orphans).
- [x] All Phase 2.x tests still pass (§1.1.2); new Phase 3 tests pass.

---

## 9. Current Implementation Status (informative)

This section is **informative**, not normative — it records what already exists so
implementation focuses on the gaps.

**Already implemented** (`src/core/search_service.cpp::search_scored`):

- BM25 scoring with `k1 = 1.2`, `b = 0.75`.
- IDF as `ln(((N − df + 0.5)/(df + 0.5)) + 1)` (§2.1.2).
- TF saturation with length normalization and divide-by-zero guard (§2.1.4).
- AND/OR/NOT candidate selection via `parse_query` (`terms`, `not_terms`, `is_or`).
- Deterministic sort: score descending; `1e-9` epsilon tie → `docId` ascending via
  `score_order.h` (§4.1).
- `N` and `avgdl` persisted in `index_meta.json` and restored on load (§2.2).
- `score` already returned in the `/search` response and rendered by `ResultCard`.

**Gaps closed in Phase 3 (2026-06-28):**

- Formalize and document the model as the source of truth (this spec). ✅
- `1e-9` epsilon tie comparison in `score_order.h` / `search_service.cpp` (§4.1). ✅
- Reference-validation fixture + script + tests under `tests/phase3_ranking/` (§5). ✅
- Phase 3 tests: IDF, OR, NOT, tie-break, semantics, runtime ranking (§3, §6). ✅
- Edge cases (empty index, NOT-only query) covered in `test_ranking.cpp`. ✅
- Orphan `test_ranking.cpp` resolved — rewritten under `tests/phase3_ranking/`. ✅

**Still optional / post-Phase-3:**

- Ranking-detail exposure (§6.2) and GUI debug view (§6.3 optional).

---

**End of Phase 3 Specification (Draft)**
