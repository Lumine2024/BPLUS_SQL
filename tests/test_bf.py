from typing import Tuple, Optional
import sys

INVALID = 'INVALID'
CREATE = 'CREATE'
INSERT = 'INSERT'
ERASE = 'ERASE'
QUERY = 'QUERY'
DESTROY = 'DESTROY'


def _tolower(s: str) -> str:
	return s.lower()


def parse(line: str) -> Tuple[str, str, Optional[int]]:
	"""
	Parse a command line and return a tuple: (op, name, key)
	- op: one of the operation constants above
	- name: table name as a string (empty string if not present)
	- key: integer key if present, otherwise None

	This mirrors the C++ CmdParser but uses a simple global function and
	replaces the std::variant with a plain string (name) and int (key).
	"""
	if not line:
		return INVALID, '', None

	tokens = line.split()
	if not tokens:
		return INVALID, '', None

	t0 = _tolower(tokens[0])
	op = INVALID
	name = ''
	key = None

	if t0 == 'create':
		op = CREATE
		# look for the token 'table' then take the next token as name
		for i, tok in enumerate(tokens[1:], start=1):
			if _tolower(tok) == 'table' and i + 1 < len(tokens):
				name = tokens[i + 1]
				break
	elif t0 == 'insert':
		op = INSERT
		# find 'into' and 'key'
		for i, tok in enumerate(tokens[1:], start=1):
			lt = _tolower(tok)
			if lt == 'into' and i + 1 < len(tokens):
				name = tokens[i + 1]
			elif lt == 'key' and i + 1 < len(tokens):
				try:
					key = int(tokens[i + 1])
				except ValueError:
					key = None
	elif t0 == 'erase':
		op = ERASE
		for i, tok in enumerate(tokens[1:], start=1):
			lt = _tolower(tok)
			if lt == 'from' and i + 1 < len(tokens):
				name = tokens[i + 1]
			elif lt == 'key' and i + 1 < len(tokens):
				try:
					key = int(tokens[i + 1])
				except ValueError:
					key = None
	elif t0 == 'query':
		op = QUERY
		for i, tok in enumerate(tokens[1:], start=1):
			lt = _tolower(tok)
			if lt == 'from' and i + 1 < len(tokens):
				name = tokens[i + 1]
			elif lt == 'key' and i + 1 < len(tokens):
				try:
					key = int(tokens[i + 1])
				except ValueError:
					key = None
	elif t0 == 'destroy':
		op = DESTROY
		for i, tok in enumerate(tokens[1:], start=1):
			if _tolower(tok) == 'table' and i + 1 < len(tokens):
				name = tokens[i + 1]
				break
	else:
		op = INVALID

	return op, name, key


def input() -> str:
	s = sys.stdin.readline()
	if not s:
		raise EOFError()
	return s.strip()

# Simple self-checks for the parser
if __name__ == '__main__':
	has_key = [0] * 100005
	try:
		while True:
			s = input()
			[o, n, k] = parse(s)
			if o == INVALID or o == CREATE or o == DESTROY:
				continue
			if o == INSERT:
				has_key[k] = 1
			elif o == ERASE:
				has_key[k] = 0
			else:
				print(has_key[k])
	except EOFError:
		pass