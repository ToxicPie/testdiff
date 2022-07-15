# test program to analyze binaries

## setup

1. install [radare2](https://github.com/radareorg/radare2) headers and library
2. run `make`

## usage

```
./testdiff <file1> <file2> | grep -F "[match]"
```

it prints the following info
- debug messages about blocks
- matched functions and their similarities

## TODO

- consider function call graphs too
    - currently, expanding graphs makes the entire CFG too large to handle
- optimize or use approximate graph matching
    - the current algorithm matches n edges in O(n^3) time
- implement the following for better feature matching:
    - match function calls (especially library functions)
    - match data and string references (not just the counts)
    - diff instructions
- fix bugs
- fix bugs in radare2
