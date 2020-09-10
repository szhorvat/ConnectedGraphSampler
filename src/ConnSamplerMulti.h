#ifndef CDS_CONN_MULTI_SAMPLER_H
#define CDS_CONN_MULTI_SAMPLER_H

#include "Common.h"
#include "DegreeSequenceMulti.h"
#include "EquivClass.h"

#include <vector>
#include <stdexcept>
#include <tuple>
#include <numeric>
#include <map>
#include <random>

namespace CDS {

template<typename RNG>
std::tuple<edgelist_t, double> sample_conn_multi(DegreeSequenceMulti ds, double alpha, RNG &rng) {
    using std::vector;   

    if (! ds.is_multigraphical())
        throw std::invalid_argument("The degree sequence is not multigraphical.");

    // The null graph is considered non-connected.
    if (ds.n == 0)
        throw std::invalid_argument("The degree sequence is not potentially connected.");

    EquivClass conn_tracker(ds); // Connectivity tracker
    if (! conn_tracker.is_potentially_connected())
        throw std::invalid_argument("The degree sequence is not potentially connected.");

    edgelist_t edges;
    double logprob = 0;

    int vertex = 0; // The current vertex that we are connecting up

    // List of vertices that the current vertex can connect to without breaking multigraphicality.
    vector<int> allowed;

    // Vertices are chosen with a weight equal to the number of their stubs.
    // This is equivalent to choosing stubs uniformly.
    vector<double> weights;

    while (true) {
        if (ds[vertex] == 0) { // No more stubs left on current vertex
            if (vertex == ds.n - 1) // All vertices have been processed
                break;

            // Advance to next vertex
            vertex += 1;
            continue;
        }

        allowed.clear();
        weights.clear();

        // Construct allowed set
        {
            if (ds.dsum > 2*ds.dmax || ds[vertex] == ds.dmax) {
                // We can connect to any other vertex

                for (int v=vertex+1; v < ds.n; ++v)
                    if ( conn_tracker.connectable(vertex, v) ) {
                        allowed.push_back(v);
                        weights.push_back(std::pow(ds[v], alpha));
                    }
            } else {
                // We can only connect to max degree vertices

                for (int v=vertex+1; v < ds.n; ++v)
                    if (ds[v] == ds.dmax)
                        if ( conn_tracker.connectable(vertex, v) ) {
                            allowed.push_back(v);
                            weights.push_back(std::pow(ds[v], alpha));
                        }
            }
        }

        Assert(! allowed.empty());

        double tot = std::accumulate(weights.begin(), weights.end(), 0.0);
        logprob -= std::log(tot);

        std::discrete_distribution<> choose(weights.begin(), weights.end());

        int u = allowed[choose(rng)];

        logprob += (alpha - 1) * std::log(ds[u]);

        ds.connect(u, vertex);
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

    return std::make_tuple(edges, logprob);
}

} // namespace CDS

#endif // CSD_CONN_MULTI_SAMPLER_H
