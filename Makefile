
# Compiler and flags
CXX = gcc
FLAG= 
CFLAGS = -Wall -g -lreadline $(FLAG)
DEBUG_FLAGS = -O0 -DDEBUG
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

.PHONY: all daemonize debug clean pool static

# Default: Release build
all: $(TARGET) $(TARGET_CLIENT)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(OBJ)
	$(CXX) $(CFLAGS) -o $(DEBUG_TARGET) $^

daemonize: CFLAGS += -DDAEMON
daemonize: $(OBJ)
	$(CXX) $(CFLAGS) -o $(TARGET) $^

static: $(OBJ)
	export CC="musl-gcc"
	export CFLAGS="-g -static"
	if [ ! -d ncurses-6.5 ]; then wget https://ftp.gnu.org/gnu/ncurses/ncurses-6.5.tar.gz; tar -xf ncurses-6.5.tar.gz; cd ncurses-6.5 && ./configure --enable-static; fi
	cd ncurses-6.5 && make
	if [ ! -d readline-8.2 ]; then wget https://mirrors.dotsrc.org/gnu/readline/readline-8.2.tar.gz; tar -xf readline-8.2.tar.gz; cd readline-8.2 && ./configure --enable-static; fi
	cd readline-8.2 && make static
	gcc -g -o $(TARGET) $(OBJ) readline-8.2/libreadline.a ncurses-6.5/lib/libncursesw.a
pool: CFLAGS += -DMYHEAP
pool: all
# Release build of main target
$(TARGET): $(OBJ)
	$(CXX) $(CFLAGS) $(RELEASE_FLAGS) -o $@ $^

# Client target (always release, static)
$(TARGET_CLIENT): $(CLIENT_SRC)
	$(CXX) -o $@ $^

# Rule for compiling .c to .o
%.o: %.c $(HDR)
	$(CXX) $(CFLAGS) -c $< -o $@

# Clean all builds
clean:
	rm -f $(OBJ) $(TARGET) $(TARGET_CLIENT) $(DEBUG_TARGET) *h.gch


