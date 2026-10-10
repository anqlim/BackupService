#include "service.h"

#include "backup/worker.h"

#include <tchar.h>

namespace {

SERVICE_STATUS serviceStatus{};
SERVICE_STATUS_HANDLE serviceStatusHandle = nullptr;
HANDLE stopEvent = nullptr;

void WINAPI ServiceControlHandler(DWORD controlCode) {
    if (controlCode == SERVICE_CONTROL_STOP || controlCode == SERVICE_CONTROL_SHUTDOWN) {
        serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        serviceStatus.dwControlsAccepted = 0;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);
        SetEvent(stopEvent);
    }
}

} // namespace

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    serviceStatus.dwControlsAccepted = 0;
    serviceStatus.dwWin32ExitCode = NO_ERROR;
    serviceStatus.dwServiceSpecificExitCode = 0;
    serviceStatus.dwCheckPoint = 0;
    serviceStatus.dwWaitHint = 0;

    serviceStatusHandle = RegisterServiceCtrlHandler(kServiceName, ServiceControlHandler);
    if (serviceStatusHandle == nullptr) {
        return;
    }
    SetServiceStatus(serviceStatusHandle, &serviceStatus);

    stopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (stopEvent == nullptr) {
        serviceStatus.dwCurrentState = SERVICE_STOPPED;
        serviceStatus.dwWin32ExitCode = GetLastError();
        SetServiceStatus(serviceStatusHandle, &serviceStatus);
        return;
    }

    HANDLE workerThread = CreateThread(nullptr, 0, BackupWorker, stopEvent, 0, nullptr);
    if (workerThread == nullptr) {
        serviceStatus.dwCurrentState = SERVICE_STOPPED;
        serviceStatus.dwWin32ExitCode = GetLastError();
        SetServiceStatus(serviceStatusHandle, &serviceStatus);
        CloseHandle(stopEvent);
        return;
    }

    serviceStatus.dwCurrentState = SERVICE_RUNNING;
    serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    SetServiceStatus(serviceStatusHandle, &serviceStatus);

    WaitForSingleObject(workerThread, INFINITE);

    DWORD workerExitCode = NO_ERROR;
    GetExitCodeThread(workerThread, &workerExitCode);

    CloseHandle(workerThread);
    CloseHandle(stopEvent);

    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    serviceStatus.dwControlsAccepted = 0;
    if (workerExitCode != NO_ERROR) {
        serviceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        serviceStatus.dwServiceSpecificExitCode = workerExitCode;
    }
    SetServiceStatus(serviceStatusHandle, &serviceStatus);
}
