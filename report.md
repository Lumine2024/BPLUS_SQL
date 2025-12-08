# BPLUS_SQL: A Database that Store Integers, Based on B+ Tree

## Summary

BPLUS_SQL is a disk-based database engine implementation for storing and managing integers using a B+ tree data structure. This project was developed as part of the 2024 Data Structures and Algorithms Challenge Course Design. The system features a complete SQL-like command parser, persistent storage through a custom paging system, an LRU cache for performance optimization, and comprehensive testing infrastructure including performance comparisons with Red-Black trees.

The project consists of approximately 1,967 lines of C++ code and implements core database operations (CREATE, INSERT, QUERY, ERASE, DESTROY) with full disk persistence, allowing databases to survive program restarts. The implementation uses modern C++23 features and has undergone extensive testing to ensure correctness and performance.

## What is a B+ Tree?

A B+ tree is a self-balancing tree data structure that maintains sorted data and allows searches, sequential access, insertions, and deletions in logarithmic time. It is an extension of the B-tree and is widely used in database systems and file systems.

**Key characteristics of B+ trees:**

1. **Multi-way tree structure**: Unlike binary trees, each node can have multiple keys (up to 128 in this implementation) and children (up to 129), which reduces tree height and improves cache locality.

2. **All data in leaf nodes**: Internal nodes only store routing keys to guide searches, while all actual data resides in leaf nodes. This design makes range queries efficient.

3. **Linked leaf nodes**: Leaf nodes are linked together in a sequential chain (via the `next` pointer), enabling efficient sequential traversal and range queries.

4. **Balanced structure**: All leaf nodes are at the same depth, guaranteeing O(log n) worst-case performance for all operations.

5. **High fanout**: With a maximum of 128 keys per node and minimum of 64 keys (except root), the tree stays shallow even with millions of records, minimizing disk I/O operations.

**Why B+ trees for databases?**
- Minimizes disk accesses due to shallow tree height
- Efficient range queries through linked leaf nodes
- Excellent cache performance from reading entire pages at once
- Predictable performance characteristics
- Natural fit for page-based storage systems

## What issues did I encounter, and how I conquer them

Throughout the development of BPLUS_SQL, I encountered several significant challenges that required careful debugging and algorithmic thinking:

### 1. **Heap Corruption Error (PR #4)**
**Problem**: The program crashed with heap corruption errors during intensive insert operations.

**Root Cause**: The internal node splitting logic was incorrectly implemented. When an internal node needed to split, the key distribution and child pointer management were not properly handling the middle key that should be promoted to the parent.

**Solution**: Redesigned the `splitNonLeaf()` function to properly:
- Create temporary arrays for all keys and children
- Find the correct split point at the middle
- Keep the split key in the right sibling (valid for B+ trees since internal keys are routing keys)
- Correctly distribute children pointers to both nodes

### 2. **Pager Windows Crash (PR #3)**
**Problem**: On Windows systems, the program crashed when working with large datasets due to in-memory vector storage limitations.

**Root Cause**: The original design stored all tree nodes in memory using vectors, which quickly exhausted available RAM and caused crashes when handling large amounts of data.

**Solution**: Implemented a complete file-based paging system (`pager.h`) that:
- Stores nodes persistently in 4KB pages on disk
- Uses proper file I/O with `std::fstream` for cross-platform compatibility
- Implements dynamic file expansion as needed
- Separates metadata (stored in the first page) from data pages
- Uses heap allocation for large buffers to avoid stack overflow

### 3. **Test Existing Tree Failure (PR #5)**
**Problem**: After closing and reopening a database file, the tree structure was lost and queries returned incorrect results.

**Root Cause**: Tree metadata (root page ID and next available page ID) was not being persisted to disk, causing the tree to reinitialize on every program start.

**Solution**: 
- Designed a `TreeMetadata` structure to store critical tree state
- Reserved the first page of each database file for metadata
- Implemented `saveMetadata()` and `loadMetadata()` methods
- Saved metadata on tree destruction to ensure persistence
- Modified constructor to check for existing metadata and load it if available

### 4. **Pressure Test Performance Issues (PR #2)**
**Problem**: The tree failed stress tests with large numbers of random insert/delete operations.

**Root Cause**: Two issues were found:
1. Node capacity was temporarily reduced for testing, causing excessive splitting
2. Memory management needed optimization for high-performance scenarios

**Solution**:
- Restored proper node capacity (128 keys, 64 minimum)
- Implemented proper dynamic memory allocation for temporary arrays during splits
- Optimized the split logic to minimize memory allocations

