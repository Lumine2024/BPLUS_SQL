Set-Location ..
# build project
cmake -S . -B build
cmake --build build --config Debug
if($LASTEXITCODE -ne 0) {
    Write-Host "Compile error" -ForegroundColor Red
    Set-Location tests
    exit 1
}

# run tests
function RunTest([string]$TestName) {
    Write-Host "Running test $TestName"
    "./build/Debug/$TestName.exe"
    if($LASTEXITCODE -ne 0) {
        Write-Host "Test $TestName failed" -ForegroundColor Red
        Set-Location tests
        exit 1
    }
    Write-Host "Test $TestName passed" -ForegroundColor Green
}

RunTest "pressure_test"
Remove-Item data/test.bin
RunTest "always_insert"
RunTest "test_existing_tree"
Remove-Item data/test.bin
RunTest "parse_commands"

Write-Host "Running test main"
python tests/gentest.py > tests/test_cmd.sql
Get-Content tests/test_cmd.sql | python tests/test_bf.py > tests/test_result_bf.txt
# we may ensure that test_bf.py exits normally
Get-Content tests/test_cmd.sql | ./build/Debug/main.exe > tests/test_result_bplus.txt
if($LASTEXITCODE -ne 0) {
    Write-Host "Test main failed: Runtime error" -ForegroundColor Red
    Set-Location tests
    exit 1
}
$f1 = (Get-Content tests/test_result_bf.txt -Raw) -replace '\s+', ''
$f2 = (Get-Content tests/test_result_bplus.txt -Raw) -replace '\s+', ''
if($f1 -ne $f2) {
    Write-Host "Test main failed: Wrong answer" -ForegroundColor Red
    Set-Location tests
    exit 1
}
Write-Host "Test main passed" -ForegroundColor Green

Set-Location tests