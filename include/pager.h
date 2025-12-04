#ifndef __PAGER_H__
#define __PAGER_H__

#include "bplus_node.h"
#include <string>
#include <fstream>
#include <cstring>
#include <vector>
#include <cstdint>
#include <iostream>

namespace bplus_sql {

class Pager {
public:
    explicit Pager(const std::string &) {}
    Pager &readPage(unsigned pageId, BPlusNode &node) {
        while(nodes.size() <= pageId) {
            BPlusNode newNode{};
            std::memset(&newNode, 0, sizeof(BPlusNode));
            nodes.push_back(newNode);
        }
        std::memcpy(&node, &nodes[pageId], sizeof(BPlusNode));
        return *this;
    }
    Pager &writePage(unsigned pageId, BPlusNode &node) {
        while(nodes.size() <= pageId) {
            BPlusNode newNode{};
            std::memset(&newNode, 0, sizeof(BPlusNode));
            nodes.push_back(newNode);
        }
        std::memcpy(&nodes[pageId], &node, sizeof(BPlusNode));
        return *this;
    }
private:
    std::vector<BPlusNode> nodes;
};

// class Pager {
// public:
//     explicit Pager(const std::string &fileName) {
//         m_file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
//         if (!m_file.is_open()) {
//             // File doesn't exist, create it
//             m_file.clear();
//             m_file.open(fileName, std::ios::out | std::ios::binary);
//             m_file.close();
//             m_file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
//         }
//     }

//     static constexpr size_t PAGE_SIZE = 1ull << 12;

//     void ensurePageExists(size_t pageId) {
//         // Ensure the file has at least (pageId+1)*PAGE_SIZE bytes. If not, append zero bytes.
//         std::uint64_t need = (std::uint64_t)(pageId + 1) * PAGE_SIZE;
//         m_file.clear();
//         m_file.seekg(0, std::ios::end);
//         std::streamoff curOff = m_file.tellg();
//         std::uint64_t curSize = 0;
//         if (curOff != static_cast<std::streamoff>(-1)) curSize = static_cast<std::uint64_t>(curOff);
//         if (curSize >= need) return;
//         // append zeros in PAGE_SIZE chunks
//         m_file.clear();
//         m_file.seekp(0, std::ios::end);
//         const size_t CHUNK = PAGE_SIZE;
//         std::vector<char> zeros(CHUNK, 0);
//         std::uint64_t remaining = need - curSize;
//         while (remaining > 0) {
//             size_t toWrite = remaining > CHUNK ? CHUNK : static_cast<size_t>(remaining);
//             m_file.write(zeros.data(), static_cast<std::streamsize>(toWrite));
//             remaining -= toWrite;
//         }
//         m_file.flush();
//         // clear flags to allow subsequent reads/writes
//         m_file.clear();
//     }

//     Pager &readPage(size_t pageId, BPlusNode &node) {
//         // Read a whole page. If the page doesn't exist or is short, zero-fill the page
//         // ensure the page exists (file expanded and zero-filled if needed)
//         ensurePageExists(pageId);
//         std::vector<char> pageBuf(PAGE_SIZE);
//         std::streampos off = static_cast<std::streampos>(pageId) * static_cast<std::streamoff>(PAGE_SIZE);
//         m_file.clear(); // clear any eof/fail bits before seeking
//         m_file.seekg(off, std::ios::beg);
//         if (!m_file.good()) {
//             std::memset(&node, 0, sizeof(node));
//             return *this;
//         }
//         m_file.read(pageBuf.data(), static_cast<std::streamsize>(PAGE_SIZE));
//         // Copy node-sized data from page into node (node is <= PAGE_SIZE as asserted)
//         std::memcpy(&node, pageBuf.data(), sizeof(node));
//         return *this;
//     }

//     Pager &writePage(size_t pageId, const BPlusNode &node) {
//         // Ensure file is large enough, zero-initializing intermediate pages as needed
//         ensurePageExists(pageId);
//         std::vector<char> pageBuf(PAGE_SIZE);
//         std::memset(pageBuf.data(), 0, PAGE_SIZE);
//         std::memcpy(pageBuf.data(), &node, sizeof(node));
//         std::streampos off = static_cast<std::streampos>(pageId) * static_cast<std::streamoff>(PAGE_SIZE);
//         m_file.clear();
//         m_file.seekp(off, std::ios::beg);
//         m_file.write(pageBuf.data(), static_cast<std::streamsize>(PAGE_SIZE));
//         m_file.flush();
//         return *this;
//     }
// private:
//     std::fstream m_file;
// };

}

#endif