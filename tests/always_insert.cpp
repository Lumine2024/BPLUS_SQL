#include "bplus_tree.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <set>
#include <filesystem>
#include <cassert>

int main(int argc, char *argv[]) {
	auto dst = std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "test.bin";
	bplus_sql::BPlusTree tree(dst.string());
    for(int i = 0; i < 10000000; ++i) tree.insert(i);
    for(int i = 0; i < 10000000; ++i) assert(tree.search(i));
    return 0;
}