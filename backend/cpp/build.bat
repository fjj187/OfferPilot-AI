@echo off
setlocal

g++ -std=c++17 -DCPPHTTPLIB_OPENSSL_SUPPORT ^
  -Iinclude -Iinclude/types -Iinclude/providers -Iinclude/repositories -Iinclude/services -Iinclude/Client -Iinclude/Controller -Iinclude/Routes -Iinclude/Hasher -Iinclude/builder -Ithird_part ^
  -I"C:/Program Files/MySQL/MySQL Server 8.0/include" ^
  -L"C:/Program Files/MySQL/MySQL Server 8.0/lib" ^
  src/main.cpp src/Config/app_config.cpp src/platform/DeploymentBootstrap.cpp src/builder/ReportPromptBuilder.cpp src/Client/OpenAIReportAiClient.cpp src/Controller/AuthController.cpp src/Controller/InterviewStreamController.cpp src/Hasher/PasswordHasher.cpp src/Controller/interview_controller.cpp src/Routes/InterviewRoutes.cpp src/providers/MockInterviewProviders.cpp src/providers/OpenAIInterviewProvider.cpp src/Pool/MySQLConnectionPool.cpp src/manager/BoundedStreamEventQueuer.cpp src/manager/InterviewStreamManager.cpp src/manager/StreamSession.cpp src/repositories/MySQLAuthSessionRepository.cpp src/repositories/MySQLAuthUserRepository.cpp src/repositories/MySQLReportRepository.cpp src/repositories/MySQLSessionRepository.cpp src/repositories/MySQLStreamCheckpointRepository.cpp src/Routes/AuthRoutes.cpp src/services/AuthService.cpp src/services/InterviewService.cpp src/services/ReportService.cpp src/MySQLConn.cpp include/http_server.cpp ^
  -lws2_32 -lssl -lcrypto -lcrypt32 -lmysql -o offerpilot_backend.exe

endlocal
