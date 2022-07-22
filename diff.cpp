#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "cfg.hpp"
#include "diff.hpp"
#include "matching.hpp"


double similarity(long long x, long long y, double thres) {
    if (x == y) {
        return 1.0;
    }
    double ratio = x < y ? double(x) / y : double(y) / x;
    if (ratio <= thres) {
        return 0.0;
    }
    return pow((ratio - thres) / (1.0 - thres), 2);
}

double similarity(const BasicBlock &a, const BasicBlock &b) {
    // weights are arbitrarily selected
    constexpr double HASH_WEIGHT = 10.0;
    constexpr double CALL_WEIGHT = 8.0;
    constexpr double SIZE_WEIGHT = 6.0;
    constexpr double CODE_REF_WEIGHT = 3.0;
    constexpr double DATA_REF_WEIGHT = 2.0;
    constexpr double STR_REF_WEIGHT = 2.0;
    constexpr double DIST_WEIGHT = 4.0;
    constexpr double DEG_WEIGHT = 5.0;
    constexpr double TOTAL_WEIGHT = HASH_WEIGHT + CALL_WEIGHT + SIZE_WEIGHT +
                                    CODE_REF_WEIGHT + DATA_REF_WEIGHT +
                                    STR_REF_WEIGHT + DIST_WEIGHT + DEG_WEIGHT;

    double hash_score = a.inst_hash == b.inst_hash ? HASH_WEIGHT : 0.0;
    double call_score = CALL_WEIGHT * similarity(a.call_cnt, b.call_cnt, 0.0);
    double size_score = SIZE_WEIGHT * similarity(a.inst_cnt, b.inst_cnt, 0.0);
    double code_ref_score =
        CODE_REF_WEIGHT *
        similarity(a.code_refs.size(), b.code_refs.size(), 0.0);
    double data_ref_score =
        DATA_REF_WEIGHT *
        similarity(a.data_refs.size(), b.data_refs.size(), 0.0);
    double str_ref_score =
        STR_REF_WEIGHT * similarity(a.str_refs.size(), b.str_refs.size(), 0.0);
    double dist_score = DIST_WEIGHT * similarity(a.dist, b.dist, 0.0);
    double deg_score = DEG_WEIGHT * similarity(a.in_deg, b.in_deg, 0.0);
    double total_score = hash_score + call_score + size_score + code_ref_score +
                         data_ref_score + str_ref_score + dist_score +
                         deg_score;

    return sqrt(total_score / TOTAL_WEIGHT);
}

double similarity(const Function &a, const Function &b) {
    std::vector<std::pair<int, int>> a_edges, b_edges;

    for (int i = 0; i < a.edges.size(); i++) {
        for (auto j : a.edges[i]) {
            a_edges.emplace_back(i, j);
        }
    }
    for (int i = 0; i < b.edges.size(); i++) {
        for (auto j : b.edges[i]) {
            b_edges.emplace_back(i, j);
        }
    }

    bool swapped_v = a.blocks.size() > b.blocks.size();

    const Function &left_v = swapped_v ? b : a;
    const Function &right_v = swapped_v ? a : b;

    std::vector vert_weights(left_v.blocks.size(),
                             KACTL::vd(right_v.blocks.size()));
    for (int i = 0; i < left_v.blocks.size(); i++) {
        for (int j = 0; j < right_v.blocks.size(); j++) {
            double vertex_similarity =
                similarity(left_v.blocks[i], right_v.blocks[j]);
            vert_weights[i][j] = -vertex_similarity;
        }
    }

    bool swapped_e = a_edges.size() > b_edges.size();

    const auto &left_edges = swapped_e ? b_edges : a_edges;
    const auto &right_edges = swapped_e ? a_edges : b_edges;

    std::vector edge_weights(left_edges.size(), KACTL::vd(right_edges.size()));
    for (int i = 0; i < left_edges.size(); i++) {
        for (int j = 0; j < right_edges.size(); j++) {
            double sim_x = -(
                swapped_e == swapped_v
                    ? vert_weights[left_edges[i].first][right_edges[i].first]
                    : vert_weights[right_edges[i].first][left_edges[i].first]);
            double sim_y = -(
                swapped_e == swapped_v
                    ? vert_weights[left_edges[i].second][right_edges[i].second]
                    : vert_weights[right_edges[i].second]
                                  [left_edges[i].second]);
            double edge_similarity = sim_x * sim_y;
            double vertex_similarity = (sim_x + sim_y) / 2.0;
            edge_weights[i][j] = -(edge_similarity + vertex_similarity) / 2.0;
        }
    }
    const auto [match_res_e, _ignored_e] = KACTL::hungarian(edge_weights);
    double match_score_e = -match_res_e;
    int total_edges = right_edges.size();

    const auto [match_res_v, _ignored_v] = KACTL::hungarian(vert_weights);
    double match_score_v = -match_res_v;
    int total_verts = right_v.blocks.size();

    double total_score = match_score_v / total_verts;
    if (total_edges != 0) {
        double edge_score = match_score_e / total_edges;
        total_score = (total_score + edge_score) / 2;
    }

    std::cout << "[info] " << a.address << " vs " << b.address << " score "
              << total_score << '\n';

    return total_score;
}

void diff(const BinaryFile &a, const BinaryFile &b) {
    bool swapped = a.funcs.size() > b.funcs.size();

    const BinaryFile &left = swapped ? b : a;
    const BinaryFile &right = swapped ? a : b;

    std::vector weights(left.funcs.size(), KACTL::vd(right.funcs.size()));
    for (int i = 0; i < left.funcs.size(); i++) {
        for (int j = 0; j < right.funcs.size(); j++) {
            weights[i][j] = -similarity(left.funcs[i], right.funcs[j]);
        }
    }

    const auto [_ignored, matching] = KACTL::hungarian(weights);
    std::vector<std::tuple<double, uint64_t, uint64_t>> matches;

    for (int i = 0; i < left.funcs.size(); i++) {
        int j = matching[i];
        matches.emplace_back(-weights[i][j], left.funcs[i].address,
                             right.funcs[j].address);
    }

    constexpr double MATCH_THRES = 0.5;
    int num_matches = 0, total_funcs = right.funcs.size();
    std::sort(matches.rbegin(), matches.rend());

    for (auto [similarity, l, r] : matches) {
        if (swapped) {
            std::swap(l, r);
        }
        std::cout << "[match] " << std::hex << l << " vs " << r
                  << " similarity " << similarity << '\n';

        num_matches += similarity >= MATCH_THRES ? 1 : 0;
    }

    std::cout << std::dec << "[match] total matches: " << num_matches << " / "
              << total_funcs << '\n';
}
