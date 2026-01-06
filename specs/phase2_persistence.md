
# Phase 2: On-Disk Persistence Specification

## Status
APPROVED - 2026-01-06

## Overview

This specification defines the on-disk persistence layer for the Haystack search engine. The system SHALL persist the complete search index to disk, enabling query-ready operation after restart without re-indexing source documents. The persistence mechanism SHALL be crash-safe, deterministic, and maintain stable document identity across save/load cycles.

## Goals

1. **On-disk persistence**: The entire search index SHALL be persisted to a user-provided index directory.
2. **Load without re-indexing**: The system SHALL load a persisted index and become query-ready without re-reading or re-tokenizing original document sources.
3. **Crash-safe commits**: All writes SHALL use atomic file operations to prevent partial writes on interruption.
4. **Deterministic correctness**: After save then load, identical queries SHALL produce the same ordered docIds, scores within tolerance, and snippets containing query terms (when available).
5. **Stable document identity**: docIds SHALL remain stable across save/load cycles; the system SHALL never regenerate docIds on load.
6. **Index versioning**: The system SHALL persist schema version and fail fast with clear errors if an unsupported version is encountered.
7. **Operational CLI lifecycle**: The system SHALL support distinct index-building and serve modes with clear separation of concerns.

## Non-Goals

- Distributed indexing across multiple nodes
- Replication or backup strategies
- Cloud storage integration (S3, GCS, etc.)
- Multi-tenant partitioning or isolation
- Incremental updates or delta persistence
- Index compaction or optimization
- Concurrent read/write operations during serve mode

## Terminology

- **Index directory**: User-provided directory path containing persisted index files.
- **docId**: Externally owned document identifier (integer). Stable across save/load cycles.
- **Term frequency (tf)**: Number of times a term appears in a document.
- **Document frequency (df)**: Number of documents containing a term.
- **Postings list**: Mapping from term to list of (docId,tf) pairs.
- **BM25 corpus stats**: N (total document count) and avgdl (average document length).
- **Schema version**: Integer version number identifying the on-disk format.
- **Crash-safe**: Write operations that cannot leave partially-written final files if interrupted.

## On-disk Layout

The index directory SHALL be treated as an atomic unit.
Partial directories SHALL NOT be considered valid indexes.

The index directory SHALL contain exactly three files:

### `index_meta.json`

JSON metadata file containing:
- `schema_version` (integer): Format version (default: 1)
- `N` (integer): Total number of documents indexed
- `avgdl` (double): Average document length (token count)

Example:
```json
{
  "schema_version": 1,
  "N": 1000,
  "avgdl": 45.3
}
```

### `docs.jsonl`

JSONL (JSON Lines) file containing document texts. Each line SHALL be a JSON object with:
- `docId` (integer): Document identifier
- `text` (string): Original document text

Lines SHALL be ordered by docId (ascending). Each line SHALL be a complete, valid JSON object terminated by a newline.

Example:
```jsonl
{"docId": 1, "text": "The quick brown fox jumps over the lazy dog."}
{"docId": 2, "text": "PLM data migration: cleansing, mapping, validation."}
{"docId": 3, "text": "Schema validation checklist for integration projects."}
```

### `postings.bin`

Binary file containing the inverted index postings.

The file SHALL encode:
- All unique terms
- For each term, a postings list of  (docId, tf) pairs
- Term ordering SHALL be deterministic (lexicographic)

The exact binary encoding is an implementation detail but MUST be:
- deterministic
- forward-compatible with schema_version


## API Contracts

### SearchService

The `SearchService` class SHALL provide the following public methods:

```cpp
class SearchService {
public:
    // ... existing methods ...
    
    // Persist entire index to directory
    // Throws std::runtime_error on failure
    void save(const std::string& index_dir);
    
    // Load index from directory
    // Throws std::runtime_error on failure (missing files, unsupported version, etc.)
    void load(const std::string& index_dir);
};
```

### InvertedIndex

The `InvertedIndex` class SHALL provide the following public methods:

