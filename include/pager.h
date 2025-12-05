#ifndef __PAGER_H__
#define __PAGER_H__

#include "bplus_node.h"
#include <string>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace bplus_sql {

//class Pager {
//public:
//    explicit Pager(const std::string &) {}
//    Pager &readPage(unsigned pageId, BPlusNode &node) {
//        while(nodes.size() <= pageId) {
//            BPlusNode newNode{};
//            std::memset(&newNode, 0, sizeof(BPlusNode));
//            nodes.push_back(newNode);
//        }
//        std::memcpy(&node, &nodes[pageId], sizeof(BPlusNode));
//        return *this;
//    }
//    Pager &writePage(unsigned pageId, BPlusNode &node) {
//        while(nodes.size() <= pageId) {
//            BPlusNode newNode{};
//            std::memset(&newNode, 0, sizeof(BPlusNode));
//            nodes.push_back(newNode);
//        }
//        std::memcpy(&nodes[pageId], &node, sizeof(BPlusNode));
//        return *this;
//    }
//private:
//    std::vector<BPlusNode> nodes;
//};

class Pager {
public:
    explicit Pager(const std::string &fileName) : m_fileName(fileName) {
        // Ensure the directory exists
        std::filesystem::path filePath(fileName);
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }
        
        // Try to open existing file first
        m_file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
        if (!m_file.is_open()) {
            // File doesn't exist, create it
            m_file.open(fileName, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!m_file.is_open()) {
                throw std::runtime_error("Failed to create file: " + fileName);
            }
            m_file.close();
            // Now open for reading and writing
            m_file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
            if (!m_file.is_open()) {
                throw std::runtime_error("Failed to open file: " + fileName);
            }
        }
    }

    ~Pager() {
        if (m_file.is_open()) {
            m_file.flush();
            m_file.close();
        }
    }

    static constexpr size_t PAGE_SIZE = 1ull << 12;

    void ensurePageExists(size_t pageId) {
        // Ensure the file has at least metadata page + (pageId+1)*PAGE_SIZE bytes
        size_t need = PAGE_SIZE + (pageId + 1) * PAGE_SIZE; // First page is metadata
        
        // Get current file size
        m_file.seekg(0, std::ios::end);
        std::streampos endPos = m_file.tellg();
        size_t curSize = (endPos != std::streampos(-1)) ? static_cast<size_t>(endPos) : 0;
        
        if (curSize >= need) {
            m_file.clear(); // Clear any EOF flags
            return;
        }
        
        // Append zeros to expand the file
        m_file.clear();
        m_file.seekp(0, std::ios::end);
        
        size_t remaining = need - curSize;
        // Use heap allocation for the buffer to avoid stack overflow
        std::vector<char> zeroBuf(PAGE_SIZE, 0);
        
        while (remaining > 0) {
            size_t toWrite = (remaining > PAGE_SIZE) ? PAGE_SIZE : remaining;
            m_file.write(zeroBuf.data(), toWrite);
            remaining -= toWrite;
        }
        
        m_file.flush();
        m_file.clear(); // Clear any flags
    }

    Pager &readPage(size_t pageId, BPlusNode &node) {
        // Ensure the page exists
        ensurePageExists(pageId);
        
        // Seek to the page position (offset by metadata size)
        size_t offset = PAGE_SIZE + pageId * PAGE_SIZE; // First page is metadata
        m_file.clear();
        m_file.seekg(offset, std::ios::beg);
        
        if (!m_file.good()) {
            // Failed to seek, zero out the node and return
            std::memset(&node, 0, sizeof(node));
            return *this;
        }
        
        // Read the page into a buffer (use heap allocation to avoid stack overflow)
        std::vector<char> pageBuf(PAGE_SIZE);
        m_file.read(pageBuf.data(), PAGE_SIZE);
        
        // Copy node data from buffer
        std::memcpy(&node, pageBuf.data(), sizeof(node));
        
        return *this;
    }

    Pager &writePage(size_t pageId, BPlusNode &node) {
        // Ensure the page exists
        ensurePageExists(pageId);
        
        // Prepare page buffer with zeros (use heap allocation to avoid stack overflow)
        std::vector<char> pageBuf(PAGE_SIZE, 0);
        
        // Copy node data to buffer
        std::memcpy(pageBuf.data(), &node, sizeof(node));
        
        // Seek to the page position (offset by metadata size)
        size_t offset = PAGE_SIZE + pageId * PAGE_SIZE; // First page is metadata
        m_file.clear();
        m_file.seekp(offset, std::ios::beg);
        
        // Write the page
        m_file.write(pageBuf.data(), PAGE_SIZE);
        m_file.flush();
        
        return *this;
    }
    
    // Metadata operations - store at the beginning of file (before first page)
    template<typename T>
    void writeMetadata(const T& metadata) {
        static_assert(sizeof(T) <= PAGE_SIZE, "Metadata must fit in one page");
        
        // Seek to the beginning of file
        m_file.clear();
        m_file.seekp(0, std::ios::beg);
        
        // Write metadata
        m_file.write(reinterpret_cast<const char*>(&metadata), sizeof(T));
        m_file.flush();
    }
    
    template<typename T>
    void readMetadata(T& metadata) {
        static_assert(sizeof(T) <= PAGE_SIZE, "Metadata must fit in one page");
        
        // Seek to the beginning of file
        m_file.clear();
        m_file.seekg(0, std::ios::beg);
        
        // Read metadata
        m_file.read(reinterpret_cast<char*>(&metadata), sizeof(T));
    }
    
    bool fileExists() const {
        return std::filesystem::exists(m_fileName);
    }
    
    size_t getFileSize() {
        if (!fileExists()) return 0;
        
        m_file.clear();
        m_file.seekg(0, std::ios::end);
        std::streampos endPos = m_file.tellg();
        m_file.clear();
        
        return (endPos != std::streampos(-1)) ? static_cast<size_t>(endPos) : 0;
    }
    
private:
    std::string m_fileName;
    std::fstream m_file;
};

}

#endif