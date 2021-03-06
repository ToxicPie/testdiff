#include "cfg.hpp"

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <stdexcept>

#include <r_core.h>


const std::vector<uint64_t> BasicBlock::primes = [] {
    // sieve
    constexpr size_t MAX_PRIME = 1'000;
    std::vector<uint64_t> primes;
    std::bitset<MAX_PRIME> is_prime;
    is_prime.set();
    for (uint64_t x = 2; x < MAX_PRIME; x++) {
        if (is_prime[x]) {
            primes.push_back(x);
        }
        for (const auto &y : primes) {
            if (x * y >= MAX_PRIME) {
                break;
            }
            is_prime[x * y] = false;
        }
    }
    // remove 2; keep only odd primes
    primes.erase(primes.begin());
    return primes;
}();

BasicBlock::BasicBlock(const RAnalBlock *block)
    : dist{}, in_deg{}, inst_cnt{}, call_cnt{}, address{block->addr}, inst_hash{
                                                                          1} {
    auto *op = r_anal_op_new();
    auto *anal = block->anal;
    std::basic_string<uint8_t> buffer(block->size, 0);

    anal->iob.read_at(anal->iob.io, block->addr, &buffer[0], block->size);

    for (int idx = 0; idx < block->size;) {
        while (idx < block->size) {
            int oplen = r_anal_op(anal, op, 0, &buffer[idx], block->size - idx,
                                  R_ANAL_OP_MASK_BASIC);
            if (oplen < 1) {
                break;
            }
            instrs += std::basic_string(&buffer[idx], &buffer[idx + oplen]);
            // max op type is ~47
            inst_hash *= primes[op->type & 0xffU];

            idx += oplen;
            inst_cnt += 1;
            if (op->type == R_ANAL_OP_TYPE_CALL) {
                call_cnt += 1;
            }

            // analyze refs
            auto *ref_list = r_anal_refs_get(block->anal, address + idx);
            // for some reason, r2 returns null for an empty ref list
            if (ref_list != nullptr) {
                for (auto *iter = ref_list->head; iter != nullptr;
                     iter = iter->n) {
                    auto *ref = reinterpret_cast<RAnalRef *>(iter->data);
                    uint64_t ref_addr = ref->addr;
                    switch (ref->type) {
                    case R_ANAL_REF_TYPE_NULL: break;
                    case R_ANAL_REF_TYPE_CODE:
                    case R_ANAL_REF_TYPE_CALL:
                        code_refs.push_back(ref_addr);
                        break;
                    case R_ANAL_REF_TYPE_DATA:
                        data_refs.push_back(ref_addr);
                        break;
                    case R_ANAL_REF_TYPE_STRING:
                        str_refs.push_back(ref_addr);
                        break;
                    }
                    std::cout << "[info] " << std::hex << address + idx
                              << " references " << ref_addr << "\n";
                }
                r_list_free(ref_list);
            }
        }
    }

    r_anal_op_free(op);

    std::sort(instrs.begin(), instrs.end());
    std::cout << "[info] " << std::hex << address << " hash is " << inst_hash
              << "\n";
}

Function::Function(const RAnalFunction *func) : address(func->addr) {
    r_list_sort(func->bbs, [](const void *x, const void *y) {
        const auto *a = reinterpret_cast<const RAnalBlock *>(x);
        const auto *b = reinterpret_cast<const RAnalBlock *>(y);
        if (a->addr > b->addr) {
            return 1;
        }
        if (a->addr == b->addr) {
            return 0;
        }
        return -1;
    });

    // basic block address -> index mappings
    std::map<uint64_t, int> addrs;
    // addr -> addr edges
    std::vector<std::pair<uint64_t, uint64_t>> edge_list;

    auto *fcns = func->bbs;
    for (auto *iter = fcns->head; iter != nullptr; iter = iter->n) {
        auto *block = reinterpret_cast<RAnalBlock *>(iter->data);
        blocks.emplace_back(block);
        addrs.emplace(block->addr, blocks.size() - 1);
        edge_list.emplace_back(block->addr, block->jump);
        edge_list.emplace_back(block->addr, block->fail);
    }

    // build function cfg
    edges.resize(blocks.size());
    redges.resize(blocks.size());
    for (const auto &[from, to] : edge_list) {
        if (auto it1 = addrs.find(from), it2 = addrs.find(to);
            it1 != addrs.end() && it2 != addrs.end()) {
            edges[it1->second].push_back(it2->second);
            redges[it2->second].push_back(it1->second);
            std::cout << "[info] " << std::hex << it1->first << " -> "
                      << it2->first << "\n";
        }
    }

    // bfs on cfg
    std::vector<int> dists(blocks.size(), INT_MAX);
    int start = addrs[func->addr];
    dists[start] = 0;

    std::queue<int> queue;
    queue.push(start);
    while (!queue.empty()) {
        int cur = queue.front();
        queue.pop();
        if (dists[cur] != INT32_MAX) {
            for (int next : edges[cur]) {
                if (dists[next] == INT32_MAX) {
                    dists[next] = dists[cur] + 1;
                    queue.push(next);
                }
            }
        }
    }

    for (int i = 0; i < blocks.size(); i++) {
        blocks[i].in_deg = redges[i].size();
        blocks[i].dist = dists[i];
    }
}

BinaryFile::BinaryFile(const std::filesystem::path &filename) {
    auto *core = r_core_new();

    r_core_loadlibs(core, R_CORE_LOADLIBS_ALL, nullptr);
    r_config_set_i(core->config, "scr.interactive", 0);

    const auto *file = r_core_file_open(core, filename.c_str(), 0, 0);
    if (file == nullptr) {
        r_core_free(core);
        throw std::runtime_error("Couldn't open file");
    }

    // load and analyze all
    r_core_bin_load(core, nullptr, UINT64_MAX);
    r_core_bin_update_arch_bits(core);
    r_core_cmd_str(core, "aaa");
    r_cons_flush();

    auto *fcns = r_anal_get_fcns(core->anal);
    for (auto *iter = fcns->head; iter != nullptr; iter = iter->n) {
        auto *func = reinterpret_cast<RAnalFunction *>(iter->data);
        funcs.emplace_back(func);
    }

    r_core_free(core);
}
