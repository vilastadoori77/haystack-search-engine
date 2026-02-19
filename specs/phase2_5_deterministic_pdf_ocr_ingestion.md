# Phase 2.5: Deterministic PDF + OCR Ingestion

## Spec Status

**Status:** Finalized and Approved  
**Version:** 1.0  
**Date:** 2026-02-08  
**Dependencies:** Phase 2.0 (Core indexing) - COMPLETE, Phase 2.1 (Persistence) - COMPLETE, Phase 2.2 (CLI Lifecycle) - COMPLETE, Phase 2.3 (Runtime Lifecycle) - COMPLETE, Phase 2.4 (Runtime Operational Guarantees) - COMPLETE  

## Overview

Phase 2.5 extends the indexing engine to support deterministic ingestion of `.txt` and `.pdf` files, automatic OCR for image-based text inside PDFs, mixed folder ingestion, and CPU-safe parallel indexing. This phase is strictly additive and MUST NOT introduce regressions to Phases 2.0 through 2.4.

## Goals

- Support folder-based ingestion of `.txt` and `.pdf` with mixed content
- Provide deterministic, page-level PDF indexing with optional OCR
- Index text inside diagrams and image-based content via OCR
- Preserve backward compatibility with all existing phases
- Enforce deterministic docId assignment and output ordering
- Limit CPU usage and avoid unbounded concurrency

## Non-Goals

Phase 2.5 SHALL NOT include:

- Semantic search
- Vector embeddings
- Distributed indexing
- Cloud processing
- AI visual interpretation
- Modifications to existing CLI flags, exit codes, or persistence formats
- Changes to `/search` or `/health` contracts

## Terminology

- **Text layer**: Native text embedded in a PDF (extracted by pdftotext or equivalent)
- **OCR**: Optical character recognition applied to rendered page images
- **Canonical text**: Final merged text (text layer + OCR) used for indexing
- **MIN_TEXT_CHARS**: Minimum character count below which OCR is triggered (e.g., 50)
- **MIN_TOKEN_COUNT**: Minimum token count below which OCR is triggered (e.g., 10)

---

## 1. Backward Compatibility

### 1.1 Non-Breaking Guarantee

**Requirement 1.1.1: No Behavioral Regressions**

Phase 2.5 SHALL NOT:

- Modify existing CLI flags
- Change exit codes (0, 2, 3 semantics remain unchanged)
- Change persistence file formats (index_meta.json, docs.jsonl, postings.bin)
- Change search scoring behavior
- Alter `/search` or `/health` request/response contracts

**Requirement 1.1.2: Test Compatibility**

All existing Phase 2.0–2.4 tests MUST pass without modification.

**Acceptance Criteria:**

- All Phase 2.0–2.4 tests pass unchanged
- No new required CLI flags; existing `--docs` and `--out` behavior preserved for current use cases
- Exit codes 0 (success), 2 (validation), 3 (runtime/operational) unchanged

---

## 2. Functional Requirements

### 2.1 Folder-Based Ingestion

**Requirement 2.1.1: Supported Content**

The indexer SHALL accept a folder (e.g., when `--docs` points to a directory) containing:

- `.txt` files
- `.pdf` files
- Mixed content (both in the same folder)

**Requirement 2.1.2: Traversal Order**

Folder traversal MUST be:

- Recursive (subdirectories included)
- Alphabetically sorted by normalized full path (UTF-8 byte order). The sort key SHALL be the normalized, absolute path of each file as a UTF-8 string; comparison SHALL use byte-order comparison of the UTF-8 encoding to ensure reproducibility across macOS, Linux, and CI regardless of locale.
- Deterministic: identical input MUST produce identical document order and docId assignment

**Requirement 2.1.3: Unsupported File Extensions**

Files with unsupported extensions SHALL be ignored deterministically. The system SHALL NOT fail when encountering unsupported files. Only `.txt` and `.pdf` are supported; all other files SHALL be skipped in a deterministic order (e.g., consistent with traversal order) without causing exit code 3.

**Acceptance Criteria:**

- Indexing a directory containing only `.txt`, only `.pdf`, or both produces a deterministic index
- Re-indexing the same folder produces identical docId ordering and search results
- Directories containing unsupported file types (e.g., .docx, .jpg) complete successfully; unsupported files are ignored

### 2.2 Text Files

