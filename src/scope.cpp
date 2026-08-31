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
    if (pos == std::string::npos) return "(none)";
    return filename.substr(pos);
}

void scan_directory(const std::string& root, std::vector<FileEntry>& entries, int max_depth) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(
                 root, fs::directory_options::skip_permission_denied)) {
            int depth = entry.depth();
            if (depth > max_depth) continue;

            FileEntry fe;
            fe.path = entry.path().string();
            fe.name = entry.path().filename().string();
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

DiskStats analyze(const std::string& root) {
    std::vector<FileEntry> entries;
    scan_directory(root, entries);

    DiskStats stats{};
    stats.root = root;
    stats.total_size = 0;
    stats.file_count = 0;
    stats.dir_count = 0;
    stats.largest_file_size = 0;

    std::map<std::string, uint64_t> ext_sizes;
    std::map<std::string, uint64_t> dir_sizes;

    for (const auto& e : entries) {
        if (e.is_directory) {
            stats.dir_count++;
        } else {
            stats.file_count++;
            stats.total_size += e.size;

            if (e.size > stats.largest_file_size) {
                stats.largest_file_size = e.size;
                stats.largest_file = e.name;
            }

            ext_sizes[get_extension(e.name)] += e.size;

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

    return stats;
}

void print_tree(const std::string& root, int max_depth) {
    try {
        std::cout << root << "/\n";
        for (const auto& entry : fs::directory_iterator(root)) {
            std::string name = entry.path().filename().string();
            if (entry.is_directory()) {
                std::cout << "├── [" << name << "]\n";
            } else {
                std::cout << "├── " << name
                          << " (" << format_size(get_file_size(entry.path().string())) << ")\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void print_stats(const DiskStats& stats) {
    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout << "║           D I S K   S C O P E            ║\n";
    std::cout << "╠══════════════════════════════════════════╣\n";
    std::cout << "║ Root:          " << stats.root << "\n";
    std::cout << "║ Total size:    " << format_size(stats.total_size) << "\n";
    std::cout << "║ Files:         " << stats.file_count << "\n";
    std::cout << "║ Directories:   " << stats.dir_count << "\n";
    std::cout << "║ Largest file:  " << stats.largest_file
              << " (" << format_size(stats.largest_file_size) << ")\n";
    std::cout << "╠══════════════════════════════════════════╣\n";

    if (!stats.top_extensions.empty()) {
        std::cout << "║ Top Extensions by Size:\n";
        for (const auto& [ext, size] : stats.top_extensions) {
            std::cout << "║   " << std::setw(12) << std::left << ext
                      << " " << format_size(size) << "\n";
        }
    }

    if (!stats.top_directories.empty()) {
        std::cout << "╠══════════════════════════════════════════╣\n";
        std::cout << "║ Top Directories by Size:\n";
        for (const auto& [dir, size] : stats.top_directories) {
            std::cout << "║   " << std::setw(20) << std::left << dir
                      << " " << format_size(size) << "\n";
        }
    }

    std::cout << "╚══════════════════════════════════════════╝\n";
}
