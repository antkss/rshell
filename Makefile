
# Compiler and flags
CXX = gcc
FLAG= 
CFLAGS = -Wall -g $(FLAG) -I./readline  
DEBUG_FLAGS = -O0 -DDEBUG
RELEASE_FLAGS = -O2

# Target names
TARGET = rshell
CLIENT_SRC = cli_src/client.c
TARGET_CLIENT = client
DEBUG_TARGET = debug

# Source and headers
EMBED = $(shell find ./readline -maxdepth 1 -type f -name "*.c" | sort) 
ALTER = $(shell find ./alter -maxdepth 1 -type f -name "*.c" | sort)
SRC = $(shell find . -maxdepth 1 -type f -name "*.c" | sort) $(EMBED)
OBJ = $(SRC:.c=.o)
HDR = $(shell find . -maxdepth 1 -type f -name "*.h")

.PHONY: all daemonize debug clean pool static musl-static alter

# Default: Release build
all: CFLAGS += -lncurses
all: $(TARGET) $(TARGET_CLIENT)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(OBJ)
	$(CXX) $(CFLAGS) -o $(DEBUG_TARGET) $^

daemonize: CFLAGS += -DDAEMON
daemonize: $(OBJ)
	$(CXX) $(CFLAGS) -o $(TARGET) $^

alter: CFLAGS += -I./alter
alter: $(OBJ) $(ALTER:.c=.o)
	$(CXX) $(RELEASE_FLAGS) -o $(TARGET) $^ $(CFLAGS)

static: $(OBJ)
	export CFLAGS="-g -static"
	if [ ! -d ncurses-6.5 ]; then wget https://ftp.gnu.org/gnu/ncurses/ncurses-6.5.tar.gz; tar -xf ncurses-6.5.tar.gz; rm ncurses-6.5.tar.gz; cd ncurses-6.5 && ./configure --enable-static; fi
	cd ncurses-6.5 && make
	gcc -g -o $(TARGET) $(OBJ) ncurses-6.5/lib/libncursesw.a
musl-static: $(OBJ)
	export CFLAGS="-g -static"
	if [ ! -d ncurses-6.5 ]; then wget https://ftp.gnu.org/gnu/ncurses/ncurses-6.5.tar.gz; tar -xf ncurses-6.5.tar.gz; rm ncurses-6.5.tar.gz; cd ncurses-6.5 && ./configure --enable-static; fi
	cd ncurses-6.5 && make
	musl-gcc -g -o $(TARGET) $(OBJ) ncurses-6.5/lib/libncursesw.a -static
pool: CFLAGS += -DMYHEAP
pool: all
# Release build of main target
$(TARGET): $(OBJ)
	$(CXX)  $(RELEASE_FLAGS) -o $@ $^ $(CFLAGS)

# Client target 
$(TARGET_CLIENT): $(CLIENT_SRC)
	$(CXX) -o $@ $^

# Rule for compiling .c to .o
%.o: %.c $(HDR)
	$(CXX)  -c $< -o $@ $(CFLAGS)

# Clean all builds
clean:
	rm -f $(OBJ) $(TARGET) $(TARGET_CLIENT) $(DEBUG_TARGET) *.h.gch readline/*.h.gch alter/*.h.gch


