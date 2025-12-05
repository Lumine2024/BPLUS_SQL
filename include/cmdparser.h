#pragma once

#include <string>
#include <variant>
#include <sstream>

namespace bplus_sql {

class CmdParser {
public:
	enum Operation {
		INVALID = -1,
		CREATE,
		INSERT,
		ERASE,
		QUERY,
		DESTROY
	};
	struct CreateCommand {
		std::string tableName;
	};
	struct InsertCommand {
		std::string tableName;
		int key;
	};
	struct EraseCommand {
		std::string tableName;
		int key;
	};
	struct QueryCommand {
		std::string tableName;
		int key;
	};
	struct DestroyCommand {
		std::string tableName;
	};
	struct Command {
		Operation op;
		std::variant<CreateCommand, InsertCommand, EraseCommand, QueryCommand, DestroyCommand> cmd;
	};
	CmdParser() = delete;
	// we assert that the line is valid
	static Command parse(std::string line) {
		std::stringstream ss(line);
		Command result;
		if(!(ss >> line)) return Command{INVALID, {}};
		if(tolower(line) == "create") {
			result.op = CREATE;
			while(ss >> line) {
				if(tolower(line) == "table") {
					ss >> line;
					result.cmd = CreateCommand{line};
				}
			}
		} else if(tolower(line) == "insert") {
			result.op = INSERT;
			InsertCommand cmd;
			while(ss >> line) {
				if(tolower(line) == "into") {
					ss >> line;
					cmd.tableName = line;
				} else if(tolower(line) == "key") {
					ss >> cmd.key;
				}
			}
			result.cmd = cmd;
		} else if(tolower(line) == "erase") {
			result.op = ERASE;
			EraseCommand cmd;
			while(ss >> line) {
				if(tolower(line) == "from") {
					ss >> line;
					cmd.tableName = line;
				} else if(tolower(line) == "key") {
					ss >> cmd.key;
				}
			}
			result.cmd = cmd;
		} else if(tolower(line) == "query") {
			result.op = QUERY;
			QueryCommand cmd;
			while(ss >> line) {
				if(tolower(line) == "from") {
					ss >> line;
					cmd.tableName = line;
				} else if(tolower(line) == "key") {
					ss >> cmd.key;
				}
			}
			result.cmd = cmd;
		} else if(tolower(line) == "destroy") {
			result.op = DESTROY;
			DestroyCommand cmd;
			while(ss >> line) {
				if(tolower(line) == "table") {
					ss >> line;
					cmd.tableName = line;
				}
			}
			result.cmd = cmd;
		} else {
			result.op = INVALID;
		}
		return result;
	}
private:
	static std::string tolower(std::string str) {
		for(char &c : str) {
			c = std::tolower(c);
		}
		return str;
	}
};

}