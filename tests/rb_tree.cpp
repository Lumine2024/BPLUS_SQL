#include "bplus_tree.h"
#include "pager.h"
#include "lru.h"
#include <iostream>
#include <random>
#include <chrono>
#include <cassert>
#include <memory>
#include <vector>
#include <unordered_map>
#include <list>

// Red-Black Tree node colors
enum class Color : uint8_t { RED = 0, BLACK = 1 };

// Red-Black Tree node structure for disk storage
struct RBNode {
    int key;
    Color color;
    char _padding1[3]; // Align to 8 bytes
    size_t parent;
    size_t left;   // left child page id
    size_t right;  // right child page id
    char padding[4064]; // 4096 - 4 - 1 - 3 - 8 - 8 - 8 = 4064
};

static_assert(sizeof(RBNode) <= 4096, "RBNode must fit in a page");

// LRU Cache for RB tree nodes
class RBNodeCache {
public:
    static constexpr size_t CAPACITY = 1024;
    
    std::shared_ptr<RBNode> get(size_t pageId) {
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
    
    void put(size_t pageId, std::shared_ptr<RBNode> node) {
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
        return m_cache.find(pageId) != m_cache.end();
    }
    
    size_t size() const {
        return m_cache.size();
    }
    
    std::pair<size_t, std::shared_ptr<RBNode>> tail() const {
        if (m_list.empty()) {
            return {0, nullptr};
        }
        size_t backId = m_list.back();
        return {backId, m_cache.at(backId).first};
    }
    
    template<typename Func>
    void traverse(Func&& fn) {
        for(auto& [pageId, pair] : m_cache) {
            fn(pageId, pair.first);
        }
    }
    
private:
    void evictLRU() {
        if (!m_list.empty()) {
            size_t lru_page = m_list.back();
            m_list.pop_back();
            m_cache.erase(lru_page);
        }
    }
    
    std::list<size_t> m_list;
    std::unordered_map<size_t, std::pair<std::shared_ptr<RBNode>, std::list<size_t>::iterator>> m_cache;
};

class RbTree {
public:
    static constexpr size_t NIL = 0; // NIL node is at page 0
    
    RbTree() : m_pager((std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "_test_for_rb_tree.bin").string()),
               m_root(NIL), m_nextPageId(1) {
        // Initialize NIL node
        RBNode nilNode;
        nilNode.key = 0;
        nilNode.color = Color::BLACK;
        nilNode.parent = NIL;
        nilNode.left = NIL;
        nilNode.right = NIL;
        writeNode(NIL, nilNode);
    }
    
    ~RbTree() {
        // Flush cache to disk
        m_cache.traverse([this](size_t pageId, std::shared_ptr<RBNode> node) {
            m_pager.writePage(pageId, *node);
        });
    }
    
    void insert(int key) {
        size_t z = allocateNode();
        RBNode zNode;
        zNode.key = key;
        zNode.color = Color::RED;
        zNode.parent = NIL;
        zNode.left = NIL;
        zNode.right = NIL;
        writeNode(z, zNode);
        
        size_t y = NIL;
        size_t x = m_root;
        
        while (x != NIL) {
            y = x;
            RBNode xNode = readNode(x);
            if (key < xNode.key) {
                x = xNode.left;
            } else if (key > xNode.key) {
                x = xNode.right;
            } else {
                // Key already exists, don't insert
                return;
            }
        }
        
        zNode.parent = y;
        writeNode(z, zNode);
        
        if (y == NIL) {
            m_root = z;
        } else {
            RBNode yNode = readNode(y);
            if (key < yNode.key) {
                yNode.left = z;
            } else {
                yNode.right = z;
            }
            writeNode(y, yNode);
        }
        
        insertFixup(z);
    }
    
    void erase(int key) {
        size_t z = findNode(key);
        if (z == NIL) {
            return; // Key not found
        }
        
        RBNode zNode = readNode(z);
        size_t y = z;
        Color yOriginalColor = zNode.color;
        size_t x;
        
        if (zNode.left == NIL) {
            x = zNode.right;
            transplant(z, zNode.right);
        } else if (zNode.right == NIL) {
            x = zNode.left;
            transplant(z, zNode.left);
        } else {
            y = minimum(zNode.right);
            RBNode yNode = readNode(y);
            yOriginalColor = yNode.color;
            x = yNode.right;
            
            if (yNode.parent == z) {
                if (x != NIL) {
                    RBNode xNode = readNode(x);
                    xNode.parent = y;
                    writeNode(x, xNode);
                }
            } else {
                transplant(y, yNode.right);
                yNode = readNode(y);
                yNode.right = zNode.right;
                writeNode(y, yNode);
                
                RBNode rightNode = readNode(yNode.right);
                rightNode.parent = y;
                writeNode(yNode.right, rightNode);
            }
            
            transplant(z, y);
            yNode = readNode(y);
            yNode.left = zNode.left;
            writeNode(y, yNode);
            
            RBNode leftNode = readNode(yNode.left);
            leftNode.parent = y;
            writeNode(yNode.left, leftNode);
            
            yNode = readNode(y);
            yNode.color = zNode.color;
            writeNode(y, yNode);
        }
        
        if (yOriginalColor == Color::BLACK) {
            deleteFixup(x);
        }
    }
    
