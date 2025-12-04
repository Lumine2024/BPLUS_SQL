#ifndef __BPLUS_TREE_H__
#define __BPLUS_TREE_H__

#include "bplus_node.h"
#include "pager.h"
#include "lru.h"
#include <memory>

namespace bplus_sql {

class BPlusTree {
public:
    static constexpr int MIN_KEYS = 64;  // Minimum keys per node (half of max)
    static constexpr int MAX_KEYS = 4096; // Increased to handle pressure test
    
    explicit BPlusTree(const std::string& fileName);
    ~BPlusTree();
    
    bool insert(int key);
    bool search(int key);
    bool erase(int key);
    
private:
    // Node management
    BPlusNode* getNode(size_t pageId);
    void putNode(size_t pageId, BPlusNode* node);
    size_t allocatePage();
    BPlusNode* createNode(bool isLeaf);
    
    // B+ tree operations
    size_t searchLeaf(int key);
    bool insertIntoLeaf(size_t leafPageId, int key);
    bool insertIntoNonLeaf(size_t nodePageId, int key, size_t newChildPageId);
    size_t splitLeaf(size_t leafPageId, int key);
    size_t splitNonLeaf(size_t nodePageId, int key, size_t newChildPageId);
    
    bool deleteFromLeaf(size_t leafPageId, int key);
    void mergeOrRedistribute(size_t pageId);
    
    // Utility methods
    int findKeyIndex(const BPlusNode* node, int key);
    int findChildIndex(const BPlusNode* node, int key);
    
    size_t m_rootPageId;
    size_t m_nextPageId;
    std::unique_ptr<Pager> m_pager;
    LRUCache m_cache;
};

}

#endif