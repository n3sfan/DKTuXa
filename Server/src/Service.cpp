#include "Service.h"

std::string Service::listRunningServices(){
    std::stringstream result;
    SC_HANDLE scmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!scmHandle) {
        std::cerr << "Cannot open Service Control Manager. Error: " << GetLastError() << std::endl;
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

    std::vector<BYTE> buffer(bytesNeeded);
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
        

       // In tiêu đề cột
        constexpr int colWidth = 40; // Độ rộng mỗi cột
        result << "List of Running Services:\n";
        result << std::left << std::setw(colWidth) << "Service Name" 
               << std::setw(colWidth) << "Display Name" << std::endl;
        result << std::string(2 * colWidth, '-') << std::endl;

        // In thông tin từng dịch vụ đang chạy
        for (DWORD i = 0; i < serviceCount; ++i) {
            ENUM_SERVICE_STATUS_PROCESS& service = services[i];
            if (service.ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING) {
                result << std::left << std::setw(colWidth) << service.lpServiceName 
                       << std::setw(colWidth) << service.lpDisplayName << std::endl;
            }
        }
    } else {
        result << "Cannot list service. Error: " << GetLastError() << std::endl;
    }

    CloseServiceHandle(scmHandle);
    return result.str();
}

bool Service::StartServiceByName(const std::string& serviceName){
    // Chuyển đổi từ std::string sang std::wstring
    std::wstring wideServiceName(serviceName.begin(), serviceName.end());

    // Mở trình quản lý dịch vụ
    SC_HANDLE scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scManager) {
        std::wcerr << L"Cannot open Service Control Manager. Error: " << GetLastError() << std::endl;
        return false;
    }

    // Mở dịch vụ
    SC_HANDLE service = OpenServiceW(scManager, wideServiceName.c_str(), SERVICE_START);
    if (!service) {
        std::wcerr << L"Cannot open service. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(scManager);
        return false;
    }

    // Khởi động dịch vụ
    if (!StartService(service, 0, nullptr)) {
        DWORD error = GetLastError();
        if (error == ERROR_SERVICE_ALREADY_RUNNING) {
            std::wcout << L"Service is already running." << std::endl;
        } else {
            std::wcout << L"Cannot start service. Error: " << error << std::endl;
        }
    } else {
        std::wcout << L"Service started successfully." << std::endl;
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;

}

bool Service::StopServiceByName(const std::string& serviceName){
    // Chuyển đổi từ std::string sang std::wstring
    std::wstring wideServiceName(serviceName.begin(), serviceName.end());

    // Mở trình quản lý dịch vụ
    SC_HANDLE scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scManager) {
        std::wcout << L"Cannot open Service Control Manager. Error: " << GetLastError() << std::endl;
        return false;
    }

    // Mở dịch vụ
    SC_HANDLE service = OpenServiceW(scManager, wideServiceName.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!service) {
        std::wcout << L"Cannot open service. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(scManager);
        return false;
    }

    // Dừng dịch vụ
    SERVICE_STATUS status;
    if (!ControlService(service, SERVICE_CONTROL_STOP, &status)) {
        std::wcout << L"Cannot stop service. Error: " << GetLastError() << std::endl;
    } else {
        std::wcout << L"Service stopped successfully." << std::endl;
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;

}
