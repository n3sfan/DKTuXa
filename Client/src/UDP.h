#ifndef UDP_H_
#define UDP_H_

#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <memory>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "IMAPClient.h"
#include "SMTPClient.h"
#include "common/Request.h"
#include "common/Utils.h"
#include "common/FileUpDownloader.h"
#include "common/broadcastIP.h"
#include "common/AccountTable.h"

#define DEFAULT_PORT "5555"
// #define DEFAULT_PORT 5555

//broadcast 
extern bool isBroadcast; 
extern std::map<std::string, std::string> pcNameIPMap; // Map lưu PC name - IP

int setupSocket(SOCKET &SendSocket, struct addrinfo *&result, const std::string &host, bool enableBroadcast);
int sendUDP(Request &request, Response &response);
int sendUDPBroadcast(Request &request, Response &response, bool &isBroadcast, std::map<std::string, std::string> &pcNameIPMap);
void listenToInboxUDP();

#endif