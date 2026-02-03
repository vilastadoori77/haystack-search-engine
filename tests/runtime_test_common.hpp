#pragma once

#include <string>
#include <utility>

namespace runtime_test {

// Run a shell command with a timeout. Returns (exit_code, stdout+stderr combined).
// Uses fork + process group + waitpid with timeout; on timeout kills the whole
// process group so server subprocesses don't hang the test.
std::pair<int, std::string> run_command_capture_output(const std::string& command,
                                                       int timeout_seconds);

// Run a shell command with a timeout, capturing stdout and stderr separately.
// Returns exit code. Outputs are written to stdout_output and stderr_output parameters.
// Uses fork + process group + waitpid with timeout; on timeout kills the whole
// process group so server subprocesses don't hang the test.
int run_command_split_output(const std::string& command,
                             std::string& stdout_output,
                             std::string& stderr_output,
                             int timeout_seconds);

} // namespace runtime_test
