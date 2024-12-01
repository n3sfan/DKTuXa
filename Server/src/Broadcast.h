#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

std::string getPCName();
std::string getIPAddress();
std::string getPCInfo(); //Tổng hợp


