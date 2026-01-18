# Phase 2.3: Runtime Lifecycle & Robustness

## Spec Status

**Status:** Approved  
**Version:** 1.0  
**Date:** 2025-01-17  
**Dependencies:** Phase 2.2 (CLI Lifecycle) - COMPLETE  





## Overview

Phase 2.3 defines runtime correctness and lifecycle guarantees that apply AFTER CLI validation succeeds. This phase strengthens robustness, ensures deterministic failure handling, and guarantees clean startup and shutdown behavior.

Phase 2.3 does NOT modify any Phase 2.2 behavior:
- No changes to CLI flags, arguments, or validation
- No changes to exit codes (0, 2, 3)
- No changes to Phase 2.2 error messages
- No changes to indexing or search logic

## Goals

- Ensure deterministic startup and shutdown behavior in serve mode
- Guarantee that port binding failures are detected before server startup
- Provide clear, single-error reporting for runtime failures
- Enforce clean signal handling and graceful shutdown
- Maintain strict separation between stdout (success/info) and stderr (errors)

## Non-Goals

- New CLI flags or arguments
- Changes to Phase 2.2 exit codes or error messages
- Changes to indexing or search algorithm logic
- Configuration files
- Authentication, metrics, or logging frameworks
- Frontend or UI changes
- New API endpoints or HTTP behavior

## Terminology

- **Runtime failure**: Any failure that occurs after CLI validation succeeds
- **Startup sequence**: The sequence of operations from CLI validation completion until the HTTP server is ready to accept connections
- **Clean shutdown**: Termination in response to SIGINT or SIGTERM without error output
- **Port binding**: The successful reservation of a network port for the HTTP server

## Serve Mode Startup Guarantees

### Port Binding Guarantee

**Requirement 1.1: Port Binding Must Succeed Before Startup**

When running in `--serve` mode, the process SHALL:

1. Complete all CLI validation (per Phase 2.2)
2. Load the index from disk (if successful)
3. Attempt to bind the requested port BEFORE starting the HTTP server
4. If port binding fails (e.g., port already in use, permission denied, invalid port):
   - Print an error message to stderr: `"Error: Failed to bind to port <port>: <error_message>\n"`
   - Exit immediately with exit code 3
   - The HTTP server MUST NOT start
   - No startup message SHALL be printed to stdout

**Note:** Port binding MAY be performed implicitly by the HTTP framework during server startup. Regardless of implementation approach, failure to bind the requested port MUST be detected, reported to stderr, cause exit code 3, and occur BEFORE any startup success message is printed and BEFORE the server accepts any connections.

**Acceptance Criteria:**
- If port 8900 is already in use, the process exits with code 3 and prints an error to stderr
- If port binding fails due to permission denied, the process exits with code 3 and prints an error to stderr
- The HTTP server never starts if port binding fails

### Successful Startup Signal

**Requirement 1.2: Single Startup Message**

After both index loading AND port binding succeed, the process SHALL:

1. Print exactly ONE startup message to stdout
2. The message SHALL include:
   - The port number that was bound
   - The index directory path that was loaded
3. The message format SHALL be: `"Server started on port <port> using index: <index_dir>\n"`
4. Only after printing this message MAY the process begin accepting HTTP connections

**Acceptance Criteria:**
- Successful serve mode startup prints exactly one line to stdout
- The message contains both the port number and index directory
- No startup message is printed if port binding fails
- No startup message is printed if index loading fails

## Graceful Shutdown

### Signal Handling

**Requirement 2.1: Signal Response**

When running in serve mode and receiving SIGINT or SIGTERM, the process SHALL:

1. Immediately stop accepting new HTTP connections
2. Allow existing connections to complete (if possible within framework capabilities)
3. Shut down the HTTP server cleanly
4. Exit with exit code 0

**Note:** SIGINT and SIGTERM MUST be handled explicitly to ensure clean shutdown, exit code 0, and no stderr output.

**Acceptance Criteria:**
- Sending SIGINT (Ctrl+C) to the running server causes clean shutdown with exit code 0
- Sending SIGTERM to the running server causes clean shutdown with exit code 0
- No stack traces or fatal errors appear during shutdown

### Clean Shutdown Output

**Requirement 2.2: No Error Output on Clean Shutdown**

On clean shutdown (via SIGINT or SIGTERM):

- stderr SHALL remain empty (no error messages)
- No stack traces or fatal errors SHALL be printed
- Exit code SHALL be 0

**Acceptance Criteria:**
- Clean shutdown produces no output to stderr
- Clean shutdown does not trigger any error handlers
- Exit code is 0 on clean shutdown

## Index Load Runtime Validation

### Index Validation

**Requirement 3.1: Runtime Index Validation**

When loading an index in serve mode (after CLI validation succeeds), the process SHALL:

1. Verify that all required index files exist (per Phase 2.2 validation)
2. Verify that the index schema version is compatible
3. If schema version is unsupported:
   - Print an error to stderr: `"Error loading index: Unsupported schema version: <version>\n"`
   - Exit with code 3
   - The server MUST NOT start
4. If index file corruption is detected during load:
   - Print an error to stderr: `"Error loading index: <specific_error>\n"`
   - Exit with code 3
   - The server MUST NOT start

**Note:** Basic file existence checks are handled in Phase 2.2. Phase 2.3 handles schema version compatibility and parsing errors during actual load.

