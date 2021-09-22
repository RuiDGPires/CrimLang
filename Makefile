TARGET=crimlang
CC=gcc

C_FILES = $(wildcard src/*.cpp)
H_FILES = $(wildcard src/*.hpp)

default: debug

$(TARGET)_dbg: $(C_FILES) $(H_FILES)
	@$(CC) -Wall -O0 -D DEBUG -g $^ -o $@

$(TARGET): $(C_FILES) $(H_FILES)
	@$(CC) -Wall -O3 $(CFLAGS) $^ -o $@

debug: $(TARGET)_dbg

release: $(TARGET)

clean:
	@rm -f $(TARGET)*
