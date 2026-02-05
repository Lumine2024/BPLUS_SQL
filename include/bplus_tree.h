#ifndef __BPLUS_TREE_H__
#define __BPLUS_TREE_H__

#include <memory>
#include <string>

namespace bplus_sql {

class BPlusTree {
public:
    static constexpr int MIN_KEYS = 64;
    static constexpr int MAX_KEYS = 128;

    explicit BPlusTree(const std::string &fileName);
    ~BPlusTree();

    BPlusTree(const BPlusTree &) = delete;
    BPlusTree &operator=(const BPlusTree &) = delete;
    BPlusTree(BPlusTree &&) noexcept;
    BPlusTree &operator=(BPlusTree &&) noexcept;

    bool insert(int key);
    bool search(int key);
    bool erase(int key);
    void dfs();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}

#endif
