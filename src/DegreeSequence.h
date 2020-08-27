#ifndef CDS_DEGREE_SEQUENCE_H
#define CDS_DEGREE_SEQUENCE_H

#include "Common.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <stdexcept>

namespace CDS {

using std::vector;

// Stores a degree sequence of degrees 0 <= d < n.
// Keeps track of information useful for sampling simple graphs.
class DegreeSequence {

    vector<deg_t> degseq;     // the degree sequence
    const int n;              // number of degrees

    vector<int> deg_counts;   // deg_counts[d] is the number of vertices with degree d
    vector<int> accum_counts; // accum_counts[d] is the number of vertices with degree <= d
    vector<int> sorted_verts; // vertex indices sorted by vertex degree
    vector<int> sorted_index; // sorted_index[u] is the index of vertex u in sorted_verts, i.e. sorted_verts[sorted_index[u]] == u

    deg_t dmax, dmin;         // largest and smallest non-zero (!) degree, assuming n_nonzero != 0
    int n_nonzero;            // number of non-zero degrees
    int dsum;                 // the sum of degrees

    // d(i) returns d_i in the non-increasingly sorted degree sequence.
    // Note that i is assumed to use 1-based indexing!
    deg_t d(int i) const { return degseq[sorted_verts[n - i]]; }

public:

    DegreeSequence() : n(0), dmax(0), dmin(0), n_nonzero(0), dsum(0) { }

    // Initialize degree sequence, O(n)
    template<typename It>
    DegreeSequence(It first, It last) :
        degseq(first, last),
        n(degseq.size()),
        deg_counts(n),
        sorted_verts(n), sorted_index(n),
        accum_counts(n)
    {
        // Initialize sorted_verts
        std::iota(sorted_verts.begin(), sorted_verts.end(), 0);
        std::sort(sorted_verts.begin(), sorted_verts.end(), [this] (int u, int v) { return degseq[u] < degseq[v]; } );

        // Initialize sorted_index
        for (int i=0; i < n; ++i)
            sorted_index[sorted_verts[i]] = i;

        dmax = 0;
        dmin = 0;
        n_nonzero = 0;
        dsum = 0;

        for (int i=0; i < n; ++i) {
            deg_t d = degseq[i];

            if (d < 0)
                throw std::domain_error("Degrees must be non-negative.");

            if (d > n-1)
                throw std::invalid_argument("The degree sequence is not graphical.");

            if (d != 0) {
                if (d < dmin)
                    dmin = d;
                if (d > dmax)
                    dmax = d;
                n_nonzero++;
            }

            deg_counts[d] += 1;

            dsum += d;
        }

        /*
        if (! is_graphical())
            throw std::invalid_argument("The degree sequence is not graphical.");
        */

        // Initialize accum_counts
        std::partial_sum(deg_counts.begin(), deg_counts.end(), accum_counts.begin());
    }

    // Decrement the degree of vertex u, O(1)
    void decrement(int u) {
        int d = degseq[u];
        Assert(d > 0);

        degseq[u]--;

        deg_counts[d]--;
        deg_counts[d-1]++;

        if (deg_counts[dmax] == 0)
            dmax -= 1;

        if (d == 1)
            n_nonzero--;

        if (d == dmin) {
            // We only let dmin drop to 0 if there are no more non-zero degrees left
            if (dmin > 1 || n_nonzero == 0)
                dmin -= 1;
        }

        int si_old = sorted_index[u];
        int si_new = accum_counts[d-1];

        int v = sorted_verts[si_new];
        sorted_index[u] = si_new;
        sorted_index[v] = si_old;

        std::swap(sorted_verts[si_old], sorted_verts[si_new]);

        accum_counts[d-1]++;
    }

    // Increment the degree of vertex u, O(1)
    void increment(int u) {
        int d = degseq[u];
        Assert(d < n-1);

        degseq[u]++;

        deg_counts[d]--;
        deg_counts[d+1]++;

        if (dmax == d)
            dmax += 1;

        if (d == 0) {
            n_nonzero++;
            dmin = 0;
        }

        int si_old = sorted_index[u];
        int si_new = accum_counts[d] - 1;

        int v = sorted_verts[si_new];
        sorted_index[u] = si_new;
        sorted_index[v] = si_old;

        std::swap(sorted_verts[si_old], sorted_verts[si_new]);

        accum_counts[d]--;
    }

    // Connect vertices u and v, O(1)
    void connect(int u, int v) {
        decrement(u);
        decrement(v);
    }

    // Graphicality test, O(n)
    bool is_graphical() const {

        if (dsum % 2 == 1)
            return false;

        if (n_nonzero == 0 || 4*dmin*n_nonzero >= sqr(dmax + dmin + 1))
            return true;

        int k = 0, sum_deg = 0, sum_ni = 0, sum_ini = 0;
        for (int dk = dmax; dk >= dmin; --dk) {
            if (dk < k+1)
                return true;

            int run_size = deg_counts[dk];
            if (run_size > 0) {
                if (dk < k + run_size) {
                    run_size = dk - k;
                }
                sum_deg += run_size * dk;
                for (int v=0; v < run_size; ++v) {
                    sum_ni  += deg_counts[k+v];
                    sum_ini += (k+v) * deg_counts[k+v];
                }
                k += run_size;
                if (sum_deg > k*(n-1) - k*sum_ni + sum_ini)
                    return false;
            }
        }

        return true;
    }

    // The smallest degree which may be connected to without breaking graphicality, O(n)
    // Before calling this function, all but one degree of the current vertex must have been
    // connected to the largest non-excluded other degrees, then the current vertex must have
    // been removed. Thefore, at this point the degree sum is odd. We check which degree
    // may be decremented by one (i.e. connected to) while maintaining the Erdős-Gallai inequalities.
    deg_t watershed() const {
        int wd = 0; // the watershed degree

        int lhs = 0;

        int s = n;
        int r = 0;

        for (int k=1; k <= n; ++k) {
            lhs += d(k);

            while (d(s) < k && s >= k) {
                r += d(s);
                s--;
            }

            if (s < k)
                break;

            int rhs = k*(s-1) + r;

            int diff = lhs - rhs;

            Assert(diff <= 1);

            if (diff == 1)
                return d(k);

            if (diff == 0)
                wd = k+1;
        }
        return wd;
    }

    // Access to degrees:

    const deg_t & operator [] (int v) const { return degseq[v]; }

    auto begin() const { return degseq.begin(); }
    auto end() const { return degseq.end(); }

    int size() const { return n; }

    const vector<deg_t> &degrees() const { return degseq; }
    const vector<int> &degree_distribution() const { return deg_counts; }

    // Sampling functions have access to internals:

    template<typename RNG>
    friend std::tuple<edgelist_t, double> sample(DegreeSequence ds, double alpha, RNG &rng);

    template<typename RNG>
    friend std::tuple<edgelist_t, double> sample_conn(DegreeSequence ds, double alpha, RNG &rng);
};

} // namespace CDS

#endif // CDS_DEGREE_SEQUENCE_H
