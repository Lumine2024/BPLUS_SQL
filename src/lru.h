#ifndef __LRU_H__
#define __LRU_H__

#include "bplus_node.h"
#include "pager.h"
#include <unordered_map>
#include <list>
#include <memory>

namespace bplus_sql {

class LRUCache {
public:
    static constexpr size_t CAPACITY = 1ull << 10;
    
    LRUCache() = default;
    ~LRUCache() = default;
    
    // Delete copy constructor and assignment operator to prevent issues
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    
    std::shared_ptr<BPlusNode> get(size_t pageId) {
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
    void put(size_t pageId, std::shared_ptr<BPlusNode> node) {
        auto it = m_cache.find(pageId);
        if (it != m_cache.end()) {
            // Update existing entry
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
            m_list.erase(it->second.second);
            m_cache.erase(it);
        }
    }
    bool contains(size_t pageId) const {
        return m_cache.contains(pageId);
    }
    
    size_t size() const {
        return m_cache.size();
    }
    
    // Traverse all nodes in cache and apply function fn to each
    template<typename Func>
    void traverse(Func&& fn) {
        for(auto& [pageId, pair_of_node_and_iterator] : m_cache) {
            fn(pageId, pair_of_node_and_iterator.first);
        }
    }
    
    // Get the tail (least recently used) node without removing it
    std::pair<size_t, std::shared_ptr<BPlusNode>> tail() const {
        if (m_list.empty()) {
            return {0, nullptr};
        }
        size_t backId = m_list.back();
        return std::make_pair(backId, m_cache.at(backId).first);
    }
    
private:
    void evictLRU() {
        if (!m_list.empty()) {
            size_t lru_page = m_list.back();
            m_list.pop_back();
            auto it = m_cache.find(lru_page);
            if (it != m_cache.end()) {
                m_cache.erase(it);
            }
        }
    }
    
    std::list<size_t> m_list;
    std::unordered_map<size_t, std::pair<std::shared_ptr<BPlusNode>, std::list<size_t>::iterator>> m_cache;
};

}

#endif