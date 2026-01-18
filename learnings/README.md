# Learnings Documentation

This folder contains documentation of debugging experiences, problem-solving approaches, and key insights from implementing the Haystack search engine.

## Documents

### [phase2_persistence_debugging.md](./phase2_persistence_debugging.md)
**Phase 2 - On-Disk Persistence**

Covers debugging experiences during initial persistence implementation:
- Index files not being created (`filesystem error`)
- Binary path resolution issues (`./build/searchd: No such file`)
- Default document loading in serve mode (hardcoded `data/docs.json`)
- macOS compatibility (`timeout` command not found)
- Port conflicts (`Address already in use`)
- Logic bug in `intersect_sorted` (`||` vs `&&`)

**Key Insight**: Exit code 0 doesn't guarantee side effects (files created). Always verify actual state changes.

---

### [phase2_2_cli_lifecycle_debugging.md](./phase2_2_cli_lifecycle_debugging.md)
**Phase 2.2 - CLI Lifecycle**

Documents CLI validation and test infrastructure issues:
- Catch2 `operator||` limitations in `REQUIRE` assertions
- Validation order issues (wrong error messages)
- Invalid port error message format mismatches
- Exit code capture failures in tests
- Missing index directory validation
- Success message capture issues

**Key Insight**: Validation order matters - check flag conflicts before mode-specific requirements. Match error message formats exactly per specification.

---

### [startup_message_debugging.md](./startup_message_debugging.md)
**Phase 2.3 - Runtime Lifecycle: Startup Message Tests**

A comprehensive guide documenting:
- Multiple implementation attempts (5 different approaches)
- Test infrastructure issues (shell variable expansion, subshells, file synchronization)
- Root cause analysis: Invalid test data preventing code execution
- Final solutions for both test data and output capture
- Key learnings about debugging methodology

**Key Insight**: The tests were failing not because of unreliable output capture, but because invalid test fixtures (empty `postings.bin`) caused index loading to fail, preventing the startup message code path from ever executing.

---

## Debugging Methodology Best Practices

Based on our experiences, here are key principles for debugging test failures:

1. **Check Error Output First**: Always inspect stderr files/logs - they often reveal why code paths aren't executing.

2. **Verify Test Fixtures**: Ensure test data matches production formats (especially binary files).

3. **Trace Execution Flow**: Verify that the code path being tested actually executes before debugging output capture.

4. **Isolate Variables**: Separate issues of test data, output method, and capture method.

5. **Use Low-Level I/O for Redirected Output**: `write()` + `fsync()` is more reliable than `std::cout` when stdout is redirected to files in background processes.

---

## Future Learnings

As more debugging experiences accumulate, additional documents will be added here:
- Port binding issues and socket programming challenges
- Index serialization/deserialization edge cases
- Signal handling and graceful shutdown
- Performance optimization discoveries
- Cross-platform compatibility issues
