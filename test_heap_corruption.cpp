#include "bplus_tree.h"
#include <iostream>
#include <random>
#include <chrono>
#include <set>
#include <filesystem>
#include <cassert>

// Test to reproduce the heap corruption bug that occurred when internal nodes
// exceeded MAX_KEYS (128) bounds. The bug was in the insert function where
// full internal nodes were allowed to exceed their capacity instead of being split.

int main() {
    std::set<int> reference;
    auto dbPath = std::filesystem::path(__FILE__).parent_path() / "data" / "test_heap_corruption.bin";
    
    // Clean up any existing test database
    if (std::filesystem::exists(dbPath)) {
        std::filesystem::remove(dbPath);
    }
    
    bplus_sql::BPlusTree tree(dbPath.string());
    std::mt19937 rng(12345); // Fixed seed for reproducibility
    
    // Use parameters suggested in the bug report:
    // Random keys from 1 to 100000, repeat 10000000 times
    std::uniform_int_distribution<int> keyDist(1, 100000);
    std::uniform_int_distribution<int> opDist(1, 2); // 1=insert, 2=erase
    
    std::cout << "Testing heap corruption fix with 10M operations..." << std::endl;
    std::cout << "Key range: 1-100000" << std::endl;
    
    const int TOTAL_OPS = 10000000;
    const int REPORT_INTERVAL = 1000000;
    
    for (int i = 0; i < TOTAL_OPS; ++i) {
        int operation = opDist(rng);
        int key = keyDist(rng);
        
        // Verify search consistency before operation
        bool treeHasKey = tree.search(key);
        bool refHasKey = reference.contains(key);
        
        if (treeHasKey != refHasKey) {
            std::cerr << "ERROR: Search inconsistency at operation " << i << std::endl;
            std::cerr << "Key: " << key << ", Tree: " << treeHasKey 
                      << ", Reference: " << refHasKey << std::endl;
            return 1;
        }
        
        // Perform operation
        if (operation == 1) {
            tree.insert(key);
            reference.insert(key);
        } else {
            tree.erase(key);
            reference.erase(key);
        }
        
        // Progress reporting
        if ((i + 1) % REPORT_INTERVAL == 0) {
            std::cout << "Completed " << (i + 1) << " operations. "
                      << "Current tree size: " << reference.size() << std::endl;
        }
    }
    
    // Final verification
    std::cout << "\nFinal verification..." << std::endl;
    for (int key : reference) {
        if (!tree.search(key)) {
            std::cerr << "ERROR: Key " << key << " missing from tree!" << std::endl;
            return 1;
        }
    }
    
    std::cout << "\n✓ Test passed! All " << TOTAL_OPS << " operations completed successfully." << std::endl;
    std::cout << "✓ Final tree size: " << reference.size() << " keys" << std::endl;
    std::cout << "✓ No heap corruption detected." << std::endl;
    
    return 0;
}
