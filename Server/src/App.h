#ifndef APP_H_
#define APP_H_

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
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
        
        std::vector<std::pair<std::string, DWORD>> getRunningTaskBarAppsbyPID();
        bool closeApplication(DWORD pid);
        bool closeApplicationByPIDandName(DWORD pid, const std::string &targetWindowName);

        bool runApplication(const std::string& executablePath); // done
        std::vector<AppInfo> getInstalledApps(); // done

        std::string getInstalledAppsHTML();

        std::string getRunningAppsHTML();

        std::string conflictApps(std::vector<std::pair<std::string, DWORD>> matchingApps, int pid);
    private:        
        void scanDirectory(const std::wstring directory, std::vector<AppInfo>& appList); // done
};

#endif // APP_H_