**Requirement 2.2.1: Text File Handling**

For each `.txt` file:

- The entire file content SHALL be read as the document text
- One file SHALL produce one indexed document
- Existing behavior (encoding, line endings) SHALL remain unchanged

**Acceptance Criteria:**

- `.txt` files are indexed as single documents with full content
- No regression in existing text-file indexing behavior

### 2.3 PDF Page-Level Indexing

**Requirement 2.3.1: Per-Page Processing**

PDF ingestion SHALL operate per page. For each page:

1. Extract text-layer content (native PDF text)
2. Evaluate OCR necessity (see Section 3)
3. Perform OCR if required (render at fixed DPI, run OCR engine)
4. Merge content into canonical text (text layer + newline + OCR content if executed)
5. Index canonical text

**Requirement 2.3.2: One Document Per Page**

Each PDF page SHALL produce exactly one indexed document.

**Requirement 2.3.3: OCR Failure Isolation**

OCR failure on a single page SHALL NOT terminate entire indexing. The implementation SHALL continue with remaining pages (e.g., use text layer only for that page or skip OCR for that page).

**Requirement 2.3.4: Large File / Parse Failure Mode**

If a PDF file cannot be opened or parsed (e.g., corrupted, password-protected, or invalid), the system SHALL log a deterministic error (e.g., to stderr, with file path and reason) and continue indexing remaining files. The system SHALL NOT exit with code 3 for a single unreadable PDF unless the failure prevents index persistence (e.g., inability to write the index). Remaining files in the folder SHALL be processed; the failed file SHALL be skipped.

**Acceptance Criteria:**

- Text inside diagrams and image-based content is searchable when OCR runs
- Single-page OCR failure does not cause full indexing run to fail with exit code 3
- A single unreadable or unparseable PDF does not abort the entire indexing run; other files are indexed

---

## 3. Deterministic OCR Policy

**Requirement 3.1: No User OCR Toggle**

There SHALL be no CLI or configuration toggle to enable/disable OCR. OCR execution SHALL be determined solely by the policy below.

**Requirement 3.2: OCR Trigger Conditions**

OCR SHALL execute for a page if either:

- Extracted text length < `MIN_TEXT_CHARS` (e.g., 50), OR
- Token count < `MIN_TOKEN_COUNT` (e.g., 10)

**Requirement 3.2.1: Token Count Definition**

Token count SHALL be computed using the same tokenizer used by the indexing pipeline (e.g., the same tokenizer that produces terms for the inverted index). This ensures that OCR trigger decisions are deterministic and consistent with the rest of the system; implementations SHALL NOT use a different tokenization scheme (e.g., ad-hoc whitespace splitting) for MIN_TOKEN_COUNT.

**Requirement 3.3: OCR Execution Parameters**

When OCR runs, it SHALL:

- Render the page at a fixed DPI (e.g., 300)
- Use a fixed language (e.g., English default)
- Operate per page (no cross-page OCR batching that would affect determinism)
- Not terminate indexing on single-page failure (see 2.3.3)

**Requirement 3.4: OCR Determinism**

OCR configuration SHALL be fixed and deterministic for identical input. No adaptive or random OCR configuration SHALL be used. For the same page image (or same PDF page), the OCR engine SHALL produce the same output across runs (e.g., fixed thread count, fixed seed if any, no adaptive models that change behavior per run). This prevents OCR engine variability (e.g., internal multi-threading or adaptive models) from breaking docId or content determinism.

**Acceptance Criteria:**

- Pages with little or no text layer get OCR; pages with sufficient text may skip OCR
- Same PDF always yields same OCR decisions (deterministic)
- Fixed DPI and language ensure reproducible results
- Same input always yields same OCR output (no adaptive/random OCR config)

---

## 4. Canonical Indexed Text

**Requirement 4.1: Merge Format**

Final indexed content for a page SHALL be:

```
text_layer_content
+ newline
+ ocr_content (if OCR was executed; otherwise empty)
```

Trailing/leading whitespace handling (e.g., trimming) MAY be implementation-defined but MUST be deterministic.

**Requirement 4.2: Mixed OCR + Text Duplication**

The system MAY allow duplicated content between text-layer and OCR content. If the same or similar text appears in both the native text layer and the OCR output, both MAY be included in the canonical text. Deduplication is not required in Phase 2.5. This avoids fuzzy deduplication logic that could complicate determinism and reproducibility.

