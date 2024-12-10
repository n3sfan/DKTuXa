#include "App.h"

void App::shutdownSystem(){
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    
    // Lấy token cho tiến trình hiện tại
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)){
        return;
    }
    // Lấy LUID cho đặc quyền shutdown
    LookupPrivilegeValue(nullptr, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1; // one privillege to set;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    // Kích hoạt đặc quyền shutdown cho tiến trình
    AdjustTokenPrivileges(hToken, false, &tkp, 0, (PTOKEN_PRIVILEGES)nullptr, 0);
    if (GetLastError() != ERROR_SUCCESS)
        return;
    
    // Tắt hệ thống, buộc đóng tất cả ứng dụng
    if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER)){
        std::cout << "Shutdown failed: Error: " << GetLastError() << "\n";
    }
    std::cout << "System is shutting down.....\n";
}

void App::restartSystem(){
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Lấy token cho tiến trình hiện tại
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return;
    }

    // Lấy LUID cho đặc quyền shutdown
    LookupPrivilegeValue(nullptr, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Kích hoạt đặc quyền shutdown cho tiến trình
    AdjustTokenPrivileges(hToken, false, &tkp, 0, (PTOKEN_PRIVILEGES)nullptr, 0);
    if (GetLastError() != ERROR_SUCCESS) {
        return;
    }

    // Khởi động lại hệ thống, buộc đóng tất cả ứng dụng
    if (!ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER)) {
        std::cout << "Restart failed: Error: " << GetLastError() << "\n";
    }
    std::cout << "System is restarting.....\n";
}

std::string wideCharToString(const wchar_t* wideStr){
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) return "";
    std::string str(sizeNeeded - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &str[0], sizeNeeded, nullptr, nullptr);
    return str;
}

std::wstring stringToWideChar(const char* str) {
    if (!str) return L"";
    size_t sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
    if (sizeNeeded == 0) return L"";
    std::wstring wstr(sizeNeeded - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, &wstr[0], sizeNeeded);
    return wstr;
}

std::vector<std::pair<std::string, DWORD>> App::getRunningTaskBarAppsbyPID(){
    std::vector<std::pair<std::string, DWORD>> runningApps;

    // Hàm callback để duyệt các cửa sổ
    auto enumWindowsCallback = [](HWND hwnd, LPARAM lParam) -> BOOL {
        if (IsWindowVisible(hwnd)) { // Chỉ xử lý các cửa sổ hiển thị
            wchar_t windowTitle[256];
            GetWindowTextW(hwnd, windowTitle, 256);

            if (wcslen(windowTitle) > 0) { // Chỉ lấy các cửa sổ có tiêu đề
                DWORD processId;
                GetWindowThreadProcessId(hwnd, &processId);

                std::string title = wideCharToString(windowTitle);
                ((std::vector<std::pair<std::string, DWORD>>*)lParam)
                    ->emplace_back(title, processId);
            }
        }
        return TRUE;
    };

    EnumWindows(enumWindowsCallback, (LPARAM)&runningApps);

    // Sắp xếp danh sách theo tên cửa sổ
    std::sort(runningApps.begin(), runningApps.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    return runningApps;
}

bool App::closeApplication(DWORD processId){
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
    if (hProcess) {
        if (TerminateProcess(hProcess, 0)) {
            CloseHandle(hProcess);
            return true;
        } else {
            std::cerr << "Failed to terminate process with PID: " << processId << "\n";
        }
        CloseHandle(hProcess);
    } else {
        std::cerr << "Could not open process with PID: " << processId << "\n";
    }
    return false;
}

bool App::closeApplicationByPIDandName(DWORD targetPID, const std::string &targetWindowName){
    bool isClosed = false;

    // Hàm callback để kiểm tra từng cửa sổ
    auto enumWindowsCallback = [](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* params = reinterpret_cast<std::pair<DWORD, std::string>*>(lParam);
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid); // Lấy PID của cửa sổ hiện tại

        wchar_t windowTitle[256];
        GetWindowTextW(hwnd, windowTitle, 256); // Lấy tiêu đề cửa sổ

        if (pid == params->first) { // So sánh PID
            std::string windowName = wideCharToString(windowTitle);

            if (windowName.find(params->second) != std::string::npos) { // So sánh tên cửa sổ
                // Gửi thông điệp WM_CLOSE để yêu cầu đóng cửa sổ
                SendMessage(hwnd, WM_CLOSE, 0, 0);
                params->second = "closed"; // Đánh dấu cửa sổ đã được xử lý
                return FALSE; // Dừng việc duyệt thêm cửa sổ
            }
        }
        return TRUE; // Tiếp tục duyệt nếu không khớp
    };

    // Gói các tham số PID và tên cửa sổ vào một cặp
    std::pair<DWORD, std::string> searchParams = {targetPID, targetWindowName};

    // Duyệt qua tất cả cửa sổ hiển thị
    EnumWindows(enumWindowsCallback, reinterpret_cast<LPARAM>(&searchParams));

    // Kiểm tra kết quả
    if (searchParams.second == "closed") {
        isClosed = true;
    }

    return isClosed;
}

