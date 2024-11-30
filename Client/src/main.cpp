#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <memory>
#include <regex>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "IMAPClient.h"
#include "SMTPClient.h"
#include "common/Request.h"
#include "common/Utils.h"
#include "common/FileUpDownloader.h"
#include "common/SHA256.h"

using namespace std;

#define DEFAULT_PORT "5555"

const string kAppPass = "sigc xldk cuzd bjhr";

/**
 * Protocol 2.
 */
int send(Request &request, Response &response) {
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    unique_ptr<char[]> recvbuf(new char[8192]());
    const int recvbuflen = 8192;
    int iResult;
    
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    string host = request.getParam(kIPAttr);
    iResult = getaddrinfo(host.c_str(), DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    // std::cout << "Socket addr: " << ConnectSocket << "\n";
    PacketBuffer buffer(ConnectSocket, false);
    request.serialize(buffer);

    // printf("Bytes Sent: %ld / %d\n", len, total);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    buffer = PacketBuffer(ConnectSocket, true);
    response.getParams().clear();
    response.deserialize(buffer);

    closesocket(ConnectSocket);
    
    // Download Manager - New sockets
    Downloader downloader;

    for (const std::pair<std::string, std::string> &pr : response.getParams()) {
        if (!startsWith(pr.first, kFilePrefix))
            continue;

        std::string file = pr.first.substr(kFilePrefix.size());
        cout << "DEBUG: Downloading file " << file << "\n";
        downloader.downloadFile(host, DEFAULT_PORT, file);
        cout << "DEBUG: Downloaded file " << file << "\n";
    }

    downloader.joinThread();

    return 0;
}

bool isHtmlContent(const std::string& content) {
    // Biểu thức chính quy để kiểm tra thẻ HTML
    std::regex html_tags(R"(<[a-z][\s\S]*?>)");
    return std::regex_search(content, html_tags);
}

void listenToInbox() {
    string str;

    CSMTPClient SMTPClient([](const std::string& s){ cout << s << "\n"; return; });  
    SMTPClient.SetCertificateFile("curl-ca-bundle.crt");
    SMTPClient.InitSession("smtp.gmail.com:465", "quangminhcantho43@gmail.com", kAppPass,
			CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL);

    // Receive mail
    while (true) {
        CIMAPClient IMAPClient([](const std::string& s){ cout << s << "\n"; return; });  
        IMAPClient.SetCertificateFile("curl-ca-bundle.crt");
        IMAPClient.InitSession("imap.gmail.com:993", "quangminhcantho43@gmail.com", kAppPass,
                CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL);
       
        str = "";
        bool ok = IMAPClient.Search(str, CIMAPClient::SearchOption::UNSEEN);
        cout << str << "\n";

        vector<string> mailIds = split(str.substr(string("* SEARCH ").size()), " ");
        for (string &mailId : mailIds) {
            mailId = trim(mailId);
            if (mailId.size() == 0)
                continue;
            
            string mailHeaders = "", mailBody = "";
            ok = IMAPClient.GetString(mailId, mailHeaders);
            ok = IMAPClient.GetString(mailId, mailBody, true); 
            if (!ok) {
                // TODO NOTIFY
                continue;
            } 

            // cout << mailHeaders << " headers5\n";
            // cout << mailBody << " body\n";

            string mailFrom, mailSubject;
            Request request;
            request.parseFromMail(mailHeaders, mailBody, mailFrom, mailSubject);

            if (!isPassWordValid(request.getParam(kPassWord))){
                // TODO NOTIFY
                cout << "Invalid password.\n";
                continue;
            }
            if (request.getAction() == ACTION_INVALID) {
                // TODO NOTIFY
                continue;
            }
            cout << "Processing request, action " << request.getAction() << "\n";
            Response response;
            if (send(request, response) != 0) {
                // TODO ERROR
                continue;
            }
            
            // Send mail response
            string mailStr; 
            response.toMailString(mailSubject, mailStr);
            // response.saveFiles();

            bool use_html = isHtmlContent(mailBody);

            SMTPClient.SendMIME(mailFrom, {"Subject: " + mailSubject}, mailStr, response.getFiles(), use_html);
            
            // response.deleteFiles();'

            cout << "---------\nResponse: " << response << "\n-------------\n";
            // Add to queue
            // responsesQueue.push();
        }   

        IMAPClient.CleanupSession();

        // TODO SLOW
        this_thread::sleep_for(4s);
    }    

    SMTPClient.CleanupSession();
}

int main() { 
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    // Create folder "files"
    CreateDirectoryA("files", NULL);

    listenToInbox();
 
    WSACleanup();

    return 0;
}