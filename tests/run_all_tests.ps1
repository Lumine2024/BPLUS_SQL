# Move to project root (script is in tests/)
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location (Join-Path $scriptDir "..")

# erase executables built before
if (Test-Path "build") {
    Remove-Item "build" -Recurse -Force
}
# build project
cmake -S . -B build
cmake --build build --config Debug
if($LASTEXITCODE -ne 0) {
    Write-Output "Compile error"
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
    Write-Output "Running test $TestName"
    $exe = Find-Executable $TestName
    if ($null -eq $exe) {
        Write-Output "Test $TestName failed: executable not found"
        exit 1
    }
    & $exe
    if($LASTEXITCODE -ne 0) {
        Write-Output "Test $TestName failed"
        exit 1
    }
    Write-Output "Test $TestName passed"
}

# Remove test data if present
if (Test-Path data/test.bin) {
    Remove-Item data/test.bin
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

Write-Output "Running test main"
$gentest = Find-Executable "gentest"
if ($null -eq $gentest) {
    Write-Output "gentest executable not found"
    exit 1
}
& $gentest | Out-File -FilePath tests/test_cmd.sql -Encoding utf8
if($LASTEXITCODE -ne 0) {
    Write-Output "gentest failed"
    exit 1
}

$testbf = Find-Executable "test_bf"
if ($null -eq $testbf) {
    Write-Output "test_bf executable not found"
    exit 1
}
& $testbf tests/test_cmd.sql | Out-File -FilePath tests/test_result_bf.txt -Encoding utf8
if($LASTEXITCODE -ne 0) {
    Write-Output "test_bf failed"
    exit 1
}

$mainExe = Find-Executable "main"
if ($null -eq $mainExe) {
    Write-Output "main executable not found"
    exit 1
}
& $mainExe tests/test_cmd.sql | Out-File -FilePath tests/test_result_bplus.txt -Encoding utf8
if($LASTEXITCODE -ne 0) {
    Write-Output "Test main failed: Runtime error"
    exit 1
}
$f1 = (Get-Content tests/test_result_bf.txt -Raw) -replace '\s+', ''
$f2 = (Get-Content tests/test_result_bplus.txt -Raw) -replace '\s+', ''
if($f1 -ne $f2) {
    Write-Output "Test main failed: Wrong answer"
    exit 1
}
Write-Output "Test main passed"

# Remove test data if present
if (Test-Path data/test.bin) {
    Remove-Item data/test.bin
}
if (Test-Path data/_test_for_rb_tree.bin) {
    Remove-Item data/_test_for_rb_tree.bin
}

RunTest "rb_tree"

# Remove test data if present
if (Test-Path data/test.bin) {
    Remove-Item data/test.bin
}
if (Test-Path data/_test_for_rb_tree.bin) {
    Remove-Item data/_test_for_rb_tree.bin
}

Write-Output "`nAll tests passed!"