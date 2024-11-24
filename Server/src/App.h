#ifndef APP_H_
#define APP_H_

#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include <winsock2.h>
#include <Windows.h>
#include <shlobj.h>

struct AppInfo {
    std::string name;
    std::string fullpath;
};
struct WindowSearchContext {
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

#endif // APP_H_