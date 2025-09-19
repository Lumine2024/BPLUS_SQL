
#include "bplus_tree.h"
#include <iostream>
#include <vector>

int main(int argc, char *argv[]) {
    try {
        std::cout << "Creating B+ tree...\n";
        
        // Create a B+ tree with a test database file
        bplus_sql::BPlusTree tree("test_db.bin");
        
        std::cout << "B+ tree created successfully\n";
        std::cout << "Testing single insert...\n";
        
        // Test single insertion
        bool success = tree.insert(10);
        std::cout << "Insert result: " << (success ? "SUCCESS" : "FAILED") << "\n";
        
        std::cout << "Testing search...\n";
        bool found = tree.search(10);
        std::cout << "Search result: " << (found ? "FOUND" : "NOT FOUND") << "\n";
        
        std::cout << "Test completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}