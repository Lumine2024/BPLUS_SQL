# Heap Corruption Fix

## Problem
When running the B+ tree implementation on MSVC, the program terminated with a heap corruption error:
```
Debug Error!
HEAP CORRUPTION DETECTED: after Normal block (#537129) at 0x016D3048.
CRT detected that the application wrote to memory after end of heap buffer.
```

## Root Cause
The bug was in the `insert` function in `src/bplus_tree.cpp`. When an internal (non-leaf) node became full (reached MAX_KEYS = 128), the code attempted to insert additional keys without splitting the node. This was indicated by the comment:

```cpp
// For simplicity in this implementation, we'll just refuse to split internal nodes
// and allow them to exceed MAX_KEYS slightly
```

This caused writes beyond the bounds of the fixed-size arrays:
- `int keys[128]` - could be written to at index 128 or higher
- `size_t children[129]` - could be written to at index 129 or higher

## Solution
Implemented proper internal node splitting through a new `splitNonLeaf` function that:

1. Creates temporary vectors to hold all keys and children (including the new entry)
2. Splits the keys at the midpoint
3. Distributes keys and children properly between the old and new nodes
4. Returns the new node so the parent can retrieve the split key

The implementation keeps the middle key in the right sibling (similar to leaf splits), which is valid for B+ trees since internal node keys serve as routing keys.

## Testing
The fix has been verified with:

1. **Stress test with suggested parameters**: 10 million operations with random keys from 1-100000
2. **Original test suite**: All existing tests pass
3. **Heap corruption test**: New test (`test_heap_corruption.cpp`) that specifically targets this bug

To run the heap corruption test:
```bash
cd build
cmake ..
make test_heap_corruption
./test_heap_corruption
```

Expected output:
```
✓ Test passed! All 10000000 operations completed successfully.
✓ Final tree size: ~50000 keys
✓ No heap corruption detected.
```

## Files Changed
- `src/bplus_tree.cpp`: Added `splitNonLeaf` function and fixed internal node handling
- `test_heap_corruption.cpp`: New test to verify the fix
- `CMakeLists.txt`: Added test_heap_corruption target
