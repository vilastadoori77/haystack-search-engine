
## Spec Status
**Status** Approved
**Approval Date:** 2026-01-13
**Approved By:** Spec Review (VT) 

**Notes:**
- Specification is complete, internally consistent, and testable.
- Exit code semantics for serve mode are deterministic and CI-safe.
- Approved for test generation and TDD implementation.



# Phase 2.2: CLI Lifecycle Specification

## Overview

This specification defines the command-line interface (CLI) lifecycle for the `searchd` executable. The CLI SHALL support two mutually exclusive operational modes: index-building mode and serve mode. The specification defines flag validation, exit codes, error handling, and deterministic startup behavior.

## Goals

1. **Command mode exclusivity**: The program SHALL operate in exactly one mode per invocation.
2. **Index mode**: Build and persist index without starting HTTP server.
3. **Serve mode**: Load persisted index and start HTTP server without mutating index files.
4. **Fail-fast validation**: Validate all inputs before performing heavy operations.
5. **Clear error messaging**: Provide human-readable, specific error messages to stderr.
6. **Deterministic exit codes**: Use consistent exit codes to indicate success or failure type.
7. **Help and usage**: Provide usage information via `--help` flag or when invoked with no arguments.

## Non-Goals

- Daemonization or background process management
- Signal handling (SIGTERM, SIGHUP, etc.)
- Hot reload or index updates during serve mode
- Windows service support
- Interactive mode or REPL
- Configuration file support
- Multiple index directories
- Index merging or incremental updates

## CLI Modes

### Index Mode

**Command Pattern:**
```
searchd --index --docs <path> --out <index_dir>
```

**Behavior:**
- The program SHALL build the complete search index in memory from the documents file.
- The program SHALL persist the index to `<index_dir>` using the persistence mechanism defined in Phase 2.
- The program SHALL print a success message to stdout: `"Indexing completed. Index saved to: <index_dir>"`.
- The program SHALL exit immediately after saving (no HTTP server SHALL be started).
- The program SHALL NOT mutate any files outside of `<index_dir>`.

**Required Flags:**
- `--index`: Enables index mode
- `--docs <path>`: Path to source documents file (JSON format)
- `--out <index_dir>`: Directory where index SHALL be written

**Optional Flags:**
- None in index mode

### Serve Mode

**Command Pattern:**
```
searchd --serve --in <index_dir> --port <port>
```

**Behavior:**
- The program SHALL load the index from `<index_dir>` using the persistence mechanism defined in Phase 2.
- The program SHALL start an HTTP server listening on the specified port.
- The program SHALL register HTTP endpoints (e.g., `/health`, `/search`).
- The program SHALL NOT mutate any files in `<index_dir>` during serve mode.
- The program SHALL run until interrupted (no automatic exit).

**Required Flags:**
- `--serve`: Enables serve mode
- `--in <index_dir>`: Directory containing persisted index files
- `--port <port>`: TCP port number for HTTP server

**Optional Flags:**
- None in serve mode

## Flag Matrix

| Flag | Index Mode | Serve Mode | Mutually Exclusive With |
|------|------------|------------|------------------------|
| `--index` | Required | Invalid | `--serve` |
| `--serve` | Invalid | Required | `--index` |
| `--docs <path>` | Required | Invalid | N/A |
| `--out <index_dir>` | Required | Invalid | N/A |
| `--in <index_dir>` | Invalid | Required | N/A |
| `--port <port>` | Invalid | Required | N/A |
| `--help` | Valid | Valid | None |

**Rules:**
- Supplying both `--index` and `--serve` SHALL cause immediate failure with exit code 2.
- Supplying `--index` with `--in` or `--port` SHALL cause failure with exit code 2.
- Supplying `--serve` with `--docs` or `--out` SHALL cause failure with exit code 2.
- Supplying `--help` with any other flag SHALL print help and exit with code 0.

## Exit Codes

The program SHALL use the following exit codes:

| Code | Meaning | When Used |
|------|---------|-----------|
| 0 | Success | Index mode: successful save<br>Serve mode: successful startup (see below)<br>Help: printed successfully |
| 2 | CLI misuse | Missing required flags<br>Conflicting flags (`--index` + `--serve`)<br>Invalid flag combinations<br>Invalid flag values (non-numeric port) |
| 3 | Operational failure | Index mode: document file not found<br>Index mode: indexing/save failure<br>Serve mode: index directory not found<br>Serve mode: index load failure<br>Serve mode: HTTP server startup failure |

### Serve Mode Exit Code Semantics (Clarification)