    bool contains(int key) const {
        return const_cast<RbTree*>(this)->findNode(key) != NIL;
    }
    
private:
    RBNode readNode(size_t pageId) {
        if (m_cache.contains(pageId)) {
            return *m_cache.get(pageId);
        }
        
        RBNode node;
        m_pager.readPage(pageId, node);
        
        if (m_cache.size() >= RBNodeCache::CAPACITY) {
            auto [tailId, tailNode] = m_cache.tail();
            if (tailNode != nullptr) {
                m_pager.writePage(tailId, *tailNode);
            }
        }
        
        m_cache.put(pageId, std::make_shared<RBNode>(node));
        return node;
    }
    
    void writeNode(size_t pageId, const RBNode& node) {
        auto cached = std::make_shared<RBNode>(node);
        
        if (m_cache.contains(pageId)) {
            m_cache.put(pageId, cached);
        } else {
            if (m_cache.size() >= RBNodeCache::CAPACITY) {
                auto [tailId, tailNode] = m_cache.tail();
                if (tailNode != nullptr) {
                    m_pager.writePage(tailId, *tailNode);
                }
            }
            m_cache.put(pageId, cached);
        }
    }
    
    size_t allocateNode() {
        return m_nextPageId++;
    }
    
    size_t findNode(int key) {
        size_t current = m_root;
        while (current != NIL) {
            RBNode node = readNode(current);
            if (key == node.key) {
                return current;
            } else if (key < node.key) {
                current = node.left;
            } else {
                current = node.right;
            }
        }
        return NIL;
    }
    
    void leftRotate(size_t x) {
        RBNode xNode = readNode(x);
        size_t y = xNode.right;
        RBNode yNode = readNode(y);
        
        xNode.right = yNode.left;
        writeNode(x, xNode);
        
        if (yNode.left != NIL) {
            RBNode leftNode = readNode(yNode.left);
            leftNode.parent = x;
            writeNode(yNode.left, leftNode);
        }
        
        yNode.parent = xNode.parent;
        writeNode(y, yNode);
        
        if (xNode.parent == NIL) {
            m_root = y;
        } else {
            RBNode parentNode = readNode(xNode.parent);
            if (x == parentNode.left) {
                parentNode.left = y;
            } else {
                parentNode.right = y;
            }
            writeNode(xNode.parent, parentNode);
        }
        
        yNode = readNode(y);
        yNode.left = x;
        writeNode(y, yNode);
        
        xNode = readNode(x);
        xNode.parent = y;
        writeNode(x, xNode);
    }
    
    void rightRotate(size_t y) {
        RBNode yNode = readNode(y);
        size_t x = yNode.left;
        RBNode xNode = readNode(x);
        
        yNode.left = xNode.right;
        writeNode(y, yNode);
        
        if (xNode.right != NIL) {
            RBNode rightNode = readNode(xNode.right);
            rightNode.parent = y;
            writeNode(xNode.right, rightNode);
        }
        
        xNode.parent = yNode.parent;
        writeNode(x, xNode);
        
        if (yNode.parent == NIL) {
            m_root = x;
        } else {
            RBNode parentNode = readNode(yNode.parent);
            if (y == parentNode.left) {
                parentNode.left = x;
            } else {
                parentNode.right = x;
            }
            writeNode(yNode.parent, parentNode);
        }
        
        xNode = readNode(x);
        xNode.right = y;
        writeNode(x, xNode);
        
        yNode = readNode(y);
        yNode.parent = x;
        writeNode(y, yNode);
    }
    
