#include "Config/app_config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <initializer_list>

namespace {

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }

    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string readEnv(const char* key, const std::string& fallback = "") {
    const char* value = std::getenv(key);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }
    return std::string(value);
}

std::string readEnvAny(std::initializer_list<const char*> keys, const std::string& fallback = "") {
    for (const char* key : keys) {
        const std::string value = readEnv(key);
        if (!value.empty()) {
            return value;
        }
    }
    return fallback;
}

bool parseBool(const std::string& raw, bool fallback) {
    if (raw.empty()) {
        return fallback;
    }

    const std::string value = toLower(trim(raw));
    if (value == "1" || value == "true" || value == "yes" || value == "on") {
        return true;
    }
    if (value == "0" || value == "false" || value == "no" || value == "off") {
        return false;
    }
    return fallback;
}

int parseInt(const std::string& raw, int fallback) {
    if (raw.empty()) {
        return fallback;
    }

    try {
        size_t consumed = 0;
        const int value = std::stoi(trim(raw), &consumed);
        if (consumed == 0) {
            return fallback;
        }
        return value;
    } catch (...) {
        return fallback;
    }
}

bool isPublicBindHost(const std::string& host) {
    const std::string normalized = toLower(trim(host));
    return normalized == "0.0.0.0" || normalized == "::" || normalized == "[::]" || normalized == "*";
}

bool isBlank(const std::string& value) {
    return trim(value).empty();
}

}  // namespace

AppConfig::AppConfig()
    : httpPort(3030),
      httpBindHost("127.0.0.1"),
      dbHost("127.0.0.1"),
      dbPort(3306),
      dbName(""),
      dbUser("root"),
      dbPassword(""),
      logLevel("info"),
      useMockInterviewProvider(false),
      allowPublicHttpBind(false),
      openAiApiKey(""),
      openAiBaseUrl("https://api.openai.com/v1"),
      openAiInterviewModel("gpt-4o-mini"),
      openAiReportModel("gpt-4o-mini"),
      openAiTimeoutMs(30000),
      authTokenTtlSeconds(86400) {
}

AppConfig::~AppConfig() = default;

AppConfig& AppConfig::loadFromEnv() {
    static AppConfig instance;

    instance.httpPort = parseInt(readEnv("HTTP_PORT", "3030"), 3030);
    instance.httpBindHost = trim(readEnv("HTTP_BIND_HOST", "127.0.0.1"));
    instance.dbHost = trim(readEnv("DB_HOST", "127.0.0.1"));
    instance.dbPort = parseInt(readEnv("DB_PORT", "3306"), 3306);
    instance.dbName = trim(readEnv("DB_NAME"));
    instance.dbUser = trim(readEnv("DB_USER", "root"));
    instance.dbPassword = readEnv("DB_PASSWORD");
    instance.logLevel = trim(readEnv("LOG_LEVEL", "info"));
    instance.useMockInterviewProvider = parseBool(readEnv("USE_MOCK_INTERVIEW_PROVIDER", "0"), false);
    instance.allowPublicHttpBind = parseBool(readEnv("ALLOW_PUBLIC_HTTP_BIND", "0"), false);
    instance.openAiApiKey = readEnvAny({
        "OPENAI_API_KEY",
        "ALIYUN_API_KEY",
        "DASHSCOPE_API_KEY",
        "INTERVIEW_REMOTE_API_KEY"
    });
    instance.openAiBaseUrl = trim(readEnvAny({
        "OPENAI_BASE_URL",
        "ALIYUN_BASE_URL",
        "DASHSCOPE_BASE_URL",
        "INTERVIEW_REMOTE_BASE_URL"
    }, "https://api.openai.com/v1"));
    instance.openAiInterviewModel = trim(readEnvAny({
        "OPENAI_INTERVIEW_MODEL",
        "ALIYUN_INTERVIEW_MODEL",
        "DASHSCOPE_INTERVIEW_MODEL",
        "INTERVIEW_REMOTE_INTERVIEW_MODEL",
        "INTERVIEW_REMOTE_MODEL"
    }, "gpt-4o-mini"));
    instance.openAiReportModel = trim(readEnvAny({
        "OPENAI_REPORT_MODEL",
        "ALIYUN_REPORT_MODEL",
        "DASHSCOPE_REPORT_MODEL",
        "INTERVIEW_REMOTE_REPORT_MODEL",
        "INTERVIEW_REMOTE_MODEL"
    }, "gpt-4o-mini"));
    instance.openAiTimeoutMs = parseInt(readEnv("OPENAI_TIMEOUT_MS", "30000"), 30000);
    instance.authTokenTtlSeconds = parseInt(readEnv("AUTH_TOKEN_TTL_SECONDS", "86400"), 86400);

    return instance;
}

AppConfig& AppConfig::getInstance() {
    static AppConfig instance;
    return instance;
}

bool AppConfig::isConfigValid(std::string* errorMessage) const {
    auto fail = [&](const std::string& message) {
        if (errorMessage != nullptr) {
            *errorMessage = message;
        }
        return false;
    };

    if (httpPort <= 0 || httpPort > 65535) {
        return fail("HTTP_PORT must be between 1 and 65535.");
    }

    if (isBlank(httpBindHost)) {
        return fail("HTTP_BIND_HOST must not be empty.");
    }

    if (isPublicBindHost(httpBindHost) && !allowPublicHttpBind) {
        return fail("HTTP_BIND_HOST cannot be 0.0.0.0 or :: unless ALLOW_PUBLIC_HTTP_BIND=1.");
    }

    if (dbHost.empty()) {
        return fail("DB_HOST must not be empty.");
    }

    if (dbPort <= 0 || dbPort > 65535) {
        return fail("DB_PORT must be between 1 and 65535.");
    }

    if (isBlank(dbName)) {
        return fail("DB_NAME must not be empty.");
    }

    if (isBlank(dbUser)) {
        return fail("DB_USER must not be empty.");
    }

    if (dbPassword.empty()) {
        return fail("DB_PASSWORD must not be empty.");
    }

    if (authTokenTtlSeconds <= 0) {
        return fail("AUTH_TOKEN_TTL_SECONDS must be greater than 0.");
    }

    if (openAiTimeoutMs <= 0) {
        return fail("OPENAI_TIMEOUT_MS must be greater than 0.");
    }

    if (!useMockInterviewProvider) {
        if (openAiApiKey.empty()) {
            return fail("A production interview provider API key is required when USE_MOCK_INTERVIEW_PROVIDER=0.");
        }

        if (isBlank(openAiBaseUrl)) {
            return fail("OPENAI_BASE_URL must not be empty.");
        }

        if (isBlank(openAiInterviewModel)) {
            return fail("OPENAI_INTERVIEW_MODEL must not be empty.");
        }

        if (isBlank(openAiReportModel)) {
            return fail("OPENAI_REPORT_MODEL must not be empty.");
        }
    }

    return true;
}
