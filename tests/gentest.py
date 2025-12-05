import random

DB_NAME = "test_db"

def gen_insert(x: int) -> str:
	return f"INSERT INTO {DB_NAME} KEY {x}"
def gen_erase(x: int) -> str:
	return f"ERASE FROM {DB_NAME} KEY {x}"
def gen_query(x: int) -> str:
	return f"QUERY FROM {DB_NAME} KEY {x}"

print(f"CREATE TABLE {DB_NAME}")

for i in range(1000000):
	op = random.randint(0, 2)
	if op == 0:
		print(gen_insert(random.randint(1, 100000)))
	elif op == 1:
		print(gen_erase(random.randint(1, 100000)))
	else:
		print(gen_query(random.randint(1, 100000)))


print(f"DESTROY TABLE {DB_NAME}")