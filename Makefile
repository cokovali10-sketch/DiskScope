CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic
SRC = src/main.cpp src/scope.cpp
OBJ = $(SRC:.cpp=.o)
OUT = diskscope
PREFIX = /usr/local

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(OUT)
	@echo "Running basic tests..."
	./$(OUT) . -d 1 > /dev/null 2>&1 && echo "  [PASS] Default scan"
	./$(OUT) . -d 0 > /dev/null 2>&1 && echo "  [PASS] Depth 0 scan"
	./$(OUT) . --exclude .git -d 1 > /dev/null 2>&1 && echo "  [PASS] Exclude pattern"
	./$(OUT) . --export json > /dev/null 2>&1 && echo "  [PASS] JSON export"
	./$(OUT) . --export csv > /dev/null 2>&1 && echo "  [PASS] CSV export"
	./$(OUT) --help > /dev/null 2>&1 && echo "  [PASS] --help flag"
	./$(OUT) --version > /dev/null 2>&1 && echo "  [PASS] --version flag"
	@echo "All tests passed."

install: $(OUT)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(OUT) $(DESTDIR)$(PREFIX)/bin/$(OUT)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(OUT)

clean:
	rm -f $(OUT) $(OBJ)

.PHONY: all test install uninstall clean
