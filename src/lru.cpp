#include "lru.h"

namespace bplus_sql {

BPlusNode* LRUCache::get(size_t pageId) {
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

void LRUCache::put(size_t pageId, BPlusNode* node) {
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

void LRUCache::remove(size_t pageId) {
    auto it = m_cache.find(pageId);
    if (it != m_cache.end()) {
        delete it->second.first;
        m_list.erase(it->second.second);
        m_cache.erase(it);
    }
}

bool LRUCache::contains(size_t pageId) const {
    return m_cache.find(pageId) != m_cache.end();
}

void LRUCache::evictLRU() {
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

}