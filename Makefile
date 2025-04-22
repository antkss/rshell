
# Compiler and flags
CXX = gcc
CXXFLAGS = -Wall -lreadline -ltcmalloc
DEBUG_FLAGS = -g -O0 -DDEBUG
RELEASE_FLAGS = -O2

# Target names
TARGET = rshell
CLIENT_SRC = cli_src/client.c
TARGET_CLIENT = client
DEBUG_TARGET = debug

# Source and headers
SRC = $(shell find . -maxdepth 1 -type f -name "*.c" | sort)
OBJ = $(SRC:.c=.o)
HDR = $(shell find . -maxdepth 1 -type f -name "*.h")

.PHONY: all daemonize debug clean

# Default: Release build
all: $(TARGET) $(TARGET_CLIENT)

# Debug target
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(DEBUG_TARGET) $^
daemonize: CXXFLAGS += -DDAEMON
daemonize: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $^
# Release build of main target
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) -o $@ $^

# Client target (always release, static)
$(TARGET_CLIENT): $(CLIENT_SRC)
	$(CXX) -static -o $@ $^

# Rule for compiling .c to .o
%.o: %.c $(HDR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean all builds
clean:
	rm -f $(OBJ) $(TARGET) $(TARGET_CLIENT) $(DEBUG_TARGET)

