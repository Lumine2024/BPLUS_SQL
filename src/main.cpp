
#include "bplus_tree.h"
#include <iostream>
#include <vector>

int main(int argc, char *argv[]) {
    try {
        std::cout << "Creating B+ tree...\n";
        
        // Create a B+ tree with a test database file
        bplus_sql::BPlusTree tree("test_db.bin");
        
        std::cout << "Testing B+ Tree operations...\n";
        
        // Test insertions with a small dataset first
        std::vector<int> testKeys = {10, 20, 5, 15, 25};
        
        std::cout << "Inserting keys: ";
        for (int key : testKeys) {
            std::cout << key << " ";
            bool success = tree.insert(key);
            if (!success) {
                std::cout << "(failed) ";
            }
        }
        std::cout << "\n";
        
        // Test search
        std::cout << "Search results:\n";
        for (int key : testKeys) {
            bool found = tree.search(key);
            std::cout << "Key " << key << ": " << (found ? "FOUND" : "NOT FOUND") << "\n";
        }
        
        // Test search for non-existent keys
        std::vector<int> nonExistentKeys = {1, 3, 8, 50};
        std::cout << "Searching for non-existent keys:\n";
        for (int key : nonExistentKeys) {
            bool found = tree.search(key);
            std::cout << "Key " << key << ": " << (found ? "FOUND" : "NOT FOUND") << "\n";
        }
        
        // Test deletion
        std::cout << "Deleting key 15...\n";
        bool deleted = tree.erase(15);
        std::cout << "Deletion " << (deleted ? "successful" : "failed") << "\n";
        
        // Verify deletion
        bool found = tree.search(15);
        std::cout << "Key 15 after deletion: " << (found ? "FOUND" : "NOT FOUND") << "\n";
        
        std::cout << "B+ Tree test completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}