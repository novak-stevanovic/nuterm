# Nuterm

__Nuterm__ is a lightweight C library for terminal control: for each terminal function(changing color, style, clearing display, etc.) it emits the correct escape codes(based on auto-detected terminal emulator and how many colors it supports). It offers optional output buffering for smooth redraws, unifies input events (key presses and window resizes) without pulling in larger dependencies.

## Makefile instructions:

1. `make [LIB_TYPE=so/ar] [OPT={0...3}]` - This will compile the source files(and thirdparty dependencies, if they exist) and build the library file.
2. `make install [LIB_TYPE=so/ar] [PREFIX={prefix}] [PC_PREFIX={pc_prefix}]` - This will place the public headers inside _{prefix}/include_ and the built library file inside _{prefix}/lib_.

Default options are `PREFIX=/usr/local`, `OPT=2`, `LIB_TYPE=so`, `PC_PREFIX=/usr/local/lib/pkgconfig`.

## Usage instructions:

To use the library in your project, you must first install it. This can be done on your system - globally, or locally, inside a project that is using this library.
1. Install with desired `PREFIX` and `PC_PREFIX`.
2. Compile your project with cflags: `pkfconf --cflags nuterm` and link with flags: `pkfconf --libs nuterm`. For this to work, make sure that pkgconf seaches in the directory `PC_PREFIX` when using pkgconfig.
