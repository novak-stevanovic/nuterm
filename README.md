# Nuterm

__Nuterm__ is a lightweight C library for terminal control: for each terminal function(changing color, style, clearing display, etc.) it emits the correct escape codes(based on auto-detected terminal emulator and how many colors it supports). It offers optional output buffering for smooth redraws, unifies input events (key presses and window resizes) without pulling in large dependencies and provides built-in support for binding specific
key events to key event handlers.

## Makefile instructions:

1. make \[LIB\_TYPE=so/ar\] \[OPT={0...3}\] - This will compile the source files(and thirdparty dependencies, if they exist) and build the .so/.a file.
2. make install \[PREFIX={prefix}\] \[LIB\_TYPE=so/ar\] - This will place the public headers inside PREFIX/include and the built .so/.a file inside PREFIX/lib.

Default options are PREFIX=/usr/local, OPT=2, LIB\_TYPE=so.
