#pragma once
#include <string>

namespace persist
{
    // Ensure directory exists (create if missing)
    void ensure_dir(const std::string &dir);

    // Write file atomically: write temp -> fsync options -> rename
    void write_text_atomic(const std::string &path, const std::string &content);
    void write_binary_atomic(const std::string &path, const std::string &bytes);

    // Read whole file

    std::string read_all_text(const std::string &path);
    std::string read_all_binary(const std::string &path);
}