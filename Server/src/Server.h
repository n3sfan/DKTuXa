#ifndef SERVER_H_
#define SERVER_H_

#include "common/Request.h"

class Server {
    public:
        bool keylog(Request& request, Response &response);
        bool processRequest(Request& request, Response &response);
};


#endif