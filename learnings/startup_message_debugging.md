# Startup Message Test Debugging - Phase 2.3

**Date**: 2025-01-17  
**Feature**: Runtime Lifecycle - Startup Message Tests  
**Test File**: `tests/test_runtime_startup_message.cpp`  
**Implementation File**: `apps/searchd/main.cpp`

---

## Problem Statement

Three startup message tests were consistently failing in Phase 2.3:
1. `Successful startup: prints exactly one startup message to stdout`
2. `Successful startup: message contains port number and index directory`
3. `Startup message: appears on stdout, not stderr`

All tests were failing to capture the startup message that should be printed to stdout after successful index load and port binding.

---

## Initial Implementation Attempts

### Attempt 1: Basic `std::cout` with flush

**Code**:
```cpp
std::cout << "Server started on port " << port << " using index: " << in_dir << "\n";
std::cout.flush();
```

**Result**: Failed - message not captured in redirected output files.

**Root Cause**: Output buffering issues when stdout is redirected to a file via shell redirection.

---

### Attempt 2: Unbuffered I/O with `setvbuf`

**Code**:
```cpp
std::setvbuf(stdout, NULL, _IONBF, 0);
std::setvbuf(stderr, NULL, _IONBF, 0);
// ... later ...
std::cout << "Server started on port " << port << " using index: " << in_dir << "\n";
std::cout.flush();
```

**Result**: Failed - still not captured reliably.

**Root Cause**: Even with unbuffered mode, `std::cout` can have issues with background processes and shell redirection.

---

### Attempt 3: `fprintf` with `fflush` and `fsync`

**Code**:
```cpp
fprintf(stdout, "Server started on port %d using index: %s\n", port, in_dir.c_str());
fflush(stdout);
fsync(1); // Force sync file descriptor 1 (stdout)
```

**Result**: Failed - still not captured.

**Root Cause**: Same issues as above, plus potential race conditions with file descriptor setup.

---

### Attempt 4: Low-level `write()` with `fsync()`

**Code**:
```cpp
std::string msg = "Server started on port " + std::to_string(port) + " using index: " + in_dir + "\n";
write(1, msg.c_str(), msg.length()); // Direct write to fd 1 (stdout)
fsync(1); // Force sync
```

**Result**: Still failed initially.

**Root Cause**: Discovered later - the real issue was not the output method, but that the code path never executed due to index loading failure.

---

### Attempt 5: Combined approach (BOTH `write()` AND `fprintf()`)

**Code**:
```cpp
std::string msg = "Server started on port " + std::to_string(port) + " using index: " + in_dir + "\n";
write(1, msg.c_str(), msg.length());
fprintf(stdout, "%s", msg.c_str());
fflush(stdout);
fsync(1);
```

**Result**: Message appeared, but **count == 2** instead of 1 (printed twice).

**Error Message**:
```
REQUIRE( count == 1 )
with expansion:
  2 == 1
```

**Root Cause**: Duplicate print statements.

---

## Test Infrastructure Issues Encountered

### Issue 1: Shell variable expansion (`$$`)

**Problem**: Using `$$` in shell commands within C++ string literals doesn't expand correctly.

**Initial Code**:
```cpp
std::string full_cmd = cmd + " >/tmp/haystack_stdout_$$ & PID=$!; ...";
```

**Error**: `$$` not expanded, causing file path issues.

**Solution**: Use process PID from C++ `getpid()`:
```cpp
pid_t pid = getpid();
std::string stdout_file_path = "/tmp/haystack_startup_stdout_" + std::to_string(pid);
```

---

### Issue 2: Subshell overhead and file handle race conditions

**Problem**: Using subshell `(cmd >file &)` can delay file handle setup.

**Initial Code**:
```cpp
std::string full_cmd = "(cmd >file 2>file2 &); PID=$!; sleep 2; kill $PID; wait $PID";
```

**Solution**: Remove subshell - run command directly in main shell:
```cpp
std::string full_cmd = "cmd >file 2>file2 & SERVE_PID=$!; sleep 2; kill $SERVE_PID; wait $SERVE_PID; sleep 0.2; sync";
```

**Rationale**: File handles are ready before process starts when redirection happens in the same shell context.

---

### Issue 3: Filesystem synchronization timing

**Problem**: Output files might not be flushed to disk before reading.

