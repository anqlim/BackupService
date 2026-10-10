#include "worker.h"

#include "config.h"
#include "file_scanner.h"
#include "zip_archive.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

std::filesystem::path getConfigPath() {
    std::wstring executablePath(MAX_PATH, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, executablePath.data(), MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        throw std::runtime_error("Cannot determine executable path");
    }

    executablePath.resize(length);
    return std::filesystem::path(executablePath).parent_path() / L"config.json";
}

bool isInsideDirectory(const std::filesystem::path& path, const std::filesystem::path& directory) {
    auto pathPart = path.begin();
    for (const auto& directoryPart : directory) {
        if (pathPart == path.end() || *pathPart != directoryPart) {
            return false;
        }
        ++pathPart;
    }
    return true;
}

void validateConfig(const Config& config) {
    if (!std::filesystem::exists(config.sourceDirectory)) {
        throw std::runtime_error("Source directory does not exist");
    }
    if (!std::filesystem::is_directory(config.sourceDirectory)) {
        throw std::runtime_error("sourceDirectory is not a directory");
    }

    const auto archiveDirectory = config.archivePath.parent_path();
    if (archiveDirectory.empty()) {
        throw std::runtime_error("archivePath must include a directory");
    }

    std::filesystem::create_directories(archiveDirectory);
    if (!std::filesystem::is_directory(archiveDirectory)) {
        throw std::runtime_error("Cannot create archive directory");
    }

    const auto sourceDirectory = std::filesystem::weakly_canonical(config.sourceDirectory);
    const auto archivePath = std::filesystem::weakly_canonical(config.archivePath);
    if (isInsideDirectory(archivePath, sourceDirectory)) {
        throw std::runtime_error("archivePath must not be inside sourceDirectory");
    }
}

struct FileState {
    std::filesystem::file_time_type lastWriteTime;
    std::uintmax_t size;

    bool operator==(const FileState&) const = default;
};

using FileSnapshot = std::map<std::filesystem::path, FileState>;

FileState getFileState(const std::filesystem::path& sourcePath) {
    return {
        .lastWriteTime = std::filesystem::last_write_time(sourcePath),
        .size = std::filesystem::file_size(sourcePath)
    };
}

FileSnapshot createInitialArchive(const Config& config, const FileScanner& scanner) {
    ZipArchive archive(config.archivePath);
    FileSnapshot snapshot;

    archive.beginUpdate();
    scanner.scan(config.sourceDirectory, config.masks,
        [&archive, &snapshot](const std::filesystem::path& sourcePath,
                              const std::filesystem::path& relativePath) {
            archive.addOrUpdateFile(sourcePath, relativePath);
            snapshot.emplace(relativePath, getFileState(sourcePath));
        });
    archive.finishUpdate();

    return snapshot;
}

FileSnapshot updateChangedFiles(const Config& config,
                                const FileScanner& scanner,
                                const FileSnapshot& previousSnapshot) {
    FileSnapshot currentSnapshot;
    std::optional<ZipArchive> archive;

    scanner.scan(config.sourceDirectory, config.masks,
        [&config, &previousSnapshot, &currentSnapshot, &archive](const std::filesystem::path& sourcePath,
                                                                  const std::filesystem::path& relativePath) {
            const FileState currentState = getFileState(sourcePath);
            currentSnapshot.emplace(relativePath, currentState);

            const auto previous = previousSnapshot.find(relativePath);
            if (previous != previousSnapshot.end() && previous->second == currentState) {
                return;
            }

            if (!archive.has_value()) {
                archive.emplace(config.archivePath);
                archive->beginUpdate();
            }
            archive->addOrUpdateFile(sourcePath, relativePath);
        });

    if (archive.has_value()) {
        archive->finishUpdate();
    }

    return currentSnapshot;
}

} // namespace

DWORD WINAPI BackupWorker(LPVOID stopEvent) {
    try {
        ConfigLoader configLoader;
        const Config config = configLoader.load(getConfigPath());
        validateConfig(config);

        FileScanner scanner;
        FileSnapshot snapshot = createInitialArchive(config, scanner);

        while (true) {
            const DWORD waitResult = WaitForSingleObject(stopEvent, 1000);
            if (waitResult == WAIT_OBJECT_0) {
                break;
            }
            if (waitResult != WAIT_TIMEOUT) {
                throw std::runtime_error("Cannot wait for service stop event");
            }

            snapshot = updateChangedFiles(config, scanner, snapshot);
        }
    } catch (const std::exception&) {
        return ERROR_INVALID_DATA;
    }

    return 0;
}
