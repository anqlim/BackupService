#include "zip_archive.h"

#include <stdexcept>
#include <string>
#include <utility>

#include <windows.h>

#include <zip.h>

struct ZipArchive::ArchiveHandle {
    zip_t* value = nullptr;
};

namespace {

std::string toUtf8(const std::filesystem::path& path) {
    const std::wstring value = path.wstring();
    if (value.empty()) {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) {
        throw std::runtime_error("Cannot convert path to UTF-8");
    }

    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
    result.pop_back();
    return result;
}

std::string toGenericUtf8(const std::filesystem::path& path) {
    const auto value = path.generic_u8string();

    std::string result;
    result.reserve(value.size());
    for (const char8_t character : value) {
        result.push_back(static_cast<char>(character));
    }
    return result;
}

std::string zipErrorMessage(int errorCode) {
    zip_error_t error;
    zip_error_init_with_code(&error, errorCode);
    const std::string message = zip_error_strerror(&error);
    zip_error_fini(&error);
    return message;
}

} // namespace

ZipArchive::ZipArchive(std::filesystem::path archiveFilePath)
    : archiveFilePath_(std::move(archiveFilePath)) {
}

ZipArchive::~ZipArchive() {
    if (archive_ != nullptr && archive_->value != nullptr) {
        zip_discard(archive_->value);
    }
}

void ZipArchive::beginUpdate() {
    if (archive_ != nullptr) {
        throw std::runtime_error("Archive update is already in progress");
    }

    int errorCode = 0;
    zip_t* archive = zip_open(toUtf8(archiveFilePath_).c_str(), ZIP_CREATE, &errorCode);
    if (archive == nullptr) {
        throw std::runtime_error("Cannot open ZIP archive: " + zipErrorMessage(errorCode));
    }

    archive_ = std::make_unique<ArchiveHandle>();
    archive_->value = archive;
}

void ZipArchive::addOrUpdateFile(const std::filesystem::path& sourcePath,
                                 const std::filesystem::path& relativePath) {
    if (archive_ == nullptr) {
        throw std::runtime_error("Archive update has not started");
    }

    const std::string sourceName = toUtf8(sourcePath);
    const std::string archiveName = toGenericUtf8(relativePath);
    zip_source_t* source = zip_source_file(archive_->value, sourceName.c_str(), 0, -1);
    if (source == nullptr) {
        throw std::runtime_error("Cannot read file for ZIP archive");
    }

    const zip_int64_t index = zip_file_add(
        archive_->value,
        archiveName.c_str(),
        source,
        ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE
    );
    if (index < 0) {
        zip_source_free(source);
        throw std::runtime_error("Cannot add file to ZIP archive: " + std::string(zip_strerror(archive_->value)));
    }

    if (zip_set_file_compression(archive_->value, static_cast<zip_uint64_t>(index), ZIP_CM_DEFLATE, 0) < 0) {
        throw std::runtime_error("Cannot set ZIP compression: " + std::string(zip_strerror(archive_->value)));
    }
}

void ZipArchive::finishUpdate() {
    if (archive_ == nullptr) {
        throw std::runtime_error("Archive update has not started");
    }

    if (zip_close(archive_->value) < 0) {
        const std::string error = zip_strerror(archive_->value);
        zip_discard(archive_->value);
        archive_.reset();
        throw std::runtime_error("Cannot save ZIP archive: " + error);
    }

    archive_.reset();
}
