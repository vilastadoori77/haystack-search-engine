# Phase 2.2 CLI Lifecycle Debugging Experiences

**Date**: 2025-01-17  
**Feature**: Phase 2.2 - CLI Lifecycle  
**Related Files**: `tests/test_cli_*.cpp`, `apps/searchd/main.cpp`

---

## Overview

Phase 2.2 defined the command-line interface lifecycle for `searchd`, including mode exclusivity, flag validation, exit codes, and error messaging. This document covers debugging experiences during test implementation and validation fixes.

---

## Error 1: Catch2 `operator||` in REQUIRE Assertions

### Initial Error Message

```
/Users/vilastadoori/_Haystack_proj/build/_deps/catch2-src/src/catch2/../catch2/internal/catch_decomposer.hpp:420:27: 
error: static assertion failed due to requirement 'always_false<bool>::value': 
operator|| is not supported inside assertions, wrap the expression inside parentheses, or decompose it
```

### Problem Analysis

**Test Code**:
```cpp
// tests/test_cli_validation.cpp
REQUIRE(stderr_output.find("Error: --index and --serve cannot be used together") != std::string::npos ||
        stderr_output.find("Error: Invalid flag combination") != std::string::npos); // ❌ Fails compilation
```

**Root Cause**: Catch2's `REQUIRE` macro does not allow `||` (OR) operators directly within assertions. This is a Catch2 design limitation.

### Solution

**Option 1: Use boolean variable**
```cpp
// tests/test_cli_validation.cpp
bool has_expected_error = (stderr_output.find("Error: --index and --serve cannot be used together") != std::string::npos ||
                           stderr_output.find("Error: Invalid flag combination") != std::string::npos);
REQUIRE(has_expected_error); // ✅ Works
```

**Option 2: Separate REQUIRE statements**
```cpp
// If either message is acceptable
bool msg1_found = stderr_output.find("Error: --index and --serve cannot be used together") != std::string::npos;
bool msg2_found = stderr_output.find("Error: Invalid flag combination") != std::string::npos;
REQUIRE(msg1_found || msg2_found); // ✅ Works (evaluated outside REQUIRE)
```

**Option 3: Wrap in parentheses (if Catch2 version supports)**
```cpp
REQUIRE((stderr_output.find("Error:") != std::string::npos || 
         stderr_output.find("Warning:") != std::string::npos)); // ✅ Sometimes works
```

### Files Affected

- `tests/test_cli_validation.cpp`
- `tests/test_cli_lifecycle.cpp`
- `tests/test_cli_error_messages.cpp`
- `tests/test_runtime_shutdown.cpp`
- `tests/test_runtime_output_discipline.cpp`

### Learning

- **Catch2 limitations** - `REQUIRE` doesn't support `||` or `&&` directly
- **Decompose expressions** - evaluate boolean logic outside `REQUIRE`
- **Consistent pattern** - use boolean variables for complex conditions

---

## Error 2: Validation Order Issue (Wrong Error Message)

### Initial Error Message

**Test Expected**:
```
Error: --index and --serve cannot be used together
```

**Test Received**:
```
Error: --out <index_dir> is required when using --index mode
```

### Problem Analysis

**Test Command**: `./searchd --index --serve`

**Root Cause**: Validation order in `main.cpp` was incorrect. The code checked for missing `--out` **before** checking for conflicting flags (`--index` and `--serve`).

**Initial Code (Wrong Order)**:
```cpp
// apps/searchd/main.cpp
if (index_mode) {
    if (out_dir.empty()) { // ❌ Checked FIRST
        std::cerr << "Error: --out <index_dir> is required when using --index mode\n";
        return 2;
    }
}

if (index_mode && serve_mode) { // ❌ Checked SECOND
    std::cerr << "Error: --index and --serve cannot be used together\n";
    return 2;
}
```

**Problem**: When both `--index` and `--serve` are present, the missing `--out` check runs first and reports the wrong error.

### Solution

**Fixed Code** - Check flag conflicts first (per Phase 2.2 spec):

```cpp
// apps/searchd/main.cpp - Validation order per spec
// 1. Check flag conflicts (mutually exclusive flags)
if (index_mode && serve_mode) { // ✅ Checked FIRST
    std::cerr << "Error: --index and --serve cannot be used together\n";
    return 2;
}

// 2. Check required flags for each mode
if (index_mode && out_dir.empty()) { // ✅ Checked SECOND
    std::cerr << "Error: --out <index_dir> is required when using --index mode\n";
    return 2;
}

if (serve_mode && in_dir.empty()) {
    std::cerr << "Error: --in <index_dir> is required when using --serve mode\n";
    return 2;
}
```

### Learning

