#ifndef CDS_COMMON_H
#define CDS_COMMON_H

#include <vector>

#ifndef Assert
#include <cassert>
#define Assert assert
#endif

namespace CDS {

typedef int deg_t;

typedef std::pair<int, int> edge;
typedef std::vector<edge> edgelist_t;

typedef std::vector<char> bitmask_t;

template<typename T>
T sqr(const T &x) { return x*x; }

} // namespace CDS

#endif // CDS_COMMON_H
