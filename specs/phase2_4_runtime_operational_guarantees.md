# Phase 2.4: Runtime Operational Guarantees

## Spec Status

**Status:** Approved  
**Version:** 1.0  
**Date:** 2026-01-17  
**Dependencies:** Phase 2.2 (CLI Lifecycle) - COMPLETE, Phase 2.3 (Runtime Lifecycle) - COMPLETE  

## Overview

Phase 2.4 defines operational guarantees for a long-running search server process. This phase focuses on runtime safety, observability, and deterministic operational behavior after successful startup. Phase 2.4 builds strictly on top of Phase 2.3 and does not modify any behavior from Phase 2.2 or Phase 2.3.

Phase 2.4 does NOT modify any Phase 2.2 or Phase 2.3 behavior:
- No changes to CLI flags, arguments, or validation
- No changes to exit codes (0, 2, 3)
- No changes to Phase 2.2 or Phase 2.3 error messages
- No changes to startup message format or timing
- No changes to shutdown behavior (SIGINT/SIGTERM handling)

## Goals

- Define server readiness guarantees and behavior before readiness
- Establish deterministic health endpoint behavior
- Enforce runtime safety with no silent failures
- Ensure thread safety during shutdown
- Guarantee shutdown idempotency for multiple signals
- Reinforce output discipline from Phase 2.3

## Non-Goals

- New CLI flags or arguments
- Changes to Phase 2.2 or Phase 2.3 behavior
- Configuration files
- Frontend or UI changes
- Authentication, metrics, or logging frameworks
- Performance tuning
- New HTTP endpoints beyond existing ones

## Terminology

- **Ready**: Server state where all startup operations have completed successfully
- **Pre-readiness**: State between process start and readiness
- **Shutdown**: Process termination in response to SIGINT or SIGTERM
- **Runtime failure**: Any failure that occurs after successful startup (server is ready)
- **Health endpoint**: The existing `/health` HTTP endpoint

## 1. Server Readiness Guarantees

### 1.1 Readiness Definition

**Requirement 1.1.1: Readiness Criteria**

The server SHALL be considered "READY" only AFTER all of the following conditions are met:

1. Index load succeeds (per Phase 2.3)
2. Port binding succeeds (per Phase 2.3)
3. Startup message is printed to stdout (per Phase 2.3 Requirement 1.2)
4. HTTP server is accepting connections

**Requirement 1.1.2: Readiness Observability**

Readiness MUST be observable via `/health` endpoint behavior:
- When server is ready, `/health` MUST return HTTP 200
- When server is not ready, `/health` MUST NOT return HTTP 200

**Acceptance Criteria:**
- Server is ready only after all four readiness conditions are met
- Readiness is observable via `/health` returning HTTP 200
- Server readiness state is deterministic and testable

### 1.2 Pre-Readiness Behavior

**Requirement 1.2.1: Pre-Readiness Request Handling**

Before the server is ready:
- `/health` MUST NOT return HTTP 200
- `/health` MAY return non-200 (e.g., 503) or connection refused
- HTTP requests MAY be rejected or delayed (implementation-dependent)

**Acceptance Criteria:**
- `/health` does not return HTTP 200 before readiness
- Requests sent before readiness may be rejected or delayed
- No request processing occurs before readiness

**Requirement 1.2.2: Pre-Readiness Output**

Before the server is ready:
- No success output SHALL be printed to stdout
- No startup message SHALL be printed until all readiness criteria are met (per Phase 2.3)

**Acceptance Criteria:**
- No stdout output appears before index loading and port binding succeed
- Startup message only appears after successful readiness

## 2. Health Endpoint Guarantees

### 2.1 /health Behavior

**Requirement 2.1.1: Readiness-Based Response**

The existing `/health` endpoint SHALL:

- Return HTTP 200 ONLY when the server is ready and not shutting down (per Section 1.1)
- Return a non-200 HTTP response (e.g., 503 Service Unavailable) if the server is shutting down
- Return a non-200 HTTP response or connection refused if the server is not yet ready

**Note:** The exact non-200 status code during shutdown is implementation-dependent, but MUST be non-200.

**Acceptance Criteria:**
- `/health` returns HTTP 200 when server is ready and not shutting down
- `/health` returns non-200 when server is shutting down
- `/health` returns non-200 or connection refused when server is not yet ready

### 2.2 Deterministic Response

**Requirement 2.2.1: Constant Response Body**

The `/health` endpoint SHALL return a deterministic, constant response body when returning HTTP 200.

- The response body SHALL be the same for all successful `/health` requests
- No dynamic or environment-dependent content SHALL be included
- No timestamps, random values, or environment variables SHALL be present
- The response body MAY be empty or contain a simple status string

