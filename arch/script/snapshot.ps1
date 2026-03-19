# 获取脚本所在目录 (/arch/script)
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# sslist.txt
$listFile = Join-Path $scriptDir "sslist.txt"
if (-not (Test-Path $listFile)) {
    Write-Error "sslist.txt not found: $listFile"
    exit 1
}

# ../../archive 目录
$archiveBase = Join-Path (Join-Path $scriptDir "..\..\..") "archive"

# 时间戳
$timestamp = (Get-Date).ToString("MMdd-HHmmss-")
$targetDir = Join-Path $archiveBase "$timestamp"

# 如果目录存在则退出（避免覆盖）
if (Test-Path $targetDir) {
    Write-Host "Target folder exists: $targetDir"
    Write-Host "Abort to avoid overwrite."
    exit 0
}

# 创建目标目录
New-Item -ItemType Directory -Path $targetDir | Out-Null
Write-Host "Created: $targetDir"
Write-Host ""

# 逐行读取 sslist.txt
Get-Content $listFile | ForEach-Object {
    $item = $_.Trim()
    if ($item -eq "") {
        Write-Host "Skip empty line."
        return
    }

    $source = Join-Path (Join-Path $scriptDir "..\..") $item
    Write-Host "Processing $item ..."

    if (Test-Path $source) {
        Write-Host "  Found: $source"
        Write-Host "  Copying..."
        Copy-Item $source -Destination $targetDir -Recurse -Force
        Write-Host "  OK."
    } else {
        Write-Warning "  Not found: $source"
    }

    Write-Host ""
}

Write-Host "Snapshot finished."