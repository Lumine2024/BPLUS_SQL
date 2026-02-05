#ifndef __BPLUS_NODE_H__
#define __BPLUS_NODE_H__

#include <cstddef>

namespace bplus_sql {

/// @brief B+ Tree Node
struct BPlusNode {
    bool isLeaf;
    int keyCount;
    int keys[128];
    size_t children[129];
    size_t next;
};

static_assert(sizeof(BPlusNode) <= (1ull << 12));

}

#endif