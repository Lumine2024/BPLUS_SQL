#ifndef __BPLUS_TREE_H__
#define __BPLUS_TREE_H__

#include "bplus_node.h"
#include "node_manager.h"
#include "pager.h"
#include <memory>
#include <iostream>
#include <format>
#include <functional>

namespace bplus_sql {

class BPlusTree {
public:
    static constexpr int MIN_KEYS = 64;  // Minimum keys per node (half of max)
    static constexpr int MAX_KEYS = 128; // Maximum keys per node
    
    // Metadata structure to persist tree state
    struct TreeMetadata {
        size_t rootPageId;
        size_t nextPageId;
        char padding[Pager::PAGE_SIZE - 2 * sizeof(size_t)]; // Pad to page size
    };
    
    explicit BPlusTree(const std::string& fileName)
        : m_nodeManager(std::make_unique<NodeManager>(fileName)) {
        
        // Check if file exists and has valid metadata
        if (m_nodeManager->fileExists() && m_nodeManager->getFileSize() >= sizeof(TreeMetadata)) {
            // Load existing tree metadata
            loadMetadata();
        } else {
            // Initialize new tree
            m_rootPageId = 0;
            m_nextPageId = 1;
            
            // Create root node
            BPlusNode root = createNode(true); // Start with leaf as root
            putNode(m_rootPageId, root);
            
            // Save initial metadata
            saveMetadata();
        }
    }
    
    ~BPlusTree() {
        // Save metadata on destruction
        saveMetadata();
    }
    
    bool insert(int key) {
        // Helper function to recursively insert and handle splits
        struct SplitInfo {
            bool didSplit;
            int splitKey;
            size_t newPageId;
        };
        
        auto insertRecursive = [&](auto &&self, size_t pageId, int key, bool isLeafLevel) -> SplitInfo {
            
            BPlusNode node = getNode(pageId);
            
            if (node.isLeaf) {
                // Check if key already exists
                int index = findKeyIndex(&node, key);
                if (index < node.keyCount && node.keys[index] == key) {
                    return {false, 0, 0}; // Key already exists
                }
                
                // Check if leaf is full
                if (node.keyCount >= MAX_KEYS) {
                    // Split the leaf
                    size_t newLeafPageId = splitLeaf(pageId, key);
                    BPlusNode newLeaf = getNode(newLeafPageId);
                    int splitKey = newLeaf.keys[0];
                    return {true, splitKey, newLeafPageId};
                }
                
                insertIntoLeaf(pageId, key);
                return {false, 0, 0};
            } else {
                // Internal node - find which child to go to
                int childIndex = findChildIndex(&node, key);
                size_t childPageId = node.children[childIndex];
                
                SplitInfo childSplit = self(self, childPageId, key, true);
                
                if (!childSplit.didSplit) {
                    return {false, 0, 0};
                }
                
                // Child split, need to insert split key into this node
                node = getNode(pageId);
                
                if (node.keyCount < MAX_KEYS) {
                    // Room in this node, insert the split key
                    int insertPos = findKeyIndex(&node, childSplit.splitKey);
                    
                    // Shift keys and children
                    for (int i = node.keyCount; i > insertPos; i--) {
                        node.keys[i] = node.keys[i - 1];
                        node.children[i + 1] = node.children[i];
                    }
                    
                    node.keys[insertPos] = childSplit.splitKey;
                    node.children[insertPos + 1] = childSplit.newPageId;
                    node.keyCount++;
                    
                    putNode(pageId, node);
                    return {false, 0, 0};
                } else {
                    // This node is also full, need to split it
                    size_t newNodePageId = splitNonLeaf(pageId, childSplit.splitKey, childSplit.newPageId);
                    BPlusNode newNode = getNode(newNodePageId);
                    int splitKey = newNode.keys[0];
                    return {true, splitKey, newNodePageId};
                }
            }
        };
        
        SplitInfo rootSplit = insertRecursive(insertRecursive, m_rootPageId, key, true);
        
        if (rootSplit.didSplit) {
            // Root split, need to create new root
            size_t newRootPageId = allocatePage();
            BPlusNode newRoot = createNode(false); // Internal node
            
            newRoot.keys[0] = rootSplit.splitKey;
            newRoot.children[0] = m_rootPageId;
            newRoot.children[1] = rootSplit.newPageId;
            newRoot.keyCount = 1;
            
            putNode(newRootPageId, newRoot);
            
            m_rootPageId = newRootPageId;
        }
        
        return true;
    }
    bool search(int key) {
        size_t leafPageId = searchLeaf(key);
        BPlusNode leaf = getNode(leafPageId);
        
        int index = findKeyIndex(&leaf, key);
        bool found = index < leaf.keyCount && leaf.keys[index] == key;
        return found;
    }
    bool erase(int key) {
        size_t leafPageId = searchLeaf(key);
        return deleteFromLeaf(leafPageId, key);
    }
    
private:
    // Node management
    BPlusNode getNode(size_t pageId) {
        return m_nodeManager->getNode(pageId);
    }
    void putNode(size_t pageId, const BPlusNode& node) {
        m_nodeManager->putNode(pageId, node);
    }
    size_t allocatePage() {
        return m_nextPageId++;
    }
    BPlusNode createNode(bool isLeaf) {
        BPlusNode node;
        std::memset(&node, 0, sizeof(BPlusNode));
        node.isLeaf = isLeaf;
        node.keyCount = 0;
        node.next = 0;
        return node;
    }
    
