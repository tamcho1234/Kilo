# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

A from-scratch C implementation of the `kilo` text editor (following the antirez/snaptoken "Build Your Own Text Editor" tutorial). Currently at the earliest stage: `kilo.c` only reads bytes from stdin in raw-style loop until `q`. Expect the file to grow into a single-file editor with terminal raw mode, row buffer, rendering, and file I/O — keep everything in `kilo.c` rather than splitting into modules.

## Build & Run

```sh
make        # builds ./kilo with -Wall -Wextra -pedantic -std=c99
./kilo
```

There is no test suite, linter, or formatter configured. The compiler warning flags in the Makefile are the only correctness gate — keep builds warning-clean.
