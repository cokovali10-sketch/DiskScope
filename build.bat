@echo off
echo Building DiskScope...
g++ -std=c++17 -O2 -o diskscope.exe src/main.cpp src/scope.cpp
echo Done: diskscope.exe