    // B+ tree operations
    size_t searchLeaf(int key) {
        size_t currentPageId = m_rootPageId;
        
        while (true) {
            BPlusNode current = getNode(currentPageId);
            
            if (current.isLeaf) {
                return currentPageId;
            }
            
            int childIndex = findChildIndex(&current, key);
            size_t nextPageId = current.children[childIndex];
            currentPageId = nextPageId;
        }
    }
    bool insertIntoLeaf(size_t leafPageId, int key) {
        BPlusNode leaf = getNode(leafPageId);
        
        int index = findKeyIndex(&leaf, key);
        
        // Shift keys to make room
        for (int i = leaf.keyCount; i > index; i--) {
            leaf.keys[i] = leaf.keys[i - 1];
        }
        
        leaf.keys[index] = key;
        leaf.keyCount++;
        
        putNode(leafPageId, leaf);
        return true;
    }
    size_t splitLeaf(size_t leafPageId, int key) {
        BPlusNode oldLeaf = getNode(leafPageId);
        size_t newLeafPageId = allocatePage();
        BPlusNode newLeaf = createNode(true);
        
        // Create temporary array with all keys including new one (use dynamic allocation to avoid large stack allocation)
        std::vector<int> allKeys(MAX_KEYS + 1);
        int insertPos = findKeyIndex(&oldLeaf, key);
        
        // Copy keys before insertion point
        for (int i = 0; i < insertPos; i++) {
            allKeys[i] = oldLeaf.keys[i];
        }
        
        // Insert new key
        allKeys[insertPos] = key;
        
        // Copy keys after insertion point
        for (int i = insertPos; i < oldLeaf.keyCount; i++) {
            allKeys[i + 1] = oldLeaf.keys[i];
        }
        
        int totalKeys = oldLeaf.keyCount + 1;
        int midPoint = totalKeys / 2;
        
        // Update old leaf
        oldLeaf.keyCount = midPoint;
        for (int i = 0; i < midPoint; i++) {
            oldLeaf.keys[i] = allKeys[i];
        }
        
        // Set up new leaf
        newLeaf.keyCount = totalKeys - midPoint;
        for (int i = 0; i < newLeaf.keyCount; i++) {
            newLeaf.keys[i] = allKeys[midPoint + i];
        }
        
        // Update next pointers
        newLeaf.next = oldLeaf.next;
        oldLeaf.next = newLeafPageId;
        
        putNode(leafPageId, oldLeaf);
        putNode(newLeafPageId, newLeaf);
        
        return newLeafPageId;
    }
    size_t splitNonLeaf(size_t nodePageId, int key, size_t newChildPageId) {
        BPlusNode oldNode = getNode(nodePageId);
        size_t newNodePageId = allocatePage();
        BPlusNode newNode = createNode(false);
        
        // Create temporary arrays with all keys and children including new one
        std::vector<int> allKeys(MAX_KEYS + 1);
        std::vector<size_t> allChildren(MAX_KEYS + 2);
        
        // Find insertion position
        int insertPos = findKeyIndex(&oldNode, key);
        
        // Copy keys before insertion point
        for (int i = 0; i < insertPos; i++) {
            allKeys[i] = oldNode.keys[i];
        }
        
        // Insert new key
        allKeys[insertPos] = key;
        
        // Copy keys after insertion point
        for (int i = insertPos; i < oldNode.keyCount; i++) {
            allKeys[i + 1] = oldNode.keys[i];
        }
        
        // Copy children before insertion point
        for (int i = 0; i <= insertPos; i++) {
            allChildren[i] = oldNode.children[i];
        }
        
        // Insert new child
        allChildren[insertPos + 1] = newChildPageId;
        
        // Copy children after insertion point
        for (int i = insertPos + 1; i <= oldNode.keyCount; i++) {
            allChildren[i + 1] = oldNode.children[i];
        }
        
        int totalKeys = oldNode.keyCount + 1;
        int midPoint = totalKeys / 2;
        
        // Update old node - keep first half of keys (0 to midPoint-1)
        oldNode.keyCount = midPoint;
        for (int i = 0; i < midPoint; i++) {
            oldNode.keys[i] = allKeys[i];
            oldNode.children[i] = allChildren[i];
        }
        oldNode.children[midPoint] = allChildren[midPoint];
        
        // Set up new node - keep second half including the middle key
        // In this implementation, we keep the split key in the right sibling
        // (similar to leaf splits) which is valid for B+ trees since internal
        // keys are just routing keys. The parent will also store this key.
        newNode.keyCount = totalKeys - midPoint;
        for (int i = 0; i < newNode.keyCount; i++) {
            newNode.keys[i] = allKeys[midPoint + i];
            newNode.children[i] = allChildren[midPoint + i];
        }
        newNode.children[newNode.keyCount] = allChildren[totalKeys];
        
        putNode(nodePageId, oldNode);
        putNode(newNodePageId, newNode);
        
        return newNodePageId;
    }
    
