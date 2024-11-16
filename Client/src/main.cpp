#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <memory>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "IMAPClient.h"
#include "SMTPClient.h"
#include "common/Request.h"
#include "common/Utils.h"

using namespace std;

#define DEFAULT_PORT "4444"

const string kAppPass = "sigc xldk cuzd bjhr";

int send(Request &request, Response &response) {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    unique_ptr<char[]> recvbuf(new char[8192]());
    const int recvbuflen = 8192;
    int iResult;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

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
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
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

    // Construct request TODO TEST
    // Send an initial buffer
    string sendbuf = request.serialize();
    int total = sendbuf.size();
    int len = 0;
    while ((iResult = send(ConnectSocket, sendbuf.c_str() + len, total - len, 0)) > 0) {
        if (iResult < 0) { 
            cout << "Send error: " <<  WSAGetLastError() << "\n";
            // NOTIFY
            break;
        }

        len += iResult;
    }

    printf("Bytes Sent: %ld / %d\n", len, total);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    string buf = "";
    do {
        iResult = recv(ConnectSocket, recvbuf.get(), recvbuflen, 0);
        // std::cout << iResult << " result\n";

        if (iResult > 0) { 
            // printf("Bytes received: %d\n", iResult);
            buf += string(recvbuf.get(), iResult);
        } else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while(iResult > 0);

    cout << "Debug: Received " << buf.size() << " bytes\n";

    response.getParams().clear();
    response.deserialize(buf);

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
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
            response.saveFiles();

            SMTPClient.SendMIME(mailFrom, {"Subject: " + mailSubject}, mailStr, response.getFiles());
            
            response.deleteFiles();

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
    listenToInbox();

    return 0;
}