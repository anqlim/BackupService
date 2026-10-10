#include "scp.h"

#include "service.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

std::runtime_error makeWinApiError(const std::string& action) {
    const DWORD errorCode = GetLastError();
    return std::runtime_error(action + "; error code: " + std::to_string(errorCode));
}

std::wstring getExecutablePath() {
    std::wstring path(MAX_PATH, L'\0');
    const DWORD length = GetModuleFileName(nullptr, path.data(), MAX_PATH);
    if (length == 0 || length == MAX_PATH) {
        throw makeWinApiError("Cannot determine executable path");
    }

    path.resize(length);
    return path;
}

SERVICE_STATUS_PROCESS queryServiceStatus(SC_HANDLE service) {
    SERVICE_STATUS_PROCESS status{};
    DWORD bytesNeeded = 0;
    if (!QueryServiceStatusEx(
            service,
            SC_STATUS_PROCESS_INFO,
            reinterpret_cast<LPBYTE>(&status),
            sizeof(status),
            &bytesNeeded)) {
        throw makeWinApiError("Cannot query service status");
    }
    return status;
}

void waitUntilStopped(SC_HANDLE service) {
    for (int attempt = 0; attempt < 150; ++attempt) {
        if (queryServiceStatus(service).dwCurrentState == SERVICE_STOPPED) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    throw std::runtime_error("Service did not stop within 30 seconds");
}

} // namespace

void ServiceController::install() {
    SC_HANDLE manager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (manager == nullptr) {
        throw makeWinApiError("Cannot open Service Control Manager");
    }

    const std::wstring commandLine = L"\"" + getExecutablePath() + L"\"";
    SC_HANDLE service = CreateService(
        manager,
        kServiceName,
        kServiceName,
        SERVICE_START | SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        commandLine.c_str(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );
    if (service == nullptr) {
        CloseServiceHandle(manager);
        throw makeWinApiError("Cannot install service");
    }

    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    std::wcout << L"Service installed\n";
}

void ServiceController::start() {
    SC_HANDLE manager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (manager == nullptr) {
        throw makeWinApiError("Cannot open Service Control Manager");
    }

    SC_HANDLE service = OpenService(manager, kServiceName, SERVICE_START);
    if (service == nullptr) {
        CloseServiceHandle(manager);
        throw makeWinApiError("Cannot open service");
    }

    if (!StartService(service, 0, nullptr)) {
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        throw makeWinApiError("Cannot start service");
    }

    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    std::wcout << L"Service started\n";
}

void ServiceController::stop() {
    SC_HANDLE manager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (manager == nullptr) {
        throw makeWinApiError("Cannot open Service Control Manager");
    }

    SC_HANDLE service = OpenService(manager, kServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (service == nullptr) {
        CloseServiceHandle(manager);
        throw makeWinApiError("Cannot open service");
    }

    const SERVICE_STATUS_PROCESS status = queryServiceStatus(service);
    if (status.dwCurrentState != SERVICE_STOPPED) {
        SERVICE_STATUS ignoredStatus{};
        if (!ControlService(service, SERVICE_CONTROL_STOP, &ignoredStatus)) {
            CloseServiceHandle(service);
            CloseServiceHandle(manager);
            throw makeWinApiError("Cannot stop service");
        }
        waitUntilStopped(service);
    }

    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    std::wcout << L"Service stopped\n";
}

void ServiceController::remove() {
    SC_HANDLE manager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (manager == nullptr) {
        throw makeWinApiError("Cannot open Service Control Manager");
    }

    SC_HANDLE service = OpenService(manager, kServiceName, DELETE | SERVICE_QUERY_STATUS);
    if (service == nullptr) {
        CloseServiceHandle(manager);
        throw makeWinApiError("Cannot open service");
    }

    if (queryServiceStatus(service).dwCurrentState != SERVICE_STOPPED) {
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        throw std::runtime_error("Stop the service before removing it");
    }
    if (!DeleteService(service)) {
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        throw makeWinApiError("Cannot remove service");
    }

    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    std::wcout << L"Service removed\n";
}
