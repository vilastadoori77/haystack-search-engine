# Phase 2.5 --- Deterministic PDF + OCR Ingestion (Non-Breaking Extension)

## 1. Purpose

Phase 2.5 extends the indexing engine to support:

-   Deterministic ingestion of `.txt` and `.pdf` files
-   Automatic OCR for image-based text inside PDFs
-   Mixed folder ingestion
-   CPU-safe parallel indexing on macOS

This phase is strictly additive and MUST NOT introduce regressions to:

-   Phase 2.0 --- Core indexing
-   Phase 2.1 --- Persistence
-   Phase 2.2 --- CLI lifecycle
-   Phase 2.3 --- Runtime lifecycle
-   Phase 2.4 --- Operational guarantees

------------------------------------------------------------------------

## 2. Backward Compatibility

### 2.1 Non-Breaking Guarantee

Phase 2.5 SHALL NOT:

-   Modify existing CLI flags
-   Change exit codes
-   Change persistence file formats
-   Change search scoring behavior
-   Alter `/search` or `/health` contracts

All existing Phase 2.0--2.4 tests MUST pass without modification.

------------------------------------------------------------------------

## 3. Functional Requirements

### 3.1 Folder-Based Ingestion

The indexer SHALL accept a folder containing:

-   `.txt`
-   `.pdf`
-   Mixed content

Traversal MUST be:

-   Recursive
-   Alphabetically sorted
-   Deterministic

------------------------------------------------------------------------

### 3.2 Text Files

For `.txt` files:

-   Entire file SHALL be read.
-   One file SHALL produce one indexed document.
-   Existing behavior SHALL remain unchanged.

------------------------------------------------------------------------

### 3.3 PDF Page-Level Indexing

PDF ingestion SHALL operate per page.

For each page:

1.  Extract text-layer content.
2.  Evaluate OCR necessity.
3.  Perform OCR if required.
4.  Merge content.
5.  Index canonical text.

Each page SHALL produce exactly one indexed document.

------------------------------------------------------------------------

## 4. Deterministic OCR Policy

There SHALL be no OCR toggle.

OCR SHALL execute if:

-   Extracted text length \< `MIN_TEXT_CHARS` (e.g., 50), OR
-   Token count \< `MIN_TOKEN_COUNT` (e.g., 10)

OCR SHALL:

-   Render at fixed DPI (e.g., 300)
-   Use fixed language (English default)
-   Operate per page
-   Not terminate indexing on single-page failure

------------------------------------------------------------------------

## 5. Canonical Indexed Text

Final indexed content:

    text_layer_content
    + newline
    + ocr_content (if executed)

------------------------------------------------------------------------

## 6. Metadata Requirements

Each indexed document SHALL store:

-   `docId`
-   `source_path`
-   `file_name`
-   `file_type`
-   `page_number`
-   `text`
-   `did_ocr`

Search responses SHALL include:

-   `file_name`
-   `page_number`
-   `score`
-   `snippet`

------------------------------------------------------------------------

## 7. Deterministic docId Assignment

-   Files sorted alphabetically
-   Pages processed in numeric order
-   docIds assigned via atomic counter
-   Output sorted before persistence

Identical input MUST produce identical output.

------------------------------------------------------------------------

## 8. Performance and CPU Safety

### 8.1 Concurrency

-   Worker threads = `max(1, hardware_concurrency() / 2)`
-   Dedicated smaller OCR pool (e.g., 2 threads)
-   No unbounded async execution

### 8.2 Streaming

-   Page-level processing
-   Immediate indexing after processing
-   No full-PDF memory retention

### 8.3 CPU Protection

-   Avoid full-core saturation
-   Fixed DPI
-   Bounded processing queues

------------------------------------------------------------------------

## 9. Exit Codes

-   `0` → Success
-   `2` → CLI validation error
-   `3` → Fatal indexing error

OCR failure on individual pages SHALL NOT terminate entire indexing.

------------------------------------------------------------------------

## 10. Acceptance Criteria

1.  Text inside diagrams is searchable.
2.  Existing phases pass without regression.
3.  1000-page PDFs do not saturate CPU uncontrollably.
4.  docId ordering remains deterministic.
5.  Search results display file name and page number.

------------------------------------------------------------------------

## 11. Architecture Overview

    Folder Scanner
          ↓
    File Dispatcher
          ↓
    PDF Page Processor
          ↓
    Worker Thread Pool
          ↓
    OCR Pool (conditional)
          ↓
    SearchService::add_document()
          ↓
    Persistence

------------------------------------------------------------------------

## 12. Non-Goals

Phase 2.5 SHALL NOT include:

-   Semantic search
-   Vector embeddings
-   Distributed indexing
-   Cloud processing
-   AI visual interpretation

------------------------------------------------------------------------

**End of Phase 2.5 Requirements**
