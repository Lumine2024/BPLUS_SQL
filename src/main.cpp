
#include "bplus_tree.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <set>
#include <filesystem>
#include <cassert>

int main(int argc, char *argv[]) {
    // pressure test
    std::set<int> cmp;
	auto dst = std::filesystem::path(__FILE__).parent_path() / "data" / "test.bin";
	bplus_sql::BPlusTree tree(dst.string());
	std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<int> ukey(1, 10000), uop(1, 2);
	for(int i = 0; i < 10000000; ++i) {
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
    return 0;
}