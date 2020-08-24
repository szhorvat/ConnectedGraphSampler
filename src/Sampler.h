#ifndef CDS_SAMPLER_H
#define CDS_SAMPLER_H

#include "Common.h"
#include "DegreeSequence.h"

#include <vector>
#include <stdexcept>
#include <tuple>
#include <random>

namespace CDS {

template<typename RNG>
std::tuple<edgelist_t, double> sample(DegreeSequence ds, RNG &rng) {
    if (! ds.is_graphical())
        throw std::invalid_argument("The degree sequence is not graphical.");

    typedef vector<char> bitmask_t;

    edgelist_t edges;
    double logprob = 0;

    if (ds.n == 0)
        return {edges, logprob};

    bitmask_t exclusion(ds.n);
    int vertex = 0;

    vector<int> allowed;
    vector<double> weights;

    while (true) {
        if (ds[vertex] == 0) { // No more stubs left on current vertex
            if (vertex == ds.n - 1) // All vertices have been processed
                return {edges, logprob};

            // Advance to next vertex and clear exclusion
            vertex += 1;
            std::fill(exclusion.begin(), exclusion.end(), 0);
            continue;
        }

        allowed.clear();
        weights.clear();

        // Construct allowed set
        {
            // Temporarily connect all but one stub of 'vertex' to highest-degree
            // non-excluded vertices. All of these are allowed connections.
            DegreeSequence work = ds;

            int d = ds[vertex];

            int i=ds.n-1;
            while (d > 1) {
                int v = ds.sorted_verts[i--];
                Assert(work[v] > 0);
                if (v != vertex && ! exclusion[v]) {
                    work.connect(vertex, v);
                    allowed.push_back(v);
                    weights.push_back(ds[v]);
                    d--;
                }
            }

            // Remove the final stub of 'vertex'.
            work.decrement(vertex);

            // Find watershed degree.
            int wd = work.watershed();

            // Of the rest of the vertices, determine if a connection is allowed
            // based on the watershed degree.
            for (; i >= 0; --i) {
                int v = ds.sorted_verts[i];

                if (ds[v] >= wd) {
                    if (v != vertex && ! exclusion[v]) {
                        allowed.push_back(v);
                        weights.push_back(ds[v]);
                    }
                } else {
                    break;
                }
            }
        }

        Assert(! allowed.empty());

        double tot = std::accumulate(weights.begin(), weights.end(), 0.0);
        logprob -= std::log(tot);

        std::discrete_distribution<> choose(weights.begin(), weights.end());

        int u = allowed[choose(rng)];

        exclusion[u] = 1;

        ds.connect(u, vertex);
        edges.push_back({vertex, u});
    }
}

} // namespace CDS

#endif // CDS_SAMPLER_H
