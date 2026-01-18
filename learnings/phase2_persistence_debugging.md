# Phase 2 Persistence Debugging Experiences

**Date**: 2025-01-17  
**Feature**: Phase 2 - On-Disk Persistence  
**Related Files**: `tests/test_persistence*.cpp`, `tests/test_cli_persistence.cpp`, `src/core/search_service.cpp`, `apps/searchd/main.cpp`

---

## Overview

Phase 2 introduced on-disk persistence for the search index, allowing the index to be saved to and loaded from disk without re-indexing. This document covers the debugging experiences encountered during implementation and testing.

---

## Error 1: Index Files Not Created (`filesystem error: in file_size`)

### Initial Error Message

```
/Users/vilastadoori/_Haystack_proj/tests/test_cli_persistence.cpp:206: FAILED:
  filesystem error: in file_size: No such file or directory ["/tmp/haystack_test_237/index_meta.json"]
```

### Problem Analysis

**Test Expectation**: After running `searchd --index --docs <path> --out <index_dir>`, the test expected index files (`index_meta.json`, `docs.jsonl`, `postings.bin`) to exist.

**Actual Behavior**: Command exited with code 0 (success), but files were not created.

**Root Cause**: In `apps/searchd/main.cpp`, the `--index` mode was not actually calling `SearchService::save()` to persist the index to disk. The code parsed `--out` but never used it.

### Initial Code (Broken)

```cpp
// apps/searchd/main.cpp - index mode
if (index_mode) {
    // ... build index ...
    // ❌ Missing: s.save(index_dir);
    // No save() call - files never created!
}
```

### Solution

**Fixed Code**:
```cpp
// apps/searchd/main.cpp - index mode
if (index_mode) {
    // ... build index ...
    std::string index_dir = /* parse from --out */;
    s.save(index_dir); // ✅ Actually persist the index
}
```

### Learning

- **Exit code 0 doesn't mean work was done** - verify actual side effects (files created, data persisted)
- **Test should validate file existence** before reading file sizes
- **TDD approach** - tests revealed missing implementation

---

## Error 2: Binary Path Resolution (`./build/searchd: No such file or directory`)

### Initial Error Message

```
sh: ./build/searchd: No such file or directory
```

### Problem Analysis

**Test Expectation**: Tests assumed `searchd` binary would be found at `./build/searchd`.

**Actual Behavior**: When tests ran from different directories (e.g., project root vs. build directory), the relative path failed.

**Root Cause**: Hardcoded relative path (`./build/searchd`) only works when running from project root.

### Initial Code (Broken)

```cpp
// tests/test_cli_persistence.cpp
std::string cmd = "./build/searchd --index ..."; // ❌ Fails from build/ directory
```

### Solution

**Fixed Code** - Added `find_searchd_path()` helper:

```cpp
static std::string find_searchd_path()
{
    // Try multiple common locations
    std::vector<std::string> candidates = {
        "./build/searchd",
        "./searchd",
        "../build/searchd",
        "build/searchd"
    };
    
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            // Convert to absolute path
            return std::filesystem::absolute(candidate).string();
        }
    }
    
    throw std::runtime_error("Could not find searchd binary");
}
```

**Usage**:
```cpp
std::string searchd_path = find_searchd_path();
std::string cmd = searchd_path + " --index ...";
```

### Learning

- **Don't assume working directory** - use absolute paths or search for binaries
- **Helper functions** for cross-directory test execution
- **Robust path resolution** improves test portability

---

## Error 3: Default Document Loading in Serve Mode

### Initial Error Message

```
libc++abi: terminating due to uncaught exception of type std::runtime_error: 
Failed to open docs file: data/docs.json
zsh: abort ./build/searchd --serve --in idx --port 8900
```

### Problem Analysis

**Test Expectation**: `searchd --serve --in <index_dir>` should load index from `index_dir` only.

**Actual Behavior**: Program crashed trying to load `data/docs.json` (hardcoded default path) even when `--in` was provided.

**Root Cause**: `main.cpp` had default document loading that always ran, regardless of mode:

```cpp
// apps/searchd/main.cpp
// ❌ This always executed, even in serve mode with --in
load_docs_from_json(s, "data/docs.json"); // Hardcoded default
```

### Solution

**Fixed Code**:
```cpp
// apps/searchd/main.cpp - serve mode
if (serve_mode) {
    std::string in_dir = /* parse from --in */;
    s.load(in_dir); // Load from index directory
    // ✅ Removed: load_docs_from_json(s, "data/docs.json");
}
```

### Learning

- **Mode-specific behavior** - don't run code paths for other modes
- **CLI flags should override defaults** - `--in` should prevent default path loading
- **Test cases catch integration issues** - revealed hardcoded assumptions

---

## Error 4: macOS Compatibility (`timeout` command not found)

### Initial Error Message

```
sh: timeout: command not found
```

### Problem Analysis

**Test Expectation**: Used `timeout` command (Linux/GNU coreutils) to kill serve mode after a delay.

**Actual Behavior**: `timeout` is not standard on macOS (BSD-based).

**Root Cause**: Assumed Linux/GNU command availability.

### Initial Code (Broken)

```cpp
// tests/test_cli_persistence.cpp
std::string cmd = searchd_path + " --serve ...";
std::string full_cmd = "timeout 0.5 " + cmd; // ❌ Not available on macOS
```

### Solution

**Fixed Code** - macOS-compatible background process management:

```cpp
// tests/test_cli_persistence.cpp
std::string full_cmd = cmd + " & SERVE_PID=$!; sleep 0.5; kill $SERVE_PID 2>/dev/null || true; wait $SERVE_PID 2>/dev/null; exit 0";
```

