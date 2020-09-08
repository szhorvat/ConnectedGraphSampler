
#ifndef CONNECTED_GRAPH_SAMPLER_MULTI
#define CONNECTED_GRAPH_SAMPLER_MULTI

#include <LTemplate.h>

#define Assert massert

#include "../../../../src/SamplerMulti.h"
#include "../../../../src/ConnSamplerMulti.h"

#include <random>

using namespace CDS;

class ConnectedGraphSamplerMulti {

    std::mt19937 rng;
    DegreeSequenceMulti *ds;

    edgelist_t edges;
    double logprob;

public:   

    ConnectedGraphSamplerMulti() :
        rng{std::random_device{}()},
        ds(new DegreeSequenceMulti),
        logprob(0)
    { }

    void seed(mint s) { rng.seed(s); }

    void setDS(mma::IntTensorRef degseq) {
        auto old_ds = ds;
        ds = new DegreeSequenceMulti(degseq.begin(), degseq.end());
        delete old_ds;

        edges.clear();
        logprob = 0;
    }

    mma::IntTensorRef getDS() const {
        return mma::makeVector<mint>(ds->degrees().size(), ds->degrees().data());
    }

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
        std::tie(edges, logprob) = CDS::sample_multi(*ds, alpha, rng);
        return getEdges();
    }

    mma::IntMatrixRef generateConnSample(double alpha) {
        std::tie(edges, logprob) = CDS::sample_conn_multi(*ds, alpha, rng);
        return getEdges();
    }
};

#endif // CONNECTED_GRAPH_SAMPLER_MULTI
