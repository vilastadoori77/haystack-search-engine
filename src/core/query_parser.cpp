#include "core/query_parser.h"
#include "core/tokenizer.h"

ParsedQuery parse_query(const std::string &q)
{
    ParsedQuery pq;

    // Simple split by spaces to preserve '-' and OR token
    std::vector<std::string> parts;
    parts.reserve(16); // OPTIMIZATION: Reserve typical query size

    std::string cur;
    for (char c : q)
    {
        if (c == ' ')
        {
            if (!cur.empty())
            {
                parts.push_back(std::move(cur)); // OPTIMIZATION: Move instead of copy
                cur.clear();
            }
        }
        else
        {
            cur.push_back(c);
        }
    }
    if (!cur.empty())
        parts.push_back(std::move(cur));

    // Reserve space for terms (optimization)
    pq.terms.reserve(parts.size());
    pq.not_terms.reserve(parts.size() / 4); // Usually fewer NOT terms

    for (auto &p : parts)
    {
        if (p == "OR" || p == "or")
        {
            pq.is_or = true;
            continue;
        }
        if (!p.empty() && p[0] == '-')
        {
            pq.not_terms.push_back(p.substr(1));
        }
        else
        {
            pq.terms.push_back(std::move(p)); // OPTIMIZATION: Move
        }
    }

    // Normalize (lowercase + split punctuation) using tokenizer
    auto norm = [](const std::vector<std::string> &in) -> std::vector<std::string>
    {
        std::vector<std::string> out;
        out.reserve(in.size() * 2); // OPTIMIZATION: Reserve (terms may split)

        for (const auto &x : in)
        {
            auto t = tokenize(x);
            for (auto &token : t)
            {
                out.push_back(std::move(token)); // Move tokens
            }
        }
        return out;
    };

    pq.terms = norm(pq.terms);
    pq.not_terms = norm(pq.not_terms);

    return pq;
}