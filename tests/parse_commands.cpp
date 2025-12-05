#include "cmdparser.h"
#include <cassert>

void check(const std::string &str, const bplus_sql::CmdParser::Command &expected_cmd) {
	auto parsed = bplus_sql::CmdParser::parse(str);
	assert(parsed.op == expected_cmd.op);
	assert(parsed.cmd.index() == expected_cmd.cmd.index());
	try {
		switch(parsed.op) {
			case bplus_sql::CmdParser::CREATE: {
				auto parsed_cmd = std::get<bplus_sql::CmdParser::CreateCommand>(parsed.cmd);
				auto expected = std::get<bplus_sql::CmdParser::CreateCommand>(expected_cmd.cmd);
				assert(parsed_cmd.tableName == expected.tableName);
				break;
			}
			case bplus_sql::CmdParser::INSERT: {
				auto parsed_cmd = std::get<bplus_sql::CmdParser::InsertCommand>(parsed.cmd);
				auto expected = std::get<bplus_sql::CmdParser::InsertCommand>(expected_cmd.cmd);
				assert(parsed_cmd.tableName == expected.tableName);
				assert(parsed_cmd.key == expected.key);
				break;
			}
			case bplus_sql::CmdParser::ERASE: {
				auto parsed_cmd = std::get<bplus_sql::CmdParser::EraseCommand>(parsed.cmd);
				auto expected = std::get<bplus_sql::CmdParser::EraseCommand>(expected_cmd.cmd);
				assert(parsed_cmd.tableName == expected.tableName);
				assert(parsed_cmd.key == expected.key);
				break;
			}
			case bplus_sql::CmdParser::QUERY: {
				auto parsed_cmd = std::get<bplus_sql::CmdParser::QueryCommand>(parsed.cmd);
				auto expected = std::get<bplus_sql::CmdParser::QueryCommand>(expected_cmd.cmd);
				assert(parsed_cmd.tableName == expected.tableName);
				assert(parsed_cmd.key == expected.key);
				break;
			}
			case bplus_sql::CmdParser::DESTROY: {
				auto parsed_cmd = std::get<bplus_sql::CmdParser::DestroyCommand>(parsed.cmd);
				auto expected = std::get<bplus_sql::CmdParser::DestroyCommand>(expected_cmd.cmd);
				assert(parsed_cmd.tableName == expected.tableName);
				break;
			}
		}
	} catch(...) {
		assert(false);
	}
}

int main() {
	check("CREATE TABLE BPlusSql", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::CREATE,
		bplus_sql::CmdParser::CreateCommand{"BPlusSql"}
		});
	check("INSERT INTO BPlusSql KEY 114514", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::INSERT,
		bplus_sql::CmdParser::InsertCommand{"BPlusSql", 114514}
		});
	check("ERASE FROM BPlusSql KEY 1919810", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::ERASE,
		bplus_sql::CmdParser::EraseCommand{"BPlusSql", 1919810}
		});
	check("QUERY FROM BPlusSql KEY 31415926", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::QUERY,
		bplus_sql::CmdParser::QueryCommand("BPlusSql", 31415926)
		});
	check("DESTROY TABLE BPlusSql", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::DESTROY,
		bplus_sql::CmdParser::DestroyCommand("BPlusSql")
		});
	check("create table users", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::CREATE,
		bplus_sql::CmdParser::CreateCommand{"users"}
		});
	check("CrEaTe TaBlE MixedTable", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::CREATE,
		bplus_sql::CmdParser::CreateCommand{"MixedTable"}
		});
	check("CREATE TABLE t123", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::CREATE,
		bplus_sql::CmdParser::CreateCommand{"t123"}
		});

	check("insert into users key 42", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::INSERT,
		bplus_sql::CmdParser::InsertCommand{"users", 42}
		});
	check("Insert INTO People KEY 100", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::INSERT,
		bplus_sql::CmdParser::InsertCommand{"People", 100}
		});

	check("ERASE from MixedTable key 7", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::ERASE,
		bplus_sql::CmdParser::EraseCommand{"MixedTable", 7}
		});
	check("query FROM users KEY 8", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::QUERY,
		bplus_sql::CmdParser::QueryCommand("users", 8)
		});

	check("DESTROY table users", bplus_sql::CmdParser::Command{
		bplus_sql::CmdParser::DESTROY,
		bplus_sql::CmdParser::DestroyCommand("users")
		});

	return 0;
}