    void insertFixup(size_t z) {
        while (true) {
            RBNode zNode = readNode(z);
            if (zNode.parent == NIL) break;
            
            RBNode parentNode = readNode(zNode.parent);
            if (parentNode.color != Color::RED) break;
            
            RBNode grandparentNode = readNode(parentNode.parent);
            
            if (zNode.parent == grandparentNode.left) {
                size_t y = grandparentNode.right;
                RBNode yNode = readNode(y);
                
                if (yNode.color == Color::RED) {
                    parentNode.color = Color::BLACK;
                    writeNode(zNode.parent, parentNode);
                    
                    yNode.color = Color::BLACK;
                    writeNode(y, yNode);
                    
                    grandparentNode.color = Color::RED;
                    writeNode(parentNode.parent, grandparentNode);
                    
                    z = parentNode.parent;
                } else {
                    if (z == parentNode.right) {
                        z = zNode.parent;
                        leftRotate(z);
                        zNode = readNode(z);
                        parentNode = readNode(zNode.parent);
                    }
                    
                    parentNode.color = Color::BLACK;
                    writeNode(zNode.parent, parentNode);
                    
                    grandparentNode = readNode(parentNode.parent);
                    grandparentNode.color = Color::RED;
                    writeNode(parentNode.parent, grandparentNode);
                    
                    rightRotate(parentNode.parent);
                }
            } else {
                size_t y = grandparentNode.left;
                RBNode yNode = readNode(y);
                
                if (yNode.color == Color::RED) {
                    parentNode.color = Color::BLACK;
                    writeNode(zNode.parent, parentNode);
                    
                    yNode.color = Color::BLACK;
                    writeNode(y, yNode);
                    
                    grandparentNode.color = Color::RED;
                    writeNode(parentNode.parent, grandparentNode);
                    
                    z = parentNode.parent;
                } else {
                    if (z == parentNode.left) {
                        z = zNode.parent;
                        rightRotate(z);
                        zNode = readNode(z);
                        parentNode = readNode(zNode.parent);
                    }
                    
                    parentNode.color = Color::BLACK;
                    writeNode(zNode.parent, parentNode);
                    
                    grandparentNode = readNode(parentNode.parent);
                    grandparentNode.color = Color::RED;
                    writeNode(parentNode.parent, grandparentNode);
                    
                    leftRotate(parentNode.parent);
                }
            }
        }
        
        RBNode rootNode = readNode(m_root);
        rootNode.color = Color::BLACK;
        writeNode(m_root, rootNode);
    }
    
    void deleteFixup(size_t x) {
        while (x != m_root && x != NIL) {
            RBNode xNode = readNode(x);
            if (xNode.color == Color::BLACK) {
                if (xNode.parent == NIL) break;
                
                RBNode parentNode = readNode(xNode.parent);
                
                if (x == parentNode.left) {
                    size_t w = parentNode.right;
                    RBNode wNode = readNode(w);
                    
                    if (wNode.color == Color::RED) {
                        wNode.color = Color::BLACK;
                        writeNode(w, wNode);
                        
                        parentNode.color = Color::RED;
                        writeNode(xNode.parent, parentNode);
                        
                        leftRotate(xNode.parent);
                        
                        xNode = readNode(x);
                        parentNode = readNode(xNode.parent);
                        w = parentNode.right;
                        wNode = readNode(w);
                    }
                    
                    RBNode wLeftNode = readNode(wNode.left);
                    RBNode wRightNode = readNode(wNode.right);
                    
                    if (wLeftNode.color == Color::BLACK && wRightNode.color == Color::BLACK) {
                        wNode.color = Color::RED;
                        writeNode(w, wNode);
                        x = xNode.parent;
                    } else {
                        if (wRightNode.color == Color::BLACK) {
                            wLeftNode.color = Color::BLACK;
                            writeNode(wNode.left, wLeftNode);
                            
                            wNode.color = Color::RED;
                            writeNode(w, wNode);
                            
                            rightRotate(w);
                            
                            xNode = readNode(x);
                            parentNode = readNode(xNode.parent);
                            w = parentNode.right;
                            wNode = readNode(w);
                        }
                        
                        wNode.color = parentNode.color;
                        writeNode(w, wNode);
                        
                        parentNode.color = Color::BLACK;
                        writeNode(xNode.parent, parentNode);
                        
                        wRightNode = readNode(wNode.right);
                        wRightNode.color = Color::BLACK;
                        writeNode(wNode.right, wRightNode);
                        
                        leftRotate(xNode.parent);
                        x = m_root;
                    }
                } else {
                    size_t w = parentNode.left;
                    RBNode wNode = readNode(w);
                    
                    if (wNode.color == Color::RED) {
                        wNode.color = Color::BLACK;
                        writeNode(w, wNode);
                        
                        parentNode.color = Color::RED;
                        writeNode(xNode.parent, parentNode);
                        
                        rightRotate(xNode.parent);
                        
                        xNode = readNode(x);
                        parentNode = readNode(xNode.parent);
                        w = parentNode.left;
                        wNode = readNode(w);
                    }
                    
                    RBNode wLeftNode = readNode(wNode.left);
                    RBNode wRightNode = readNode(wNode.right);
                    
                    if (wRightNode.color == Color::BLACK && wLeftNode.color == Color::BLACK) {
                        wNode.color = Color::RED;
                        writeNode(w, wNode);
                        x = xNode.parent;
                    } else {
                        if (wLeftNode.color == Color::BLACK) {
                            wRightNode.color = Color::BLACK;
                            writeNode(wNode.right, wRightNode);
                            
                            wNode.color = Color::RED;
                            writeNode(w, wNode);
                            
                            leftRotate(w);
                            
                            xNode = readNode(x);
                            parentNode = readNode(xNode.parent);
                            w = parentNode.left;
                            wNode = readNode(w);
                        }
                        
                        wNode.color = parentNode.color;
                        writeNode(w, wNode);
                        
                        parentNode.color = Color::BLACK;
                        writeNode(xNode.parent, parentNode);
                        
                        wLeftNode = readNode(wNode.left);
                        wLeftNode.color = Color::BLACK;
                        writeNode(wNode.left, wLeftNode);
                        
                        rightRotate(xNode.parent);
                        x = m_root;
                    }
                }
            } else {
                break;
            }
        }
        
        if (x != NIL) {
            RBNode xNode = readNode(x);
            xNode.color = Color::BLACK;
            writeNode(x, xNode);
        }
    }
    
