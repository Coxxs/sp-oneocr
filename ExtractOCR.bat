@echo off
setlocal DisableDelayedExpansion

net session >nul 2>&1
if %errorLevel% NEQ 0 (
    echo Requesting administrative privileges...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

set "TARGET_DIR=%~dp0"
cd /d "%TARGET_DIR%"

echo Extracting Snipping Tool OCR Files...
echo.

set "PS_CMD=$pkg = Get-AppxPackage -Name 'Microsoft.ScreenSketch' -AllUsers | Sort-Object Version -Descending | Select-Object -First 1; if ($pkg) { $srcDir = Join-Path $pkg.InstallLocation 'SnippingTool'; $files = @('oneocr.dll', 'onnxruntime.dll', 'oneocr.onemodel'); foreach ($f in $files) { $srcFile = Join-Path $srcDir $f; if (Test-Path $srcFile) { Copy-Item -Path $srcFile -Destination $env:TARGET_DIR -Force; Write-Host ('[+] Copied: ' + $f) -ForegroundColor Green; } else { Write-Host ('[-] File missing: ' + $f) -ForegroundColor Yellow; } } } else { Write-Host '[-] Snipping Tool (Microsoft.ScreenSketch) app not found on this system.' -ForegroundColor Red; }"

powershell -NoProfile -ExecutionPolicy Bypass -Command "%PS_CMD%"

echo.
echo Done.
pause