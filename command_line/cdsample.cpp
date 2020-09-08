
#include "Sampler.h"
#include "ConnSampler.h"
#include "SamplerMulti.h"
#include "ConnSamplerMulti.h"

#include <boost/program_options.hpp>
#include <random>
#include <string>
#include <limits>
#include <iostream>
#include <fstream>

namespace po = boost::program_options;
using namespace CDS;
using namespace std;


int main(int argc, char *argv[]) {

    try {

        // Set up program options

        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("file,f",        po::value<string>(),                    "file containing degree sequence")
            ("degrees,d",   po::value<vector<deg_t>>()->multitoken(), "degree sequence")
            ("connected,c", po::bool_switch(),                        "generate connected graphs")
            ("multi,m",     po::bool_switch(),                        "generate loop-free multigraphs")
            ("alpha,a",     po::value<double>()->default_value(1.0),  "set parameter for the heuristic")
            ("count,n",     po::value<long>()->default_value(1L),     "how many graphs to generate")
            ("seed,s",      po::value<long>(),                        "set random seed")
        ;

        po::positional_options_description p;
        p.add("file", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                  options(desc).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help") || ! (vm.count("file") || vm.count("degrees"))) {
            if (! vm.count("help"))
                cerr << "Error: No degree sequence was given!\n";

            cout << "Usage:\n"
                 << argv[0] << " input_file\n"
                 << argv[0] << " --degrees d1 d2 d3\n\n"
                 << desc << "\n";

            if (vm.count("help"))
                return 0;
            else
                return 1;
        }

        if (vm.count("file") && vm.count("degrees")) {
            cerr << "Error: On the command line, provide either an input file, or an explicit degree sequence, but not both!\n";
            return 1;
        }

        // Read program options and degree sequence

        double alpha = vm["alpha"].as<double>();
        long n = vm["count"].as<long>();


        vector<deg_t> degrees;

        if (vm.count("file")) {
            ifstream dsfile(vm["file"].as<string>());

            if (! dsfile) {
                cerr << "Error: Could not open " << vm["file"].as<string>() << "!\n";
                return 1;
            }

            deg_t d;
            while (dsfile >> d)
                degrees.push_back(d);

            if (! dsfile.eof()) {
                cerr << "Error: Unexpected input in degree sequence file!\n";
                return 1;
            }
        } else {
            degrees = vm["degrees"].as<vector<deg_t>>();
        }

        // Set up random number generator

        mt19937 rng(random_device{}());
        if (vm.count("seed"))
            rng.seed(vm["seed"].as<long>());

        // Generate samples

        // Ensure that no precision is lost when printing weight values
        cout.precision(numeric_limits<double>::max_digits10);

        for (; n > 0; --n) {
            edgelist_t edges;
            double logprob;

            if (vm["multi"].as<bool>()) {
                if (vm["connected"].as<bool>()) {
                    DegreeSequenceMulti ds(degrees.begin(), degrees.end());
                    tie(edges, logprob) = sample_conn_multi(ds, alpha, rng);
                } else {
                    DegreeSequenceMulti ds(degrees.begin(), degrees.end());
                    tie(edges, logprob) = sample_multi(ds, alpha, rng);
                }
            } else {
                if (vm["connected"].as<bool>()) {
                    DegreeSequence ds(degrees.begin(), degrees.end());
                    tie(edges, logprob) = sample_conn(ds, alpha, rng);
                } else {
                    DegreeSequence ds(degrees.begin(), degrees.end());
                    tie(edges, logprob) = sample(ds, alpha, rng);
                }
            }

            cout << logprob << '\n';
            for (const auto &edge : edges) {
                // 'edges' uses 0-based indexing. Increment vertex names to output with 1-based indexing.
                cout << edge.first+1 << '\t' << edge.second+1 << '\n';
            }

            cout << "\n";
        }
    }
    catch(exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";
    }

    return 0;
}
