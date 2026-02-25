// Implementation added in Phase 2.5 step-by-step.
#include "ingestion/file_scanner.hpp"
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

namespace haystack
{
    namespace phase2_5
    {

        std::vector<fs::path> scan_inputs(const fs::path &root)
        {
            std::vector<fs::path> result;
            // 1.Root Validation: Check if root exists and is a directory. If not, return empty result.
            if (!fs::exists(root) || !fs::is_directory(root))
                return result;

            // 2. Recursive traversal (symlinks not followed by default; skip permission-denied dirs)
            fs::recursive_directory_iterator it(
                root,
                fs::directory_options::skip_permission_denied);

            for (const auto &entry : it)
            {
                // Only regular files
                if (!entry.is_regular_file())
                    continue;

                const fs::path p = entry.path();

                // Filter by supported extension (case-sensitive)
                if (!is_supported_extension(p))
                    continue;

                // Normalize and make absolute
                fs::path normalized = fs::absolute(p).lexically_normal();

                result.push_back(normalized);
            }

            // 3. UTF-8 Byte-Order Sorting: Sort the result by comparing UTF-8 byte order of the normalized absolute paths.
            std::sort(result.begin(), result.end(), [](const fs::path &a, const fs::path &b)
                      { return a.generic_string() < b.generic_string(); });

            return result;
        }

    } // namespace phase2_5
} // namespace haystack
