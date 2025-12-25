#include "core/tokenizer.h"
#include <cctype>
std::vector<std::string> tokenize(const std::string &text)
{

    std::vector<std::string> out;
    std::string cur;
    auto flush = [&]()
    {
        if (!cur.empty())
        {
            out.push_back(cur);
            cur.clear();
        }
    };
    for (unsigned char ch : text)
    {
        if (std::isalnum(ch))
            cur.push_back((char)std::tolower(ch));
        else
            flush();
    }
    flush();
    return out;
}
