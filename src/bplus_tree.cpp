#include "bplus_tree.h"

#include "bplus_node.h"
#include "node_manager.h"
#include "pager.h"

#include <cstring>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

namespace bplus_sql {

class BPlusTree::Impl {
public:
    struct TreeMetadata {
        size_t rootPageId;
        size_t nextPageId;
        char padding[Pager::PAGE_SIZE - 2 * sizeof(size_t)];
    };

    explicit Impl(const std::string &fileName)
        : m_nodeManager(std::make_unique<NodeManager>(fileName)) {
        if (m_nodeManager->fileExists() && m_nodeManager->getFileSize() >= sizeof(TreeMetadata)) {
            loadMetadata();
        } else {
            m_rootPageId = 0;
            m_nextPageId = 1;

            BPlusNode root = createNode(true);
            putNode(m_rootPageId, root);
            saveMetadata();
        }
    }

    ~Impl() {
        saveMetadata();
    }

    bool insert(int key) {
        struct SplitInfo {
            bool didSplit;
            int splitKey;
            size_t newPageId;
        };

        auto insertRecursive = [&](auto &&self, size_t pageId, int key) -> SplitInfo {
            BPlusNode node = getNode(pageId);

            if (node.isLeaf) {
                int index = findKeyIndex(&node, key);
                if (index < node.keyCount && node.keys[index] == key) {
                    return {false, 0, 0};
                }

                if (node.keyCount >= BPlusTree::MAX_KEYS) {
                    size_t newLeafPageId = splitLeaf(pageId, key);
                    BPlusNode newLeaf = getNode(newLeafPageId);
                    return {true, newLeaf.keys[0], newLeafPageId};
                }

                insertIntoLeaf(pageId, key);
                return {false, 0, 0};
            }

            int childIndex = findChildIndex(&node, key);
            size_t childPageId = node.children[childIndex];

            SplitInfo childSplit = self(self, childPageId, key);
            if (!childSplit.didSplit) {
                return {false, 0, 0};
            }

            node = getNode(pageId);
            if (node.keyCount < BPlusTree::MAX_KEYS) {
                int insertPos = findKeyIndex(&node, childSplit.splitKey);
                for (int i = node.keyCount; i > insertPos; i--) {
                    node.keys[i] = node.keys[i - 1];
                    node.children[i + 1] = node.children[i];
                }

                node.keys[insertPos] = childSplit.splitKey;
                node.children[insertPos + 1] = childSplit.newPageId;
                node.keyCount++;
                putNode(pageId, node);
                return {false, 0, 0};
            }

            size_t newNodePageId = splitNonLeaf(pageId, childSplit.splitKey, childSplit.newPageId);
            BPlusNode newNode = getNode(newNodePageId);
            return {true, newNode.keys[0], newNodePageId};
        };

        SplitInfo rootSplit = insertRecursive(insertRecursive, m_rootPageId, key);
        if (rootSplit.didSplit) {
            size_t newRootPageId = allocatePage();
            BPlusNode newRoot = createNode(false);

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
        return index < leaf.keyCount && leaf.keys[index] == key;
    }

    bool erase(int key) {
        size_t leafPageId = searchLeaf(key);
        return deleteFromLeaf(leafPageId, key);
    }

    void dfs() {
        auto dfsImpl = [&](auto &&self, size_t pageId) -> void {
            BPlusNode node = getNode(pageId);
            std::cout << std::format("Node PageID: {}, isLeaf: {}, keyCount: {}\n", pageId, node.isLeaf, node.keyCount);
            std::cout << "Keys: ";
            for (int i = 0; i < node.keyCount; ++i) {
                std::cout << node.keys[i] << " ";
            }

            if (!node.isLeaf) {
                for (int i = 0; i <= node.keyCount; ++i) {
                    std::cout << "\nChild " << i << " PageID: " << node.children[i] << "\n";
                }
                for (int i = 0; i <= node.keyCount; ++i) {
                    self(self, node.children[i]);
                }
            }
        };

        dfsImpl(dfsImpl, m_rootPageId);
    }

private:
    BPlusNode getNode(size_t pageId) {
        return m_nodeManager->getNode(pageId);
    }

    void putNode(size_t pageId, const BPlusNode &node) {
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

    size_t searchLeaf(int key) {
        size_t currentPageId = m_rootPageId;
        while (true) {
            BPlusNode current = getNode(currentPageId);
            if (current.isLeaf) {
                return currentPageId;
            }

            int childIndex = findChildIndex(&current, key);
            currentPageId = current.children[childIndex];
        }
    }

    bool insertIntoLeaf(size_t leafPageId, int key) {
        BPlusNode leaf = getNode(leafPageId);
        int index = findKeyIndex(&leaf, key);

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

        std::vector<int> allKeys(BPlusTree::MAX_KEYS + 1);
        int insertPos = findKeyIndex(&oldLeaf, key);

        for (int i = 0; i < insertPos; i++) {
            allKeys[i] = oldLeaf.keys[i];
        }

        allKeys[insertPos] = key;

        for (int i = insertPos; i < oldLeaf.keyCount; i++) {
            allKeys[i + 1] = oldLeaf.keys[i];
        }

        int totalKeys = oldLeaf.keyCount + 1;
        int midPoint = totalKeys / 2;

        oldLeaf.keyCount = midPoint;
        for (int i = 0; i < midPoint; i++) {
            oldLeaf.keys[i] = allKeys[i];
        }

        newLeaf.keyCount = totalKeys - midPoint;
        for (int i = 0; i < newLeaf.keyCount; i++) {
            newLeaf.keys[i] = allKeys[midPoint + i];
        }

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

        std::vector<int> allKeys(BPlusTree::MAX_KEYS + 1);
        std::vector<size_t> allChildren(BPlusTree::MAX_KEYS + 2);

        int insertPos = findKeyIndex(&oldNode, key);

        for (int i = 0; i < insertPos; i++) {
            allKeys[i] = oldNode.keys[i];
        }

        allKeys[insertPos] = key;

        for (int i = insertPos; i < oldNode.keyCount; i++) {
            allKeys[i + 1] = oldNode.keys[i];
        }

        for (int i = 0; i <= insertPos; i++) {
            allChildren[i] = oldNode.children[i];
        }

        allChildren[insertPos + 1] = newChildPageId;

        for (int i = insertPos + 1; i <= oldNode.keyCount; i++) {
            allChildren[i + 1] = oldNode.children[i];
        }

        int totalKeys = oldNode.keyCount + 1;
        int midPoint = totalKeys / 2;

        oldNode.keyCount = midPoint;
        for (int i = 0; i < midPoint; i++) {
            oldNode.keys[i] = allKeys[i];
            oldNode.children[i] = allChildren[i];
        }
        oldNode.children[midPoint] = allChildren[midPoint];

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
            return false;
        }

        for (int i = index; i < leaf.keyCount - 1; i++) {
            leaf.keys[i] = leaf.keys[i + 1];
        }

        leaf.keyCount--;
        int newKeyCount = leaf.keyCount;
        putNode(leafPageId, leaf);

        if (newKeyCount < BPlusTree::MIN_KEYS && leafPageId != m_rootPageId) {
            mergeOrRedistribute(leafPageId);
        }

        return true;
    }

    void mergeOrRedistribute(size_t pageId) {
        (void)pageId;
    }

    void saveMetadata() {
        TreeMetadata metadata;
        metadata.rootPageId = m_rootPageId;
        metadata.nextPageId = m_nextPageId;
        std::memset(metadata.padding, 0, sizeof(metadata.padding));
        m_nodeManager->writeMetadata(metadata);
    }

    void loadMetadata() {
        TreeMetadata metadata;
        m_nodeManager->readMetadata(metadata);
        m_rootPageId = metadata.rootPageId;
        m_nextPageId = metadata.nextPageId;
    }

    int findKeyIndex(const BPlusNode *node, int key) {
        int index = 0;
        while (index < node->keyCount && node->keys[index] < key) {
            index++;
        }
        return index;
    }

    int findChildIndex(const BPlusNode *node, int key) {
        int index = 0;
        while (index < node->keyCount && key >= node->keys[index]) {
            index++;
        }
        return index;
    }

    size_t m_rootPageId;
    size_t m_nextPageId;
    std::unique_ptr<NodeManager> m_nodeManager;
};

BPlusTree::BPlusTree(const std::string &fileName)
    : m_impl(std::make_unique<Impl>(fileName)) {}

BPlusTree::~BPlusTree() = default;

BPlusTree::BPlusTree(BPlusTree &&other) noexcept = default;

BPlusTree &BPlusTree::operator=(BPlusTree &&other) noexcept = default;

bool BPlusTree::insert(int key) {
    return m_impl->insert(key);
}

bool BPlusTree::search(int key) {
    return m_impl->search(key);
}

bool BPlusTree::erase(int key) {
    return m_impl->erase(key);
}

void BPlusTree::dfs() {
    m_impl->dfs();
}

}