**Acceptance Criteria:**

- Search matches against both text-layer and OCR-derived content
- Order is always: text layer first, then newline, then OCR content
- Duplicate or overlapping text from text layer and OCR is acceptable; no deduplication required

---

## 5. Metadata Requirements

**Requirement 5.1: Stored Metadata**

Each indexed document SHALL store (at least conceptually; persistence format may extend existing schema):

- `docId`
- `source_path` (full path to the source file)
- `file_name` (base name of the source file)
- `file_type` (e.g., "txt" or "pdf")
- `page_number` (for PDFs: 1-based page index; for .txt files SHALL be 1)
- `text` (canonical indexed text)
- `did_ocr` (boolean: whether OCR was run for this document)

**Requirement 5.2: Search Response Metadata**

Search responses SHALL include, for each hit, at least:

- `file_name`
- `page_number`
- `score`
- `snippet`

Existing fields (e.g., `docId`) MAY remain; new fields SHALL be additive and SHALL NOT remove or break existing response fields.

**Requirement 5.3: Page Numbering for .txt**

For `.txt` files, `page_number` SHALL be 1. (PDFs use 1-based page numbers per page.)

**Requirement 5.4: Metadata Persistence Compatibility**

If new metadata fields are introduced (e.g., `source_path`, `file_name`, `file_type`, `page_number`, `did_ocr`), they SHALL be additive and optional. Older index loaders (e.g., from Phase 2.0–2.4) SHALL continue to function: they MAY ignore unknown fields and SHALL not fail when loading an index that contains Phase 2.5 metadata. Backward compatibility of the on-disk format SHALL be preserved.

**Acceptance Criteria:**

- Search results display file name and page number
- Clients can distinguish OCR-derived content via metadata if needed
- Indexes written with Phase 2.5 metadata can be loaded by older loaders without failure

---

## 6. Deterministic docId Assignment

**Requirement 6.1: Assignment Rules**

- Files SHALL be processed in alphabetical order by normalized full path (UTF-8 byte order); see Requirement 2.1.2
- Within a PDF, pages SHALL be processed in numeric order (1, 2, 3, …)
- docIds SHALL be assigned via an atomic counter (or equivalent deterministic sequence)
- Output SHALL be sorted before persistence (if applicable)

**Requirement 6.2: Reproducibility**

Identical input (same folder, same files, same content) MUST produce identical output (same docIds, same ordering, same index content).

**Requirement 6.3: Concurrency and Determinism**

Even when pages are processed in parallel, docId assignment SHALL reflect the deterministic logical order (e.g., file order then page order), not the completion order of threads. Implementations SHALL ensure that docIds are assigned as if processing were strictly sequential (e.g., via pre-assignment by logical index or by committing results in order).

**Acceptance Criteria:**

- Re-running index on the same folder produces identical docId ordering
- No flaky or non-deterministic docId assignment
- Parallel execution does not change docId assignment compared to sequential processing

---

## 7. Performance and CPU Safety

### 7.1 Concurrency

**Requirement 7.1.1: Worker Pool**

- Worker threads for general indexing SHALL be limited to `max(1, hardware_concurrency() / 2)` or equivalent
- A dedicated, smaller OCR pool (e.g., 2 threads) SHALL be used for OCR work
- There SHALL be no unbounded async execution (queue sizes or thread counts SHALL be bounded)

**Requirement 7.1.2: Bounded Queues**

Processing queues (e.g., page queue, OCR queue) SHALL be bounded to avoid unbounded memory growth.

### 7.2 Streaming and Memory

**Requirement 7.2.1: Page-Level Processing**

- Processing SHALL be page-level (or file-level for .txt)
- Immediate indexing after processing a page/file SHALL be allowed (no requirement to hold full PDF in memory)
- Full-PDF memory retention SHALL be avoided where possible (e.g., stream or process page-by-page)

**Requirement 7.2.2: Extremely Large PDFs (10k+ pages)**

The system SHALL NOT load all page renderings simultaneously. Page renderings (e.g., for OCR) SHALL be produced and consumed in a bounded, streaming fashion (e.g., render and OCR one page at a time or a small bounded batch). This applies to both normal and extremely large PDFs (e.g., 10k+ pages) to avoid unbounded memory use.

### 7.3 CPU Protection

**Requirement 7.3.1: Saturation Avoidance**

