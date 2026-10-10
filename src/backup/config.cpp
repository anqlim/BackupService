#include "config.h"

#include <fstream>
#include <stdexcept>

#include <windows.h>

#include <nlohmann/json.hpp>

namespace {

std::wstring utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return {};
    }

    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (size == 0) {
        throw std::runtime_error("Cannot convert UTF-8 string to wide string");
    }

    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), size);
    result.pop_back();
    return result;
}

std::string readRequiredString(const nlohmann::json& json, const char* fieldName) {
    if (!json.contains(fieldName) || !json.at(fieldName).is_string()) {
        throw std::runtime_error(std::string("Missing or invalid field: ") + fieldName);
    }

    const auto value = json.at(fieldName).get<std::string>();
    if (value.empty()) {
        throw std::runtime_error(std::string("Empty field: ") + fieldName);
    }

    return value;
}

void validateUncPath(const std::string& value, const char* fieldName) {
    if (!value.starts_with("\\\\")) {
        throw std::runtime_error(std::string("Field must contain a UNC path: ") + fieldName);
    }
}

} // namespace

Config ConfigLoader::load(const std::filesystem::path& configPath) const {
    std::ifstream input(configPath);
    if (!input) {
        throw std::runtime_error("Cannot open config file: " + configPath.string());
    }

    nlohmann::json json;
    try {
        input >> json;
    } catch (const nlohmann::json::parse_error& error) {
        throw std::runtime_error(std::string("Invalid JSON config: ") + error.what());
    }

    Config config;
    const auto sourceDirectory = readRequiredString(json, "sourceDirectory");
    const auto archivePath = readRequiredString(json, "archivePath");
    validateUncPath(sourceDirectory, "sourceDirectory");
    validateUncPath(archivePath, "archivePath");

    config.sourceDirectory = utf8ToWide(sourceDirectory);
    config.archivePath = utf8ToWide(archivePath);

    if (!json.contains("masks") || !json.at("masks").is_array() || json.at("masks").empty()) {
        throw std::runtime_error("Missing, invalid, or empty field: masks");
    }

    for (const auto& mask : json.at("masks")) {
        if (!mask.is_string() || mask.get<std::string>().empty()) {
            throw std::runtime_error("Each mask must be a non-empty string");
        }
        config.masks.push_back(utf8ToWide(mask.get<std::string>()));
    }

    return config;
}
