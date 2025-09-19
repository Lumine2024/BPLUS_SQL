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
    
    LRUCache() = default;
    ~LRUCache() {
        for (auto& pair : m_cache) {
            delete pair.second.first;
        }
    }
    
    // Delete copy constructor and assignment operator to prevent issues
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    
    BPlusNode* get(size_t pageId);
    void put(size_t pageId, BPlusNode* node);
    void remove(size_t pageId);
    bool contains(size_t pageId) const;
    
private:
    void evictLRU();
    
    std::list<size_t> m_list;
    std::unordered_map<size_t, std::pair<BPlusNode *, std::list<size_t>::iterator>> m_cache;
};

}

#endif