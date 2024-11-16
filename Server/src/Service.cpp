#include "Service.h"

string Service::listRunningServices(){
    stringstream result;
    SC_HANDLE scmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!scmHandle) {
        cerr << "Cannot open Service Control Manager. Error: " << GetLastError() << endl;
        return result.str();
    }

    DWORD bytesNeeded = 0;
    DWORD serviceCount = 0;
    DWORD resumeHandle = 0;

    // Lần đầu tiên gọi hàm để lấy số byte cần thiết
    EnumServicesStatusEx(
        scmHandle,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_STATE_ALL,
        nullptr,
        0,
        &bytesNeeded,
        &serviceCount,
        &resumeHandle,
        nullptr
    );

    vector<BYTE> buffer(bytesNeeded);
    LPENUM_SERVICE_STATUS_PROCESS services = reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESS>(buffer.data());

    // Lần thứ hai gọi hàm với buffer đã được cấp phát
    if (EnumServicesStatusEx(
            scmHandle,
            SC_ENUM_PROCESS_INFO,
            SERVICE_WIN32,
            SERVICE_STATE_ALL,
            buffer.data(),
            bytesNeeded,
            &bytesNeeded,
            &serviceCount,
            &resumeHandle,
            nullptr)) {
        

        result << "List running service:\n";
        result << left << setw(40) << "Service Name" << setw(40) << "Display Name" << endl;
        result << "-------------------------------------------------------------" << endl;
        for (DWORD i = 0; i < serviceCount; ++i) {
            ENUM_SERVICE_STATUS_PROCESS& service = services[i];
            if (service.ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING) {
                result << left << setw(40) << service.lpServiceName << setw(40) << service.lpDisplayName << endl;
            }
        }
    } else {
        result << "Cannot list service. Error: " << GetLastError() << endl;
    }

    CloseServiceHandle(scmHandle);
    return result.str();
}

bool Service::StartServiceByName(const string& serviceName){
    // Chuyển đổi từ string sang wstring
    wstring wideServiceName(serviceName.begin(), serviceName.end());

    // Mở trình quản lý dịch vụ
    SC_HANDLE scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scManager) {
        wcerr << L"Cannot open Service Control Manager. Error: " << GetLastError() << endl;
        return false;
    }

    // Mở dịch vụ
    SC_HANDLE service = OpenServiceW(scManager, wideServiceName.c_str(), SERVICE_START);
    if (!service) {
        wcerr << L"Cannot open service. Error: " << GetLastError() << endl;
        CloseServiceHandle(scManager);
        return false;
    }

    // Khởi động dịch vụ
    if (!StartService(service, 0, nullptr)) {
        DWORD error = GetLastError();
        if (error == ERROR_SERVICE_ALREADY_RUNNING) {
            wcout << L"Service is already running." << endl;
        } else {
            wcerr << L"Cannot start service. Error: " << error << endl;
        }
    } else {
        wcout << L"Service started successfully." << endl;
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;

}

bool Service::StopServiceByName(const string& serviceName){
    // Chuyển đổi từ string sang wstring
    wstring wideServiceName(serviceName.begin(), serviceName.end());

    // Mở trình quản lý dịch vụ
    SC_HANDLE scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scManager) {
        wcerr << L"Cannot open Service Control Manager. Error: " << GetLastError() << endl;
        return false;
    }

    // Mở dịch vụ
    SC_HANDLE service = OpenServiceW(scManager, wideServiceName.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!service) {
        wcerr << L"Cannot open service. Error: " << GetLastError() << endl;
        CloseServiceHandle(scManager);
        return false;
    }

    // Dừng dịch vụ
    SERVICE_STATUS status;
    if (!ControlService(service, SERVICE_CONTROL_STOP, &status)) {
        wcerr << L"Cannot stop service. Error: " << GetLastError() << endl;
    } else {
        wcout << L"Service stopped successfully." << endl;
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;

}
