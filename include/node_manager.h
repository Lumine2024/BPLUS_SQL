#ifndef __NODE_MANAGER_H__
#define __NODE_MANAGER_H__

#include "bplus_node.h"
#include "lru.h"
#include "pager.h"
#include <string>
#include <cstring>
#include <memory>

namespace bplus_sql {

class NodeManager {
public:
    NodeManager(const std::string &fileName) : m_lru(), m_pager(fileName) {}
    ~NodeManager() {
        // Write all nodes in m_lru back to m_pager
        m_lru.traverse([&](size_t pageId, std::shared_ptr<BPlusNode> node) {
            m_pager.writePage(pageId, *node);
        });
    }
    NodeManager(const NodeManager &) = delete;
    NodeManager &operator=(const NodeManager &) = delete;
    
    // Return a copy of the node - caller doesn't need to manage memory
    BPlusNode getNode(size_t pageId) {
        if(m_lru.contains(pageId)) {
            // Return a copy of the cached node
            auto cachedNode = m_lru.get(pageId);
            return *cachedNode;
        } else {
            BPlusNode result;
            m_pager.readPage(pageId, result);
            
            // Check if we're at capacity and need to evict
            if(m_lru.size() >= LRUCache::CAPACITY) {
                // Get the tail node (least recently used) and write it back to pager
                auto [tailPageId, tailNode] = m_lru.tail();
                if(tailNode != nullptr) {
                    m_pager.writePage(tailPageId, *tailNode);
                }
            }
            
            // Store a copy in cache
            m_lru.put(pageId, std::make_shared<BPlusNode>(result));
            
            return result;
        }
    }
    
    // Update the cache with the node value
    void putNode(size_t pageId, const BPlusNode &node) {
        // Update the cache with a copy of the node
        auto cachedCopy = std::make_shared<BPlusNode>(node);
        
        if(m_lru.contains(pageId)) {
            m_lru.put(pageId, cachedCopy);
        } else {
            // Check if we're at capacity and need to evict
            if(m_lru.size() >= LRUCache::CAPACITY) {
                // Get the tail node (least recently used) and write it back to pager
                auto [tailPageId, tailNode] = m_lru.tail();
                if(tailNode != nullptr) {
                    m_pager.writePage(tailPageId, *tailNode);
                }
            }
            m_lru.put(pageId, cachedCopy);
        }
    }
    
    // Expose pager methods for metadata operations
    template<typename T>
    void writeMetadata(const T& metadata) {
        m_pager.writeMetadata(metadata);
    }
    
    template<typename T>
    void readMetadata(T& metadata) {
        m_pager.readMetadata(metadata);
    }
    
    bool fileExists() const {
        return m_pager.fileExists();
    }
    
    size_t getFileSize() {
        return m_pager.getFileSize();
    }
    
private:
    LRUCache m_lru;
    Pager m_pager;
};



}



#endif