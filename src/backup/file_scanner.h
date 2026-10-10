#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

class FileScanner {
public:
    using FileCallback = std::function<void(const std::filesystem::path& sourcePath,
                                            const std::filesystem::path& relativePath)>;

    void scan(const std::filesystem::path& sourceDirectory,
              const std::vector<std::wstring>& masks,
              const FileCallback& onFile) const;

private:
    static bool matchesMask(const std::wstring& fileName, const std::wstring& mask);
};