### 5. **Test Runner Script Issues (PR #7)**
**Problem**: Test automation scripts failed when run from different directories or when test files didn't exist.

**Root Cause**: Hard-coded paths and missing file existence checks.

**Solution**: Refactored both PowerShell and Bash test runner scripts to:
- Calculate paths relative to script location
- Check for file existence before execution
- Provide clear error messages
- Support execution from any directory

### 6. **LRU Cache Implementation (PR #6)**
**Problem**: Frequent disk I/O was slowing down operations significantly.

**Root Cause**: Every node access required a disk read and write operation.

**Solution**: Implemented a complete LRU cache system:
- Created `LRUCache` class with configurable capacity (1024 nodes)
- Used `std::list` for LRU ordering and `std::unordered_map` for O(1) lookups
- Implemented `NodeManager` to coordinate between cache and pager
- Added automatic write-back on cache eviction
- Ensured all dirty nodes are flushed on program termination

## How can I validate the correctness of my data structure?

The project employs a comprehensive multi-layered testing strategy to ensure correctness:

### 1. **Comparison Testing with std::set**
The primary validation approach (`pressure_test.cpp`) uses C++'s `std::set` as a reference implementation:
```cpp
// For 10 million random operations:
// - Compare search results with std::set
// - Perform identical insert/erase operations on both structures
// - Assert that results always match
```
This test performs 10 million random operations, continuously validating that the B+ tree's behavior matches the proven-correct standard library implementation.

### 2. **Persistence Testing**
`test_existing_tree.cpp` validates that the database can survive program restarts:
- Creates a tree and inserts specific values
- Closes the tree (destructor called)
- Reopens the same file
- Verifies all previously inserted values are still present
- Ensures tree structure is properly maintained across sessions

### 3. **Command Parser Testing**
`parse_commands.cpp` tests the SQL-like command interface:
- Validates parsing of CREATE, INSERT, QUERY, ERASE, and DESTROY commands
- Tests case-insensitive command recognition
- Ensures proper extraction of table names and key values

### 4. **Performance Benchmarking**
`rb_tree.cpp` implements a complete disk-based Red-Black tree for performance comparison:
- Runs identical workloads on both B+ tree and Red-Black tree
- Measures and compares execution times
- Validates that B+ tree performance is competitive or superior
- Demonstrates the advantages of B+ trees for disk-based storage (better cache locality, fewer disk seeks)

### 5. **Continuous Insertion Testing**
`always_insert.cpp` performs sustained insertion operations:
- Tests that the tree can handle continuously growing datasets
- Validates splitting behavior under continuous load
- Ensures no memory leaks or resource exhaustion

### 6. **Automated Test Suite**
Scripts (`run_all_tests.sh` and `run_all_tests.ps1`) automate the entire test process:
- Compile all test executables
- Run each test in sequence
- Report pass/fail status
- Support for cross-platform testing (Linux/macOS via Bash, Windows via PowerShell)

This comprehensive testing approach ensures that the B+ tree implementation is:
- **Correct**: Matches behavior of proven standard library containers
- **Persistent**: Maintains data across program restarts
- **Performant**: Meets or exceeds alternative data structures
- **Robust**: Handles edge cases and stress conditions

## How to use it?

### Building the Project

The project uses CMake for cross-platform builds. Requirements:
- C++23 compatible compiler (GCC 12+, Clang 16+, MSVC 2022+)
- CMake 3.10 or higher

```bash
# Clone the repository
git clone https://github.com/Lumine2024/BPLUS_SQL.git
cd BPLUS_SQL

# Create build directory and compile
mkdir build
cd build
cmake ..
cmake --build .
```

### Running the Database

The main program accepts SQL-like commands interactively or from a file:

```bash
# Interactive mode
./main

# Batch mode from file
./main commands.txt
```

### Command Syntax

The database supports the following operations:

1. **CREATE TABLE** - Initialize a new database table
   ```sql
   CREATE TABLE users
   ```

2. **INSERT** - Add an integer key to the table
   ```sql
   INSERT INTO users KEY 42
   INSERT INTO users KEY 123
   ```

3. **QUERY** - Search for a key (returns 1 if found, 0 if not)
   ```sql
   QUERY FROM users KEY 42
   ```

4. **ERASE** - Remove a key from the table
   ```sql
   ERASE FROM users KEY 42
   ```

5. **DESTROY TABLE** - Delete the entire table and its disk file
   ```sql
   DESTROY TABLE users
   ```

6. **EXIT** - Close the database
   ```sql
   EXIT
   ```

