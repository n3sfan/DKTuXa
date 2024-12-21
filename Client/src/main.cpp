#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <memory>
#include <stdexcept>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "IMAPClient.h"
#include "SMTPClient.h"
#include "common/Request.h"
#include "common/Utils.h"
#include "common/FileUpDownloader.h"
#include "common/SHA256.h"
#include "common/AccountTable.h"
#include "UDP.h"

using namespace std;

#define DEFAULT_PORT "5555"

/**
 * Protocol 2.
 */
int send(Request &request, Response &response) {
    if (request.getParam(kSubAction).empty() || (request.getParam(kIPAttr).empty() && request.getParam(kPcName).empty())){
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    int iResult;
    
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_protocol = IPPROTO_TCP; 

    // Resolve the server address and port
    string host;
    if (!request.getParam(kIPAttr).empty()) {
        host = request.getParam(kIPAttr);
    }
    else if (!request.getParam(kPcName).empty()){
        auto pi = pcNameIPMap.find(request.getParam(kPcName));
        if (pi != pcNameIPMap.end()){
            host = pi->second;
        }
    }
    iResult = getaddrinfo(host.c_str(), DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        // WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            // WSACleanup();
            return 1;
        }

        // Connect to server.
        setSockOptions(ConnectSocket);
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
        // WSACleanup();
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
        // WSACleanup();
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

void listenToInbox() {
    string str;

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
            try {
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

                string mailFrom, mailSubject, mailMessageId;
                Request request;
                request.parseFromMail(mailHeaders, mailBody, mailFrom, mailSubject, mailMessageId);

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

                if (request.getAction() == ACTION_BROADCAST) {
                     if (sendUDP(request, response) != 0) {
                        // TODO ERROR
                        continue;
                    }

                    // List PC Name and IP Address
                    std::string valueBodyBroadcast = pcNameIPHTML(pcNameIPMap);
                    response.putParam(kUseHtml, "true");
                    response.putParam(kBody, valueBodyBroadcast);
                } else {
                    if (send(request, response) != 0) {
                        // TODO ERROR
                        continue;
                    }
                }
               
                // Send mail response
                string mailStr; 
                response.toMailString(mailSubject, mailStr);
                // response.saveFiles();

                // cout << mailHeaders << " headers\n";
                // cout << mailMessageId << " mid\n";

                CSMTPClient SMTPClient([](const std::string& s){ cout << s << "\n"; return; });  
                SMTPClient.SetCertificateFile("curl-ca-bundle.crt");
                SMTPClient.InitSession("smtp.gmail.com:465", "quangminhcantho43@gmail.com", kAppPass, 
                                    CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL);
                SMTPClient.SendMIME(mailFrom, {"References: " + mailMessageId, "In-Reply-To: " + mailMessageId, "Subject: Re: " + mailSubject}, mailStr, response.getFiles(), toLower(response.getParam(kUseHtml)) == "true");
                SMTPClient.CleanupSession();
                
                // response.deleteFiles();'

                // cout << "---------\nResponse: " << response << "\n-------------\n";
                // Add to queue
                // responsesQueue.push();
            } catch (std::exception& e) {
                std::cerr << e.what() << "\n";
            }
        }   

        IMAPClient.CleanupSession();

        // TODO SLOW
        this_thread::sleep_for(4s);
    }    
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
    
    // listenToInboxUDP();
    listenToInbox();

    WSACleanup();

    return 0;
}