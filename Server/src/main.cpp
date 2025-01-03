#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <thread>
#include <atomic>
#include <cstring>
#include <future>
#include <cstdlib>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "KeyLogger.h"
#include "Server.h"
#include "common/Request.h"
#include "common/Utils.h"
#include "common/FileUpDownloader.h"

#define DEFAULT_PORT "5555"
#define DEFAULT_PORT2 "5556"

SOCKET ListenSocket = INVALID_SOCKET;
std::atomic_bool stopServer;
std::atomic_bool serverFinished(false);

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
    serverFinished = false;

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
            std::cout << "DEBUG: Uploading file " << filename << "\n";
            uploader.uploadFile(ClientSocket, filename);
            std::cout << "DEBUG: Uploaded file " << filename << "\n";
        }

        std::cout << "Finished downloading\n"; 
        uploader.joinThread(); 
    }

    closesocket(ListenSocket);
    serverFinished = true;

    return 0;
}

int serverUDP() {
    serverFinished = false;
    
    int iResult;

    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM; // Sử dụng SOCK_DGRAM cho UDP
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT2, &hints, &result);
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

    // Setup the UDP listening socket (bind với cổng)
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    // Set the socket to be asynchronous
    // if (WSAAsyncSelect(ListenSocket, hwnd, WM_WSAASYNC, FD_READ | FD_WRITE) == SOCKET_ERROR) {
    //     std::cerr << "WSAAsyncSelect failed. Error: " << WSAGetLastError() << std::endl;
    //     DestroyWindow(hwnd);
    //     closesocket(ListenSocket);
    //     WSACleanup();
    //     return 1;
    // }

    // Server Side
    std::cout << "Server started!\n";

    Server server;
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

     // Server loop
    // while (!stopServer) {
    // Xử lý request
    PacketBuffer buffer(ListenSocket, true, &clientAddr);
    // buffer.getBuffer() = std::string(recvbuf.get(), iResult); // Chuyển đổi buffer thành chuỗi
    Request request;
    Response response;
    request.deserialize(buffer);
    server.processRequest(request, response);
   
    for (int tries = 0; tries < 5; ++tries) {
        // std::cout << "DEBUG: Received data from " << inet_ntoa(clientAddr.sin_addr) << "\n";

        // Gửi phản hồi lại client qua UDP
        SOCKET ResponseSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        buffer = PacketBuffer(ResponseSocket, false, &clientAddr);
        response.serialize(buffer);
        std::cout << "DEBUG: Sent response to " << inet_ntoa(clientAddr.sin_addr) << "\n";

        closesocket(ResponseSocket);
        int r = rand() % 1000;
        std::this_thread::sleep_for(std::chrono::milliseconds(r));
    }

    closesocket(ListenSocket);
    serverFinished = true;

    return 0;
}

// void scheduleServer() {
//     bool udp = false;
//     while (true) {
//         std::thread serverThread(udp ? serverUDP : server);
//         std::this_thread::sleep_for(std::chrono::seconds(udp ? 5 : 10));
//         std::cout << "Stopping server\n";
//         stopServer = true;
//         while (!serverFinished) {
//             std::this_thread::sleep_for(std::chrono::seconds(1));
//         }
//         udp = !udp;
//         stopServer = false;
//     }
// }

int main() {
    srand(time(0));

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
    serverUDP();
    server();

    // cleanup
    WSACleanup();
    return 0;
}