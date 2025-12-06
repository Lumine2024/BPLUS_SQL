
#!/usr/bin/env bash
set -euo pipefail

# Move to project root (script is in tests/)
cd "$(dirname "$0")/.."

# Build project
cmake -S . -B build
cmake --build build --config Debug

# Find the executable produced by CMake. Handles MSVC (build/Debug/*.exe)
# and Make/Ninja (build/<name>.exe) layouts. Returns empty string if not found.
find_exe() {
	local name="$1"
	if [ -f "build/Debug/${name}.exe" ]; then
		printf '%s' "build/Debug/${name}.exe"
		return
	fi
	if [ -f "build/${name}.exe" ]; then
		printf '%s' "build/${name}.exe"
		return
	fi
	if [ -f "build/${name}" ]; then
		printf '%s' "build/${name}"
		return
	fi
	# Fallback: search build tree (case-insensitive for .exe)
	if command -v find >/dev/null 2>&1; then
		local p
		p=$(find build -type f -iname "${name}.exe" -print -quit 2>/dev/null || true)
		if [ -n "$p" ]; then
			printf '%s' "$p"
			return
		fi
	fi
	printf ''
}

run_test() {
	local name="$1"
	echo "Running test $name"
	local exe
	exe=$(find_exe "$name")
	if [ -z "$exe" ]; then
		echo "Test $name failed: executable not found" >&2
		exit 1
	fi
	# execute
	"$exe"
	local rc=$?
	if [ $rc -ne 0 ]; then
		echo "Test $name failed: exit code $rc" >&2
		exit $rc
	fi
	echo "Test $name passed"
}

run_test "pressure_test"

# remove test data if present
[ -f data/test.bin ] && rm -f data/test.bin

run_test "always_insert"
run_test "test_existing_tree"
[ -f data/test.bin ] && rm -f data/test.bin
run_test "parse_commands"

echo "Running test main"
# generate commands and bf expected result
python tests/gentest.py > tests/test_cmd.sql
if ! python tests/test_bf.py < tests/test_cmd.sql > tests/test_result_bf.txt; then
	echo "test_bf.py failed" >&2
	exit 1
fi

main_exe=$(find_exe main)
if [ -z "$main_exe" ]; then
	echo "main executable not found" >&2
	exit 1
fi

# Run main with the generated commands and capture output
if ! cat tests/test_cmd.sql | "$main_exe" > tests/test_result_bplus.txt; then
	echo "Test main failed: Runtime error" >&2
	exit 1
fi

# Compare ignoring whitespace
f1=$(tr -d '[:space:]' < tests/test_result_bf.txt || true)
f2=$(tr -d '[:space:]' < tests/test_result_bplus.txt || true)
if [ "$f1" != "$f2" ]; then
	echo "Test main failed: Wrong answer" >&2
	exit 1
fi
echo "Test main passed"

# return to tests dir
cd tests