void App::scanDirectory(const std::wstring directory, std::vector<AppInfo>& appList){
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind;
    std::wstring searchPattern = directory + L"\\*";

    hFind = FindFirstFileW(searchPattern.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Can't locate the file: " << wideCharToString(directory.c_str()) << "\n";
        return;
    }

    do {
        if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0) {
            continue;
        }

        std::wstring fullPath = directory + L"\\" + findFileData.cFileName;
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            scanDirectory(fullPath, appList); // Quét thư mục con
        } else if (wcsstr(findFileData.cFileName, L".lnk")) {
            std::wstring appName = findFileData.cFileName;
            appName = appName.substr(0, appName.find_last_of(L"."));
            appList.push_back({ wideCharToString(appName.c_str()), wideCharToString(fullPath.c_str()) });
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);

    FindClose(hFind);
}

std::vector<AppInfo> App::getInstalledApps() {
    std::vector<AppInfo> appList;
    std::vector<std::wstring> startMenuPaths = {
        L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs",
        std::wstring(L"C:\\Users\\") + stringToWideChar(getenv("USERNAME")) + L"\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs"
    };

    for (const auto& path : startMenuPaths) {
        scanDirectory(path, appList);
    }
    return appList;
}

std::string App::getInstalledAppsHTML() {
    std::vector<AppInfo> apps = getInstalledApps();

    std::ostringstream html;
    html << "<!DOCTYPE html><html><head>";
    html << "<style>"
         << "table { width: 100%; border-collapse: collapse; font-family: Arial, sans-serif; }"
         << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; font-size: 14px; }"
         << "th { background-color: #f2f2f2; }"
         << "tr:nth-child(even) { background-color: #f9f9f9; }"
         << "tr:hover { background-color: #f1f1f1; }"
         << "</style></head><body>";

    html << "<h2 style=\"font-family: Arial, sans-serif; text-align: center;\">Installed Applications</h2>";
    html << "<table>";
    html << "<thead><tr><th>STT</th><th>Application Name</th><th>Path</th></tr></thead>";
    html << "<tbody>";

    for (size_t i = 0; i < apps.size(); ++i) {
        html << "<tr>";
        html << "<td>" << (i + 1) << "</td>";
        html << "<td>" << apps[i].name << "</td>";
        html << "<td style=\"color: #555;\">" << apps[i].fullpath << "</td>";
        html << "</tr>";
    }

    html << "</tbody></table>";
    html << "</body></html>";

    return html.str();
}

std::string App::getRunningAppsHTML(){
    std::vector<std::pair<std::string, DWORD>> runningApps = getRunningTaskBarAppsbyPID();
    std::ostringstream result;

    result  << "<!DOCTYPE html><html><head>";
    result  << "<style>"
            << "table { width: 100%; border-collapse: collapse; font-family: Arial, sans-serif; }"
            << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; font-size: 14px; }"
            << "th { background-color: #f2f2f2; }"
            << "tr:nth-child(even) { background-color: #f9f9f9; }"
            << "tr:hover { background-color: #f1f1f1; }"
            << "</style></head><body>";

    result << "<h1 style=\"text-align: center; color: #333;\">Running Applications</h1>";
    result << "<table>";
    result << "<thead>";
    result << "<tr>";
    result << "<th>STT</th>";
    result << "<th>Application Name</th>";
    result << "<th>PID</th>";
    result << "</tr>";
    result << "</thead>";
    result << "<tbody>";

    for (size_t i = 0; i < runningApps.size(); ++i) {
        result << "<tr style=\"border: 1px solid #ddd;\">";
        result << "<td>" << (i + 1) << "</td>";
        result << "<td>" << runningApps[i].first << "</td>";
        result << "<td>" << runningApps[i].second << "</td>";
        result << "</tr>";
    }

    result << "</tbody>";
    result << "</table>";

    result << "<p style=\"text-align: center; margin-top: 20px; color: #666; font-size: 14px;\">Total Applications: " << runningApps.size() << "</p>";

    result << "</body>";
    result << "</html>";

    return result.str();
}


bool App::runApplication(const std::string &executablePath){
    HINSTANCE result = ShellExecuteA(NULL, "open", executablePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return reinterpret_cast<intptr_t>(result) > 32;
}