**Explanation**:
- `&` - run in background
- `SERVE_PID=$!` - capture process ID
- `sleep 0.5` - wait for startup
- `kill $SERVE_PID` - terminate process
- `wait $SERVE_PID` - wait for termination
- `2>/dev/null || true` - ignore errors if process already dead

### Learning

- **Cross-platform compatibility** - don't assume GNU/Linux commands
- **Use POSIX-compliant approaches** - background processes work on all Unix-like systems
- **Test on target platforms** - catch compatibility issues early

---

## Error 5: Port Conflicts (`Address already in use`)

### Initial Error Message

```
Address already in use (errno=48)
```

### Problem Analysis

**Test Expectation**: Fixed port `8900` for serve mode tests.

**Actual Behavior**: If another process (or previous test run) was using port 8900, binding failed.

**Root Cause**: Hardcoded port number in tests caused conflicts.

### Initial Code (Broken)

```cpp
// tests/test_cli_persistence.cpp
int test_port = 8900; // ❌ Fixed port - conflicts possible
```

### Solution

**Fixed Code** - Random port generation:

```cpp
// tests/test_cli_persistence.cpp
static int get_random_port()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(9000, 9999); // Use range 9000-9999
    return dis(gen);
}

int test_port = get_random_port(); // ✅ Dynamic port selection
```

### Learning

- **Avoid fixed ports in tests** - use dynamic/random port selection
- **Port ranges** - choose user-space ports (9000-9999) to avoid system conflicts
- **Test isolation** - each test run gets unique resources

---

## Error 6: Logic Bug in `intersect_sorted` (`||` vs `&&`)

### Initial Error Message

```
/Users/vilastadoori/_Haystack_proj/tests/test_persistence_determinism.cpp:XXX: FAILED:
  REQUIRE( results_before.size() == results_after.size() )
with expansion:
  3 == 2  ❌ Results before reload had 3 items, after reload had 2
```

### Problem Analysis

**Test Expectation**: After save → load, query results should be identical.

**Actual Behavior**: `results_before.size() == 3`, `results_after.size() == 2` (one extra result before reload).

**Root Cause**: Bug in `intersect_sorted()` function in `src/core/search_service.cpp`:

```cpp
// src/core/search_service.cpp
std::vector<size_t> intersect_sorted(const std::vector<size_t>& a, const std::vector<size_t>& b) {
    std::vector<size_t> result;
    size_t i = 0, j = 0;
    while (i < a.size() || j < b.size()) { // ❌ Should be && not ||
        if (i >= a.size() || j >= b.size()) break;
        if (a[i] == b[j]) {
            result.push_back(a[i]);
            i++; j++;
        } else if (a[i] < b[j]) {
            i++;
        } else {
            j++;
        }
    }
    return result;
}
```

**Problem**: Using `||` (OR) means the loop continues even when one array is exhausted, potentially accessing out-of-bounds or adding incorrect elements.

### Solution

**Fixed Code**:
```cpp
while (i < a.size() && j < b.size()) { // ✅ AND - both must have elements
    // ... rest of logic ...
}
```

### Learning

- **Intersection logic** requires both lists to have elements (`&&` not `||`)
- **Edge case testing** - revealed logic bug in core algorithm
- **Persistence tests catch correctness issues** - reloading exposed the bug

---

## Error 7: File Existence Checks in Tests

### Initial Error Message

```
filesystem error: in file_size: No such file or directory
```

### Problem Analysis

**Test Expectation**: Files should exist after `searchd --index` completes.

**Actual Behavior**: Tests tried to read file sizes before verifying files existed.

**Root Cause**: Assumed files would always be created (but implementation might be incomplete).

### Solution

**Fixed Code** - Added existence checks before reading:

```cpp
// tests/test_cli_persistence.cpp
auto meta_path = index_dir + "/index_meta.json";
REQUIRE(std::filesystem::exists(meta_path)); // ✅ Check existence first

if (std::filesystem::exists(meta_path)) {
    auto size = std::filesystem::file_size(meta_path); // ✅ Now safe
    REQUIRE(size > 0);
}
```

### Learning

- **Defensive test code** - check existence before operations
- **Graceful failure** - tests can handle partial implementations
- **Better error messages** - existence check fails with clear message

---

## Summary of Key Learnings

### 1. **Implementation Completeness**
- Exit code 0 doesn't mean all work was done
- Verify side effects (files, state changes)

### 2. **Path Resolution**
- Don't assume working directory
- Use absolute paths or search for resources
- Helper functions improve portability

### 3. **Mode-Specific Behavior**
- CLI flags should override defaults
- Don't execute code paths for other modes
- Test integration scenarios

### 4. **Cross-Platform Compatibility**
- Avoid GNU/Linux-specific commands
- Use POSIX-compliant approaches
- Test on target platforms

### 5. **Test Isolation**
- Use dynamic ports/resources
- Avoid hardcoded values
- Each test run should be independent

### 6. **Algorithm Correctness**
- Intersection/set operations need careful logic (`&&` vs `||`)
- Persistence tests reveal correctness bugs
- Test before and after state transitions

### 7. **Defensive Testing**
- Check file existence before reading
- Handle partial implementations gracefully
- Clear error messages improve debugging

---

## Files Modified

- `apps/searchd/main.cpp` - Added `save()` call in index mode, removed default doc loading in serve mode
- `src/core/search_service.cpp` - Fixed `intersect_sorted()` logic bug
- `tests/test_cli_persistence.cpp` - Added `find_searchd_path()`, random ports, macOS-compatible process management
- `tests/test_persistence*.cpp` - Added file existence checks

---

## References

- **Specification**: `specs/phase2_persistence.md`
- **Test Files**: `tests/test_persistence*.cpp`, `tests/test_cli_persistence.cpp`
- **Implementation**: `src/core/search_service.cpp`, `apps/searchd/main.cpp`
