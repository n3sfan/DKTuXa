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

std::vector<std::string> App::getRunningTaskbarApps(){
    std::vector<std::string> runningApps;

    // Hàm callback để kiểm tra từng cửa sổ
    auto enumWindowsCallback = [](HWND hwnd, LPARAM lParam) -> BOOL {
        if (IsWindowVisible(hwnd)) {  // Kiểm tra chỉ cần hiển thị
            wchar_t windowTitle[256];
            GetWindowTextW(hwnd, windowTitle, 256);

            if (wcslen(windowTitle) > 0) {  // Chỉ lấy các cửa sổ có tiêu đề
                std::string title;
                for (int i = 0; windowTitle[i] != L'\0'; ++i){
                    title += static_cast<char>(windowTitle[i]);
                }
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
        std::string windowTitle;
        for (int i = 0; currentTitle[i] != L'\0'; ++i) {
            windowTitle += static_cast<char>(currentTitle[i]);
        }

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
void App::handleCloseApp(const std::vector<std::string>& appList, SOCKET newSocket){
    std::string appList_str = "Choose an app to close: \n";
    for (int i = 0; i < appList.size(); ++i){
        appList_str += std::to_string(i + 1) + ". " + std::string(appList[i].begin(), appList[i].end()) + "\n";
    }
    send(newSocket, appList_str.c_str(), appList_str.size() + 1, 0);
    char appIndexBuffer[10];
    int appIndexReceived = recv(newSocket, appIndexBuffer, sizeof(appIndexBuffer), 0);
    if (appIndexReceived <= 0){
        std::cout << "Failed to receive app index from client.\n";
        return;
    }
    appIndexBuffer[appIndexReceived] = '\0';
    int appIndex = std::stoi(appIndexBuffer);

    if (appIndex <= 0 || appIndex > appList.size()){
        std::string error_msg = "Invalid app index.\n";
        send(newSocket, error_msg.c_str(), error_msg.size() + 1, 0);
        return;
    }
    std::string appName = appList[appIndex - 1];
    bool success = closeApplication(appName);

    std::string result_msg = success ? "Application closed succesfully." : "Failed to close application.";
    send(newSocket, result_msg.c_str(), result_msg.size() + 1, 0);
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
void App::handleRunApp(const std::vector<AppInfo>& appList, SOCKET newSocket){
    std::string appListStr = "Choose an app to run:\n";
    for (int i = 0; i < appList.size(); i++) {
        appListStr += std::to_string(i + 1) + ". " + appList[i].name + "\n";
    }
    // Gửi danh sách ứng dụng cho client
    send(newSocket, appListStr.c_str(), appListStr.size() + 1, 0);

    // Nhận index ứng dụng từ client
    char appIndexBuffer[10];
    int appIndexReceived = recv(newSocket, appIndexBuffer, sizeof(appIndexBuffer), 0);
    if (appIndexReceived <= 0) {
        std::cout << "Failed to receive app index from client.\n";
        return;
    }

    appIndexBuffer[appIndexReceived] = '\0';
    int appIndex = std::stoi(appIndexBuffer);

    // Kiểm tra tính hợp lệ của index
    if (appIndex <= 0 || appIndex > appList.size()) {
        std::string errorMsg = "Invalid app index.\n";
        send(newSocket, errorMsg.c_str(), errorMsg.size() + 1, 0);
        return;
    }

    // Lấy đường dẫn của ứng dụng và mở nó
    std::string appPath = appList[appIndex - 1].fullpath;
    bool success = runApplication(appPath);

    // Gửi kết quả lại cho client
    std::string resultMsg = success ? "Application started successfully." : "Failed to start application.";
    send(newSocket, resultMsg.c_str(), resultMsg.size() + 1, 0);
}