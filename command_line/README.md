This directory contains a command line program `cdsample` that provides an interface to the graph sampler.

### Prerequisites

 - A C++ compiler with C++14 support.
 - [CMake](https://cmake.org/).
 - The [Boost](https://www.boost.org/) library for [Boost Program Options](https://www.boost.org/doc/libs/release/libs/program_options/). On Debian-based Linux systems, it can be installed using `sudo apt install libboost-program-options-dev`.

The program was tested with Apple Clang 11.0 on macOS and with gcc 5.4 on Linux, using boost 1.58.

### Compiling

Create a new directory, enter it, then run `cmake`:

```
mkdir build
cd build
cmake ..
```

If the configuration has succeeded, build the program using

```
cmake --build .
```

An executable named `cdsample` will be created in the current directory.


### Example usage

Getting help:

```
$ ./cdsample -h
Usage:
./cdsample input_file
./cdsample --degrees d1 d2 d3

Allowed options:
  -h [ --help ]           produce help message
  -f [ --file ] arg       file containing degree sequence
  -d [ --degrees ] arg    degree sequence
  -c [ --connected ]      generate connected graphs
  -m [ --multi ]          generate loop-free multigraphs
  -a [ --alpha ] arg (=1) set parameter for the heuristic
  -n [ --count ] arg (=1) how many graphs to generate
  -s [ --seed ] arg       set random seed
  ```

Generate one graph with the degree sequence (1, 1, 2, 2, 3, 3):

```
$ ./cdsample -d 1 1 2 2 3 3
-7.3677085723743714
1	2
3	5
3	6
4	6
4	5
5	6
```

The first line is the natural logarithm of this graph's sampling weight. The following are tab-separated pairs of vertices representing the edges of the graph.

By default, not-necessarily-connected simple graphs are generated. The above graph is not connected: it contains the edge `(1,2)`. Vertices 1 and 2 are both of degree 1, thus in this graph they form a closed connected component.

To sample only connected graphs, use the `-c` option. To allow loop-free multigraphs, use the `-m` option.

To generate multiple graphs, use the `-n` option. Multiple outputs will be separated by a single empty line. The following example generates three loopless multigraphs:

```
$ ./cdsample -d 1 1 2 2 3 3 -mc -n 3
-8.6586927536899356
1	3
2	4
3	5
4	6
5	6
5	6

-8.5251613610654147
1	5
2	3
3	6
4	6
4	5
5	6

-8.9306264691735784
1	5
2	3
3	5
4	6
4	6
5	6
```

The degree sequence can be read from a file. Instead of using the `-d` argument, simply specify the file name, e.g. `cdsample degrees.txt`. An example degree sequence file, `degrees.txt`, is included.