- **Validation order matters** - check conflicts before mode-specific requirements
- **Spec-compliant ordering** - Phase 2.2 spec defines validation sequence
- **Most specific errors first** - flag conflicts are more specific than missing args

---

## Error 3: Invalid Port Error Message Format

### Initial Error Message

**Test Expected** (per spec):
```
Error: Invalid port number: <port>
```

**Test Received**:
```
Error: Port must be a number between 1024 and 65535
```

### Problem Analysis

**Test Command**: `./searchd --serve --in <dir> --port invalid`

**Root Cause**: Error messages in `main.cpp` did not match the spec's exact format.

**Initial Code**:
```cpp
// apps/searchd/main.cpp
if (port_str is not numeric) {
    std::cerr << "Error: Port must be a number between 1024 and 65535\n"; // ❌ Wrong format
    return 2;
}
```

**Spec Requirement**: `"Error: Invalid port number: <port>\n"` (must include the invalid value).

### Solution

**Fixed Code**:
```cpp
// apps/searchd/main.cpp
try {
    port = std::stoi(port_str);
    if (port < 1024 || port > 65535) {
        std::cerr << "Error: Invalid port number: " << port_str << "\n"; // ✅ Matches spec
        return 2;
    }
} catch (const std::exception&) {
    std::cerr << "Error: Invalid port number: " << port_str << "\n"; // ✅ Matches spec
    return 2;
}
```

### Learning

- **Match spec exactly** - error messages must match specification format
- **Include context** - error messages should include the invalid value
- **Test error formats** - `test_cli_error_messages.cpp` validates exact message format

---

## Error 4: Exit Code Capture Failure

### Initial Error Message

```
/Users/vilastadoori/_Haystack_proj/tests/test_cli_lifecycle.cpp:144: FAILED:
  REQUIRE( exit_code == 0 )
with expansion:
  -1 == 0  ❌ Exit code not captured correctly
```

### Problem Analysis

**Test Code**:
```cpp
// tests/test_cli_lifecycle.cpp (initial attempt)
std::string full_cmd = cmd + "; echo $?";
// ❌ Problem: echo $? appended exit code to stdout, making parsing difficult
```

**Root Cause**: Using `echo $?` in the command appended exit code to stdout, making it hard to parse correctly. Exit code was not being captured reliably.

### Solution

**Fixed Code** - Use separate file for exit code:

```cpp
// tests/test_cli_lifecycle.cpp
static int run_command_capture_output(const std::string& cmd, 
                                     std::string& stdout_output, 
                                     std::string& stderr_output)
{
    pid_t pid = getpid();
    std::string stdout_file = "/tmp/haystack_cmd_stdout_" + std::to_string(pid);
    std::string stderr_file = "/tmp/haystack_cmd_stderr_" + std::to_string(pid);
    std::string exit_code_file = "/tmp/haystack_cmd_exit_" + std::to_string(pid);
    
    // Run command, capture stdout, stderr, and exit code separately
    std::string full_cmd = "(" + cmd + " >" + stdout_file + " 2>" + stderr_file + "; echo $? >" + exit_code_file + ")";
    int result = std::system(full_cmd.c_str());
    
    // Read stdout
    std::ifstream stdout_stream(stdout_file);
    if (stdout_stream) {
        stdout_output.assign((std::istreambuf_iterator<char>(stdout_stream)),
                            std::istreambuf_iterator<char>());
        stdout_stream.close();
    }
    
    // Read stderr
    std::ifstream stderr_stream(stderr_file);
    if (stderr_stream) {
        stderr_output.assign((std::istreambuf_iterator<char>(stderr_stream)),
                            std::istreambuf_iterator<char>());
        stderr_stream.close();
    }
    
    // Read exit code
    int exit_code = -1;
    std::ifstream exit_stream(exit_code_file);
    if (exit_stream) {
        exit_stream >> exit_code;
        exit_stream.close();
    }
    
    // Cleanup
    std::remove(stdout_file.c_str());
    std::remove(stderr_file.c_str());
    std::remove(exit_code_file.c_str());
    
    return exit_code; // ✅ Return actual exit code
}
```

### Learning

- **Separate capture streams** - use different files for stdout, stderr, exit code
- **Parse exit code separately** - don't mix with stdout content
- **Unique filenames** - use PID to avoid conflicts between parallel tests

---

## Error 5: Missing Index Directory Validation

### Initial Error Message

**Test Expected**:
```
Error: Index directory not found: <path>
```

**Test Received**:
```
Error loading index: File does not exist
```

### Problem Analysis

**Test Command**: `./searchd --serve --in /nonexistent/path --port 8900`

**Root Cause**: `main.cpp` called `s.load(in_dir)` directly without first validating that `in_dir` exists or contains required files.

