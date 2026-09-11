#include "scope.h"
#include <iostream>
#include <string>
#include <cstring>

static void print_usage() {
    std::cout << "DiskScope - Disk Space Analyzer\n\n";
    std::cout << "Usage: diskscope [path] [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -d, --depth <N>        Maximum scan depth (default: 3)\n";
    std::cout << "  -e, --exclude <dir>    Exclude directory by name (repeatable)\n";
    std::cout << "  --export <fmt>         Export as 'json' or 'csv' to stdout\n";
    std::cout << "  -h, --help             Show this help message\n";
    std::cout << "  -v, --version          Show version\n";
    std::cout << "\nExamples:\n";
    std::cout << "  diskscope .\n";
    std::cout << "  diskscope /home/user -d 5 --exclude node_modules\n";
    std::cout << "  diskscope . --export json > report.json\n";
}

static void print_version() {
    std::cout << "DiskScope 1.0.0\n";
}

int main(int argc, char* argv[]) {
    ScanOptions options;

    int positional_count = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            print_version();
            return 0;
        } else if (arg == "-d" || arg == "--depth") {
            if (i + 1 < argc) {
                options.max_depth = std::atoi(argv[++i]);
                if (options.max_depth < 0) options.max_depth = 0;
            } else {
                std::cerr << "Error: " << arg << " requires a value\n";
                return 1;
            }
        } else if (arg == "-e" || arg == "--exclude") {
            if (i + 1 < argc) {
                options.exclude_patterns.insert(argv[++i]);
            } else {
                std::cerr << "Error: " << arg << " requires a value\n";
                return 1;
            }
        } else if (arg == "--export") {
            if (i + 1 < argc) {
                std::string fmt = argv[++i];
                if (fmt == "json") {
                    options.export_format = ExportFormat::JSON;
                } else if (fmt == "csv") {
                    options.export_format = ExportFormat::CSV;
                } else {
                    std::cerr << "Error: unknown export format '" << fmt << "'\n";
                    std::cerr << "Supported formats: json, csv\n";
                    return 1;
                }
            } else {
                std::cerr << "Error: --export requires a value\n";
                return 1;
            }
        } else if (positional_count == 0) {
            options.root = arg;
            positional_count++;
        } else {
            std::cerr << "Error: unexpected argument '" << arg << "'\n";
            return 1;
        }
    }

    if (!options.exclude_patterns.empty()) {
        std::cout << "Excluding directories:";
        for (const auto& p : options.exclude_patterns) {
            std::cout << " " << p;
        }
        std::cout << "\n";
    }

    std::cout << "Scanning " << options.root
              << " (depth " << options.max_depth << ")...\n";

    DiskStats stats = analyze(options);

    if (options.export_format == ExportFormat::JSON) {
        export_json(stats);
    } else if (options.export_format == ExportFormat::CSV) {
        export_csv(stats);
    } else {
        print_stats(stats);
        print_file_type_stats(stats);

        std::cout << "\nDirectory tree (depth " << options.max_depth << "):\n";
        print_tree(options.root, options.max_depth, options.exclude_patterns);
    }

    return 0;
}
