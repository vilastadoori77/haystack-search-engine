#pragma once

#include <array>
#include <filesystem>
#include <string_view>
#include <vector>

namespace haystack
{
    namespace phase2_5
    {

        inline constexpr std::array<std::string_view, 2> SUPPORTED_EXTENSIONS = {".txt", ".pdf"};

        inline bool is_supported_extension(const std::filesystem::path &p)
        {
            const auto ext = p.extension().string();
            for (const auto &supported : SUPPORTED_EXTENSIONS)
            {
                if (ext == supported)
                    return true;
            }
            return false;
        }

        /**
         * Scans a root path for supported input files.
         *
         * - Recursive traversal: all subdirectories under root are traversed. Directory
         *   symlinks are not followed (only physical directories); ensures deterministic results.
         * - Absolute path normalization: each file path is normalized (e.g. lexically_normal).
         * - UTF-8 byte-order sorting: the returned list is sorted by comparing UTF-8 byte order
         *   of the normalized absolute path, for deterministic ordering across runs and locales.
         * - Filtering: only supported extensions (.txt, .pdf) are included; others are ignored.
         *   Extension comparison is case-sensitive (.TXT and .PDF are not included).
         *
         * @param root Directory root to scan (e.g. --docs path).
         * @return Sorted list of normalized absolute paths to supported files. Returns an empty
         *         vector if root does not exist or is not a directory.
         */
        std::vector<std::filesystem::path> scan_inputs(const std::filesystem::path &root);

    } // namespace phase2_5
} // namespace haystack
