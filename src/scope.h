#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct FileEntry {
    std::string path;
    std::string name;
    uint64_t size;
    bool is_directory;
    int depth;
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
};

uint64_t get_file_size(const std::string& path);
std::string format_size(uint64_t bytes);
void scan_directory(const std::string& root, std::vector<FileEntry>& entries, int max_depth = 10);
DiskStats analyze(const std::string& root);
void print_tree(const std::string& root, int max_depth = 3);
void print_stats(const DiskStats& stats);
