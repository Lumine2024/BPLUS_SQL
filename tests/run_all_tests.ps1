# Move to project root (script is in tests/)
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location (Join-Path $scriptDir "..")

# erase old built project that are maybe compiled before and cannot reference changes
Remove-Item -LiteralPath 'D:\BPLUS_SQL\build' -Recurse -Force
# build project
cmake -S . -B build
cmake --build build --config Debug
if($LASTEXITCODE -ne 0) {
    Write-Host "Compile error" -ForegroundColor Red
    exit 1
}

# run tests
function Find-Executable([string]$Name) {
    $locations = @(
        "./build/Debug/$Name.exe",
        "./build/$Name.exe",
        "./build/$Name"
    )
    
    foreach ($path in $locations) {
        if (Test-Path $path) {
            return $path
        }
    }
    
    return $null
}

function RunTest([string]$TestName) {
    Write-Host "Running test $TestName"
    $exe = Find-Executable $TestName
    if ($null -eq $exe) {
        Write-Host "Test $TestName failed: executable not found" -ForegroundColor Red
        exit 1
    }
    & $exe
    if($LASTEXITCODE -ne 0) {
        Write-Host "Test $TestName failed" -ForegroundColor Red
        exit 1
    }
    Write-Host "Test $TestName passed" -ForegroundColor Green
}

RunTest "pressure_test"

# Remove test data if present
if (Test-Path data/test.bin) {
    Remove-Item data/test.bin
}

RunTest "always_insert"
RunTest "test_existing_tree"

# Remove test data if present
if (Test-Path data/test.bin) {
    Remove-Item data/test.bin
}

RunTest "parse_commands"

Write-Host "Running test main"
# Use C++ helpers gentest and test_bf instead of Python
$gentest = Find-Executable "gentest"
if ($null -eq $gentest) {
    Write-Host "gentest executable not found" -ForegroundColor Red
    exit 1
}
& $gentest | Out-File -FilePath tests/test_cmd.sql -Encoding utf8
if($LASTEXITCODE -ne 0) {
    Write-Host "gentest failed" -ForegroundColor Red
    exit 1
}

$testbf = Find-Executable "test_bf"
if ($null -eq $testbf) {
    Write-Host "test_bf executable not found" -ForegroundColor Red
    exit 1
}
Get-Content tests/test_cmd.sql | & $testbf | Out-File -FilePath tests/test_result_bf.txt -Encoding utf8
if($LASTEXITCODE -ne 0) {
    Write-Host "test_bf failed" -ForegroundColor Red
    exit 1
}

$mainExe = Find-Executable "main"
if ($null -eq $mainExe) {
    Write-Host "main executable not found" -ForegroundColor Red
    exit 1
}
Get-Content tests/test_cmd.sql | & $mainExe | Out-File -FilePath tests/test_result_bplus.txt -Encoding utf8
if($LASTEXITCODE -ne 0) {
    Write-Host "Test main failed: Runtime error" -ForegroundColor Red
    exit 1
}
$f1 = (Get-Content tests/test_result_bf.txt -Raw) -replace '\s+', ''
$f2 = (Get-Content tests/test_result_bplus.txt -Raw) -replace '\s+', ''
if($f1 -ne $f2) {
    Write-Host "Test main failed: Wrong answer" -ForegroundColor Red
    exit 1
}
Write-Host "Test main passed" -ForegroundColor Green

Write-Host "`nAll tests passed!" -ForegroundColor Green