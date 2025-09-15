# FatFS for MatrixOS

This directory contains a copy of the FatFS library with MatrixOS-specific configuration.

## Source

Original FatFS library: http://elm-chan.org/fsw/ff/00index_e.html
GitHub mirror: https://github.com/abbrev/fatfs

## MatrixOS Modifications

- **`ffconf.h`**: Custom configuration with Long File Name (LFN) support enabled
  - `FF_USE_LFN = 2` - Enables long filenames with dynamic stack buffer
  - `FF_CODE_PAGE = 437` - U.S. ASCII code page