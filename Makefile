TARGET=crimlang
CC=g++

VM_FILES = $(wildcard CrimVm/src/*.c)
CPP_FILES = $(wildcard src/*.cpp)
HPP_FILES = $(wildcard src/*.hpp)

default: debug

$(TARGET)_dbg: $(CPP_FILES) $(VM_FILES) $(HPP_FILES)
	@$(CC) -Wall -fpermissive -pthread -O0 -D DEBUG -g $^ -o $@

$(TARGET): $(CPP_FILES) $(VM_FILES) $(HPP_FILES)
	@$(CC) -Wall -fpermissive -pthread -O3 $(CFLAGS) $^ -o $@

debug: $(TARGET)_dbg

release: $(TARGET)

clean:
	@rm -f $(TARGET)*