```cpp
class InvertedIndex {
public:
    // ... existing methods ...
    
    // Persist inverted index to binary file
    // Throws std::runtime_error on failure
    void save(const std::string& postings_path);
    
    // Load inverted index from binary file
    // Throws std::runtime_error on failure (file not found, format error, etc.)
    void load(const std::string& postings_path);
};
```

## Crash-Safe Commit Procedure

save() SHALL NOT be invoked concurrently with search() or load().
Serve mode SHALL be read-only.


All file writes SHALL follow this procedure:

1. **Write to temporary file**: Write complete file content to `<final_path>.tmp` in the same directory as the final file.
2. **Flush and sync**: Ensure all data is written to disk (flush output stream).
3. **Atomic rename**: Rename `<final_path>.tmp` to `<final_path>` using filesystem rename operation.
4. **Error handling**: If any step fails, the temporary file SHALL remain; the final file SHALL not exist in a partially-written state.

For the index directory commit:
1. Write `index_meta.json.tmp`, then rename to `index_meta.json`
2. Write `docs.jsonl.tmp`, then rename to `docs.jsonl`
3. Write `postings.bin.tmp`, then rename to `postings.bin`

If any step fails, previously written files SHALL remain valid; the implementation MAY clean up temporary files on error, but SHALL NOT leave partially-written final files.

## Determinism & Ordering Rules

### Document Ordering

After save then load, query results SHALL maintain deterministic ordering:
- Documents SHALL be ordered by BM25 score (descending), then by docId (ascending) for ties.
- Score ties SHALL be defined as absolute difference < 1e-9.

### Score Stability

BM25 scores SHALL be computed identically before and after save/load:
- Same term frequencies (tf) per document
- Same document frequencies (df) per term
- Same corpus statistics (N, avgdl)
- Same BM25 formula parameters (k1, b)

Scores SHALL match within tolerance: `abs(score_before - score_after) < 1e-9`.

### Snippet Generation

Snippets SHALL contain query terms when available in the document text. Snippet generation logic SHALL be deterministic and produce identical snippets for the same document and query before and after save/load.

### Document Identity

docIds SHALL be preserved exactly as stored. The system SHALL NOT regenerate, remap, or reassign docIds during load.

## Acceptance Criteria

- [ ] `SearchService::save(dir)` creates `index_meta.json`, `docs.jsonl`, and `postings.bin` in the specified directory
- [ ] `SearchService::load(dir)` loads all three files and restores the index to query-ready state
- [ ] After save then load, `search(query)` returns identical docIds in identical order
- [ ] After save then load, `search_scored(query)` returns identical scores within 1e-9 tolerance
- [ ] After save then load, `search_with_snippets(query)` returns identical snippets containing query terms
- [ ] docIds remain stable across save/load cycles (no remapping)
- [ ] Loading an index with `schema_version != 1` throws `std::runtime_error` with clear error message
- [ ] Loading from a directory missing any required file throws `std::runtime_error` with clear error message
- [ ] Interrupting `save()` during write does not leave partially-written final files (only `.tmp` files may exist)
- [ ] `searchd --index --docs <path> --out <index_dir>` builds index, persists it, and exits without starting HTTP server
- [ ] `searchd --serve --in <index_dir> --port <port>` loads index, starts HTTP server, and does not mutate index
- [ ] Index mode and serve mode are mutually exclusive (cannot specify both `--index` and `--serve` simultaneously)
- [ ] All three index files are written atomically (temp file then rename)
- [ ] Binary postings format is correctly serialized and deserialized (term order, docId/tf pairs)

## Test Plan

### Unit Tests

**File: `tests/test_persistence.cpp`**
- Test `InvertedIndex::save()` and `InvertedIndex::load()` round-trip
- Test binary postings format correctness (term order, encoding)
- Test `SearchService::save()` creates all three files
- Test `SearchService::load()` restores index state
- Test save/load preserves docIds exactly
- Test save/load preserves BM25 corpus stats (N, avgdl)
- Test save/load preserves document texts

