#include "core/snippet.h"
#include <algorithm>
#include <cctype>

static std::string lower(std::string s)
{
    for (auto &c : s)
        c = (char)std::tolower((unsigned char)c);
    return s;
}

std::string make_snippet(const std::string &text,
                         const std::vector<std::string> &terms,
                         size_t window_chars)
{
    std::string ltext = lower(text);
    // find earliest occurence of any term
    size_t best = std::string::npos;
    for (const auto &t : terms)
    {
        auto pos = ltext.find(lower(t));
        if (pos != std::string::npos)
            best = std::min(best, pos);
    }
    if (best == std::string::npos)
    {
        // fallback : first window
        return text.substr(0, std::min(window_chars, text.size()));
    }

    size_t start = (best > window_chars / 3) ? best - window_chars / 3 : 0;
    size_t end = std::min(start + window_chars, text.size());
    return text.substr(start, end - start);
}