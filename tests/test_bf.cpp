#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "cmdparser.h"

using bplus_sql::CmdParser;

char buffer[1 << 24];

int main(int argc, char* argv[]) {
    // speed_up IO
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::ifstream inFile;
    if (argc > 1) {
        std::string fileName = argv[1];
        inFile.open(fileName, std::ios_base::in);
        if (!inFile.is_open()) {
            std::cout << "File didn't open" << std::endl;
            return -1;
        }
        inFile.rdbuf()->pubsetbuf(buffer, 1 << 24);
        std::cin.rdbuf(inFile.rdbuf());
    } else {
        std::cin.rdbuf()->pubsetbuf(buffer, 1 << 24);
    }

    std::vector<int> has_key(100005, 0);
    std::string line;

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        auto cmd = CmdParser::parse(line);
        switch (cmd.op) {
            case CmdParser::INVALID:
            case CmdParser::CREATE:
            case CmdParser::DESTROY:
                break;
            case CmdParser::INSERT: {
                auto ins = std::get<CmdParser::InsertCommand>(cmd.cmd);
                if (ins.key >= 0) {
                    if (ins.key >= (int)has_key.size()) has_key.resize(ins.key + 5);
                    has_key[ins.key] = 1;
                }
                break;
            }
            case CmdParser::ERASE: {
                auto er = std::get<CmdParser::EraseCommand>(cmd.cmd);
                if (er.key >= 0) {
                    if (er.key >= (int)has_key.size()) has_key.resize(er.key + 5);
                    has_key[er.key] = 0;
                }
                break;
            }
            case CmdParser::QUERY: {
                auto q = std::get<CmdParser::QueryCommand>(cmd.cmd);
                if (q.key >= 0) {
                    if (q.key >= (int)has_key.size()) has_key.resize(q.key + 5);
                    std::cout << has_key[q.key] << '\n';
                } else {
                    std::cout << 0 << '\n';
                }
                break;
            }
        }
    }

    return 0;
}
