#include "file_scanner.h"

#include <cwctype>
#include <stdexcept>

bool FileScanner::matchesMask(const std::wstring& fileName, const std::wstring& mask) {
    std::size_t filePosition = 0;
    std::size_t maskPosition = 0;
    std::size_t starPosition = std::wstring::npos;
    std::size_t retryPosition = 0;

    while (filePosition < fileName.size()) {
        if (maskPosition < mask.size()
            && (mask[maskPosition] == L'?'
                || std::towlower(mask[maskPosition]) == std::towlower(fileName[filePosition]))) {
            ++filePosition;
            ++maskPosition;
        } else if (maskPosition < mask.size() && mask[maskPosition] == L'*') {
            starPosition = maskPosition++;
            retryPosition = filePosition;
        } else if (starPosition != std::wstring::npos) {
            maskPosition = starPosition + 1;
            filePosition = ++retryPosition;
        } else {
            return false;
        }
    }

    while (maskPosition < mask.size() && mask[maskPosition] == L'*') {
        ++maskPosition;
    }

    return maskPosition == mask.size();
}

void FileScanner::scan(const std::filesystem::path& sourceDirectory,
                       const std::vector<std::wstring>& masks,
                       const FileCallback& onFile) const {
    if (!onFile) {
        throw std::invalid_argument("File callback is not set");
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto fileName = entry.path().filename().wstring();
        for (const auto& mask : masks) {
            if (matchesMask(fileName, mask)) {
                onFile(entry.path(), entry.path().lexically_relative(sourceDirectory));
                break;
            }
        }
    }
}