    void transplant(size_t u, size_t v) {
        RBNode uNode = readNode(u);
        
        if (uNode.parent == NIL) {
            m_root = v;
        } else {
            RBNode parentNode = readNode(uNode.parent);
            if (u == parentNode.left) {
                parentNode.left = v;
            } else {
                parentNode.right = v;
            }
            writeNode(uNode.parent, parentNode);
        }
        
        if (v != NIL) {
            RBNode vNode = readNode(v);
            vNode.parent = uNode.parent;
            writeNode(v, vNode);
        }
    }
    
    size_t minimum(size_t x) {
        while (true) {
            RBNode xNode = readNode(x);
            if (xNode.left == NIL) {
                return x;
            }
            x = xNode.left;
        }
    }
    
    bplus_sql::Pager m_pager;
    RBNodeCache m_cache;
    size_t m_root;
    size_t m_nextPageId;
};

int main(int argc, char *argv[]) {
    std::cout << "Initializing trees and generating test operations...\n";
    
    RbTree cmp;
	auto dst = std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "test.bin";
	bplus_sql::BPlusTree tree(dst.string());
	std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<int> ukey(1, 100000), uop(1, 2);
    
    // Preprocess operations for correctness testing
    std::vector<std::pair<int, int>> correctness_ops;
    correctness_ops.reserve(100000);
    for(int i = 0; i < 100000; ++i) {
        correctness_ops.emplace_back(uop(rng), ukey(rng));
    }
    
    std::cout << "Running correctness validation...\n";
    // validate correctness
	for(const auto& [o, v] : correctness_ops) {
		bool c1 = tree.search(v), c2 = cmp.contains(v);
		assert(c1 == c2);
		if(o == 1) {
			tree.insert(v);
			cmp.insert(v);
		} else {
			tree.erase(v);
			cmp.erase(v);
		}
	}
    
    std::cout << "Correctness validation passed!\n";
    std::cout << "Generating performance test operations...\n";
    
    // Preprocess operations for performance testing
    std::vector<std::pair<int, int>> perf_ops;
    perf_ops.reserve(100000);
    for(int i = 0; i < 100000; ++i) {
        perf_ops.emplace_back(uop(rng), ukey(rng));
    }
    
    std::cout << "Running performance tests...\n";
    
    // Test B+ tree performance
    auto before_all = std::chrono::steady_clock::now();
	for(const auto& [o, v] : perf_ops) {
		if(o == 1) {
			tree.insert(v);
		} else {
			tree.erase(v);
		}
	}
    auto after_testing_bplus = std::chrono::steady_clock::now();
    
    // Test RB tree performance
    for(const auto& [o, v] : perf_ops) {
        if(o == 1) {
            cmp.insert(v);
        } else {
            cmp.erase(v);
        }
    }
    auto after_testing_rb = std::chrono::steady_clock::now();
    
    auto bplus_time = std::chrono::duration_cast<std::chrono::milliseconds>(after_testing_bplus - before_all);
    auto rb_time = std::chrono::duration_cast<std::chrono::milliseconds>(after_testing_rb - after_testing_bplus);
    
    std::cout << "\n=== Performance Results ===\n";
    std::cout << "B+ Tree: " << bplus_time.count() << " ms\n";
    std::cout << "RB Tree: " << rb_time.count() << " ms\n";
    std::cout << "Speedup: " << (double)rb_time.count() / bplus_time.count() << "x\n";
    
    return 0;
}