Exit code `0` in serve mode SHALL only be returned after all of the following conditions are met:

- The persisted index is successfully loaded from disk
- The HTTP server successfully binds to the requested port

If either condition fails, the process SHALL exit with exit code `3` and print a corresponding error message to stderr.

This prevents false-positive success signals in automation and CI environments.

**Exit Code Contract:**
- Exit code 0 SHALL indicate successful completion of the requested operation.
- Exit code 2 SHALL indicate user error (invalid CLI usage, missing arguments, flag conflicts).
- Exit code 3 SHALL indicate system/operational error (I/O failures, parse errors, corruption).

## Validation Rules

### Pre-Validation Order

Validation SHALL occur in this order (fail-fast):

1. **Flag exclusivity**: Check for conflicting flags (`--index` + `--serve`)
2. **Required flags**: Check that all required flags for the selected mode are present
3. **Flag values**: Validate flag argument types (port is numeric, paths are non-empty)
4. **Path existence**: Verify that input paths exist and are accessible
5. **Index directory validation**: (Serve mode only) Verify index directory contains required files

### Index Mode Validation

The program SHALL validate:

- `--index` flag is present
- `--serve` flag is NOT present
- `--docs <path>` is provided and non-empty
- `--out <index_dir>` is provided and non-empty
- `--in` and `--port` flags are NOT present
- `<path>` exists and is readable (file)
- `<index_dir>` parent directory exists and is writable (directory may not exist yet)

**Validation Failure Behavior:**
- Missing `--out`: Print `"Error: --out <index_dir> is required when using --index mode\n"` to stderr, exit 2
- Missing `--docs`: Print `"Error: --docs <path> is required when using --index mode\n"` to stderr, exit 2
- Conflicting flags: Print `"Error: --index and --serve cannot be used together\n"` to stderr, exit 2
- Invalid flag combinations: Print appropriate error to stderr, exit 2

### Serve Mode Validation

The program SHALL validate:

- `--serve` flag is present
- `--index` flag is NOT present
- `--in <index_dir>` is provided and non-empty
- `--port <port>` is provided and numeric
- `--docs` and `--out` flags are NOT present
- `<index_dir>` exists and is a directory
- `<index_dir>` contains `index_meta.json`
- `<index_dir>` contains `docs.jsonl`
- `<index_dir>` contains `postings.bin`
- Port number is in valid range (1-65535)

**Validation Failure Behavior:**
- Missing `--in`: Print `"Error: --in <index_dir> is required when using --serve mode\n"` to stderr, exit 2
- Missing `--port`: Print `"Error: --port <port> is required when using --serve mode\n"` to stderr, exit 2
- Conflicting flags: Print `"Error: --index and --serve cannot be used together\n"` to stderr, exit 2
- Index directory missing: Print `"Error: Index directory not found: <index_dir>\n"` to stderr, exit 3
- Missing index files: Print `"Error: Index file not found: <file_path>\n"` to stderr, exit 3
- Invalid port: Print `"Error: Invalid port number: <port>\n"` to stderr, exit 2

## Error Messages

### Error Message Format

All error messages SHALL:
- Be printed to stderr (not stdout)
- Start with `"Error: "` prefix
- Be human-readable (no stack traces in normal CLI usage)
- Include the specific file, flag, or value that caused the error
- End with a newline character

### Error Message Catalog

**CLI Misuse (Exit Code 2):**
- `"Error: --out <index_dir> is required when using --index mode\n"`
- `"Error: --docs <path> is required when using --index mode\n"`
- `"Error: --in <index_dir> is required when using --serve mode\n"`
- `"Error: --port <port> is required when using --serve mode\n"`
- `"Error: --index and --serve cannot be used together\n"`
- `"Error: Invalid port number: <port>\n"`
- `"Error: --docs cannot be used with --serve mode\n"`
- `"Error: --in cannot be used with --index mode\n"`

**Operational Failure (Exit Code 3):**
- `"Error: Document file not found: <path>\n"`
- `"Error: Index directory not found: <index_dir>\n"`
- `"Error: Index file not found: <file_path>\n"`
- `"Error indexing/saving: <specific_error>\n"`
- `"Error loading index: <specific_error>\n"`
- `"Error: Failed to bind to port <port>: <error_message>\n"`

## Acceptance Criteria