**Acceptance Criteria:**
- All `/health` requests return identical response bodies (when HTTP 200)
- Response body does not contain timestamps, random values, or environment variables
- Response body is deterministic across multiple requests
- Response body is constant and reproducible

## 3. Runtime Safety Guarantees

### 3.1 No Silent Failures

**Requirement 3.1.1: Failure Reporting**

Any runtime failure (occurring after successful startup when server is ready) MUST:

- Print exactly ONE error message to stderr
- Start with `"Error: "` prefix (consistent with Phase 2.2 and Phase 2.3)
- Describe the specific failure that occurred
- End with a newline character
- Cause a deterministic process exit with exit code 3

**Runtime failures include but are not limited to:**
- HTTP server internal errors
- Request handling failures that prevent server operation
- Memory allocation failures
- Threading errors

**Acceptance Criteria:**
- Any runtime failure produces exactly one error message to stderr before exit
- Any runtime failure causes exit code 3
- No runtime failure causes silent exit (no stderr output)
- Error messages follow Phase 2.2/2.3 format (starting with "Error: ")

### 3.2 Thread Safety

**Requirement 3.2.1: Shutdown Thread Safety**

Runtime shutdown (via SIGINT or SIGTERM) MUST be thread-safe and MUST NOT:

- Cause data races
- Cause use-after-free behavior
- Emit undefined behavior

**Note:** This requirement applies to the shutdown sequence defined in Phase 2.3. Thread safety during shutdown is a correctness guarantee that must be enforced.

**Acceptance Criteria:**
- Shutdown does not trigger thread sanitizer errors
- Shutdown does not cause crashes or undefined behavior
- Shutdown can be tested for thread safety using standard tools (TSan, etc.)

## 4. Shutdown Idempotency

### 4.1 Multiple Signals

**Requirement 4.1.1: Signal Idempotency**

Receiving SIGINT or SIGTERM multiple times during shutdown MUST:

- Not crash the process
- Not produce duplicate shutdown output
- Exit cleanly with exit code 0 (per Phase 2.3)

**Behavior:**
- The first SIGINT or SIGTERM initiates shutdown (per Phase 2.3)
- Subsequent SIGINT or SIGTERM signals SHALL be ignored or handled gracefully
- No duplicate error messages or shutdown notifications SHALL appear
- Shutdown MUST be thread-safe when handling multiple signals

**Acceptance Criteria:**
- Sending SIGINT multiple times does not crash the process
- Sending SIGTERM multiple times does not crash the process
- Sending both SIGINT and SIGTERM does not cause issues
- Process exits with code 0 regardless of signal frequency
- No duplicate shutdown output appears in stderr or stdout

## 5. Output Discipline (Reinforced)

### 5.1 Output Stream Separation

**Requirement 5.1.1: stdout Usage**

stdout SHALL be used ONLY for:

- Startup success message (serve mode only, per Phase 2.3)
- Help/usage information (Phase 2.2)
- Indexing completion message (Phase 2.2 index mode)

**Requirement 5.1.2: stderr Usage**

stderr SHALL be used for:

- ALL runtime errors
- ALL validation errors (Phase 2.2)
- ALL operational failures
- ALL error messages (including runtime failures)

**Requirement 5.1.3: Clean Shutdown Output**

Shutdown (via SIGINT or SIGTERM) MUST:

- Produce no stdout output
- Produce no stderr output on clean shutdown

**Prohibited:**
- No error messages SHALL appear on stdout
- No success messages SHALL appear on stderr
- No output on clean shutdown

**Acceptance Criteria:**
- Runtime errors appear only on stderr
- Startup success messages appear only on stdout
- Clean shutdown produces no output to stdout or stderr
- Exit code is 0 on clean shutdown (per Phase 2.3)

## 6. Failure Ordering

### 6.1 First Failure Wins

**Requirement 6.1.1: Single Root Cause Rule**

If multiple runtime failures could occur, the process SHALL:

- Report ONLY the first fatal failure encountered
- Exit immediately after the first fatal error
- Not attempt operations that depend on the failed operation

**Note:** This requirement reinforces Phase 2.3 Requirement 4.1 and applies to runtime failures after successful startup.

**Acceptance Criteria:**
- Only one error message appears in stderr for any runtime failure sequence
- Process exits immediately after first fatal error
- No cascading error messages are produced

## 7. Exit Code Contract

### 7.1 Exit Code Semantics (Unchanged)

**Requirement 7.1.1: Exit Codes Remain Unchanged**

Exit codes SHALL remain unchanged from Phase 2.2 and Phase 2.3:

- **0**: Success or clean shutdown
  - Successful index mode completion
  - Successful serve mode startup
  - Clean shutdown via SIGINT/SIGTERM
  - Idempotent shutdown (multiple signals)

- **2**: CLI misuse / validation failure (Phase 2.2 only)
  - Invalid flags or arguments
  - Missing required flags
  - Conflicting flags
  - Invalid port format

