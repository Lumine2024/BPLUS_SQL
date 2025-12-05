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
    
    BPlusNode* get(size_t pageId) {
        auto it = m_cache.find(pageId);
        if (it == m_cache.end()) {
            return nullptr;
        }
        
        // Move to front (most recently used)
        m_list.erase(it->second.second);
        m_list.push_front(pageId);
        it->second.second = m_list.begin();
        
        return it->second.first;
    }
    void put(size_t pageId, BPlusNode* node) {
        auto it = m_cache.find(pageId);
        if (it != m_cache.end()) {
            // Update existing entry
            delete it->second.first;
            m_list.erase(it->second.second);
            m_list.push_front(pageId);
            it->second = {node, m_list.begin()};
            return;
        }
        
        // Add new entry
        if (m_cache.size() >= CAPACITY) {
            evictLRU();
        }
        
        m_list.push_front(pageId);
        m_cache[pageId] = {node, m_list.begin()};
    }
    void remove(size_t pageId) {
        auto it = m_cache.find(pageId);
        if (it != m_cache.end()) {
            delete it->second.first;
            m_list.erase(it->second.second);
            m_cache.erase(it);
        }
    }
    bool contains(size_t pageId) const {
        return m_cache.contains(pageId);
    }
    
private:
    void evictLRU() {
        if (!m_list.empty()) {
            size_t lru_page = m_list.back();
            m_list.pop_back();
            auto it = m_cache.find(lru_page);
            if (it != m_cache.end()) {
                delete it->second.first;
                m_cache.erase(it);
            }
        }
    }
    
    std::list<size_t> m_list;
    std::unordered_map<size_t, std::pair<BPlusNode *, std::list<size_t>::iterator>> m_cache;
};

}

#endif