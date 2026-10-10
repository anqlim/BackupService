cmake_minimum_required(VERSION 3.20)
project(BackupService)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BUILD_REGRESS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_DOC OFF CACHE BOOL "" FORCE)
set(BUILD_OSSFUZZ OFF CACHE BOOL "" FORCE)
set(LIBZIP_DO_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_BZIP2 OFF CACHE BOOL "" FORCE)
set(ENABLE_LZMA OFF CACHE BOOL "" FORCE)
set(ENABLE_ZSTD OFF CACHE BOOL "" FORCE)
set(ENABLE_OPENSSL OFF CACHE BOOL "" FORCE)
set(ENABLE_GNUTLS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    libzip
    URL https://libzip.org/download/libzip-1.11.4.tar.xz
)
FetchContent_MakeAvailable(libzip)

add_executable(BackupService
    src/main.cpp
    src/service.cpp
    src/scp.cpp
    src/backup/worker.cpp
    src/backup/config.cpp
    src/backup/file_scanner.cpp
    src/backup/zip_archive.cpp
)

target_link_libraries(BackupService PRIVATE advapi32)
target_link_libraries(BackupService PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(BackupService PRIVATE zip)

target_compile_definitions(BackupService PRIVATE UNICODE _UNICODE)