**File: `tests/test_persistence_determinism.cpp`**
- Test query results order is identical before and after save/load
- Test BM25 scores match within tolerance (1e-9) after save/load
- Test snippets are identical after save/load
- Test multiple save/load cycles maintain correctness

**File: `tests/test_persistence_errors.cpp`**
- Test loading unsupported schema_version throws with clear error
- Test loading missing `index_meta.json` throws with clear error
- Test loading missing `docs.jsonl` throws with clear error
- Test loading missing `postings.bin` throws with clear error
- Test loading corrupted `index_meta.json` throws with clear error
- Test loading corrupted `postings.bin` throws with clear error

**File: `tests/test_persistence_crash_safety.cpp`**
- Test that interrupting `save()` leaves only `.tmp` files (no partial final files)
- Test that existing index files are not corrupted if save fails partway through
- Test that load() does not see partially-written files

### Integration Tests

**File: `tests/test_cli_persistence.cpp`**
- Test `searchd --index --docs <path> --out <index_dir>` creates index files and exits
- Test `searchd --serve --in <index_dir> --port <port>` loads index and serves queries
- Test index mode does not start HTTP server
- Test serve mode loads index without re-reading source docs
- Test serve mode does not mutate index directory

## Failure Modes & Errors

### Unsupported Schema Version

**Condition**: `index_meta.json` contains `schema_version` != 1

**Behavior**: `SearchService::load()` SHALL throw `std::runtime_error` with message:
```
"Unsupported schema version: <version>. Expected: 1"
```

### Missing Files

**Condition**: Index directory missing `index_meta.json`, `docs.jsonl`, or `postings.bin`

**Behavior**: `SearchService::load()` SHALL throw `std::runtime_error` with message:
```
"Index file not found: <file_path>"
```

### Corrupted Files

**Condition**: File exists but contains invalid data (malformed JSON, invalid binary format, etc.)

**Behavior**: `SearchService::load()` or `InvertedIndex::load()` SHALL throw `std::runtime_error` with message:
```
"Failed to parse index file: <file_path>: <specific_error>"
```

### Directory Creation Failure

**Condition**: `save()` cannot create index directory

**Behavior**: `SearchService::save()` SHALL throw `std::runtime_error` with message:
```
"Failed to create directory: <dir_path>: <error_message>"
```

### Write Failure

**Condition**: Cannot write to disk (permissions, disk full, etc.)

**Behavior**: `save()` methods SHALL throw `std::runtime_error` with message:
```
"Failed to write index file: <file_path>: <error_message>"
```

### Inconsistent State

**Condition**: Files exist but contain inconsistent data (e.g., `docs.jsonl` has docId not in postings)

**Behavior**: `load()` MAY detect and throw `std::runtime_error`, or MAY proceed with available data (implementation choice). If error is thrown, message SHALL be:
```
"Inconsistent index state: <description>"
```

## Definition of Done

- [ ] `SearchService::save(dir)` and `SearchService::load(dir)` methods implemented
- [ ] `InvertedIndex::save(path)` and `InvertedIndex::load(path)` methods implemented
- [ ] All three index files (`index_meta.json`, `docs.jsonl`, `postings.bin`) are written/read correctly
- [ ] Crash-safe commit procedure implemented (temp files + atomic rename)
- [ ] CLI supports `--index --docs <path> --out <index_dir>` mode
- [ ] CLI supports `--serve --in <index_dir> --port <port>` mode
- [ ] Index mode and serve mode are mutually exclusive
- [ ] Schema version checking implemented with clear error messages
- [ ] All acceptance criteria tests pass
- [ ] All unit tests in test plan pass
- [ ] All integration tests in test plan pass
- [ ] Error handling covers all failure modes with clear messages
- [ ] Documentation updated (if applicable)
- [ ] Code review completed
- [ ] No memory leaks or undefined behavior (valgrind/sanitizers clean)

No open questions.
