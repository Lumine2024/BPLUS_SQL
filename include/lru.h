#ifndef __LRU_H__
#define __LRU_H__

#include "bplus_node.h"
#include "pager.h"
#include <unordered_map>
#include <list>

namespace bplus_sql {

class LRUCache {
public:
    static constexpr size_t CAPACITY = 1ull << 10;
    
private:
    std::list<size_t> m_list;
    std::unordered_map<size_t, std::pair<BPlusNode *, std::list<size_t>::iterator>> m_cache;
} lruCache;

}

#endif