#pragma once

#include <filesystem>
#include <memory>

class ZipArchive {
public:
    explicit ZipArchive(std::filesystem::path archiveFilePath);
    ~ZipArchive();

    ZipArchive(const ZipArchive&) = delete;
    ZipArchive& operator=(const ZipArchive&) = delete;

    void beginUpdate();
    void addOrUpdateFile(const std::filesystem::path& sourcePath,
                         const std::filesystem::path& relativePath);
    void finishUpdate();

private:
    struct ArchiveHandle;

    std::filesystem::path archiveFilePath_;
    std::unique_ptr<ArchiveHandle> archive_;
};
