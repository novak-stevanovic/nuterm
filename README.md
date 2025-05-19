# Nu-term

__Nu-term__ is a lightweight C library for terminal control: for each terminal function(changing color, style, clearing display, etc.) it emits the correct escape codes(based on auto-detected terminal emulator and how many colors it supports). It offers optional output buffering for smooth redraws, unifies input events (key presses and window resizes) without pulling in large dependencies and provides built-in support for binding specific
key events to key event handlers.

## Makefile instructions:

1. make \[LIB\_TYPE=shared/archive\] - This will compile the source files and build the .so/.a file.
2. make install \[PREFIX={prefix}\] \[LIB\_TYPE=shared/archive\] - This will place the public headers inside _prefix_/include and the built .so/.a file inside _prefix_/lib.

_Default options are PREFIX=/usr/local and default LIB\_TYPE=shared._
