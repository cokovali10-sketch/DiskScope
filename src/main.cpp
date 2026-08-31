#include "scope.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string root = ".";
    int max_depth = 3;

    if (argc > 1) {
        root = argv[1];
    }
    if (argc > 2) {
        max_depth = std::atoi(argv[2]);
    }

    std::cout << "Scanning " << root << " (depth " << max_depth << ")...\n\n";

    DiskStats stats = analyze(root);
    print_stats(stats);

    std::cout << "\nDirectory tree (depth " << max_depth << "):\n";
    print_tree(root, max_depth);

    return 0;
}
