# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LIBS = -lSDL2 -lm

# Build directory
BUILD_DIR = build

# Target executable
TARGET = $(BUILD_DIR)/demofx

# Source files
SOURCES = main.c font.c menu.c transitions.c plasma.c fire.c tunnel.c starfield.c scroller.c cube.c torus.c raster.c twister.c rotozoom.c metaballs.c dottunnel.c vectorballs.c textwriter.c synth.c sinescroller_large.c metaballs3d.c ripple.c voxel.c bumpmap.c kaleidoscope.c raytracer.c sierpinski.c particles.c tesseract.c matrix.c matrixcode.c lens.c voronoi.c spiky_twister.c sdf_march.c

# Object files
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link the executable
$(TARGET): $(BUILD_DIR) $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)

# Compile source files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Rebuild everything
rebuild: clean all

# Run the program
run: $(TARGET)
	$(TARGET)

.PHONY: all clean rebuild run
