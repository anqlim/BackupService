#include <exception>
#include <iostream>
#include <string>
#include <tchar.h>

#include "scp.h"
#include "service.h"

int runScpCommand(const _TCHAR* command) {
    ServiceController serviceController;

    if (std::basic_string<_TCHAR>(command) == _T("install")) {
        serviceController.install();
    } else if (std::basic_string<_TCHAR>(command) == _T("start")) {
        serviceController.start();
    } else if (std::basic_string<_TCHAR>(command) == _T("stop")) {
        serviceController.stop();
    } else if (std::basic_string<_TCHAR>(command) == _T("remove")) {
        serviceController.remove();
    } else {
        return 1;
    }

    return 0;
}

int runServiceDispatcher() {
    SERVICE_TABLE_ENTRY serviceTable[] = {
        { const_cast<LPTSTR>(kServiceName), ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcher(serviceTable)) {
        return 1;
    }

    return 0;
}

int _tmain(int argc, _TCHAR* argv[]) {
    try {
        if (argc > 1) {
            return runScpCommand(argv[1]);
        }

        return runServiceDispatcher();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
