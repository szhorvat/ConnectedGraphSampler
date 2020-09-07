#ifndef CDS_CONN_MULTI_SAMPLER_H
#define CDS_CONN_MULTI_SAMPLER_H

#include <tuple>
#include <algorithm>
#include <map>
#include <random>

#include "Common.h"
#include "EquivClass.h"

namespace CDS {

template<typename RNG>
std::tuple<edgelist_t, double> sample_conn_multi(std::vector<deg_t> ds, double alpha, RNG &rng) {
    using std::vector;   

    const int n = ds.size();

    // The null graph is considered non-connected.
    if (n == 0)
        throw std::invalid_argument("The degree sequence is not potentially connected.");

    deg_t dmax = 0;
    int dsum = 0;

    for (const auto &d : ds) {
        if (d > dmax)
            dmax = d;
        dsum += d;
    }

    if (dsum % 2 == 1 || dsum < 2*dmax)
        throw std::invalid_argument("The degree sequence is not multigraphical.");

    EquivClass conn_tracker(ds); // Connectivity tracker
    if (! conn_tracker.is_potentially_connected())
        throw std::invalid_argument("The degree sequence is not potentially connected.");

    edgelist_t edges;
    double logprob = 0;

    if (n == 0)
        return {edges, logprob};

    int vertex = 0; // The current vertex that we are connecting up

    // List of vertices that the current vertex can connect to without breaking multigraphicality.
    vector<int> allowed;

    // Vertices are chosen with a weight equal to the number of their stubs.
    // This is equivalent to choosing stubs uniformly.
    vector<double> weights;

    while (true) {
        if (ds[vertex] == 0) { // No more stubs left on current vertex
            if (vertex == n - 1) // All vertices have been processed
                break;

            // Advance to next vertex
            vertex += 1;
            continue;
        }

        allowed.clear();
        weights.clear();

        // Construct allowed set
        {
            auto supernode = conn_tracker.get_class(vertex);
            int d_supernode = supernode->degree();
            int comp_count = conn_tracker.component_count();
            int edge_count = conn_tracker.edge_count();

            if (dsum > 2*dmax || ds[vertex] == dmax) {
                // We can connect to any other vertex

                for (int v=vertex+1; v < n; ++v)
                    if ( comp_count == 1 ||
                         edge_count == 1 ||
                         (d_supernode > 2 && edge_count > comp_count - 1) ||
                         (conn_tracker.get_class(v) != supernode && (d_supernode > 1 || conn_tracker.get_class(v)->degree() > 1)) )
                    {
                        allowed.push_back(v);
                        weights.push_back(ds[v]);
                    }
            } else {
                // We can only connect to max degree vertices

                for (int v=vertex+1; v < n; ++v) {
                    if (ds[v] == dmax)
                        if ( comp_count == 1 ||
                             edge_count == 1 ||
                             (d_supernode > 2 && edge_count > comp_count - 1) ||
                             (conn_tracker.get_class(v) != supernode && (d_supernode > 1 || conn_tracker.get_class(v)->degree() > 1)) )
                        {
                            allowed.push_back(v);
                            weights.push_back(ds[v]);
                        }
                }
            }
        }

        Assert(! allowed.empty());

        double tot = std::accumulate(weights.begin(), weights.end(), 0.0);
        logprob -= std::log(tot);

        std::discrete_distribution<> choose(weights.begin(), weights.end());

        int u = allowed[choose(rng)];

        logprob += (alpha - 1) * std::log(ds[u]);

        ds[u] -= 1;
        ds[vertex] -=1;

        dsum -= 2;
        if (ds[vertex] == dmax-1 || ds[u] == dmax-1) {
            // Recompute dmax
            dmax -= 1;
            for (const auto &d : ds)
                if (d > dmax)
                    dmax = d;
        }

        conn_tracker.connect(u, vertex);
        edges.push_back({vertex, u});
    }

    // Not all multigraphs correspond to the same number of leaves on the decision tree.
    // Therefore, we must correct the sampling weight.

    std::map<edge, int> multiplicities;
    for (const auto &e : edges)
        multiplicities[e] += 1;

    for (const auto &el : multiplicities)
        if (el.second > 1)
            logprob -= logfact(el.second);

    return {edges, logprob};
}

} // namespace CDS

#endif // CSD_CONN_MULTI_SAMPLER_H