**Solution**: Added explicit sync after process termination:
```cpp
wait $SERVE_PID; sleep 0.2; sync
```

---

### Issue 4: Empty output file race condition

**Problem**: File might be empty when first read due to timing.

**Solution**: Added retry logic with delays:
```cpp
// Retry reading if file is empty (handle race conditions)
for (int attempt = 0; attempt < 5; ++attempt) {
    usleep(100000); // 100ms between attempts
    std::ifstream test_file(stdout_file_path);
    if (test_file && test_file.peek() != std::ifstream::traits_type::eof()) {
        break; // File has content, proceed to read
    }
}
```

---

## The Real Root Cause: Invalid Test Data

### Discovery Process

1. **Initial Hypothesis**: Output capture method was unreliable.
2. **Investigation**: Checked stderr files from failed test runs.
3. **Finding**: Stderr contained `"Error loading index: Failed to read u64"`.
4. **Insight**: Index loading was **failing**, so the startup message code path never executed!

### Root Cause Analysis

**File**: `tests/test_runtime_startup_message.cpp`

**Problematic Code**:
```cpp
static std::string create_test_index()
{
    // ... creates index_meta.json and docs.jsonl ...
    
    std::ofstream postings(index_dir + "/postings.bin");
    postings.close(); // ❌ Creates EMPTY file!
    
    return index_dir;
}
```

**Impact**: Empty `postings.bin` caused `InvertedIndex::load()` to fail when trying to read the `uint64_t` term count header (first 8 bytes), resulting in `"Error loading index: Failed to read u64"`.

**Error Flow**:
1. `main.cpp` calls `s.load(in_dir)` in serve mode
2. `SearchService::load()` calls `idx.load(postings_path)`
3. `InvertedIndex::load()` tries to read `uint64_t term_count` from file
4. Empty file → read fails → exception thrown → program exits with code 3
5. Startup message code (after load) **never executes**

**Evidence from stderr files**:
```bash
$ cat /tmp/haystack_startup_stderr_*
Error loading index: Failed to read u64
```

**Evidence from stdout files**:
```bash
$ ls -la /tmp/haystack_startup_stdout_*
-rw-r--r-- 1 user wheel 0 Jan 17 22:28 /tmp/haystack_startup_stdout_57279
# File size = 0 bytes (empty) ✅ Confirms nothing was printed
```

---

## Final Solution

### 1. Fix Test Data Creation

**Fixed Code** (`test_runtime_startup_message.cpp`):
```cpp
static std::string create_test_index()
{
    // ... creates index_meta.json and docs.jsonl ...
    
    // Create minimal valid postings.bin (little-endian uint64_t: 0 terms = 8 bytes of zeros)
    std::ofstream postings(index_dir + "/postings.bin", std::ios::binary);
    std::uint64_t term_count = 0;
    unsigned char bytes[8] = {
        (unsigned char)(term_count & 0xFF),
        (unsigned char)((term_count >> 8) & 0xFF),
        (unsigned char)((term_count >> 16) & 0xFF),
        (unsigned char)((term_count >> 24) & 0xFF),
        (unsigned char)((term_count >> 32) & 0xFF),
        (unsigned char)((term_count >> 40) & 0xFF),
        (unsigned char)((term_count >> 48) & 0xFF),
        (unsigned char)((term_count >> 56) & 0xFF)
    };
    postings.write(reinterpret_cast<const char*>(bytes), 8); // ✅ Write valid header
    postings.close();
    
    return index_dir;
}
```

**Key Changes**:
- Added `std::ios::binary` flag
- Added `#include <cstdint>` for `uint64_t`
- Write 8 bytes (little-endian representation of `uint64_t` term count = 0)

---

### 2. Fix Duplicate Message Print

**Problem**: Message was printed twice (once with `write()`, once with `fprintf()`).

**Error**:
```
REQUIRE( count == 1 )
with expansion:
  2 == 1  ❌ Found 2 messages
```

**Fixed Code** (`apps/searchd/main.cpp`):
```cpp
// Phase 2.3: Print startup message after port check succeeds
// Per spec: Print exactly ONE startup message
std::string msg = "Server started on port " + std::to_string(port) + " using index: " + in_dir + "\n";
write(1, msg.c_str(), msg.length()); // ✅ Only write() - no fprintf()
fsync(1); // Force sync if stdout is a file
```

