#include "Service.h"

std::string Service::listServices() {
    std::stringstream result;

    // Mở Service Control Manager
    SC_HANDLE scmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!scmHandle) {
        std::cerr << "Cannot open Service Control Manager. Error: " << GetLastError() << std::endl;
        return "<p>Error: Cannot open Service Control Manager.</p>";
    }

    DWORD bytesNeeded = 0;
    DWORD serviceCount = 0;
    DWORD resumeHandle = 0;

    // Lần gọi đầu tiên để lấy kích thước buffer cần thiết
    EnumServicesStatusEx(
        scmHandle,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_STATE_ALL, // Lấy cả dịch vụ đang chạy và dừng
        nullptr,
        0,
        &bytesNeeded,
        &serviceCount,
        &resumeHandle,
        nullptr
    );

    // Tạo buffer với kích thước phù hợp
    std::vector<BYTE> buffer(bytesNeeded);
    LPENUM_SERVICE_STATUS_PROCESS services = reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESS>(buffer.data());

    // Lấy thông tin các dịch vụ
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

        // Bắt đầu tạo bảng HTML
        // Bắt đầu tạo bảng HTML
        result << "<table width=\"100%\" style=\"font-family: Arial, sans-serif; border-collapse: collapse; background-color: #f9f9f9; text-align: center;\">";

        // Header của bảng
        result << "<tr style=\"background-color: #f2f2f2;\">";
        result << "<th style=\"border: 1px solid #dddddd; padding: 10px; font-size: 16px; color: #333333;\">Service Name</th>";
        result << "<th style=\"border: 1px solid #dddddd; padding: 10px; font-size: 16px; color: #333333;\">Display Name</th>";
        result << "<th style=\"border: 1px solid #dddddd; padding: 10px; font-size: 16px; color: #333333;\">Status</th>";
        result << "</tr>";

        // Duyệt qua danh sách dịch vụ
        for (DWORD i = 0; i < serviceCount; ++i) {
            ENUM_SERVICE_STATUS_PROCESS& service = services[i];

            // Chuyển đổi tên dịch vụ và tên hiển thị
        #ifdef UNICODE
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::string serviceName = converter.to_bytes(service.lpServiceName);
            std::string displayName = converter.to_bytes(service.lpDisplayName);
        #else
            std::string serviceName = service.lpServiceName;
            std::string displayName = service.lpDisplayName;
        #endif

            // Trạng thái dịch vụ
            std::string status = (service.ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING) ? "Running" : "Stopped";

            // Màu trạng thái
            std::string statusColor = (status == "Running") ? "#28a745" : "#dc3545";

            // Thêm hàng vào bảng
            result << "<tr>";
            result << "<td style=\"border: 1px solid #dddddd; padding: 10px; font-size: 14px; color: #333333;\">" << serviceName << "</td>";
            result << "<td style=\"border: 1px solid #dddddd; padding: 10px; font-size: 14px; color: #333333;\">" << displayName << "</td>";
            result << "<td style=\"border: 1px solid #dddddd; padding: 10px; font-size: 14px; font-weight: bold; color: " << statusColor << ";\">" << status << "</td>";
            result << "</tr>";
        }

        // Đóng bảng
        result << "</table>";

    } else {
        result << "<p>Error: Cannot list services. Error code: " << GetLastError() << "</p>";
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
