
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
SRC = $(shell find . -maxdepth 1 -type f -name "*.c" | sort) $(EMBED) $(ALTER)
OBJ = $(SRC:.c=.o)
HDR = $(shell find . -maxdepth 1 -type f -name "*.h")
ALTER_OBJ = $(ALTER:.c=.o)
.PHONY: all debug clean static musl-static

all: CFLAGS += -lncurses
all: $(TARGET) $(TARGET_CLIENT)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(OBJ)
	$(CXX) $(CFLAGS) -o $(DEBUG_TARGET) $^

static: CFLAGS += -static -DSTATIC
LIB = ncurses-6.5/lib/libncursesw.a
static: $(OBJ) $(TARGET_CLIENT)
	@if echo "$(FLAG)" | grep -q -- "-DALTER"; then \
		echo "skip downloading ncurses..."; \
		$(CXX) -o $(TARGET) $(OBJ) $(CFLAGS); \
	else \
		export CFLAGS="-g -static"; \
		if [ ! -d ncurses-6.5 ]; then \
			wget https://ftp.gnu.org/gnu/ncurses/ncurses-6.5.tar.gz; \
			tar -xf ncurses-6.5.tar.gz; \
			rm ncurses-6.5.tar.gz; \
			cd ncurses-6.5 && ./configure --enable-static; \
		fi; \
		cd ncurses-6.5 && make; cd ..;\
		$(CXX) -o $(TARGET) $(OBJ) $(LIB) $(CFLAGS); \
	fi
musl-static: $(OBJ) $(TARGET_CLIENT)
	export CFLAGS="-g -static"
	if [ ! -d ncurses-6.5 ]; then wget https://ftp.gnu.org/gnu/ncurses/ncurses-6.5.tar.gz; tar -xf ncurses-6.5.tar.gz; rm ncurses-6.5.tar.gz; cd ncurses-6.5 && ./configure --enable-static; fi
	cd ncurses-6.5 && make
	musl-gcc -o $(TARGET) $(OBJ) $(CFLAGS) $(LIB) -static
alter: CFLAGS += -I./alter -DALTER
alter: $(OBJ)
	$(CXX) -o $(TARGET) $(OBJ) $(CFLAGS)
$(TARGET): $(OBJ)
	$(CXX)  -o $@ $^ $(CFLAGS)

$(TARGET_CLIENT): $(CLIENT_SRC)
	$(CXX) -o $@ $^

%.o: %.c $(HDR)
	$(CXX)  -c $< -o $@ $(CFLAGS)

# Clean all builds
clean:
	rm -f $(OBJ) $(TARGET) $(ALTER_OBJ) $(TARGET_CLIENT) $(DEBUG_TARGET) *.h.gch readline/*.h.gch alter/*.h.gch


