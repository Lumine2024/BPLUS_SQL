#ifndef __PAGER_H__
#define __PAGER_H__

#include "bplus_node.h"
#include <string>
#include <fstream>
#include <cstring>

namespace bplus_sql {

class Pager {
public:
    explicit Pager(const std::string &fileName) {
        m_file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
        if (!m_file.is_open()) {
            // File doesn't exist, create it
            m_file.clear();
            m_file.open(fileName, std::ios::out | std::ios::binary);
            m_file.close();
            m_file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);
        }
    }

    static constexpr size_t PAGE_SIZE = 1ull << 12;

    Pager &readPage(size_t pageId, BPlusNode &node) {
        m_file.seekg(pageId * PAGE_SIZE, std::ios::beg);
        if (m_file.tellg() >= 0) {
            m_file.read(reinterpret_cast<char *>(&node), sizeof(node));
            if (m_file.gcount() != sizeof(node)) {
                // File is smaller than expected, zero-initialize
                std::memset(&node, 0, sizeof(node));
            }
        } else {
            // Seek failed, zero-initialize
            std::memset(&node, 0, sizeof(node));
        }
        return *this;
    }

    Pager &writePage(size_t pageId, const BPlusNode &node) {
        m_file.seekp(pageId * PAGE_SIZE, std::ios::beg);
        m_file.write(reinterpret_cast<const char *>(&node), sizeof(node));
        m_file.flush();
        return *this;
    }
private:
    std::fstream m_file;
};

}

#endif