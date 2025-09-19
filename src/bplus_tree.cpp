#include "bplus_tree.h"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace bplus_sql {

BPlusTree::BPlusTree(const std::string& fileName) 
    : m_rootPageId(0), m_nextPageId(1), m_pager(std::make_unique<Pager>(fileName)) {
    
    std::cout << "Pager created, creating root node...\n";
    
    // Create root node
    BPlusNode* root = createNode(true); // Start with leaf as root
    std::cout << "Root node created, putting to storage...\n";
    putNode(m_rootPageId, root);
    std::cout << "Constructor completed\n";
}

BPlusTree::~BPlusTree() = default;

BPlusNode* BPlusTree::getNode(size_t pageId) {
    BPlusNode* node = m_cache.get(pageId);
    if (node == nullptr) {
        node = new BPlusNode();
        m_pager->readPage(pageId, *node);
        m_cache.put(pageId, node);
    }
    return node;
}

void BPlusTree::putNode(size_t pageId, BPlusNode* node) {
    std::cout << "PutNode: page " << pageId << ", writing to pager...\n";
    m_pager->writePage(pageId, *node);
    std::cout << "PutNode: written to pager, adding to cache...\n";
    
    // Check if already in cache to avoid double-adding
    if (!m_cache.contains(pageId)) {
        // Create a copy for the cache since we might delete the original
        BPlusNode* cachedNode = new BPlusNode(*node);
        m_cache.put(pageId, cachedNode);
    }
    std::cout << "PutNode: completed\n";
}

size_t BPlusTree::allocatePage() {
    return m_nextPageId++;
}

BPlusNode* BPlusTree::createNode(bool isLeaf) {
    BPlusNode* node = new BPlusNode();
    std::memset(node, 0, sizeof(BPlusNode));
    node->isLeaf = isLeaf;
    node->keyCount = 0;
    node->next = 0;
    return node;
}

bool BPlusTree::search(int key) {
    size_t leafPageId = searchLeaf(key);
    BPlusNode* leaf = getNode(leafPageId);
    
    int index = findKeyIndex(leaf, key);
    return index < leaf->keyCount && leaf->keys[index] == key;
}

size_t BPlusTree::searchLeaf(int key) {
    size_t currentPageId = m_rootPageId;
    
    while (true) {
        BPlusNode* current = getNode(currentPageId);
        
        if (current->isLeaf) {
            return currentPageId;
        }
        
        int childIndex = findChildIndex(current, key);
        currentPageId = current->children[childIndex];
    }
}

bool BPlusTree::insert(int key) {
    std::cout << "Insert " << key << " starting...\n";
    
    size_t leafPageId = searchLeaf(key);
    std::cout << "Found leaf page " << leafPageId << "\n";
    
    BPlusNode* leaf = getNode(leafPageId);
    std::cout << "Got leaf node\n";
    
    // Check if key already exists
    int index = findKeyIndex(leaf, key);
    if (index < leaf->keyCount && leaf->keys[index] == key) {
        return false; // Key already exists
    }
    
    if (leaf->keyCount < MAX_KEYS) {
        return insertIntoLeaf(leafPageId, key);
    } else {
        // Need to split the leaf
        size_t newLeafPageId = splitLeaf(leafPageId, key);
        
        // If this was the root, create a new root
        if (leafPageId == m_rootPageId) {
            size_t newRootPageId = allocatePage();
            BPlusNode* newRoot = createNode(false);
            
            BPlusNode* oldRoot = getNode(m_rootPageId);
            BPlusNode* newLeaf = getNode(newLeafPageId);
            
            newRoot->keyCount = 1;
            newRoot->keys[0] = newLeaf->keys[0];
            newRoot->children[0] = m_rootPageId;
            newRoot->children[1] = newLeafPageId;
            
            putNode(newRootPageId, newRoot);
            m_rootPageId = newRootPageId;
        } else {
            // Insert new key into parent
            // This would need parent tracking or a recursive approach
            // For now, simplified implementation
        }
        
        return true;
    }
}

bool BPlusTree::insertIntoLeaf(size_t leafPageId, int key) {
    BPlusNode* leaf = getNode(leafPageId);
    
    int index = findKeyIndex(leaf, key);
    
    // Shift keys to make room
    for (int i = leaf->keyCount; i > index; i--) {
        leaf->keys[i] = leaf->keys[i - 1];
    }
    
    leaf->keys[index] = key;
    leaf->keyCount++;
    
    putNode(leafPageId, leaf);
    return true;
}

size_t BPlusTree::splitLeaf(size_t leafPageId, int key) {
    BPlusNode* oldLeaf = getNode(leafPageId);
    size_t newLeafPageId = allocatePage();
    BPlusNode* newLeaf = createNode(true);
    
    // Create temporary array with all keys including new one
    int allKeys[MAX_KEYS + 1];
    int insertPos = findKeyIndex(oldLeaf, key);
    
    // Copy keys before insertion point
    for (int i = 0; i < insertPos; i++) {
        allKeys[i] = oldLeaf->keys[i];
    }
    
    // Insert new key
    allKeys[insertPos] = key;
    
    // Copy keys after insertion point
    for (int i = insertPos; i < oldLeaf->keyCount; i++) {
        allKeys[i + 1] = oldLeaf->keys[i];
    }
    
    int totalKeys = oldLeaf->keyCount + 1;
    int midPoint = totalKeys / 2;
    
    // Update old leaf
    oldLeaf->keyCount = midPoint;
    for (int i = 0; i < midPoint; i++) {
        oldLeaf->keys[i] = allKeys[i];
    }
    
    // Set up new leaf
    newLeaf->keyCount = totalKeys - midPoint;
    for (int i = 0; i < newLeaf->keyCount; i++) {
        newLeaf->keys[i] = allKeys[midPoint + i];
    }
    
    // Update next pointers
    newLeaf->next = oldLeaf->next;
    oldLeaf->next = newLeafPageId;
    
    putNode(leafPageId, oldLeaf);
    putNode(newLeafPageId, newLeaf);
    
    return newLeafPageId;
}

bool BPlusTree::erase(int key) {
    size_t leafPageId = searchLeaf(key);
    return deleteFromLeaf(leafPageId, key);
}

bool BPlusTree::deleteFromLeaf(size_t leafPageId, int key) {
    BPlusNode* leaf = getNode(leafPageId);
    
    int index = findKeyIndex(leaf, key);
    if (index >= leaf->keyCount || leaf->keys[index] != key) {
        return false; // Key not found
    }
    
    // Shift keys to remove the key
    for (int i = index; i < leaf->keyCount - 1; i++) {
        leaf->keys[i] = leaf->keys[i + 1];
    }
    
    leaf->keyCount--;
    putNode(leafPageId, leaf);
    
    // Check if we need to merge or redistribute
    if (leaf->keyCount < MIN_KEYS && leafPageId != m_rootPageId) {
        mergeOrRedistribute(leafPageId);
    }
    
    return true;
}

void BPlusTree::mergeOrRedistribute(size_t pageId) {
    // Simplified implementation - in a full implementation,
    // this would handle merging with siblings or redistributing keys
    // when a node becomes under-full
}

int BPlusTree::findKeyIndex(const BPlusNode* node, int key) {
    int index = 0;
    while (index < node->keyCount && node->keys[index] < key) {
        index++;
    }
    return index;
}

int BPlusTree::findChildIndex(const BPlusNode* node, int key) {
    int index = 0;
    while (index < node->keyCount && key >= node->keys[index]) {
        index++;
    }
    return index;
}

}