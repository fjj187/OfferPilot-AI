@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

if not exist "offerpilot_backend.exe" (
  echo [ERROR] offerpilot_backend.exe not found. Run build-local.bat first.
  exit /b 1
)

set "PATH=%SCRIPT_DIR%lib;%PATH%"
set "HTTP_PORT=3030"
set "HTTP_BIND_HOST=127.0.0.1"
set "DB_HOST=127.0.0.1"
set "DB_PORT=3306"
set "DB_NAME=offerpilot"
set "DB_USER=root"
if "%DB_PASSWORD%"=="" set "DB_PASSWORD=123456"
set "USE_MOCK_INTERVIEW_PROVIDER=1"
set "ALLOW_PUBLIC_HTTP_BIND=0"
set "AUTH_TOKEN_TTL_SECONDS=86400"

echo [INFO] running offerpilot_backend.exe
offerpilot_backend.exe

endlocal
