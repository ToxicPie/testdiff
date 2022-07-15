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

## description

- use some simple heuristics to match basic blocks in each function
- match nodes and edges in each function's CFG
- match all functions in a binary file using bipartite matching

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

# references

- Qian Feng, Rundong Zhou, Chengcheng Xu, Yao Cheng, Brian Testa, and Heng Yin. 2016. Scalable Graph-based Bug Search for Firmware Images. In Proceedings of the 2016 ACM SIGSAC Conference on Computer and Communications Security (CCS '16). Association for Computing Machinery, New York, NY, USA, 480–491. https://doi.org/10.1145/2976749.2978370
- Dullien, Thomas & Rolles, Rolf. (2005). Graph-based comparison of executable objects (english version). SSTIC. 5.
