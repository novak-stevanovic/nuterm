# Nuterm

__Nuterm__ is a lightweight C library for terminal control: for each terminal function(changing color, style, clearing display, etc.) it emits the correct escape codes(based on auto-detected terminal emulator and how many colors it supports). It offers optional output buffering for smooth redraws, unifies input events (key presses and window resizes) without pulling in large dependencies and provides built-in support for binding specific key events to key event handlers.

## Makefile instructions:

1. `make [LIB_TYPE=so/ar] [OPT={0...3}]` - This will compile the source files(and thirdparty dependencies, if they exist) and build the library file.
2. `make install [PREFIX={prefix}] [LIB_TYPE=so/ar] [PC_PREFIX={pc_prefix}]` - This will place the public headers inside _{prefix}/include_ and the built library file inside _{prefix}/lib_.

Default options are `PREFIX=/usr/local`, `OPT=2`, `LIB_TYPE=so`, `PC_PREFIX=/usr/local/lib/pkgconfig`.

## Usage instructions:

To use the library in your project, install the library on your system and make sure to compile with flags: `$(pkgconf --cflags nuterm)` and link with flags `$(pkgconf --libs nuterm)`.
