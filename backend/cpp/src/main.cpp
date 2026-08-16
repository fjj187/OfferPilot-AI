#include <iostream>
#include <memory>
#include <string>

#include "Config/app_config.hpp"
#include "Controller/AuthController.hpp"
#include "Controller/InterviewStreamController.hpp"
#include "Controller/interview_controller.hpp"
#include "Client/OpenAIReportAiClient.hpp"
#include "Pool/MySQLConnectionPool.hpp"
#include "Routes/AuthRoutes.hpp"
#include "Routes/InterviewRoutes.hpp"
#include "Hasher/PasswordHasher.hpp"
#include "manager/InterviewStreamManager.hpp"
#include "providers/MockInterviewProviders.hpp"
#include "providers/OpenAIInterviewProvider.hpp"
#include "repositories/MySQLAuthSessionRepository.hpp"
#include "repositories/MySQLAuthUserRepository.hpp"
#include "repositories/MySQLReportRepository.hpp"
#include "repositories/MySQLSessionRepository.hpp"
#include "services/AuthService.hpp"
#include "services/InterviewService.hpp"
#include "services/ReportService.hpp"
#include "platform/DeploymentBootstrap.hpp"
#include "http_server.hpp"
#include "json.hpp"

namespace {

bool validateDatabaseConnection(MySQLConnectionPool& mysqlPool) {
    const MySQLConnHandle conn = mysqlPool.acquire();
    if (!conn) {
        std::cerr << "[startup] unable to acquire MySQL connection for readiness check" << std::endl;
        return false;
    }

    if (!conn->query("SELECT 1") || !conn->next()) {
        std::cerr << "[startup] MySQL readiness check failed" << std::endl;
        return false;
    }

    return true;
}

}  // namespace

int main() {
    deployment::initializeConsole();
    deployment::loadDefaultEnvFiles();

    AppConfig& config = AppConfig::loadFromEnv();
    std::string configError;
    if (!config.isConfigValid(&configError)) {
        std::cerr << "[startup] configuration invalid: " << configError << std::endl;
        return 1;
    }

    HttpServer httpServer(config.httpPort, config.httpBindHost);
    httpServer.setupMiddleware();
    httpServer.setupErrorHandler();

    httpServer.get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"success":true,"message":"ok"})", "application/json");
    });

    // Keep the MySQL pool sized conservatively and verify it before serving traffic.
    MySQLPoolConfig mysqlPoolCfg;
    mysqlPoolCfg.host = config.dbHost;
    mysqlPoolCfg.user = config.dbUser;
    mysqlPoolCfg.password = config.dbPassword;
    mysqlPoolCfg.dbName = config.dbName;
    mysqlPoolCfg.port = static_cast<unsigned short>(config.dbPort);
    mysqlPoolCfg.minSize = 2;
    mysqlPoolCfg.maxSize = 8;
    mysqlPoolCfg.acquireTimeoutMs = 3000;

    MySQLConnectionPool mysqlPool(mysqlPoolCfg);
    if (!validateDatabaseConnection(mysqlPool)) {
        return 1;
    }

    MySQLSessionRepository sessionRepo(mysqlPool);
    MySQLReportRepository reportRepo(mysqlPool);
    MySQLAuthUserRepository authUserRepo(mysqlPool);
    MySQLAuthSessionRepository authSessionRepo(mysqlPool);
    PasswordHasher passwordHasher;

    std::unique_ptr<InterviewProvider> interviewProvider;
    if (config.useMockInterviewProvider) {
        interviewProvider = std::make_unique<MockInterviewProvider>();
    } else {
        interviewProvider = std::make_unique<OpenAIInterviewProvider>(
            config.openAiApiKey,
            config.openAiBaseUrl,
            config.openAiInterviewModel
        );
    }

    OpenAIReportAiClient reportAiClient(
        config.openAiApiKey,
        config.openAiBaseUrl,
        config.openAiReportModel,
        config.openAiTimeoutMs
    );

    InterviewService interviewService(*interviewProvider, sessionRepo);
    InterviewStreamManager streamManager(*interviewProvider, sessionRepo, InterviewStreamManager::Config{});
    httpServer.get("/api/metrics", [&mysqlPool, &streamManager](const httplib::Request&, httplib::Response& res) {
        const auto db = mysqlPool.stats();
        const auto streams = streamManager.stats();
        nlohmann::json body = {
            {"streams", {{"accepted", streams.accepted}, {"completed", streams.completed},
                {"rejected", streams.rejected}, {"cancelled", streams.cancelled},
                {"active", streams.active}, {"peakActive", streams.peakActive},
                {"queued", streams.queued}, {"peakQueued", streams.peakQueued}}},
            {"mysql", {{"acquireRequests", db.acquireRequests}, {"idleHits", db.idleHits},
                {"newConnections", db.newConnections}, {"acquireTimeouts", db.acquireTimeouts},
                {"failedConnections", db.failedConnections}, {"hitRate", db.acquireRequests == 0 ? 0.0 : static_cast<double>(db.idleHits) / db.acquireRequests}}},
            {"pool", {{"idle", mysqlPool.idleSize()}, {"busy", mysqlPool.busySize()}, {"total", mysqlPool.totalSize()}}}
        };
        res.set_content(body.dump(), "application/json");
    });
    ReportService reportService(sessionRepo, reportRepo, reportAiClient);

    AuthService authService(
        authUserRepo,
        authSessionRepo,
        passwordHasher,
        config.authTokenTtlSeconds
    );

    InterviewController controller(interviewService, reportService);
    InterviewStreamController streamController(streamManager);
    InterviewRoutes routes(httpServer, controller, streamController);

    AuthController authController(authService);
    AuthRoutes authRoutes(httpServer, authController);
    authRoutes.registerRoutes();
    routes.registerRoutes();

    std::cout << "[startup] listening on " << config.httpBindHost << ':' << config.httpPort << std::endl;
    if (!httpServer.start()) {
        std::cerr << "[startup] failed to bind HTTP server on " << config.httpBindHost
                  << ':' << config.httpPort << std::endl;
        return 1;
    }

    return 0;
}
