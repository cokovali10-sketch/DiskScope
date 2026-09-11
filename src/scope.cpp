#include "scope.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

uint64_t get_file_size(const std::string& path) {
    try {
        return fs::file_size(path);
    } catch (...) {
        return 0;
    }
}

std::string format_size(uint64_t bytes) {
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    const double TB = GB * 1024.0;

    std::ostringstream oss;
    if (bytes >= TB) {
        oss << std::fixed << std::setprecision(2) << bytes / TB << " TB";
    } else if (bytes >= GB) {
        oss << std::fixed << std::setprecision(2) << bytes / GB << " GB";
    } else if (bytes >= MB) {
        oss << std::fixed << std::setprecision(2) << bytes / MB << " MB";
    } else if (bytes >= KB) {
        oss << std::fixed << std::setprecision(2) << bytes / KB << " KB";
    } else {
        oss << bytes << " B";
    }
    return oss.str();
}

std::string get_extension(const std::string& filename) {
    auto pos = filename.rfind('.');
    if (pos == std::string::npos || pos == 0) return "(none)";
    return filename.substr(pos);
}

bool should_exclude(const std::string& name, const std::set<std::string>& patterns) {
    return patterns.find(name) != patterns.end();
}

void scan_directory(const std::string& root, std::vector<FileEntry>& entries,
                    int max_depth, const std::set<std::string>& exclude_patterns) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(
                 root, fs::directory_options::skip_permission_denied)) {
            int depth = entry.depth();
            if (depth > max_depth) continue;

            std::string name = entry.path().filename().string();
            if (entry.is_directory() && should_exclude(name, exclude_patterns)) {
                continue;
            }

            FileEntry fe;
            fe.path = entry.path().string();
            fe.name = name;
            fe.is_directory = entry.is_directory();
            fe.depth = depth;

            if (fe.is_directory) {
                fe.size = 0;
            } else {
                fe.size = get_file_size(fe.path);
            }

            entries.push_back(fe);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning: " << e.what() << std::endl;
    }
}

DiskStats analyze(const ScanOptions& options) {
    std::vector<FileEntry> entries;
    scan_directory(options.root, entries, options.max_depth, options.exclude_patterns);

    DiskStats stats{};
    stats.root = options.root;
    stats.total_size = 0;
    stats.file_count = 0;
    stats.dir_count = 0;
    stats.largest_file_size = 0;

    std::map<std::string, uint64_t> ext_sizes;
    std::map<std::string, uint64_t> ext_counts;
    std::map<std::string, uint64_t> dir_sizes;

    for (const auto& e : entries) {
        if (e.is_directory) {
            stats.dir_count++;
        } else {
            stats.file_count++;
            stats.total_size += e.size;

            if (e.size > stats.largest_file_size) {
                stats.largest_file_size = e.size;
                stats.largest_file = e.path;
            }

            std::string ext = get_extension(e.name);
            ext_sizes[ext] += e.size;
            ext_counts[ext]++;

            if (e.depth <= 2) {
                std::string parent = fs::path(e.path).parent_path().filename().string();
                dir_sizes[parent] += e.size;
            }
        }
    }

    std::vector<std::pair<std::string, uint64_t>> ext_vec(
        ext_sizes.begin(), ext_sizes.end());
    std::sort(ext_vec.begin(), ext_vec.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    stats.top_extensions = std::vector<std::pair<std::string, uint64_t>>(
        ext_vec.begin(), ext_vec.begin() + std::min<size_t>(10, ext_vec.size()));

    std::vector<std::pair<std::string, uint64_t>> dir_vec(
        dir_sizes.begin(), dir_sizes.end());
    std::sort(dir_vec.begin(), dir_vec.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    stats.top_directories = std::vector<std::pair<std::string, uint64_t>>(
        dir_vec.begin(), dir_vec.begin() + std::min<size_t>(10, dir_vec.size()));

    for (const auto& [ext, size] : ext_sizes) {
        FileTypeStats fts;
        fts.extension = ext;
        fts.total_size = size;
        fts.file_count = ext_counts[ext];
        stats.file_type_stats.push_back(fts);
    }
    std::sort(stats.file_type_stats.begin(), stats.file_type_stats.end(),
              [](const FileTypeStats& a, const FileTypeStats& b) {
                  return a.total_size > b.total_size;
              });

    return stats;
}

static void print_tree_recursive(const fs::path& current, const std::string& prefix,
                                 int depth, int max_depth,
                                 const std::set<std::string>& exclude_patterns) {
    if (depth > max_depth) return;

    std::vector<fs::directory_entry> entries;
    try {
        for (const auto& entry : fs::directory_iterator(current)) {
            std::string name = entry.path().filename().string();
            if (entry.is_directory() && should_exclude(name, exclude_patterns)) {
                continue;
            }
            entries.push_back(entry);
        }
    } catch (const std::exception&) {
        return;
    }

    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        bool a_dir = a.is_directory();
        bool b_dir = b.is_directory();
        if (a_dir != b_dir) return a_dir > b_dir;
        return a.path().filename().string() < b.path().filename().string();
    });

    for (size_t i = 0; i < entries.size(); ++i) {
        bool last = (i == entries.size() - 1);
        std::string connector = last ? "└── " : "├── ";
        std::string name = entries[i].path().filename().string();

        if (entries[i].is_directory()) {
            std::cout << prefix << connector << "[" << name << "]\n";
            std::string new_prefix = prefix + (last ? "    " : "│   ");
            print_tree_recursive(entries[i].path(), new_prefix, depth + 1,
                                 max_depth, exclude_patterns);
        } else {
            uint64_t size = get_file_size(entries[i].path().string());
            std::cout << prefix << connector << name
                      << " (" << format_size(size) << ")\n";
        }
    }
}