- [ ] `searchd --index --docs <path> --out <index_dir>` builds index, saves it, prints success message, and exits with code 0
- [ ] `searchd --index --docs <path> --out <index_dir>` does NOT start HTTP server
- [ ] `searchd --serve --in <index_dir> --port <port>` loads index, starts HTTP server, and does NOT mutate index files
- [ ] `searchd --index --serve` exits with code 2 and prints error about conflicting flags
- [ ] `searchd --index` (missing `--out`) exits with code 2 and prints error about missing `--out`
- [ ] `searchd --index --docs <path>` (missing `--out`) exits with code 2 and prints error about missing `--out`
- [ ] `searchd --serve` (missing `--in`) exits with code 2 and prints error about missing `--in`
- [ ] `searchd --serve --in <index_dir>` (missing `--port`) exits with code 2 and prints error about missing `--port`
- [ ] `searchd --index --docs <nonexistent> --out <dir>` exits with code 3 and prints error about file not found
- [ ] `searchd --serve --in <nonexistent> --port 8900` exits with code 3 and prints error about directory not found
- [ ] `searchd --serve --in <dir> --port <port>` where `<dir>` lacks required files exits with code 3
- [ ] `searchd --index --docs <path> --out <dir> --in <dir2>` exits with code 2 (invalid flag combination)
- [ ] `searchd --serve --in <dir> --port <port> --docs <path>` exits with code 2 (invalid flag combination)
- [ ] `searchd --serve --in <dir> --port invalid` exits with code 2 (non-numeric port)
- [ ] `searchd --serve --in <dir> --port 0` exits with code 2 (invalid port range)
- [ ] `searchd --serve --in <dir> --port 70000` exits with code 2 (invalid port range)
- [ ] `searchd --help` prints usage information and exits with code 0
- [ ] `searchd` (no arguments) behaves like `--help`
- [ ] All error messages are printed to stderr
- [ ] Success messages (index mode) are printed to stdout
- [ ] Serve mode startup fails immediately if index is invalid (no partial startup)

## Test Plan

### Unit Tests (C++)

**File: `tests/test_cli_validation.cpp`**
- Test flag exclusivity: `--index` + `--serve` fails with exit code 2
- Test missing required flags in index mode
- Test missing required flags in serve mode
- Test invalid flag combinations (e.g., `--index` + `--in`)
- Test invalid port values (non-numeric, out of range)
- Test help flag behavior

### Integration Tests (CLI Invocation)

**File: `tests/test_cli_lifecycle.cpp`**
- Test index mode: successful execution, exit code 0, no server started
- Test serve mode: successful startup, exit code 0, server running
- Test index mode: missing `--out` flag, exit code 2
- Test index mode: missing `--docs` flag, exit code 2
- Test serve mode: missing `--in` flag, exit code 2
- Test serve mode: missing `--port` flag, exit code 2
- Test conflicting flags: `--index --serve`, exit code 2
- Test invalid combinations: `--index --in`, exit code 2
- Test invalid combinations: `--serve --docs`, exit code 2
- Test nonexistent document file, exit code 3
- Test nonexistent index directory, exit code 3
- Test incomplete index directory (missing files), exit code 3
- Test invalid port number, exit code 2
- Test port out of range, exit code 2
- Test `--help` flag prints usage and exits 0
- Test no arguments behaves like `--help`

**File: `tests/test_cli_error_messages.cpp`**
- Test error messages are printed to stderr (not stdout)
- Test error messages contain "Error: " prefix
- Test error messages include specific file/path/flag information
- Test error messages are human-readable (no stack traces)

**File: `tests/test_cli_exit_codes.cpp`**
- Test exit code 0 for successful index mode
- Test exit code 0 for successful serve mode startup
- Test exit code 2 for all CLI misuse scenarios
- Test exit code 3 for all operational failure scenarios

## Definition of Done

- [ ] `searchd` supports `--index` mode with required flags
- [ ] `searchd` supports `--serve` mode with required flags
- [ ] `--index` and `--serve` are mutually exclusive (exit code 2 if both provided)
- [ ] Index mode validates all required flags before processing
- [ ] Serve mode validates all required flags before processing
- [ ] Index mode validates path existence before processing
- [ ] Serve mode validates index directory and files before processing
- [ ] Exit codes follow the contract (0=success, 2=CLI misuse, 3=operational failure)
- [ ] All error messages printed to stderr with "Error: " prefix
- [ ] Error messages are specific and human-readable
- [ ] Success messages printed to stdout
- [ ] `--help` flag implemented and prints usage
- [ ] No arguments behaves like `--help`
- [ ] All acceptance criteria tests pass
- [ ] All unit tests in test plan pass
- [ ] All integration tests in test plan pass
- [ ] Code review completed
- [ ] Documentation updated (if applicable)

No open questions.
