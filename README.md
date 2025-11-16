# Old School Demo Effects

Classic demoscene effects written in C using SDL2, running at the authentic 320x200 resolution.

## Effects Included

1. **Plasma** - Animated colorful plasma waves using sine/cosine patterns
2. **Fire** - Classic rising fire effect with realistic flame propagation
3. **Tunnel** - 3D textured tunnel effect with precalculated lookup tables
4. **Starfield** - 3D starfield simulation with depth-based brightness
5. **Sine Scroller** - Horizontal text scroller with sine wave distortion
6. **Rotating Cube** - 3D wireframe cube with rotation on all axes
7. **Rotating Torus** - 3D filled torus with flat shading and lighting
8. **Raster Bars** - Classic Amiga-style colored bars with sine movement
9. **Twister** - Vertical column twist effect with animated texture rotation
10. **Rotozoom** - Rotating and zooming textured plane with checkerboard pattern
11. **Metaballs** - Organic blob effect with distance field calculations
12. **Dot Tunnel** - 3D tunnel made of individual colored dots with depth-based sizing
13. **Vector Balls** - 3D rotating sphere made of 600 colored dots with perspective projection

## Requirements

- GCC compiler
- SDL2 development libraries
- Linux/Unix environment (or WSL on Windows)

### Installing SDL2

**Ubuntu/Debian:**
```bash
sudo apt-get install libsdl2-dev
```

**Fedora:**
```bash
sudo dnf install SDL2-devel
```

**Arch Linux:**
```bash
sudo pacman -S sdl2
```

**macOS (with Homebrew):**
```bash
brew install sdl2
```

## Building

Simply run:
```bash
make
```

To clean build artifacts:
```bash
make clean
```

To rebuild from scratch:
```bash
make rebuild
```

## Running

```bash
./demofx
```

Or build and run in one step:
```bash
make run
```

## Controls

- **0** - Switch to Rotozoom effect
- **1** - Switch to Plasma effect
- **2** - Switch to Fire effect
- **3** - Switch to Tunnel effect
- **4** - Switch to Starfield effect
- **5** - Switch to Sine Scroller effect
- **6** - Switch to Rotating Cube effect
- **7** - Switch to Rotating Torus effect
- **8** - Switch to Raster Bars effect
- **9** - Switch to Twister effect
- **M** - Switch to Metaballs effect
- **D** - Switch to Dot Tunnel effect
- **V** - Switch to Vector Balls effect
- **ESC** - Quit

## Technical Details

- **Resolution:** 320x200 (classic Mode 13h), scaled 3x for modern displays
- **Frame rate:** Capped at ~60 FPS
- **Rendering:** Software pixel buffer rendered to SDL2 texture
- **Optimization:** Precalculated lookup tables for sine/cosine and tunnel mapping

## Project Structure

```
demofx/
├── main.c          - Main loop, SDL setup, effect switching
├── common.h        - Shared definitions and constants
├── plasma.c/h      - Plasma effect implementation
├── fire.c/h        - Fire effect implementation
├── tunnel.c/h      - Tunnel effect implementation
├── starfield.c/h   - Starfield effect implementation
├── scroller.c/h    - Sine scroller effect implementation
├── cube.c/h        - Rotating cube effect implementation
├── torus.c/h       - Rotating torus effect implementation
├── raster.c/h      - Raster bars effect implementation
├── twister.c/h     - Twister effect implementation
├── rotozoom.c/h    - Rotozoom effect implementation
├── metaballs.c/h   - Metaballs effect implementation
├── dottunnel.c/h   - Dot tunnel effect implementation
├── vectorballs.c/h - Vector balls effect implementation
├── Makefile        - Build configuration
└── README.md       - This file
```

## Customization

You can easily modify the effects:

- **Resolution:** Change `SCREEN_WIDTH` and `SCREEN_HEIGHT` in common.h:1
- **Window scale:** Adjust `SCALE_FACTOR` in common.h:1
- **Colors:** Modify palette generation in each effect's `*_init()` function
- **Speed:** Adjust time divisors in each effect's `*_update()` function

## License

Free to use for learning and experimentation. Have fun!
