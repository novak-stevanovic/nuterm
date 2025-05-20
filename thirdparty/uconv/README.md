# U-conv

__U-conv__ is a tiny C library for converting between UTF-8 and UTF-32 text. It provides functions to decode a byte sequence into Unicode code points and to encode code points back into UTF-8. It is designed with three things in mind: control, proper error handling and simplicity.

## Makefile instructions:

1. make \[LIB\_TYPE=shared/archive\] - This will compile the source files and build the .so/.a file.
2. make install \[PREFIX={prefix}\] \[LIB\_TYPE=shared/archive\] - This will place the header file inside _prefix_/include and the built .so/.a file inside _prefix_/lib.

_Default options are PREFIX=/usr/local and default LIB\_TYPE=shared._