- **3**: Runtime or operational failure
  - Index directory not found (Phase 2.2)
  - Index file not found (Phase 2.2)
  - Index load failure (Phase 2.3)
  - Port binding failure (Phase 2.3)
  - Document file not found (Phase 2.2)
  - Indexing/saving failure (Phase 2.2)
  - Runtime failures after successful startup (Phase 2.4)

**Acceptance Criteria:**
- Exit code 0 for clean shutdown (including multiple signals)
- Exit code 3 for runtime failures after startup
- Phase 2.2 and Phase 2.3 exit code behavior remains unchanged

## 8. Testability

### 8.1 Observable Behavior

**Requirement 8.1.1: Testable Requirements**

Every requirement in Phase 2.4 SHALL be testable via:

- **HTTP behavior**: Observable via HTTP requests to `/health` endpoint
- **Exit codes**: Observable via process exit status
- **stdout**: Observable via process standard output stream
- **stderr**: Observable via process standard error stream
- **Process lifecycle**: Observable via process start/stop and signal handling

**Acceptance Criteria:**
- Server readiness can be verified via `/health` endpoint
- Pre-readiness behavior can be tested via timing of requests relative to startup
- Health endpoint determinism can be verified via multiple requests
- Runtime failures can be verified via stderr and exit codes
- Shutdown idempotency can be tested via multiple signals

### 8.2 Deterministic Behavior

**Requirement 8.2.1: Deterministic Requirements**

All Phase 2.4 behavior SHALL be:

- Deterministic (same inputs produce same outputs)
- Observable without modifying the codebase
- Suitable for automated integration testing via subprocess execution

**Acceptance Criteria:**
- All requirements can be verified via automated tests
- No requirement depends on timing or race conditions (except where explicitly stated)
- Test results are reproducible

## Acceptance Criteria

- [ ] Server is READY only after index load succeeds, port binding succeeds, startup message is printed, and HTTP server accepts connections
- [ ] Readiness is observable via `/health` returning HTTP 200
- [ ] `/health` MUST NOT return HTTP 200 before readiness
- [ ] No HTTP requests are processed before server readiness
- [ ] `/health` endpoint returns HTTP 200 ONLY when server is ready and not shutting down
- [ ] `/health` endpoint returns non-200 when server is shutting down
- [ ] `/health` endpoint returns deterministic, constant response body (when HTTP 200)
- [ ] Response body contains no timestamps, random values, or environment variables
- [ ] Runtime failures print exactly ONE error to stderr starting with "Error: "
- [ ] Runtime failures cause exit code 3
- [ ] Shutdown is thread-safe and does not cause data races or undefined behavior
- [ ] Multiple SIGINT/SIGTERM signals do not crash the process
- [ ] Multiple SIGINT/SIGTERM signals do not produce duplicate output
- [ ] Clean shutdown produces no stdout or stderr output
- [ ] stdout contains only success/info messages, stderr contains only errors
- [ ] Only first fatal failure is reported (no cascading errors)
- [ ] Exit codes follow the contract (0=success/shutdown, 2=CLI misuse, 3=runtime failure)
- [ ] Phase 2.2 and Phase 2.3 behavior remains unchanged

## Test Plan

Suggested test files (to be created in Phase 2.4 test implementation):

- `tests/test_runtime_readiness.cpp`: Tests for server readiness guarantees and pre-readiness behavior
- `tests/test_runtime_health_endpoint.cpp`: Tests for `/health` endpoint behavior and determinism
- `tests/test_runtime_safety.cpp`: Tests for runtime failure reporting and thread safety
- `tests/test_runtime_shutdown_idempotency.cpp`: Tests for multiple signal handling

**Test Requirements:**
- Tests MUST use subprocess execution
- Tests MUST observe stdout, stderr, exit codes, and HTTP responses
- Tests MUST be deterministic and CI-safe

## Definition of Done

- [ ] Specification document reviewed and approved
- [ ] All acceptance criteria are testable and observable
- [ ] No Phase 2.2 or Phase 2.3 behavior is modified
- [ ] Specification is precise enough for implementation without guessing
- [ ] Test plan identifies test files and coverage areas
- [ ] All requirements use mandatory language (SHALL, MUST) where appropriate

## Notes

- Phase 2.4 builds on Phase 2.2 and Phase 2.3 but does not modify them
- All Phase 2.2 and Phase 2.3 tests must continue to pass unchanged
- Runtime behavior is tested via integration tests (subprocess execution, HTTP requests)
- Health endpoint implementation may vary by HTTP framework (Drogon), but behavior must be deterministic
- Thread safety during shutdown is a correctness guarantee that must be enforced
- Failure ordering reinforces Phase 2.3 behavior and applies to runtime failures after startup

No open questions.
