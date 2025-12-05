#include "cmdparser.h"
#include "bplus_tree.h"
#include <iostream>
#include <cctype>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <fstream>

std::string tolower(std::string str) {
	for(char &c : str) {
		c = std::tolower(c);
	}
	return str;
}

int main(int argc, char* argv[]) {
	std::ifstream inFile;
	if(argc > 1) {
		std::string fileName = argv[1];
		inFile.open(fileName, std::ios_base::in);
		if(!inFile.is_open()) {
			std::cout << "File didn't open" << std::endl;
			return -1;
		}
		std::cin.rdbuf(inFile.rdbuf());
	}
	std::string command;
	std::unordered_map<std::string, std::unique_ptr<bplus_sql::BPlusTree>> trees;
	auto dst = std::filesystem::path(__FILE__).parent_path().parent_path() / "data";
	while(std::getline(std::cin, command)) {
		if(command.empty()) continue;
		if(tolower(command) == "exit") return 0;
		auto parse_result = bplus_sql::CmdParser::parse(command);
		switch(parse_result.op) {
			case bplus_sql::CmdParser::INVALID: {
				std::cout << "Invalid operation. Please check and input again." << std::endl;
				break;
			}
			case bplus_sql::CmdParser::CREATE: {
				std::string name = std::get<bplus_sql::CmdParser::CreateCommand>(parse_result.cmd).tableName;
				if(!trees.contains(name)) {
					trees.emplace(name, std::make_unique<bplus_sql::BPlusTree>((dst / (name + ".bin")).string()));
				}
				break;
			}
			case bplus_sql::CmdParser::INSERT: {
				auto [name, key] = std::get<bplus_sql::CmdParser::InsertCommand>(parse_result.cmd);
				if(!trees.contains(name)) {
					trees.emplace(name, std::make_unique<bplus_sql::BPlusTree>((dst / (name + ".bin")).string()));
				}
				trees[name]->insert(key);
				break;
			}
			case bplus_sql::CmdParser::ERASE: {
				auto [name, key] = std::get<bplus_sql::CmdParser::EraseCommand>(parse_result.cmd);
				if(!trees.contains(name)) {
					trees.emplace(name, std::make_unique<bplus_sql::BPlusTree>((dst / (name + ".bin")).string()));
				}
				trees[name]->erase(key);
				break;
			}
			case bplus_sql::CmdParser::QUERY: {
				auto [name, key] = std::get<bplus_sql::CmdParser::QueryCommand>(parse_result.cmd);
				if(!trees.contains(name)) {
					trees.emplace(name, std::make_unique<bplus_sql::BPlusTree>((dst / (name + ".bin")).string()));
				}
				std::cout << trees[name]->search(key) << std::endl;
				break;
			}
			case bplus_sql::CmdParser::DESTROY: {
				std::string name = std::get<bplus_sql::CmdParser::DestroyCommand>(parse_result.cmd).tableName;
				if(!trees.contains(name)) {
					trees.emplace(name, std::make_unique<bplus_sql::BPlusTree>((dst / (name + ".bin")).string()));
				}
				trees.erase(name);
				std::error_code ec;
				std::filesystem::remove((dst / (name + ".bin")), ec);
				break;
			}
		}
	}
	return 0;
}