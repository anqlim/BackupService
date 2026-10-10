#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct Config {
    std::filesystem::path sourceDirectory;
    std::filesystem::path archivePath;
    std::vector<std::wstring> masks;
};

class ConfigLoader {
public:
    Config load(const std::filesystem::path& configPath) const;
};
