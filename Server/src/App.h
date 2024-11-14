#pragma once
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <string.h>
#include <vector>
#include <shlobj.h>

#pragma comment(lib, "ws2_32.lib")

struct AppInfo{
    std::string name;
    std::string fullpath;
};
struct WindowSearchContext{
    std::string title;
    bool found;
};
class App{
    public:
        void shutdownSystem(); // done
        void restartSystem(); // done
        
        void handleCloseApp(const std::vector<std::string>& applist, SOCKET newSocket); // done
        bool closeApplication(const std::string& executablePath); // done
        std::vector<std::string> getRunningTaskbarApps(); // done

        void handleRunApp(const std::vector<AppInfo>& appList, SOCKET newSocket); // done
        bool runApplication(const std::string& executablePath); // done
        std::vector<AppInfo> getInstalledApps(); // done
    private:        
        void scanDirectory(const std::string directory, std::vector<AppInfo>& appList); // done
};