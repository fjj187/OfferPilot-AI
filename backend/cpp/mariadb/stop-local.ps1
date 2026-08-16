$ErrorActionPreference = 'Stop'

$rootDir = Split-Path -Parent $PSScriptRoot
$mariaRoot = Get-ChildItem -LiteralPath $PSScriptRoot -Directory | Where-Object { $_.Name -like 'mariadb-*' } | Select-Object -First 1
if ($mariaRoot) {
  $sourceBaseDir = $mariaRoot.FullName
  $asciiRoot = Join-Path ([System.IO.Path]::GetTempPath()) 'offerpilot-cpp-mariadb'
  $baseDir = Join-Path $asciiRoot 'package'
  $binDir = Join-Path $baseDir 'bin'
  $adminExe = Join-Path $binDir 'mariadb-admin.exe'
  $runtimeDir = Join-Path $asciiRoot 'runtime'
  $pidFile = Join-Path $runtimeDir 'mariadb-server.pid'

  try {
    & $adminExe --protocol=tcp --host=127.0.0.1 --port=3306 --user=root shutdown | Out-Null
  }
  catch {
  }

  if (Test-Path $pidFile) {
    Remove-Item -LiteralPath $pidFile -Force
  }
}
else {
  Write-Host '[INFO] No local MariaDB package folder found; nothing to stop.'
}

Write-Host '[OK] MariaDB local server stopped.'