**Initial Code**:
```cpp
// apps/searchd/main.cpp - serve mode
if (serve_mode) {
    s.load(in_dir); // ❌ No validation - tries to load immediately
    // If directory doesn't exist, fails with generic "File does not exist"
}
```

**Problem**: The error message didn't match the spec, and validation wasn't done before heavy operations (per spec: "fail-fast validation").

### Solution

**Fixed Code** - Validate before loading (per Phase 2.2 spec):

```cpp
// apps/searchd/main.cpp - serve mode
if (serve_mode) {
    // Per spec: Validate before heavy operations (fail-fast)
    
    // 1. Check directory exists
    if (!std::filesystem::exists(in_dir)) {
        std::cerr << "Error: Index directory not found: " << in_dir << "\n";
        return 3;
    }
    
    if (!std::filesystem::is_directory(in_dir)) {
        std::cerr << "Error: Index directory not found: " << in_dir << "\n";
        return 3;
    }
    
    // 2. Check required files exist
    auto meta_path = in_dir + "/index_meta.json";
    auto docs_path = in_dir + "/docs.jsonl";
    auto postings_path = in_dir + "/postings.bin";
    
    if (!std::filesystem::exists(meta_path) || 
        !std::filesystem::exists(docs_path) || 
        !std::filesystem::exists(postings_path)) {
        std::cerr << "Error: Index directory is missing required files: " << in_dir << "\n";
        return 3;
    }
    
    // 3. Now load (validation passed)
    s.load(in_dir);
}
```

### Learning

- **Fail-fast validation** - check prerequisites before heavy operations
- **Spec-compliant error messages** - match exact format from specification
- **Explicit checks** - don't rely on error messages from deeper layers

---

## Error 6: Success Message Not Captured

### Initial Error Message

```
/Users/vilastadoori/_Haystack_proj/tests/test_cli_lifecycle.cpp:144: FAILED:
  REQUIRE( stdout_output.find(expected_msg) != std::string::npos )
with expansion:
  18446744073709551615 != 18446744073709551615
```

### Problem Analysis

**Test Expectation**: After successful `searchd --index`, stdout should contain: `"Indexing completed. Index saved to: <index_dir>\n"`

**Actual Behavior**: `stdout_output.find()` returned `std::string::npos` (not found).

**Root Cause**: Output capture issues or message not being printed.

**Solution**: 
1. Ensure `std::cout.flush()` is called after printing success message
2. Trim trailing whitespace from captured output before searching

```cpp
// Trim trailing whitespace/newlines
std::string trimmed = stdout_output;
while (!trimmed.empty() && (trimmed.back() == '\n' || trimmed.back() == '\r' || trimmed.back() == ' ')) {
    trimmed.pop_back();
}
REQUIRE(trimmed.find(expected_msg) != std::string::npos);
```

### Learning

- **Flush output** - ensure messages are written before process exits
- **Handle whitespace** - trim output before string matching
- **Check actual content** - print captured output for debugging

---

## Summary of Key Learnings

### 1. **Catch2 Assertion Limitations**
- `REQUIRE` doesn't support `||` or `&&` operators directly
- Use boolean variables or separate `REQUIRE` statements
- Consistent pattern across all test files

### 2. **Validation Order Matters**
- Check flag conflicts before mode-specific requirements
- Follow spec-defined validation sequence
- Most specific errors first

### 3. **Exact Error Message Format**
- Match specification format exactly (including invalid values)
- Include context in error messages
- Test error formats explicitly

### 4. **Exit Code Capture**
- Use separate files for stdout, stderr, and exit code
- Parse exit code independently from stdout
- Unique filenames prevent conflicts

### 5. **Fail-Fast Validation**
- Validate prerequisites before heavy operations
- Explicit checks with spec-compliant messages
- Don't rely on deeper layer error messages

### 6. **Output Capture Reliability**
- Flush output after printing
- Trim whitespace before string matching
- Use file-based capture for background processes

---

## Files Modified

- `apps/searchd/main.cpp` - Fixed validation order, error message formats, added index directory validation
- `tests/test_cli_validation.cpp` - Fixed Catch2 `||` usage, added port validation tests
- `tests/test_cli_lifecycle.cpp` - Fixed exit code capture, added output trimming
- `tests/test_cli_error_messages.cpp` - Fixed Catch2 `||` usage
- `tests/test_cli_exit_codes.cpp` - Fixed Catch2 assertion patterns

---

## References

- **Specification**: `specs/phase2_2_cli_lifecycle.md`
- **Test Files**: `tests/test_cli_*.cpp`
- **Implementation**: `apps/searchd/main.cpp`
