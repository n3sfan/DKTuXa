#ifndef SERVICE_H_
#define SERVICE_H_

#include <iostream>
#include <vector>
#include <iomanip>
#include <thread>
#include <chrono>
#include <sstream>
#include <locale>
#include <codecvt>

#include <windows.h>

class Service{
    public:
        std::string listRunningServices();
        bool StartServiceByName(const std::string& serviceName);
        bool StopServiceByName(const std::string& serviceName);
};

#endif // SERVICE_H_