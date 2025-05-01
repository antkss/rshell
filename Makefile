
# Compiler and flags
CXX = gcc
FLAG= 
CFLAGS = -Wall -g -I./readline $(FLAG)
DEBUG_FLAGS = -O0 -DDEBUG

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
ALTER_OBJ = $(ALTER:.c=.o)
.PHONY: all debug clean static musl-static

all: CFLAGS += -lncurses
all: $(TARGET) $(TARGET_CLIENT)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(OBJ)
	$(CXX) $(CFLAGS) -o $(DEBUG_TARGET) $^

static: CFLAGS += ncurses-6.5/lib/libncursesw.a
static: $(OBJ) 
	export CFLAGS="-g -static"
	if [ ! -d ncurses-6.5 ]; then wget https://ftp.gnu.org/gnu/ncurses/ncurses-6.5.tar.gz; tar -xf ncurses-6.5.tar.gz; rm ncurses-6.5.tar.gz; cd ncurses-6.5 && ./configure --enable-static; fi
	cd ncurses-6.5 && make
	$(CXX) -o $(TARGET) $(OBJ) $(CFLAGS)
musl-static: $(OBJ)
	export CFLAGS="-g -static"
	if [ ! -d ncurses-6.5 ]; then wget https://ftp.gnu.org/gnu/ncurses/ncurses-6.5.tar.gz; tar -xf ncurses-6.5.tar.gz; rm ncurses-6.5.tar.gz; cd ncurses-6.5 && ./configure --enable-static; fi
	cd ncurses-6.5 && make
musl-static:
	musl-gcc -o $(TARGET) $(OBJ) $(CFLAGS) ncurses-6.5/lib/libncursesw.a -static
alter: CFLAGS += -I./alter
alter: $(OBJ) $(ALTER_OBJ)
	$(CXX) -o $(TARGET) $(OBJ) $(ALTER_OBJ) $(CFLAGS)
$(TARGET): $(OBJ)
	$(CXX)  -o $@ $^ $(CFLAGS)

$(TARGET_CLIENT): $(CLIENT_SRC)
	$(CXX) -o $@ $^

%.o: %.c $(HDR)
	$(CXX)  -c $< -o $@ $(CFLAGS)

# Clean all builds
clean:
	rm -f $(OBJ) $(TARGET) $(ALTER_OBJ) $(TARGET_CLIENT) $(DEBUG_TARGET) *.h.gch readline/*.h.gch alter/*.h.gch


