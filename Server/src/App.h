#ifndef APP_H_
#define APP_H_

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <Windows.h>
#include <shlobj.h>

struct AppInfo {
    std::string name;
    std::string fullpath;
};

class App{
    public:
        void shutdownSystem(); // done
        void restartSystem(); // done
        
        bool closeApplication(const std::string& executablePath); // done
        std::vector<std::string> getRunningTaskbarApps(); // done

        bool runApplication(const std::string& executablePath); // done
        std::vector<AppInfo> getInstalledApps(); // done
    private:        
        void scanDirectory(const std::wstring directory, std::vector<AppInfo>& appList); // done
};

#endif // APP_H_