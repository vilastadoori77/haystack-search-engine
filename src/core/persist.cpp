#include "persist.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <sstream>

namespace fs = std::filesystem;
namespace persist
{
    static std::string tmp_path(const std::string &path)
    {
        return path + ".tmp";
    }
    void ensure_dir(const std::string &dir)
    {
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec)
            throw std::runtime_error("Failed to create directory: " + dir + ":" + ec.message());
    }
    static void write_bytes(const std::string &path, const std::string &bytes, bool binary)
    {
        const std::string t = tmp_path(path);
        {
            std::ofstream out(t, binary ? std::ios::binary : std::ios::out);
            if (!out)
                throw std::runtime_error("Failed to open file for writing: " + t);
            out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            if (!out)
                throw std::runtime_error("Failed to write data to file: " + t);
            out.flush();
            if (!out)
                throw std::runtime_error("Failed to flush data to file: " + t);
        }

        std::error_code ec;
        fs::rename(t, path, ec);
        if (ec)
            throw std::runtime_error("Failed to rename temp file: " + t + " to " + path + ": " + ec.message());
    }

    void write_text_atomic(const std::string &path, const std::string &content)
    {
        write_bytes(path, content, false);
    }

    void write_binary_atomic(const std::string &path, const std::string &bytes)
    {
        write_bytes(path, bytes, true);
    }

    static std::string read_all(const std::string &path, bool binary)
    {
        std::ifstream in(path, binary ? std::ios::binary : std::ios::in);
        if (!in)
            throw std::runtime_error("Failed to open file for reading: " + path);
        std::ostringstream ss;
        ss << in.rdbuf();
        if (!in)
            throw std::runtime_error("Failed to read data from file: " + path);
        return ss.str();
    }

    std::string read_all_text(const std::string &path)
    {
        return read_all(path, false);
    }
    std::string read_all_binary(const std::string &path)
    {
        return read_all(path, true);
    }
} // namespace persist

// Continue from Step 2 in ChatGPT response