void print_tree(const std::string& root, int max_depth,
                const std::set<std::string>& exclude_patterns) {
    try {
        std::cout << root << "/\n";
        print_tree_recursive(root, "", 1, max_depth, exclude_patterns);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void print_stats(const DiskStats& stats) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║              D I S K   S C O P E                     ║\n";
    std::cout << "╠══════════════════════════════════════════════════════╣\n";
    std::cout << "║ Root:             " << stats.root << "\n";
    std::cout << "║ Total size:       " << format_size(stats.total_size) << "\n";
    std::cout << "║ Files:            " << stats.file_count << "\n";
    std::cout << "║ Directories:      " << stats.dir_count << "\n";
    std::cout << "║ Largest file:     " << stats.largest_file
              << " (" << format_size(stats.largest_file_size) << ")\n";
    std::cout << "╠══════════════════════════════════════════════════════╣\n";

    if (!stats.top_extensions.empty()) {
        std::cout << "║ Top Extensions by Size:\n";
        for (const auto& [ext, size] : stats.top_extensions) {
            std::cout << "║   " << std::setw(12) << std::left << ext
                      << " " << format_size(size) << "\n";
        }
    }

    if (!stats.top_directories.empty()) {
        std::cout << "╠══════════════════════════════════════════════════════╣\n";
        std::cout << "║ Top Directories by Size:\n";
        for (const auto& [dir, size] : stats.top_directories) {
            std::cout << "║   " << std::setw(20) << std::left << dir
                      << " " << format_size(size) << "\n";
        }
    }

    std::cout << "╚══════════════════════════════════════════════════════╝\n";
}

void print_file_type_stats(const DiskStats& stats) {
    if (stats.file_type_stats.empty()) return;

    std::cout << "\nFile Type Statistics:\n";
    std::cout << std::left;
    for (const auto& fts : stats.file_type_stats) {
        std::cout << "  " << std::setw(10) << fts.extension
                  << std::setw(8) << fts.file_count << " files"
                  << "    " << format_size(fts.total_size) << "\n";
    }
}

void export_json(const DiskStats& stats) {
    std::cout << "{\n";
    std::cout << "  \"root\": \"" << stats.root << "\",\n";
    std::cout << "  \"total_size\": " << stats.total_size << ",\n";
    std::cout << "  \"file_count\": " << stats.file_count << ",\n";
    std::cout << "  \"dir_count\": " << stats.dir_count << ",\n";
    std::cout << "  \"largest_file\": \"" << stats.largest_file << "\",\n";
    std::cout << "  \"largest_file_size\": " << stats.largest_file_size << ",\n";

    std::cout << "  \"top_extensions\": [\n";
    for (size_t i = 0; i < stats.top_extensions.size(); ++i) {
        const auto& [ext, size] = stats.top_extensions[i];
        std::cout << "    {\"extension\": \"" << ext
                  << "\", \"size\": " << size << "}";
        if (i < stats.top_extensions.size() - 1) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "  ],\n";

    std::cout << "  \"top_directories\": [\n";
    for (size_t i = 0; i < stats.top_directories.size(); ++i) {
        const auto& [dir, size] = stats.top_directories[i];
        std::cout << "    {\"directory\": \"" << dir
                  << "\", \"size\": " << size << "}";
        if (i < stats.top_directories.size() - 1) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "  ],\n";

    std::cout << "  \"file_type_stats\": [\n";
    for (size_t i = 0; i < stats.file_type_stats.size(); ++i) {
        const auto& fts = stats.file_type_stats[i];
        std::cout << "    {\"extension\": \"" << fts.extension
                  << "\", \"file_count\": " << fts.file_count
                  << ", \"total_size\": " << fts.total_size << "}";
        if (i < stats.file_type_stats.size() - 1) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "  ]\n";
    std::cout << "}\n";
}

void export_csv(const DiskStats& stats) {
    std::cout << "type,name,size_bytes,size_human,count\n";

    std::cout << "summary,total_size," << stats.total_size << ","
              << format_size(stats.total_size) << ",\n";
    std::cout << "summary,file_count," << stats.file_count << ",," << "\n";
    std::cout << "summary,dir_count," << stats.dir_count << ",," << "\n";
    std::cout << "summary,largest_file," << stats.largest_file_size << ","
              << format_size(stats.largest_file_size) << ",\n";

    for (const auto& [ext, size] : stats.top_extensions) {
        std::cout << "extension," << ext << "," << size << ","
                  << format_size(size) << ",\n";
    }

    for (const auto& [dir, size] : stats.top_directories) {
        std::cout << "directory," << dir << "," << size << ","
                  << format_size(size) << ",\n";
    }

    for (const auto& fts : stats.file_type_stats) {
        std::cout << "filetype," << fts.extension << "," << fts.total_size << ","
                  << format_size(fts.total_size) << "," << fts.file_count << "\n";
    }
}
