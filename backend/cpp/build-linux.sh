#!/usr/bin/env bash
set -euo pipefail

# Linux 生产构建脚本。
# 约定：使用 g++/clang++、C++17、OpenSSL 和 MySQL/MariaDB 客户端库。

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

OPENSSL_INCLUDE_DIR="${OPENSSL_INCLUDE_DIR:-/usr/include}"
OPENSSL_LIB_DIR="${OPENSSL_LIB_DIR:-/usr/lib/x86_64-linux-gnu}"
MYSQL_INCLUDE_DIR="${MYSQL_INCLUDE_DIR:-/usr/include/mysql}"
MYSQL_LIB_DIR="${MYSQL_LIB_DIR:-/usr/lib/x86_64-linux-gnu}"
MYSQL_LIBS="${MYSQL_LIBS:--lmysqlclient}"
CXX="${CXX:-g++}"
OUT_BIN="${OUT_BIN:-offerpilot_backend}"

echo "[INFO] ROOT_DIR=$ROOT_DIR"
echo "[INFO] OPENSSL_INCLUDE_DIR=$OPENSSL_INCLUDE_DIR"
echo "[INFO] OPENSSL_LIB_DIR=$OPENSSL_LIB_DIR"
echo "[INFO] MYSQL_INCLUDE_DIR=$MYSQL_INCLUDE_DIR"
echo "[INFO] MYSQL_LIB_DIR=$MYSQL_LIB_DIR"
echo "[INFO] MYSQL_LIBS=$MYSQL_LIBS"

"$CXX" -std=c++17 -O2 -DNDEBUG -DCPPHTTPLIB_OPENSSL_SUPPORT -pthread \
  -Iinclude \
  -Iinclude/types \
  -Iinclude/providers \
  -Iinclude/repositories \
  -Iinclude/services \
  -Iinclude/Client \
  -Iinclude/Controller \
  -Iinclude/Routes \
  -Iinclude/Hasher \
  -Iinclude/builder \
  -Iinclude/platform \
  -Ithird_part \
  -I"$OPENSSL_INCLUDE_DIR" \
  -I"$MYSQL_INCLUDE_DIR" \
  -L"$OPENSSL_LIB_DIR" \
  -L"$MYSQL_LIB_DIR" \
  src/main.cpp \
  src/Config/app_config.cpp \
  src/platform/DeploymentBootstrap.cpp \
  src/builder/ReportPromptBuilder.cpp \
  src/Client/OpenAIReportAiClient.cpp \
  src/Controller/AuthController.cpp \
  src/Controller/InterviewStreamController.cpp \
  src/Hasher/PasswordHasher.cpp \
  src/Controller/interview_controller.cpp \
  src/Routes/InterviewRoutes.cpp \
  src/providers/MockInterviewProviders.cpp \
  src/providers/OpenAIInterviewProvider.cpp \
  src/Pool/MySQLConnectionPool.cpp \
  src/manager/BoundedStreamEventQueuer.cpp \
  src/manager/InterviewStreamManager.cpp \
  src/manager/StreamSession.cpp \
  src/repositories/MySQLAuthSessionRepository.cpp \
  src/repositories/MySQLAuthUserRepository.cpp \
  src/repositories/MySQLReportRepository.cpp \
  src/repositories/MySQLSessionRepository.cpp \
  src/repositories/MySQLStreamCheckpointRepository.cpp \
  src/Routes/AuthRoutes.cpp \
  src/services/AuthService.cpp \
  src/services/InterviewService.cpp \
  src/services/ReportService.cpp \
  src/MySQLConn.cpp \
  include/http_server.cpp \
  -lssl -lcrypto -lcrypt -ldl -lpthread \
  $MYSQL_LIBS \
  -o "$OUT_BIN"

echo "[OK] build succeeded: $ROOT_DIR/$OUT_BIN"
