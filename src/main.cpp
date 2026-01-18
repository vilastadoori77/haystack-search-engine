#include <iostream>
#include <string>

struct CliArgs
{
    bool index = false;
    bool serve = false;
    bool help = false;
    std::string docs;
    std::string out;
    std::string in;
    std::string port;
};

static CliArgs parse_args(int argc, char *argv[])
{
    CliArgs a;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--index")
        {
            a.index = true;
        }
        else if (arg == "--serve")
        {
            a.serve = true;
        }
        else if (arg == "--help")
        {
            a.help = true;
        }
        else if (arg == "--docs" && i + 1 < argc)
        {
            a.docs = argv[++i];
        }
        else if (arg == "--out" && i + 1 < argc)
        {
            a.out = argv[++i];
        }
        else if (arg == "--in" && i + 1 < argc)
        {
            a.in = argv[++i];
        }
        else if (arg == "--port" && i + 1 < argc)
        {
            a.port = argv[++i];
        }
    }
    return a;
}

static void print_help()
{
    std::cout << "Usage: searchd [--index | --serve] [options]\n";
}

static bool has_flag(int argc, char *argv[], const std::string &flag)
{
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == flag)
            return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    // 1️⃣ FLAG EXCLUSIVITY — RAW ARGV CHECK (SPEC REQUIRED)
    if (has_flag(argc, argv, "--index") && has_flag(argc, argv, "--serve"))
    {
        std::cerr << "Error: --index and --serve cannot be used together\n";
        return 2;
    }
    CliArgs args = parse_args(argc, argv);

    std::cerr << "[DEBUG] index=" << args.index
              << " serve=" << args.serve << "\n";
    // Spec: no args behaves like --help
    if (argc == 1)
    {
        print_help();
        return 0;
    }

    // Spec: --help with anything prints help and exits 0
    if (args.help)
    {
        print_help();
        return 0;
    }

    // ✅ Fix for your FIRST failing test:
    // FLAG EXCLUSIVITY - MUST BE FIRST
    if (args.index && args.serve)
    {
        std::cerr << "Error: --index and --serve cannot be used together\n";
        return 2;
    }

    // 2 Mode -Specific required flags
    if (args.index)
    {
        if (args.docs.empty())
        {
            std::cerr << "Error: --docs <path> is required when using --index mode\n";
            return 2;
        }

        if (args.out.empty())
        {
            std::cerr << "Error: --out <index_dir> is required when using --index mode\n";
            return 2;
        }
    }
    else if (args.serve)
    {
        if (args.in.empty())
        {
            std::cerr << "Error: --in <index_dir> is required when using --serve mode\n";
            return 2;
        }

        if (args.port.empty())
        {
            std::cerr << "Error: --port <port> is required when using --serve mode\n";
            return 2;
        }
    }

    // For now, everything else is unimplemented → treat as CLI misuse
    std::cerr << "Error: Missing required mode flag (--index or --serve)\n";
    return 2;
}
