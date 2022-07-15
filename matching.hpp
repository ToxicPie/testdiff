#include <climits>
#include <cmath>
#include <vector>


// modified from KACTL code
// https://github.com/kth-competitive-programming/kactl/blob/eee7d5991ad944311f157e106082528be602fe73/content/graph/WeightedMatching.h
// author: Benjamin Qi, chilli
// source license: CC0
namespace KACTL {

using namespace std;

using vd = vector<double>;
using vi = vector<int>;
#define rep(i, a, b) for (long long i = a; i < (b); ++i)
#define all(x)       begin(x), end(x)
#define sz(x)        (long long)(x).size()

pair<double, vi> hungarian(const vector<vd> &a) {
    if (a.empty())
        return {0, {}};
    int n = sz(a) + 1, m = sz(a[0]) + 1;
    vd u(n), v(m);
    vi p(m), ans(n - 1);
    rep(i, 1, n) {
        p[0] = i;
        int j0 = 0; // add "dummy" worker 0
        vd dist(m, HUGE_VAL_F64);
        vi pre(m, -1);
        vector<bool> done(m + 1);
        do { // dijkstra
            done[j0] = true;
            int i0 = p[j0], j1;
            double delta = HUGE_VAL_F64;
            rep(j, 1, m) if (!done[j]) {
                auto cur = a[i0 - 1][j - 1] - u[i0] - v[j];
                if (cur < dist[j])
                    dist[j] = cur, pre[j] = j0;
                if (dist[j] < delta)
                    delta = dist[j], j1 = j;
            }
            rep(j, 0, m) {
                if (done[j])
                    u[p[j]] += delta, v[j] -= delta;
                else
                    dist[j] -= delta;
            }
            j0 = j1;
        } while (p[j0]);
        while (j0) { // update alternating path
            int j1 = pre[j0];
            p[j0] = p[j1], j0 = j1;
        }
    }
    rep(j, 1, m) if (p[j]) ans[p[j] - 1] = j - 1;
    return {-v[0], ans}; // min cost
}

#undef rep
#undef all
#undef sz

}; // namespace KACTL
