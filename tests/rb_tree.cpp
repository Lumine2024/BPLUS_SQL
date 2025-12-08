#include "bplus_tree.h"
#include <iostream>
#include <random>
#include <chrono>
#include <cassert>

class RbTree {
public:
    RbTree() : m_pager((std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "_test_for_rb_tree.bin").string()) {
        
    }
    ~RbTree() {
        
    }
    void insert(int key) {

    }
    void erase(int key) {

    }
    bool contains(int key) const {

    }
private:
    struct RbTreeNode {
        int key;
        size_t son[2];
    };
    bplus_sql::Pager m_pager;
};

int main(int argc, char *argv[]) {
    RbTree cmp;
	auto dst = std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "test.bin";
	bplus_sql::BPlusTree tree(dst.string());
	std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<int> ukey(1, 100000), uop(1, 2);
    // validate correctness
	for(int i = 0; i < 100000; ++i) {
		int o = uop(rng), v = ukey(rng);
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
    // record time
    auto before_all = std::chrono::steady_clock::now();
	for(int i = 0; i < 100000; ++i) {
		int o = uop(rng), v = ukey(rng);
		if(o == 1) {
			tree.insert(v);
		} else {
			tree.erase(v);
		}
	}
    auto after_testing_bplus = std::chrono::steady_clock::now();
    for(int i = 0; i < 100000; ++i) {
        int o = uop(rng), v = ukey(rng);
        if(o == 1) {
            cmp.insert(v);
        } else {
            cmp.erase(v);
        }
    }
    auto after_testing_rb = std::chrono::steady_clock::now();
    std::cout << "B+ Tree cost " << (after_testing_bplus - before_all) << '\n'
              << "Rb Tree cost " << (after_testing_rb - after_testing_bplus) << '\n';
    return 0;
}