**Notes:**
- Commands are case-insensitive
- Database files are stored in the `data/` directory as `<tablename>.bin`
- Each table is a separate B+ tree with its own file
- Tables automatically persist to disk and survive program restarts

### Running Tests

```bash
# On Linux/macOS
cd tests
./run_all_tests.sh

# On Windows
cd tests
.\run_all_tests.ps1

# Run individual tests
./pressure_test      # 10M random operations validation
./test_existing_tree # Persistence testing
./rb_tree           # Performance comparison
```

## What can be enhanced furthermore?

While BPLUS_SQL successfully implements core B+ tree functionality, several enhancements could significantly improve the system:

### 1. **Support for Range Queries**
Currently, the system only supports exact key searches. Implementing range queries would leverage the linked-leaf structure:
```cpp
std::vector<int> rangeQuery(int start, int end);
```
This would enable queries like "find all keys between 100 and 200."

### 2. **Concurrent Access with Locking**
The current implementation is single-threaded. Adding support for concurrent operations would make it production-ready:
- Implement B-link tree variant for lock-free reads
- Add page-level or node-level locking
- Support MVCC (Multi-Version Concurrency Control) for better concurrency

### 3. **Support for Multiple Data Types**
Currently limited to integers. Could be extended to:
- Variable-length strings (requires special handling for page layout)
- Floating-point numbers
- Custom composite keys
- Template-based generic implementation

### 4. **Advanced Deletion with Merging/Redistribution**
Current deletion removes keys but doesn't rebalance. Full implementation should:
- Merge underfull nodes with siblings
- Redistribute keys between siblings
- Maintain optimal tree structure after deletions
- Current stub: `void mergeOrRedistribute(size_t pageId) {}`

### 5. **Transaction Support**
Add ACID properties for database reliability:
- Write-ahead logging (WAL) for durability
- Rollback capability for failed operations
- Commit/abort transaction commands
- Recovery mechanisms after crashes

### 6. **Query Optimization**
- Index statistics for query planning
- Query caching for frequently accessed keys
- Bulk loading optimization for initial data import
- Batch operations for improved performance

### 7. **Compression and Space Efficiency**
- Prefix compression for keys in internal nodes
- Variable-length page encoding to reduce wasted space
- Automatic vacuuming to reclaim deleted space

### 8. **Enhanced Monitoring and Debugging**
- Tree statistics (height, fill factor, fragmentation)
- Performance metrics (cache hit rate, I/O operations)
- Visualization tools for tree structure
- Detailed logging for operations

### 9. **Secondary Indexes**
Allow multiple B+ trees to index the same data from different perspectives, supporting complex query patterns.

### 10. **Improved Error Handling**
- More descriptive error messages
- Exception safety guarantees
- Graceful degradation on disk errors
- Automatic error recovery

## Conclusion

BPLUS_SQL successfully demonstrates a complete implementation of a disk-based B+ tree database system for integer storage. The project showcases several important systems programming concepts:

**Technical Achievements:**
- Fully functional B+ tree with insert, search, and delete operations
- Persistent storage system with custom paging implementation
- LRU cache for performance optimization
- Comprehensive metadata management for tree state persistence
- SQL-like command interface for intuitive interaction
- Extensive testing including correctness validation and performance benchmarking

**Lessons Learned:**
The development process highlighted the challenges of building database systems, including proper memory management, handling disk I/O efficiently, maintaining data structure invariants during complex operations, and ensuring data persistence across program restarts. Each problem encountered—from heap corruption to cache management—provided valuable insights into low-level systems programming and algorithm implementation.

**Project Impact:**
This implementation serves as an educational reference for understanding how real database systems work internally. It demonstrates that B+ trees are not just theoretical constructs but practical tools for building efficient, persistent data storage systems. The code is well-documented, thoroughly tested, and available under the MIT license for others to learn from and build upon.

**Future Outlook:**
While the current implementation provides a solid foundation, the suggested enhancements outline a path toward a production-ready database system. The modular architecture (separated pager, cache, node manager, and tree logic) makes future extensions feasible without major refactoring.

BPLUS_SQL stands as a testament to the power of classic data structures applied to modern storage systems, and demonstrates that with careful design and thorough testing, complex systems can be built reliably and efficiently.

---

**Project Statistics:**
- Total Lines of Code: ~1,967
- Programming Language: C++23
- License: MIT
- Test Coverage: Multiple test suites with millions of operations validated
- Development Period: September 2025 - December 2025
- Key Contributors: Lumine2024, GitHub Copilot

**Repository:** https://github.com/Lumine2024/BPLUS_SQL
