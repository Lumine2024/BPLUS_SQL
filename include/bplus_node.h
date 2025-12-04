#ifndef __BPLUS_NODE_H__
#define __BPLUS_NODE_H__

#include <cstddef>

namespace bplus_sql {

/// @brief B+ Tree Node
struct BPlusNode {
    bool isLeaf;
    int keyCount;
    int keys[4096];  // Increased to handle more keys
    size_t children[4097];  // One more than keys for B+ tree invariant
    size_t next;
};

static_assert(sizeof(BPlusNode) <= (1ull << 17));  // Allow up to 128KB

}


#endif