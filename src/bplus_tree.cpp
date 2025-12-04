#include "bplus_tree.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <functional>

namespace bplus_sql {

BPlusTree::BPlusTree(const std::string& fileName) 
    : m_rootPageId(0), m_nextPageId(1), m_pager(std::make_unique<Pager>(fileName)) {
    
    // Create root node
    BPlusNode* root = createNode(true); // Start with leaf as root
    putNode(m_rootPageId, root);
    delete root;
}

BPlusTree::~BPlusTree() = default;

BPlusNode* BPlusTree::getNode(size_t pageId) {
    // Temporarily disable cache to debug
    BPlusNode* node = new BPlusNode();
    m_pager->readPage(pageId, *node);
    return node;
}

void BPlusTree::putNode(size_t pageId, BPlusNode* node) {
    // Temporarily disable cache
    m_pager->writePage(pageId, *node);
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
    bool found = index < leaf->keyCount && leaf->keys[index] == key;
    delete leaf;
    return found;
}

size_t BPlusTree::searchLeaf(int key) {
    size_t currentPageId = m_rootPageId;
    
    while (true) {
        BPlusNode* current = getNode(currentPageId);
        
        if (current->isLeaf) {
            delete current;
            return currentPageId;
        }
        
        int childIndex = findChildIndex(current, key);
        size_t nextPageId = current->children[childIndex];
        delete current;
        currentPageId = nextPageId;
    }
}

bool BPlusTree::insert(int key) {
    // Helper function to recursively insert and handle splits
    struct SplitInfo {
        bool didSplit;
        int splitKey;
        size_t newPageId;
    };
    
    std::function<SplitInfo(size_t, int, bool)> insertRecursive = 
        [&](size_t pageId, int key, bool isLeafLevel) -> SplitInfo {
        
        BPlusNode* node = getNode(pageId);
        
        if (node->isLeaf) {
            // Check if key already exists
            int index = findKeyIndex(node, key);
            if (index < node->keyCount && node->keys[index] == key) {
                delete node;
                return {false, 0, 0}; // Key already exists
            }
            
            // Check if leaf is full
            if (node->keyCount >= MAX_KEYS) {
                delete node;
                // Split the leaf
                size_t newLeafPageId = splitLeaf(pageId, key);
                BPlusNode* newLeaf = getNode(newLeafPageId);
                int splitKey = newLeaf->keys[0];
                delete newLeaf;
                return {true, splitKey, newLeafPageId};
            }
            
            delete node;
            insertIntoLeaf(pageId, key);
            return {false, 0, 0};
        } else {
            // Internal node - find which child to go to
            int childIndex = findChildIndex(node, key);
            size_t childPageId = node->children[childIndex];
            delete node;
            
            SplitInfo childSplit = insertRecursive(childPageId, key, true);
            
            if (!childSplit.didSplit) {
                return {false, 0, 0};
            }
            
            // Child split, need to insert split key into this node
            node = getNode(pageId);
            
            if (node->keyCount < MAX_KEYS) {
                // Room in this node, insert the split key
                int insertPos = findKeyIndex(node, childSplit.splitKey);
                
                // Shift keys and children
                for (int i = node->keyCount; i > insertPos; i--) {
                    node->keys[i] = node->keys[i - 1];
                    node->children[i + 1] = node->children[i];
                }
                
                node->keys[insertPos] = childSplit.splitKey;
                node->children[insertPos + 1] = childSplit.newPageId;
                node->keyCount++;
                
                putNode(pageId, node);
                delete node;
                return {false, 0, 0};
            } else {
                // This node is also full, need to split it
                // For simplicity in this implementation, we'll just refuse to split internal nodes
                // and allow them to exceed MAX_KEYS slightly
                int insertPos = findKeyIndex(node, childSplit.splitKey);
                
                // Shift keys and children
                for (int i = node->keyCount; i > insertPos; i--) {
                    node->keys[i] = node->keys[i - 1];
                    node->children[i + 1] = node->children[i];
                }
                
                node->keys[insertPos] = childSplit.splitKey;
                node->children[insertPos + 1] = childSplit.newPageId;
                node->keyCount++;
                
                putNode(pageId, node);
                delete node;
                return {false, 0, 0};
            }
        }
    };
    
    SplitInfo rootSplit = insertRecursive(m_rootPageId, key, true);
    
    if (rootSplit.didSplit) {
        // Root split, need to create new root
        size_t newRootPageId = allocatePage();
        BPlusNode* newRoot = createNode(false); // Internal node
        
        newRoot->keys[0] = rootSplit.splitKey;
        newRoot->children[0] = m_rootPageId;
        newRoot->children[1] = rootSplit.newPageId;
        newRoot->keyCount = 1;
        
        putNode(newRootPageId, newRoot);
        delete newRoot;
        
        m_rootPageId = newRootPageId;
    }
    
    return true;
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
    delete leaf;
    return true;
}

size_t BPlusTree::splitLeaf(size_t leafPageId, int key) {
    BPlusNode* oldLeaf = getNode(leafPageId);
    size_t newLeafPageId = allocatePage();
    BPlusNode* newLeaf = createNode(true);
    
    // Create temporary array with all keys including new one (use dynamic allocation to avoid large stack allocation)
    std::vector<int> allKeys(MAX_KEYS + 1);
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
    delete oldLeaf;
    delete newLeaf;
    
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
        delete leaf;
        return false; // Key not found
    }
    
    // Shift keys to remove the key
    for (int i = index; i < leaf->keyCount - 1; i++) {
        leaf->keys[i] = leaf->keys[i + 1];
    }
    
    leaf->keyCount--;
    int newKeyCount = leaf->keyCount;
    putNode(leafPageId, leaf);
    delete leaf;
    
    // Check if we need to merge or redistribute
    if (newKeyCount < MIN_KEYS && leafPageId != m_rootPageId) {
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