- The implementation SHALL avoid full-core saturation (e.g., via limited worker count and OCR pool size)
- Fixed DPI SHALL be used for OCR (no adaptive DPI that could spike CPU)
- Processing queues SHALL be bounded (see 7.1.2)

**Requirement 7.3.2: Multi-Thread Shutdown Safety**

Indexing threads SHALL join cleanly before process exit. The implementation SHALL ensure that all worker threads (general indexing and OCR pool) are joined or otherwise terminated in a controlled way before the process exits (e.g., on success, on fatal error, or on signal). Threads SHALL NOT be left running or detached in a way that could leak resources or cause undefined behavior on exit.

**Requirement 7.3.3: Thread Pool Shutdown During Signal (Phase 2.3 Integration)**

If SIGINT or SIGTERM is received during indexing (per Phase 2.3 graceful shutdown), the system SHALL complete in-flight page processing (or drain the processing queue in a bounded way), join all worker threads, and then exit with a deterministic exit code: 0 if indexing completed successfully before exit, or 3 if a fatal error occurred. The system SHALL NOT accept new files or pages for processing after signal receipt. This integrates Phase 2.5 with Phase 2.3 so that graceful shutdown applies to the indexing process as well as the server; no threads SHALL be abandoned on signal.

**Requirement 7.3.4: macOS CPU Thermal Protection (SHOULD)**

The implementation SHOULD avoid sustained 100% CPU across all cores for extended durations. This supports macOS (and similar) thermal protection and avoids throttling or user-visible system slowdown. Bounded worker counts, OCR pool size, and fixed DPI already contribute; implementations MAY add additional throttling or yielding if needed.

**Acceptance Criteria:**

- 1000-page PDFs do not saturate CPU uncontrollably
- Memory usage remains bounded during large PDF ingestion
- Process exit does not leak threads; all indexing threads join before exit

---

## 8. Exit Codes

**Requirement 8.1: Exit Code Semantics**

Exit codes SHALL remain unchanged from Phase 2.2/2.3/2.4:

- **0** → Success
- **2** → CLI validation error (e.g., invalid flags, missing required args)
- **3** → Fatal indexing error (e.g., unable to read folder, persistence failure)

**Requirement 8.2: OCR and Exit Code**

OCR failure on individual pages SHALL NOT by itself cause exit code 3. Only fatal errors (e.g., failure to open folder, failure to write index) SHALL result in exit code 3.

**Requirement 8.3: Partial Index Write on Failure**

If indexing fails after partially processing files (e.g., crash, fatal error, or signal), no partially-written index SHALL be left in an inconsistent state. Atomic persistence guarantees from Phase 2.1 SHALL remain intact: the implementation SHALL use write-to-temp-then-rename (or equivalent) so that the on-disk index is either the previous complete index or a new complete index, never a partial or corrupted state. Parallel indexing increases crash risk; atomic rename guarantees SHALL be preserved so that a reader never sees a half-written index.

**Acceptance Criteria:**

- Exit code 0 on successful indexing even if some pages had OCR failures
- Exit code 3 only on fatal, run-level failures
- Mid-indexing failure does not leave the output directory in an inconsistent state; Phase 2.1 atomic persistence applies

---

## 9. Acceptance Criteria (Summary)

1. Text inside diagrams and image-based content is searchable.
2. All existing Phase 2.0–2.4 tests pass without regression.
3. Large PDFs (e.g., 1000-page) do not saturate CPU uncontrollably.
4. docId ordering remains deterministic for the same input.
5. Search results display file name and page number (and score, snippet as today).

---

## 10. Architecture Overview

```
Folder Scanner (recursive, sorted)
        ↓
File Dispatcher
        ↓
PDF Page Processor (per page)
        ↓
Worker Thread Pool (max 1, hardware_concurrency()/2)
        ↓
OCR Pool (conditional, e.g. 2 threads)
        ↓
SearchService::add_document() / index merge
        ↓
Persistence (existing format, extended metadata if required)
```

---

## 11. Non-Goals (Recap)

Phase 2.5 SHALL NOT include:

- Semantic search
- Vector embeddings
- Distributed indexing
- Cloud processing
- AI visual interpretation
- Changes to existing CLI flags, exit codes, or persistence schema that break Phase 2.0–2.4

---

**End of Phase 2.5 Specification**
