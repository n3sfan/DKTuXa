#ifndef SERVER_H_
#define SERVER_H_

#include "common/Request.h"
#include "common/Utils.h"

class Server {
    public:
        // Huy
        bool handleShutdownSystem(Request& request, Response& response);
        bool handleRestartSystem(Request& request, Response& response);
        bool listRunningApps(Request& request, Response& response);
        bool listInstalledApps(Request& request, Response& response);
        bool closeApp(Request& request, Response& response);
        bool runApp(Request& request, Response& response);
        bool handleApp(Request& request, Response& response);
        bool handleStatus(Request& request, Response& response);

        // Thang
        bool handleGetListFile(Request& request, Response& response);
        bool handleGetFile(Request& request, Response& response);
        bool handleDeleteFile(Request& request, Response& response);
        bool handleFile(Request& request, Response& response);
        bool listServices(Request& request, Response& response);
        bool startService(Request& request, Response& response);
        bool stopService(Request& request, Response& response);
        bool handleService(Request& request, Response& response);
        
        // bool validateChecksum(Request& request, Response& response);
        // bool processReq(Request& request, Response& response);

        
        // Thinh
        bool keylog(Request& request, Response &response);

        //Duc
        bool screenshot(Request& request, Response &response);
        bool getVideoByWebcam(Request& request, Response &response);
        bool PCnameandIP(Request& request, Response& response);
        
        // Thuc hien
        bool processRequest(Request& request, Response &response);
};


#endif