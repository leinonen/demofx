# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LIBS = -lSDL2 -lm

# Target executable
TARGET = demofx

# Source files
SOURCES = main.c plasma.c fire.c tunnel.c starfield.c scroller.c cube.c torus.c raster.c twister.c rotozoom.c metaballs.c dottunnel.c vectorballs.c textwriter.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Rebuild everything
rebuild: clean all

# Run the program
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean rebuild run
