#pragma once

#include <filesystem>
#include <string>

namespace deployment {

// 初始化控制台输出环境。
// Windows 下设置 UTF-8 代码页，Linux 下保持空实现即可。
void initializeConsole();

// 获取当前可执行文件所在目录。
std::filesystem::path getExecutableDir();

// 从 .env 文件加载环境变量。
void loadEnvFile(const std::filesystem::path& filePath);

// 按照部署约定加载默认环境文件。
// 顺序为：显式指定文件 -> /etc/offerpilot/backend.env -> 可执行文件旁边的 .env -> 当前目录 .env。
void loadDefaultEnvFiles();

}  // namespace deployment
