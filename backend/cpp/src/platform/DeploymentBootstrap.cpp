#include "platform/DeploymentBootstrap.hpp"

#include <cstdlib>
#include <fstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace {

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }

    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string stripUtf8Bom(const std::string& text) {
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        return text.substr(3);
    }
    return text;
}

bool setEnvIfMissing(const std::string& key, const std::string& value) {
    if (key.empty() || value.empty()) {
        return false;
    }

    const char* existingValue = std::getenv(key.c_str());
    if (existingValue != nullptr && *existingValue != '\0') {
        return true;
    }

#ifdef _WIN32
    return _putenv_s(key.c_str(), value.c_str()) == 0;
#else
    return ::setenv(key.c_str(), value.c_str(), 1) == 0;
#endif
}

}  // namespace

namespace deployment {

void initializeConsole() {
#ifdef _WIN32
    // Windows 控制台默认不是 UTF-8，这里统一切换编码，避免中文日志乱码。
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
}

std::filesystem::path getExecutableDir() {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    const auto len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return std::filesystem::current_path();
    }

    return std::filesystem::path(buffer).parent_path();
#else
    std::vector<char> buffer(PATH_MAX + 1, '\0');
    const ssize_t len = ::readlink("/proc/self/exe", buffer.data(), PATH_MAX);
    if (len <= 0) {
        return std::filesystem::current_path();
    }

    buffer[static_cast<size_t>(len)] = '\0';
    return std::filesystem::path(buffer.data()).parent_path();
#endif
}

void loadEnvFile(const std::filesystem::path& filePath) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        return;
    }

    std::string line;
    bool firstLine = true;
    while (std::getline(in, line)) {
        if (firstLine) {
            line = stripUtf8Bom(line);
            firstLine = false;
        }

        const std::string normalizedLine = trim(line);
        if (normalizedLine.empty() || normalizedLine.front() == '#') {
            continue;
        }

        const auto eqPos = normalizedLine.find('=');
        if (eqPos == std::string::npos) {
            continue;
        }

        const std::string key = trim(normalizedLine.substr(0, eqPos));
        std::string value = trim(normalizedLine.substr(eqPos + 1));
        if (key.empty() || value.empty()) {
            continue;
        }

        if ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\'')) {
            value = value.substr(1, value.size() - 2);
        }

        setEnvIfMissing(key, value);
    }
}

void loadDefaultEnvFiles() {
    const char* explicitEnvFile = std::getenv("OFFERPILOT_ENV_FILE");
    if (explicitEnvFile != nullptr && *explicitEnvFile != '\0') {
        loadEnvFile(std::filesystem::path(explicitEnvFile));
    }

#ifdef __linux__
    loadEnvFile("/etc/offerpilot/backend.env");
#endif

    const auto exeDir = getExecutableDir();
    loadEnvFile((exeDir / ".." / ".env").lexically_normal());
    loadEnvFile((std::filesystem::current_path() / ".env").lexically_normal());
}

}  // namespace deployment
