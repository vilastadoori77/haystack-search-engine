#pragma once

#include <vector>
#include <string>

struct ParsedQuery
{
    std::vector<std::string> terms;
    std::vector<std::string> not_terms;
    bool is_or = false;
};

// Optimization: Use String_view to avoid copying when caller has string
ParsedQuery parse_query(const std::string &q);