**Removed**:
```cpp
fprintf(stdout, "%s", msg.c_str()); // ❌ Removed duplicate
fflush(stdout); // ❌ Removed (not needed with write())
```

---

## Final Implementation in `main.cpp`

### Complete Solution

```cpp
// Phase 2.3: Print startup message after port check succeeds
// Print directly here since we've verified the port is available, and actual binding will succeed in run()
// Use write() directly to file descriptor 1 (stdout) to bypass any buffering, even when redirected
// Per spec: Print exactly ONE startup message
std::string msg = "Server started on port " + std::to_string(port) + " using index: " + in_dir + "\n";
write(1, msg.c_str(), msg.length()); // write() to fd 1 (stdout) - immediate, no buffering
fsync(1); // Force sync if stdout is a file
```

**Why `write()` + `fsync()`?**
- `write()` is a low-level syscall that bypasses stdio buffering
- Works reliably even when stdout is redirected to a file
- `fsync(1)` ensures data is written to disk (important for redirected file output)
- Single write ensures exactly one message (per spec)

---

## Key Learnings

### 1. **Always Check Error Output First**

When debugging test failures:
- **Check stderr files first** - they often reveal why code paths aren't executing
- Don't assume output capture is the problem - verify the code path actually runs

### 2. **Test Data Validity is Critical**

- Invalid test data can cause failures that mask the real issue
- Empty files can cause parsing/binary read failures
- Always validate test fixtures match production data format

### 3. **Binary File Format Matters**

- Empty binary files ≠ valid binary files
- Even "empty" binary structures need valid headers (e.g., 8 bytes for `uint64_t`)
- Use `std::ios::binary` flag when writing binary data

### 4. **Output Capture in Background Processes**

**Reliable Pattern**:
```bash
cmd >file1 2>file2 & PID=$!; 
sleep <startup_time>; 
kill $PID; 
wait $PID; 
sleep <cleanup_time>; 
sync
```

**Key Principles**:
- No subshells (`(cmd &)` → `cmd &`)
- Use PID from parent process (not shell `$$`)
- Add explicit `sync` after `wait`
- Consider retry logic for reading output files

### 5. **Low-Level I/O for Redirected Output**

- `std::cout` can be unreliable with redirected file output in background processes
- `write(fd, ...)` + `fsync(fd)` is more reliable
- Still ensure only **one** print statement (not multiple fallbacks)

### 6. **Debugging Methodology**

1. **Check error output** (stderr) for actual failure reasons
2. **Verify file contents** (what was actually written to stdout/stderr files)
3. **Check file sizes** (0 bytes = nothing printed, non-zero = something printed)
4. **Trace execution flow** (does code path even execute?)
5. **Isolate variables** (test data vs. output method vs. capture method)

---

## Error Messages Reference

### Error 1: Message Not Found (Count = 0)
```
REQUIRE( stdout_output.find("Server started on port") != std::string::npos )
with expansion:
  18446744073709551615 (0xffffffffffffffff) != 18446744073709551615
```

**Cause**: Index loading failed before startup message could be printed.

**Diagnosis**: Check stderr for "Error loading index" messages.

---

### Error 2: Duplicate Messages (Count = 2)
```
REQUIRE( count == 1 )
with expansion:
  2 == 1
```

**Cause**: Startup message printed twice (both `write()` and `fprintf()`).

**Fix**: Remove duplicate print statement.

---

### Error 3: Index Load Failure
```
Error loading index: Failed to read u64
```

**Cause**: Empty or invalid `postings.bin` file.

**Fix**: Create valid binary file with proper header (8 bytes for `uint64_t` term count).

---

## References

- **Specification**: `specs/phase2_3_runtime_lifecycle.md`
- **Test File**: `tests/test_runtime_startup_message.cpp`
- **Implementation**: `apps/searchd/main.cpp`
- **Related Fix**: `tests/test_runtime_port_binding.cpp` (similar `create_test_index()` fix)

---

## Conclusion

The startup message tests were failing not because of unreliable output capture, but because:
1. **Invalid test data** (empty `postings.bin`) prevented index loading
2. **Code path never executed** (early exit on load failure)
3. **Duplicate print statements** caused `count == 2` after fix

**Key Takeaway**: Always verify that test fixtures match production data formats and that the code paths being tested actually execute.
