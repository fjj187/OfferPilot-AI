#pragma once

#include <string>

// Application configuration loaded at startup.
// Production prerequisites are validated here before the server starts.
class AppConfig {
public:
    AppConfig();
    ~AppConfig();

    // Load configuration from environment variables.
    static AppConfig& loadFromEnv();

    // Access the singleton instance.
    static AppConfig& getInstance();

    // Validate mandatory deployment fields.
    bool isConfigValid(std::string* errorMessage = nullptr) const;

    int httpPort;
    std::string httpBindHost;
    std::string dbHost;
    int dbPort;
    std::string dbName;
    std::string dbUser;
    std::string dbPassword;
    std::string logLevel;
    bool useMockInterviewProvider;
    bool allowPublicHttpBind;
    std::string openAiApiKey;
    std::string openAiBaseUrl;
    std::string openAiInterviewModel;
    std::string openAiReportModel;
    int openAiTimeoutMs;
    int authTokenTtlSeconds;
};
