#ifndef CDS_DEGREE_SEQUENCE_MULTI_H
#define CDS_DEGREE_SEQUENCE_MULTI_H

#include "Common.h"

#include <vector>
#include <stdexcept>

namespace CDS {

using std::vector;

// Stores a degree sequence of degrees 0 <= d < n.
// Keeps track of information useful for sampling multigraphs.
class DegreeSequenceMulti {

    vector<deg_t> degseq; // the degree sequence
    const int n;          // number of degrees

    deg_t dmax;           // largest degree
    int dsum;             // the sum of degrees

public:

    DegreeSequenceMulti() : n(0), dmax(0), dsum(0) { }

    // Initialize degree sequence, O(n)
    template<typename It>
    DegreeSequenceMulti(It first, It last) :
        degseq(first, last),
        n(degseq.size())
    {
        dmax = 0;
        dsum = 0;
        for (const auto &d : degseq) {
            if (d < 0)
                throw std::domain_error("Degrees must be non-negative.");
            dsum += d;
            if (d > dmax)
                dmax = d;
        }
    }

    // Decrement the degree of vertex u, amortized O(1)
    void decrement(int u) {
        degseq[u] -= 1;
        dsum -= 1;
        if (degseq[u] == dmax - 1) {
            dmax -= 1;
            for (const auto &d : degseq) {
                if (d > dmax) {
                    dmax = d;
                    break;
                }
            }
        }
    }

    // Connect vertices u and v, O(1)
    void connect(int u, int v) {
        decrement(u);
        decrement(v);
    }

    // Multigraphicality test, O(1)
    bool is_multigraphical() const {
        return dsum % 2 == 0 && dsum >= 2*dmax;
    }

    // Access to degrees:

    const deg_t & operator [] (int v) const { return degseq[v]; }

    auto begin() const { return degseq.begin(); }
    auto end() const { return degseq.end(); }

    int size() const { return n; }

    const vector<deg_t> &degrees() const { return degseq; }

    template<typename RNG>
    friend std::tuple<edgelist_t, double> sample_multi(DegreeSequenceMulti ds, double alpha, RNG &rng);

    template<typename RNG>
    friend std::tuple<edgelist_t, double> sample_conn_multi(DegreeSequenceMulti ds, double alpha, RNG &rng);
};

} // namespace CDS

#endif // CDS_DEGREE_SEQUENCE_MULTI_H
