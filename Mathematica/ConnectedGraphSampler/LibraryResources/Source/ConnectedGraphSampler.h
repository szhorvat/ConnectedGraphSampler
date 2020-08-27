
#include <LTemplate.h>

#define Assert massert

#include "../../../../src/Sampler.h"
#include "../../../../src/ConnSampler.h"

#include <random>

using namespace CDS;

class ConnectedGraphSampler {

    DegreeSequence *ds;
    std::mt19937 rng;

    edgelist_t edges;
    double logprob;

public:   

    ConnectedGraphSampler() :
        rng{std::random_device{}()},
        logprob(0),
        ds(new DegreeSequence)
    { }

    ~ConnectedGraphSampler() { delete ds; }

    void seed(mint s) { rng.seed(s); }

    void setDS(mma::IntTensorRef degseq) {
        auto old_ds = ds;
        ds = new DegreeSequence(degseq.begin(), degseq.end());
        delete old_ds;

        edges.clear();
        logprob = 0;
    }

    mma::IntTensorRef getDS() const {
        return mma::makeVector<mint>(ds->degrees().size(), ds->degrees().data());
    }

    mma::IntTensorRef getDistrib() const {
        return mma::makeVector<mint>(ds->degree_distribution().size(), ds->degree_distribution().data());
    }

    bool graphicalQ() const { return ds->is_graphical(); }

    /*
    void decrement(int u) { ds->decrement(u); }
    void increment(int u) { ds->increment(u); }
    void connect(int u, int v) { ds->connect(u, v); }

    mint watershed() const { return ds->watershed(); }
    */

    auto getEdges() const {
        auto res = mma::makeMatrix<mint>(edges.size(), 2);
        for (int i=0; i < edges.size(); ++i) {
            res(i,0) = edges[i].first;
            res(i,1) = edges[i].second;
        }
        return res;
    }

    double getLogProb() const {
        return logprob;
    }

    mma::IntMatrixRef generateSample(double alpha) {
        std::tie(edges, logprob) = CDS::sample(*ds, alpha, rng);
        return getEdges();
    }

    mma::IntMatrixRef generateConnSample(double alpha) {
        std::tie(edges, logprob) = CDS::sample_conn(*ds, alpha, rng);
        return getEdges();
    }
};
