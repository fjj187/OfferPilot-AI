@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

if "%OPENSSL_INCLUDE_DIR%"=="" (
  if exist "C:\msys64\ucrt64\include\openssl\err.h" set "OPENSSL_INCLUDE_DIR=C:\msys64\ucrt64\include"
)
if "%OPENSSL_INCLUDE_DIR%"=="" (
  if exist "C:\msys64\mingw64\include\openssl\err.h" set "OPENSSL_INCLUDE_DIR=C:\msys64\mingw64\include"
)
if "%OPENSSL_INCLUDE_DIR%"=="" (
  if exist "C:\Program Files\Git\usr\include\openssl\err.h" set "OPENSSL_INCLUDE_DIR=C:\Program Files\Git\usr\include"
)

if "%OPENSSL_LIB_DIR%"=="" (
  if exist "C:\msys64\ucrt64\lib\libssl.dll.a" set "OPENSSL_LIB_DIR=C:\msys64\ucrt64\lib"
)
if "%OPENSSL_LIB_DIR%"=="" (
  if exist "C:\msys64\mingw64\lib\libssl.dll.a" set "OPENSSL_LIB_DIR=C:\msys64\mingw64\lib"
)
if "%OPENSSL_LIB_DIR%"=="" (
  if exist "C:\Program Files\Git\usr\lib\libssl.dll.a" set "OPENSSL_LIB_DIR=C:\Program Files\Git\usr\lib"
)

if "%MYSQL_INCLUDE_DIR%"=="" (
  if exist "C:\Program Files\MySQL\MySQL Server 8.0\include\mysql.h" set "MYSQL_INCLUDE_DIR=C:\Program Files\MySQL\MySQL Server 8.0\include"
)
if not exist "%MYSQL_INCLUDE_DIR%\mysql.h" set "MYSQL_INCLUDE_DIR="
if not exist "%MYSQL_LIB_DIR%\libmysql.lib" set "MYSQL_LIB_DIR="
if "%MYSQL_INCLUDE_DIR%"=="" (
  if exist "C:\Program Files\MariaDB 11.4\include\mysql.h" set "MYSQL_INCLUDE_DIR=C:\Program Files\MariaDB 11.4\include"
)
if "%MYSQL_INCLUDE_DIR%"=="" (
  if exist "C:\Program Files\MariaDB 11.3\include\mysql.h" set "MYSQL_INCLUDE_DIR=C:\Program Files\MariaDB 11.3\include"
)
if "%MYSQL_INCLUDE_DIR%"=="" (
  if exist "C:\Program Files\MySQL\MySQL Server 8.4\include\mysql.h" set "MYSQL_INCLUDE_DIR=C:\Program Files\MySQL\MySQL Server 8.4\include"
)
if "%MYSQL_INCLUDE_DIR%"=="" (
  if exist "C:\Program Files\MySQL\MySQL Server 9.0\include\mysql.h" set "MYSQL_INCLUDE_DIR=C:\Program Files\MySQL\MySQL Server 9.0\include"
)

if "%MYSQL_LIB_DIR%"=="" (
  if exist "C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.lib" set "MYSQL_LIB_DIR=C:\Program Files\MySQL\MySQL Server 8.0\lib"
)
if "%MYSQL_LIB_DIR%"=="" (
  if exist "C:\Program Files\MariaDB 11.4\lib\libmariadb.lib" set "MYSQL_LIB_DIR=C:\Program Files\MariaDB 11.4\lib"
)
if "%MYSQL_LIB_DIR%"=="" (
  if exist "C:\Program Files\MariaDB 11.3\lib\libmariadb.lib" set "MYSQL_LIB_DIR=C:\Program Files\MariaDB 11.3\lib"
)
if "%MYSQL_LIB_DIR%"=="" (
  if exist "C:\Program Files\MySQL\MySQL Server 8.4\lib\libmysql.lib" set "MYSQL_LIB_DIR=C:\Program Files\MySQL\MySQL Server 8.4\lib"
)
if "%MYSQL_LIB_DIR%"=="" (
  if exist "C:\Program Files\MySQL\MySQL Server 9.0\lib\libmysql.lib" set "MYSQL_LIB_DIR=C:\Program Files\MySQL\MySQL Server 9.0\lib"
)

if "%OPENSSL_INCLUDE_DIR%"=="" (
  echo [ERROR] OPENSSL_INCLUDE_DIR not found. Run check-env.ps1 first.
  exit /b 1
)
if "%OPENSSL_LIB_DIR%"=="" (
  echo [ERROR] OPENSSL_LIB_DIR not found. Run check-env.ps1 first.
  exit /b 1
)
if "%MYSQL_INCLUDE_DIR%"=="" (
  echo [ERROR] MYSQL_INCLUDE_DIR not found. Run check-env.ps1 first.
  exit /b 1
)
if "%MYSQL_LIB_DIR%"=="" (
  echo [ERROR] MYSQL_LIB_DIR not found. Run check-env.ps1 first.
  exit /b 1
)

echo [INFO] OPENSSL_INCLUDE_DIR=%OPENSSL_INCLUDE_DIR%
echo [INFO] OPENSSL_LIB_DIR=%OPENSSL_LIB_DIR%
echo [INFO] MYSQL_INCLUDE_DIR=%MYSQL_INCLUDE_DIR%
echo [INFO] MYSQL_LIB_DIR=%MYSQL_LIB_DIR%

g++ -std=c++17 -DCPPHTTPLIB_OPENSSL_SUPPORT ^
  -Iinclude -Iinclude/types -Iinclude/providers -Iinclude/repositories -Iinclude/services -Iinclude/Client -Iinclude/Controller -Iinclude/Routes -Iinclude/Hasher -Iinclude/builder -Ithird_part ^
  -I"%OPENSSL_INCLUDE_DIR%" ^
  -I"%MYSQL_INCLUDE_DIR%" ^
  -L"%OPENSSL_LIB_DIR%" ^
  -L"%MYSQL_LIB_DIR%" ^
  src/main.cpp src/Config/app_config.cpp src/platform/DeploymentBootstrap.cpp src/builder/ReportPromptBuilder.cpp src/Client/OpenAIReportAiClient.cpp src/Controller/AuthController.cpp src/Controller/InterviewStreamController.cpp src/Hasher/PasswordHasher.cpp src/Controller/interview_controller.cpp src/Routes/InterviewRoutes.cpp src/providers/MockInterviewProviders.cpp src/providers/OpenAIInterviewProvider.cpp src/Pool/MySQLConnectionPool.cpp src/manager/BoundedStreamEventQueuer.cpp src/manager/InterviewStreamManager.cpp src/manager/StreamSession.cpp src/repositories/MySQLAuthSessionRepository.cpp src/repositories/MySQLAuthUserRepository.cpp src/repositories/MySQLReportRepository.cpp src/repositories/MySQLSessionRepository.cpp src/repositories/MySQLStreamCheckpointRepository.cpp src/Routes/AuthRoutes.cpp src/services/AuthService.cpp src/services/InterviewService.cpp src/services/ReportService.cpp src/MySQLConn.cpp include/http_server.cpp ^
  -lws2_32 -lssl -lcrypto -lcrypt32 -lmysql -o offerpilot_backend.exe

if errorlevel 1 (
  echo [ERROR] build failed.
  exit /b 1
)

echo [OK] build succeeded: %SCRIPT_DIR%offerpilot_backend.exe
endlocal
