# DiskScope

Disk space analyzer that shows directory structure and file statistics.

## Features

- Recursive directory scanning
- Total size, file count, directory count
- Largest file detection
- Top extensions by size
- Top directories by size
- Configurable scan depth
- Tree view output

## Build & Run

```bash
make
./diskscope /path/to/scan

# or
g++ -std=c++17 -O2 -o diskscope src/main.cpp src/scope.cpp
./diskscope . 3
```

## License

MIT
