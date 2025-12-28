#pragma once
#include <string>
#include <vector>

std::string make_snippet(const std::string &text,
                         const std::vector<std::string> &terms,
                         size_t window_chars = 120);