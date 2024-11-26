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

std::vector<std::string> App::getRunningTaskbarApps(){
    std::vector<std::string> runningApps;

    // Hàm callback để kiểm tra từng cửa sổ
    auto enumWindowsCallback = [](HWND hwnd, LPARAM lParam) -> BOOL {
        if (IsWindowVisible(hwnd)) {  // Kiểm tra chỉ cần hiển thị
            wchar_t windowTitle[256];
            GetWindowTextW(hwnd, windowTitle, 256);

            if (wcslen(windowTitle) > 0) {  // Chỉ lấy các cửa sổ có tiêu đề
                std::string title = wideCharToString(windowTitle);
                ((std::vector<std::string>*)lParam)->push_back(title);
            }
        }
        return TRUE;
    };

    // Sử dụng EnumWindows để duyệt qua tất cả cửa sổ hiển thị
    EnumWindows(enumWindowsCallback, (LPARAM)&runningApps);
    
    return runningApps;
}

bool App::closeApplication(const std::string& executablePath){
    WindowSearchContext context = {executablePath, false};

    auto enumWindowsCallback = [](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* context = reinterpret_cast<WindowSearchContext*>(lParam);
        wchar_t currentTitle[256];
        GetWindowTextW(hwnd, currentTitle, 256);

        // Chuyển đổi wchar_t* sang std::string
        std::string windowTitle = wideCharToString(currentTitle);

        // So sánh tiêu đề cửa sổ với tiêu đề cần tìm
        if (windowTitle == context->title) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            context->found = true;
            return FALSE; // Dừng EnumWindows sau khi tìm thấy
        }
        return TRUE; // Tiếp tục tìm kiếm
    };

    // Sử dụng EnumWindows để duyệt qua tất cả cửa sổ hiển thị
    EnumWindows(enumWindowsCallback, reinterpret_cast<LPARAM>(&context));
    return context.found;
}

void App::scanDirectory(const std::string directory, std::vector<AppInfo>& appList){
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind;
    std::string searchPattern = directory + "\\*";

    hFind = FindFirstFileA(searchPattern.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Can't locate the file: " << directory << "\n";
        return;
    }

    do {
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0) {
            continue;
        }

        std::string fullPath = directory + "\\" + findFileData.cFileName;
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            scanDirectory(fullPath, appList); // Quét thư mục con
        } else if (strstr(findFileData.cFileName, ".lnk")) {
            std::string appName = findFileData.cFileName;
            appName = appName.substr(0, appName.find_last_of("."));
            appList.push_back({ appName, fullPath });
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
}

std::vector<AppInfo> App::getInstalledApps(){
    std::vector<AppInfo> appList;
    std::vector<std::string> startMenuPaths = {
        "C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs",
        std::string("C:\\Users\\") + getenv("USERNAME") + "\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs"
    };

    for (const auto& path : startMenuPaths) {
        scanDirectory(path, appList);
    }
    return appList;
}

bool App::runApplication(const std::string &executablePath){
    HINSTANCE result = ShellExecuteA(NULL, "open", executablePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return reinterpret_cast<intptr_t>(result) > 32;
}