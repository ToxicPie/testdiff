#ifndef _CFG_HPP
#define _CFG_HPP

#include <r_anal.h>

#include <filesystem>
#include <vector>


struct BasicBlock {
    static const std::vector<uint64_t> primes;
    // some stats about this block
    int dist, in_deg, inst_cnt, call_cnt;
    uint64_t address;
    // small prime product, as described in the paper
    // "Graph-based comparison of Executable Objects"
    uint64_t inst_hash;
    std::basic_string<uint8_t> instrs;
    BasicBlock() = delete;
    explicit BasicBlock(const RAnalBlock *block);
};

struct Function {
    std::vector<BasicBlock> blocks;
    // basic block graph; adjacency list
    std::vector<std::vector<int>> edges, redges;
    Function() = delete;
    explicit Function(const RAnalFunction *func);
};

struct BinaryFile {
    std::vector<Function> funcs;
    BinaryFile() = delete;
    explicit BinaryFile(const std::filesystem::path &filename);
};


#endif // _CFG_HPP
