You are working on the Haystack C++ Search Engine project.

Context:
- Phase 2.2 (CLI Lifecycle & Validation) is COMPLETE and MUST NOT be changed.
- Phase 2.3 (Runtime Lifecycle & Robustness) is COMPLETE and MUST NOT be changed.
- All existing tests for Phase 2.2 and Phase 2.3 are passing.
- Phase 2.4 specification is FINAL and APPROVED.

Task:
Implement Phase 2.4 exactly as specified in:
  specs/phase2_4_runtime_operational_guarantees.md

Strict Rules:
- Do NOT modify any Phase 2.2 or Phase 2.3 behavior.
- Do NOT change any CLI flags, arguments, exit codes, or error messages.
- Do NOT refactor unrelated code.
- Do NOT introduce new configuration files.
- Do NOT introduce new HTTP endpoints (use existing /health only).
- Do NOT change startup or shutdown behavior defined in Phase 2.3.
- Do NOT add logging frameworks, metrics, or authentication.

Implementation Scope (ONLY):
- Server readiness tracking
- /health endpoint readiness behavior
- Deterministic health responses
- Runtime failure handling after startup
- Shutdown idempotency and thread safety
- Output discipline enforcement at runtime

Requirements:
- Follow the spec word-for-word.
- Use mandatory semantics (SHALL / MUST).
- Every behavior must be observable via exit code, stdout, stderr, or HTTP response.
- Failure ordering must follow "first failure wins."
- Clean shutdown must produce NO output.

Testing:
- After implementation, generate Phase 2.4 tests under:
  tests/
  - test_runtime_readiness.cpp
  - test_runtime_health_endpoint.cpp
  - test_runtime_safety.cpp
  - test_runtime_shutdown_idempotency.cpp

- Tests MUST:
  - Use subprocess execution
  - Observe stdout, stderr, exit codes, and HTTP behavior
  - Be deterministic and CI-safe

Output:
- Modify only the necessary implementation files.
- Add new tests for Phase 2.4.
- Do NOT output explanations.
- Do NOT rewrite the spec.
