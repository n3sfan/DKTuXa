#include <iostream>
#include <vector>
#include <iomanip>
#include <thread>
#include <chrono>
#include <sstream>

#include <windows.h>

class Service{
    public:
        std::string listRunningServices();
        bool StartServiceByName(const std::string& serviceName);
        bool StopServiceByName(const std::string& serviceName);
};