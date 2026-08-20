# Nuterm

__Nuterm__ is a lightweight C library for terminal control: for each terminal function(changing color, style, clearing display, etc.) it emits the correct escape codes(based on auto-detected terminal emulator and how many colors it supports). It offers optional output buffering for smooth redraws, unifies input events (key presses, mouse clicks...) and a simple main loop abstraction that allows custom events.

## Requirements

- C99 or newer hosted implementation
- Platform support for int8\_t and int32\_t

## Dependencies

This library relies on [UConv](https://github.com/novak-stevanovic/uconv) for UTF-32 conversion needs. This is bundled internally.

## Makefile instructions:

To compile and install the library system-wide, do `sudo make && make install`. Makefile is configurable.

## Usage instructions:

Compile your project with flags: `$(pkgconf --cflags nuterm)` and link with flags: `$(pkgconf --libs nuterm)`. For this to work, make sure that pkg-config searches in the directory of the .pc file generated in the installation process. See demo.c for usage examples.
