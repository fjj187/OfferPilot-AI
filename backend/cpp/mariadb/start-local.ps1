$ErrorActionPreference = 'Stop'

$rootDir = Split-Path -Parent $PSScriptRoot
$mariaRoot = Get-ChildItem -LiteralPath $PSScriptRoot -Directory | Where-Object { $_.Name -like 'mariadb-*' } | Select-Object -First 1
$schemaFile = Join-Path $PSScriptRoot 'schema-offerpilot.sql'
$dbPassword = $env:DB_PASSWORD
$clientExe = $null
$adminExe = $null
$runtimeDir = $null
$baseDir = $null
$binDir = $null
$myIniPath = $null

if ($mariaRoot) {
  $sourceBaseDir = $mariaRoot.FullName
  $asciiRoot = Join-Path ([System.IO.Path]::GetTempPath()) 'offerpilot-cpp-mariadb'
  $baseDir = Join-Path $asciiRoot 'package'
  $binDir = Join-Path $baseDir 'bin'
  $serverExe = Join-Path $binDir 'mariadbd.exe'
  $clientExe = Join-Path $binDir 'mysql.exe'
  $adminExe = Join-Path $binDir 'mariadb-admin.exe'
  $runtimeDir = Join-Path $asciiRoot 'runtime'
  $myIniPath = Join-Path $runtimeDir 'my.ini'
  $pidFile = Join-Path $runtimeDir 'mariadb-server.pid'

  & (Join-Path $PSScriptRoot 'init-local.ps1')

  try {
    $pingArgs = @('--protocol=tcp', '--host=127.0.0.1', '--port=3306', '--user=root')
    if ($dbPassword) {
      $pingArgs += "--password=$dbPassword"
    }
    $pingArgs += 'ping'
    & $adminExe @pingArgs | Out-Null
    if ($LASTEXITCODE -ne 0) {
      throw 'MariaDB ping failed.'
    }
    Write-Host '[INFO] MariaDB already running on 127.0.0.1:3306'
  }
  catch {
    $process = Start-Process -FilePath $serverExe -ArgumentList @("--defaults-file=$myIniPath", '--standalone', '--console') -WorkingDirectory $baseDir -WindowStyle Hidden -PassThru
    Set-Content -LiteralPath $pidFile -Value $process.Id -Encoding ASCII

    $started = $false
    for ($i = 0; $i -lt 30; $i++) {
      Start-Sleep -Milliseconds 500
      try {
        $pingArgs = @('--protocol=tcp', '--host=127.0.0.1', '--port=3306', '--user=root')
        if ($dbPassword) {
          $pingArgs += "--password=$dbPassword"
        }
        $pingArgs += 'ping'
        & $adminExe @pingArgs | Out-Null
        if ($LASTEXITCODE -ne 0) {
          throw 'MariaDB ping failed.'
        }
        $started = $true
        break
      }
      catch {
      }
    }

    if (-not $started) {
      throw 'MariaDB did not start successfully.'
    }
  }
}
else {
  $mysqlCandidates = @(
    'C:\Program Files\MariaDB 11.4\bin\mysql.exe',
    'C:\Program Files\MariaDB 11.3\bin\mysql.exe',
    'C:\Program Files\MySQL\MySQL Server 9.0\bin\mysql.exe',
    'C:\Program Files\MySQL\MySQL Server 8.4\bin\mysql.exe',
    'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'
  )
  $mysqlServerCandidates = @(
    'C:\Program Files\MariaDB 11.4\bin\mariadb-admin.exe',
    'C:\Program Files\MariaDB 11.3\bin\mariadb-admin.exe',
    'C:\Program Files\MySQL\MySQL Server 9.0\bin\mysqladmin.exe',
    'C:\Program Files\MySQL\MySQL Server 8.4\bin\mysqladmin.exe',
    'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysqladmin.exe'
  )

  $clientExe = $mysqlCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
  $adminExe = $mysqlServerCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1

  if (-not $clientExe -or -not $adminExe) {
    throw 'MariaDB package folder not found, and no local MySQL/MariaDB client was found in Program Files.'
  }

  try {
    $pingArgs = @('--protocol=tcp', '--host=127.0.0.1', '--port=3306', '--user=root')
    if ($dbPassword) {
      $pingArgs += "--password=$dbPassword"
    }
    $pingArgs += 'ping'
    & $adminExe @pingArgs | Out-Null
    if ($LASTEXITCODE -ne 0) {
      throw 'MariaDB ping failed.'
    }
    Write-Host '[INFO] Existing database server is already running on 127.0.0.1:3306'
  }
  catch {
    throw 'No local MariaDB package folder was found, and 127.0.0.1:3306 is not reachable. Start a local database server first.'
  }
}

if (-not $dbPassword) {
  throw 'DB_PASSWORD is empty. Set DB_PASSWORD before running dev:cpp.'
}

$clientArgs = @('--protocol=tcp', '--host=127.0.0.1', '--port=3306', '--user=root', "--password=$dbPassword")
Get-Content -LiteralPath $schemaFile -Raw | & $clientExe @clientArgs
if ($LASTEXITCODE -ne 0) {
  throw 'MariaDB schema import failed.'
}

Write-Host '[OK] MariaDB local server is ready.'
Write-Host '  host=127.0.0.1'
Write-Host '  port=3306'
Write-Host '  database=offerpilot'
Write-Host '  demo users: admin/admin, user/user'
