#include "core/query_parser.h"
#include "core/tokenizer.h"

ParsedQuery parse_query(const std::string &q)
{
    ParsedQuery pq;

    // simple split by spaces to preserve '-' and OR token
    std::vector<std::string> parts;
    std::string cur;
    for (char c : q)
    {
        if (c == ' ')
        {
            if (!cur.empty())
            {
                parts.push_back(cur);
                cur.clear();
            }
        }
        else
            cur.push_back(c);
    }
    if (!cur.empty())
        parts.push_back(cur);

    for (auto p : parts)
    {
        if (p == "OR" || p == "or")
        {
            pq.is_or = true;
            continue;
        }
        if (!p.empty() && p[0] == '-')
            pq.not_terms.push_back(p.substr(1));
        else
            pq.terms.push_back(p);
    }

    // normalize (lowercase + split punctuation) using tokenizer
    auto norm = [&](const std::vector<std::string> &in)
    {
        std::vector<std::string> out;
        for (auto &x : in)
        {
            auto t = tokenize(x);
            out.insert(out.end(), t.begin(), t.end());
        }
        return out;
    };
    pq.terms = norm(pq.terms);
    pq.not_terms = norm(pq.not_terms);

    return pq;
}