**Acceptance Criteria:**
- If `index_meta.json` contains an unsupported schema version, the process exits with code 3
- If `postings.bin` is corrupted, the process exits with code 3 with a descriptive error
- If `docs.jsonl` is malformed, the process exits with code 3 with a descriptive error
- No server startup occurs if index load validation fails

## Deterministic Failure Ordering

### Single Root Cause Rule

**Requirement 4.1: First Failure Wins**

If multiple runtime failures could occur, the process SHALL:

1. Report only the FIRST fatal error encountered
2. Exit immediately after the first fatal error
3. Not attempt operations that depend on the failed operation

**Error Ordering:**
1. Index load validation (schema version, corruption)
2. Port binding
3. Server startup (if applicable)

**Examples:**
- If index load fails → do not attempt port binding → exit immediately with code 3
- If port binding fails → do not start server threads → exit immediately with code 3
- If both index load and port binding could fail, only the first failure is reported

**Acceptance Criteria:**
- If index load fails, no port binding attempt is made
- If port binding fails, no server startup occurs
- Only one error message appears in stderr for any startup failure
- Exit code is 3 for any runtime failure

## Output Discipline

### stdout vs stderr

**Requirement 5.1: Output Stream Separation**

The process SHALL maintain strict separation between stdout and stderr:

**stdout (Success/Information):**
- Successful startup message (serve mode only)
- Help/usage information (Phase 2.2)
- Indexing completion message (Phase 2.2 index mode)

**stderr (Errors Only):**
- ALL runtime errors
- ALL validation errors (Phase 2.2)
- ALL operational failures

**Prohibited:**
- No error messages SHALL appear on stdout
- No success messages SHALL appear on stderr

**Acceptance Criteria:**
- Runtime errors appear only on stderr
- Startup success messages appear only on stdout
- No mixed output streams (errors on stdout or success on stderr)

## Exit Code Contract

**Requirement 6.1: Exit Code Semantics (Unchanged from Phase 2.2)**

Exit codes SHALL remain unchanged from Phase 2.2:

- **0**: Success or clean shutdown
  - Successful index mode completion
  - Successful serve mode startup
  - Clean shutdown via SIGINT/SIGTERM

- **2**: CLI misuse / validation failure (Phase 2.2 only)
  - Invalid flags or arguments
  - Missing required flags
  - Conflicting flags
  - Invalid port format

- **3**: Runtime or operational failure
  - Index directory not found (Phase 2.2)
  - Index file not found (Phase 2.2)
  - Index load failure (schema version, corruption)
  - Port binding failure
  - Document file not found (Phase 2.2)
  - Indexing/saving failure (Phase 2.2)

**Acceptance Criteria:**
- Clean shutdown (SIGINT/SIGTERM) returns exit code 0
- Port binding failure returns exit code 3
- Index load validation failure returns exit code 3
- Phase 2.2 exit code behavior remains unchanged

## Testability

**Requirement 7.1: Observable Behavior**

Every requirement SHALL be testable via:

- **Exit codes**: Observable via process exit status
- **stdout**: Observable via process standard output stream
- **stderr**: Observable via process standard error stream
- **Port availability**: Observable via network port binding attempts
- **Process lifecycle**: Observable via process start/stop and signal handling

**Requirement 7.2: Deterministic Behavior**

All runtime behavior SHALL be:

- Deterministic (same inputs produce same outputs)
- Observable without modifying the codebase
- Suitable for automated integration testing via subprocess execution

**Test Coverage Areas:**
1. Port binding failure detection and error reporting
2. Successful startup message format and timing
3. Signal handling (SIGINT, SIGTERM) and clean shutdown
4. Index load validation (schema version, corruption)
5. Failure ordering (first failure wins)
6. Output stream separation (stdout vs stderr)

## Acceptance Criteria

- [ ] Serve mode fails with exit code 3 if port cannot be bound
- [ ] Serve mode prints startup message only after successful index load AND port binding
- [ ] Clean shutdown via SIGINT/SIGTERM exits with code 0 and produces no stderr output
- [ ] Index load validation failures (schema version, corruption) exit with code 3
- [ ] Only the first runtime failure is reported (no cascading errors)
- [ ] All runtime errors appear on stderr, never on stdout
- [ ] All success messages appear on stdout, never on stderr
- [ ] Phase 2.2 exit code and error message behavior remains unchanged

## Test Plan

Suggested test files (to be created in Phase 2.3 test implementation):

- `tests/test_runtime_port_binding.cpp`: Tests for port binding failure scenarios
- `tests/test_runtime_startup_message.cpp`: Tests for startup message format and timing
- `tests/test_runtime_shutdown.cpp`: Tests for signal handling and clean shutdown
- `tests/test_runtime_index_validation.cpp`: Tests for index schema version and corruption handling
- `tests/test_runtime_failure_ordering.cpp`: Tests for single root cause reporting
- `tests/test_runtime_output_discipline.cpp`: Tests for stdout/stderr separation

## Definition of Done

- [ ] Specification document reviewed and approved
- [ ] All acceptance criteria are testable and observable
- [ ] No Phase 2.2 behavior is modified
- [ ] Specification is precise enough for implementation without guessing
- [ ] Test plan identifies test files and coverage areas

## Notes

- Phase 2.3 builds on Phase 2.2 but does not modify it
- All Phase 2.2 tests must continue to pass unchanged
- Runtime behavior is tested via integration tests (subprocess execution)
- Signal handling depends on framework capabilities (drogon) but must be deterministic
- Port binding errors may vary by OS (Address already in use, Permission denied, etc.)

No open questions.
