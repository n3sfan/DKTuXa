#ifndef SERVER_H_
#define SERVER_H_

#include <filesystem>

#include <windows.h>
#include <stdio.h>

#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "common/Request.h"

// using namespace cv;

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

        // Thang
        bool handleGetFile(Request& request, Response& response);
        bool handleDeleteFile(Request& request, Response& response);
        bool listRunningService(Request& request, Response& response);
        bool handleStartService(Request& request, Response& response);
        bool handleStopService(Request& request, Response& response);

        
        // Thinh
        bool keylog(Request& request, Response &response);

        //Duc
        bool isRecording;
        bool screenshot(Request& request, Response &response);
        bool getVideoByWebcam(Request& request, Response &response);
        
        // Thuc hien
        bool processRequest(Request& request, Response &response);
};


#endif