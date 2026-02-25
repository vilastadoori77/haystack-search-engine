# Haystack Search Engine

## Phase 2.5 -- Deterministic PDF + OCR Ingestion Architecture

------------------------------------------------------------------------

## Overview

Phase 2.5 introduces deterministic PDF ingestion with conditional OCR
and page-level indexing, while preserving Phase 2.2--2.4 guarantees (CLI
lifecycle, runtime lifecycle, operational guarantees).

This phase ensures:

-   Deterministic indexing
-   Page-level document modeling
-   Conditional OCR routing
-   Canonical text merging
-   Bounded concurrency
-   Atomic persistence
-   No regression to earlier phases

------------------------------------------------------------------------

# High-Level Execution Flow

CLI (--index) ↓ Deterministic File Discovery ↓ Bounded Worker Pool ↓
File Processing Pipeline (TXT / PDF + OCR Decision) ↓ Page-Level
Document Builder ↓ Deterministic Aggregation + docId Assignment ↓ Atomic
Index Writer

------------------------------------------------------------------------

# 1. Deterministic File Discovery Layer

Responsibilities:

-   Recursive directory traversal
-   Absolute path normalization
-   UTF-8 byte-order lexicographic sorting
-   Filtering indexable files (.txt, .pdf)

Non-negotiable constraints:

-   docId MUST NOT be assigned here
-   Filesystem iteration order MUST NOT be relied upon
-   Sorting MUST use absolute normalized path

------------------------------------------------------------------------

# 2. Bounded Worker Pool

Responsibilities:

-   Fixed-size thread pool
-   Controlled task submission
-   Respect stop_requested flag (SIGINT / SIGTERM)
-   No docId assignment inside workers

Workers:

-   Extract text
-   Evaluate OCR decision
-   Build page-level document structures
-   Push results to synchronized collection

Completion order MUST NOT affect docId assignment.

------------------------------------------------------------------------

# 3. File Processing Pipeline

## TXT Processing

-   Extract full text
-   page_number = 1
-   Evaluate OCR thresholds

## PDF Processing

-   Extract text layer per page
-   Evaluate OCR threshold per page
-   If below threshold → perform OCR
-   Merge text canonically

------------------------------------------------------------------------

# 4. OCR Decision Engine

Inputs:

-   text_length
-   token_count

Decision rule:

IF text_length \< MIN_TEXT_CHARS OR token_count \< MIN_TOKEN_COUNT THEN
did_ocr = true ELSE did_ocr = false

Tokenization MUST match indexing tokenizer for determinism.

------------------------------------------------------------------------

# 5. Canonical Text Merge

If OCR occurs:

final_text = text_layer + "`\n`{=tex}" + ocr_text

If no OCR:

final_text = text_layer

Deterministic ordering is mandatory.

------------------------------------------------------------------------

# 6. Page-Level Document Model

Each indexed page produces:

Document { int docId; string file_name; string file_type; string
source_path; int page_number; string text; bool did_ocr; }

Rules:

-   One document per page
-   page_number starts at 1
-   docIds assigned AFTER aggregation

------------------------------------------------------------------------

# 7. Deterministic Aggregator

Steps:

1.  Collect all processed page-level documents
2.  Sort by:
    -   file_name (ascending)
    -   page_number (ascending)
3.  Assign docId sequentially from 1..N

docId MUST NOT depend on worker completion order.

------------------------------------------------------------------------

# 8. Atomic Index Writer

Process:

1.  Create temporary output directory
2.  Write:
    -   docs.jsonl
    -   postings.bin
    -   index_meta.json
3.  fsync
4.  Atomic rename to final output directory

Guarantees:

-   No .tmp or .partial files remain
-   Either full index exists or none
-   Exactly three index files produced

------------------------------------------------------------------------

# 9. Signal Handling Integration

Global atomic flag:

std::atomic`<bool>`{=html} stop_requested;

Workers check flag and exit safely.

On SIGINT / SIGTERM:

-   Abort processing
-   Clean temporary directory
-   Exit with code 0 or 3

------------------------------------------------------------------------

# Determinism Rules (Strict)

-   Sorted file traversal
-   Sorted page aggregation
-   Sequential docId assignment
-   Stable JSON field order
-   Stable newline handling
-   No timestamps
-   No random UUIDs
-   No unordered containers for final ordering

------------------------------------------------------------------------

# Compatibility Requirements

-   Must not break Phase 2.2 (CLI lifecycle)
-   Must not break Phase 2.3 (Runtime lifecycle)
-   Must not break Phase 2.4 (Operational guarantees)
-   Existing tests MUST continue passing

------------------------------------------------------------------------

# Result

Phase 2.5 provides deterministic, scalable, and robust PDF + OCR
ingestion, while preserving the architectural guarantees of the Haystack
Search Engine.
