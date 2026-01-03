# Nuterm

__Nuterm__ is a lightweight C library for terminal control: for each terminal function(changing color, style, clearing display, etc.) it emits the correct escape codes(based on auto-detected terminal emulator and how many colors it supports). It offers optional output buffering for smooth redraws, unifies input events (key presses and window resizes) and a simple main loop abstraction.

## Dependencies

This library relies on [UConv](https://github.com/novak-stevanovic/uconv) for UTF-32 conversion needs. To fetch compile and link flags, pkg-config is used. This means that, when compiling the library, pkg-config must be able to find UConv's .pc file.

## Makefile instructions:

1. `make [PC_WITH_PATH=...] [LIB_TYPE=so/ar] [OPT={0...3}]` - This will compile the source files and build the library file. If the library depends on packages discovered via pkg-config, you can specify where to search for their .pc files, in addition to `PKG_CONFIG_PATH`.
2. `make install [LIB_TYPE=so/ar] [PREFIX=...] [PC_PREFIX=...]` - This will place the public headers inside `PREFIX/include` and the built library file inside `PREFIX/lib`. This will also place the .pc file inside `PC_PREFIX`.

Default options are `PREFIX=/usr/local`, `PC_PREFIX=PREFIX/lib/pkgconfig`, `OPT=3`, `LIB_TYPE=so`.

## Usage instructions:

Compile your project with flags: `$(pkgconf --cflags nuterm)` and link with flags: `$(pkgconf --libs nuterm)`. For this to work, make sure that pkg-config seaches in the directory `PC_PREFIX` when using pkg-config.
