#ifndef __PAGER_H__
#define __PAGER_H__

#include "bplus_node.h"
#include <string>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <stdexcept>

namespace bplus_sql {

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
        // Ensure the file has at least (pageId+1)*PAGE_SIZE bytes
        size_t need = (pageId + 1) * PAGE_SIZE;
        
        // Get current file size
        m_file.seekg(0, std::ios::end);
        std::streampos endPos = m_file.tellg();
        size_t curSize = (endPos >= 0) ? static_cast<size_t>(endPos) : 0;
        
        if (curSize >= need) {
            m_file.clear(); // Clear any EOF flags
            return;
        }
        
        // Append zeros to expand the file
        m_file.clear();
        m_file.seekp(0, std::ios::end);
        
        size_t remaining = need - curSize;
        char zeroBuf[PAGE_SIZE];
        std::memset(zeroBuf, 0, PAGE_SIZE);
        
        while (remaining > 0) {
            size_t toWrite = (remaining > PAGE_SIZE) ? PAGE_SIZE : remaining;
            m_file.write(zeroBuf, toWrite);
            remaining -= toWrite;
        }
        
        m_file.flush();
        m_file.clear(); // Clear any flags
    }

    Pager &readPage(size_t pageId, BPlusNode &node) {
        // Ensure the page exists
        ensurePageExists(pageId);
        
        // Seek to the page position
        size_t offset = pageId * PAGE_SIZE;
        m_file.clear();
        m_file.seekg(offset, std::ios::beg);
        
        if (!m_file.good()) {
            std::memset(&node, 0, sizeof(node));
            return *this;
        }
        
        // Read the page into a buffer
        char pageBuf[PAGE_SIZE];
        m_file.read(pageBuf, PAGE_SIZE);
        
        // Copy node data from buffer
        std::memcpy(&node, pageBuf, sizeof(node));
        
        return *this;
    }

    Pager &writePage(size_t pageId, BPlusNode &node) {
        // Ensure the page exists
        ensurePageExists(pageId);
        
        // Prepare page buffer with zeros
        char pageBuf[PAGE_SIZE];
        std::memset(pageBuf, 0, PAGE_SIZE);
        
        // Copy node data to buffer
        std::memcpy(pageBuf, &node, sizeof(node));
        
        // Seek to the page position
        size_t offset = pageId * PAGE_SIZE;
        m_file.clear();
        m_file.seekp(offset, std::ios::beg);
        
        // Write the page
        m_file.write(pageBuf, PAGE_SIZE);
        m_file.flush();
        
        return *this;
    }
    
private:
    std::string m_fileName;
    std::fstream m_file;
};

}

#endif