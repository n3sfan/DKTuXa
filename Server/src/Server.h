#ifndef SERVER_H_
#define SERVER_H_

#include "common/Request.h"

class Server {
    public:
        // Huy
        bool handleShutdownSystem(Request& request, Response& response);
        bool handleRestartSystem(Request& request, Response& response);
        bool listRunningApps(Request& request, Response& response);
        bool listInstalledApps(Request& request, Response& response);
        bool closeApp(Request& request, Response& response);
        bool runApp(Request& request, Response& response);
        
        // Thinh
        bool keylog(Request& request, Response &response);
        
        // Thuc hien
        bool processRequest(Request& request, Response &response);
};


#endif