    bool deleteFromLeaf(size_t leafPageId, int key) {
        BPlusNode leaf = getNode(leafPageId);
        
        int index = findKeyIndex(&leaf, key);
        if (index >= leaf.keyCount || leaf.keys[index] != key) {
            return false; // Key not found
        }
        
        // Shift keys to remove the key
        for (int i = index; i < leaf.keyCount - 1; i++) {
            leaf.keys[i] = leaf.keys[i + 1];
        }
        
        leaf.keyCount--;
        int newKeyCount = leaf.keyCount;
        putNode(leafPageId, leaf);
        
        // Check if we need to merge or redistribute
        if (newKeyCount < MIN_KEYS && leafPageId != m_rootPageId) {
            mergeOrRedistribute(leafPageId);
        }
        
        return true;
    }
    void mergeOrRedistribute(size_t pageId) {}
    
    // Metadata persistence methods
    void saveMetadata() {
        TreeMetadata metadata;
        metadata.rootPageId = m_rootPageId;
        metadata.nextPageId = m_nextPageId;
        std::memset(metadata.padding, 0, sizeof(metadata.padding));
        
        // Write metadata to special page
        m_nodeManager->writeMetadata(metadata);
    }
    
    void loadMetadata() {
        TreeMetadata metadata;
        m_nodeManager->readMetadata(metadata);
        
        m_rootPageId = metadata.rootPageId;
        m_nextPageId = metadata.nextPageId;
    }
    
    // Utility methods
    int findKeyIndex(const BPlusNode* node, int key) {
        int index = 0;
        while (index < node->keyCount && node->keys[index] < key) {
            index++;
        }
        return index;
    }
    int findChildIndex(const BPlusNode* node, int key) {
        int index = 0;
        while (index < node->keyCount && key >= node->keys[index]) {
            index++;
        }
        return index;
    }
    
    size_t m_rootPageId;
    size_t m_nextPageId;
    std::unique_ptr<NodeManager> m_nodeManager;

public:
    void dfs() {
        auto _dfs = [&](auto &&self, int u) -> void {
			BPlusNode v = getNode(u);
			std::cout << std::format("Node PageID: {}, isLeaf: {}, keyCount: {}\n", u, v.isLeaf, v.keyCount);
            std::cout << "Keys: ";
            for(int i = 0; i < v.keyCount; ++i) {
                std::cout << v.keys[i] << " ";
            }
            if(!v.isLeaf) {
                for(int i = 0; i <= v.keyCount; ++i) {
                    std::cout << "\nChild " << i << " PageID: " << v.children[i] << "\n";
                }
				for(int i = 0; i <= v.keyCount; ++i) {
                    self(self, v.children[i]);
                }
            }
        };
		_dfs(_dfs, m_rootPageId);
    }
};

}

#endif