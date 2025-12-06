#ifndef __NODE_MANAGER_H__
#define __NODE_MANAGER_H__

#include "bplus_node.h"
#include "lru.h"
#include "pager.h"
#include <string>

namespace bplus_sql {

class NodeManager {
public:
    NodeManager(const std::string &fileName) : m_lru(), m_pager(fileName) {}
    ~NodeManager() {
        // We need to write all nodes in m_lru to m_pager
        // We need a function in LRUCache like:
        /*
        void traverse(auto &&fn) {
            for(auto [pageId, pair_of_node_and_iterator] : m_cache) {
                fn(pageId, pair_of_node_and_iterator.first);
            }
        }
        */
        // so that we can call that in destruction function:
        /*
        m_lru.traverse([&](size_t pageId, BPlusNode *node) {
            m_pager.writePage(pageId, node);
        });
        */
    }
    NodeManager(const NodeManager &) = delete;
    NodeManager &operator=(const NodeManager &) = delete;
    BPlusNode *getNode(size_t pageId) {
        if(m_lru.contains(pageId)) {
            return m_lru.get(pageId);
        } else {
            auto result = new BPlusNode();
            m_pager.readPage(pageId, *result);
            // We need to get the tail of m_lru to write to m_pager
            // We need this in LRUCache:
            /*
            std::pair<std::size_t, BPlusNode *> tail() const {
                size_t backId = m_list.back();
                return std::make_pair(backId, m_cache[backId].first);
            }
            */
            m_lru.put(pageId, result);
            return result;
        }
    }
    void putNode(size_t pageId, BPlusNode *node) {
        if(m_lru.contains(pageId)) {
            m_lru.put(pageId, node);
        } else {
            // The same tail() function needed in LRUCache
            m_lru.put(pageId, node);
        }
    }
private:
    LRUCache m_lru;
    Pager m_pager;
};



}



#endif