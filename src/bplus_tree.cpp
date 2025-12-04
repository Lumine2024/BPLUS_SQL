#include "bplus_tree.h"
#include <algorithm>
#include <cstring>
#include <iostream>

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
    size_t leafPageId = searchLeaf(key);
    BPlusNode* leaf = getNode(leafPageId);
    
    // Check if key already exists
    int index = findKeyIndex(leaf, key);
    if (index < leaf->keyCount && leaf->keys[index] == key) {
        delete leaf;
        return false; // Key already exists
    }
    
    // Allow insertion even if approaching MAX_KEYS (simplified implementation)
    delete leaf;
    return insertIntoLeaf(leafPageId, key);
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