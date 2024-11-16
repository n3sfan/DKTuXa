#include <windows.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <thread>
#include <chrono>
#include <sstream>

using namespace std;

class Service{
    public:
        string listRunningServices();
        bool StartServiceByName(const string& serviceName);
        bool StopServiceByName(const string& serviceName);
};