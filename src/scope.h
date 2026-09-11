#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <set>

struct FileEntry {
    std::string path;
    std::string name;
    uint64_t size;
    bool is_directory;
    int depth;
};

struct FileTypeStats {
    std::string extension;
    uint64_t total_size;
    uint64_t file_count;
};

struct DiskStats {
    std::string root;
    uint64_t total_size;
    uint64_t file_count;
    uint64_t dir_count;
    uint64_t largest_file_size;
    std::string largest_file;
    std::vector<std::pair<std::string, uint64_t>> top_extensions;
    std::vector<std::pair<std::string, uint64_t>> top_directories;
    std::vector<FileTypeStats> file_type_stats;
};

enum class ExportFormat {
    NONE,
    JSON,
    CSV
};

struct ScanOptions {
    std::string root = ".";
    int max_depth = 3;
    std::set<std::string> exclude_patterns;
    ExportFormat export_format = ExportFormat::NONE;
};

uint64_t get_file_size(const std::string& path);
std::string format_size(uint64_t bytes);
std::string get_extension(const std::string& filename);
bool should_exclude(const std::string& name, const std::set<std::string>& patterns);
void scan_directory(const std::string& root, std::vector<FileEntry>& entries,
                    int max_depth, const std::set<std::string>& exclude_patterns);
DiskStats analyze(const ScanOptions& options);
void print_tree(const std::string& root, int max_depth,
                const std::set<std::string>& exclude_patterns);
void print_stats(const DiskStats& stats);
void export_json(const DiskStats& stats);
void export_csv(const DiskStats& stats);
void print_file_type_stats(const DiskStats& stats);
