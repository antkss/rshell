# ... (Existing Makefile content) ...
# Compiler and flags
CXX = gcc
CXXFLAGS = -g -lreadline

# Target executable name
TARGET = rshell

# Source files
SRC = $(shell find . -depth -maxdepth 1 -type f -name "*.c" | sort | tr "\n" " ")
# Object files (same names as source files but with .o extension)
OBJ = $(SRC:.c=.o)

# Header files (not directly used in compilation, but for dependency checking)
HDR = $(shell find . -depth -maxdepth 1 -type f -name "*.h" | sort | tr "\n" " ")
.PHONY: all depend 
all: $(TARGET)
# Default rule to build the target
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ 

# Rule to compile .c files into .o files
%.o: %.c $(HDR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(CXXFLAGS)

# Rule to clean up object files and executable
clean:
	rm -f $(OBJ) $(TARGET)
# Phony targets


# ... (Rest of the Makefile) ...
