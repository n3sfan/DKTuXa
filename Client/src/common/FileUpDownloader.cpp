#include "FileUpDownloader.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstddef>
#include <stdexcept>

const int kTimeoutMillis = 5000;

void setSockOptions(SOCKET socket) {
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&kTimeoutMillis, sizeof(int));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&kTimeoutMillis, sizeof(int)); 
}

bool recvall(SOCKET ConnectSocket, char *buf, int len) {
    int received = 0;
    int iResult;
    while (received < len && (iResult = recv(ConnectSocket, buf + received, len - received, 0)) > 0) {
        if (iResult < 0) { 
            std::cout << "DEBUG: recvall error: " <<  WSAGetLastError() << "\n";
            // NOTIFY
            return false;
        }
        received += iResult;
    }
    return true;
}

bool sendall(SOCKET ConnectSocket, const char *buf, int len) {
    int sent = 0;
    int iResult;
    while (sent < len && (iResult = send(ConnectSocket, buf + sent, len - sent, 0)) > 0) {
        if (iResult < 0) { 
            std::cout << "DEBUG: sendall error: " <<  WSAGetLastError() << "\n";
            // NOTIFY
            return false;
        }
        sent += iResult;
    }
    return true;
}

SOCKET createSocket(std::string host, std::string port) {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    int iResult;
    
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return INVALID_SOCKET;
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
        return INVALID_SOCKET;
    }

    // Set socket options
    setSockOptions(ConnectSocket);

    return ConnectSocket;
}

void Downloader::downloadFile(std::string host, std::string port, const std::string &filename) {
    std::ofstream fout("files/" + filename, std::ios::binary);
    if (!fout.good()) {
        std::cout << "DEBUG: Error opening file " << filename << "\n";
        return;
    }

    // Number of bytes
    SOCKET socket = createSocket(host, port);
    if (socket == INVALID_SOCKET) {
        return;
    }

    PacketBuffer packetBuffer(socket, false);
    packetBuffer.getBuffer().resize(40);
    recvall(socket, packetBuffer.getBuffer().data(), 40);

    int headerSize = packetBuffer.readInt();
    packetBuffer.setPacketSize(headerSize);    
    packetBuffer.getBuffer().resize(headerSize);
    recvall(socket, packetBuffer.getBuffer().data() + 40, headerSize - 40);
    std::string recvFilename = packetBuffer.readString();
    int filesize = packetBuffer.readInt();
    std::string checksum = packetBuffer.readString();

    // Download file
    int received = 0;
    int iResult;
    std::string buf;
    buf.resize(kBufferSize);   
    while (received < filesize) {
        int nextReceive = kBufferSize;
        if (nextReceive + received > filesize) {
            nextReceive = filesize - received;
        }

        // Receive buffer
        recvall(socket, buf.data(), nextReceive);
        fout.write(buf.c_str(), nextReceive);
        received += nextReceive;
    }
    fout.flush();
    fout.close();

    // Close socket
    iResult = shutdown(socket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(socket);
        WSACleanup();
        return;
    }
    closesocket(socket);
}

void Downloader::joinThread() {

}

void Uploader::uploadFile(SOCKET socket, const std::string &filename) {
    std::ifstream fin("files/" + filename, std::ios::binary);
    if (!fin.is_open()) {
        std::cout << "DEBUG: Error opening file " << filename << "\n";
        return;
    }

    fin.seekg(0, std::ios::end);
    int fileSize = (int)fin.tellg();
    fin.seekg(0, std::ios::beg);

    std::string checksum = "12345678901234567890123456789012"; // TODO`
    
    // Upload file
    int headerSize = 4 + 4 + filename.size() + 4 + 4 + 32;
    PacketBuffer packetBuffer(socket, false);
    packetBuffer.setPacketSize(headerSize);
    packetBuffer.writeInt(headerSize);
    packetBuffer.writeString(filename);
    packetBuffer.writeInt(fileSize);
    packetBuffer.writeString(checksum);

    packetBuffer.flush();
    
    std::string buf;
    buf.resize(kBufferSize);

    int iResult;
    int totalRead = 0;
    while (totalRead < fileSize) {
        int nextSend = kBufferSize;
        if (fileSize - totalRead > kBufferSize) {
            nextSend = fileSize - totalRead;      
            totalRead = kBufferSize;
        } else {
            totalRead += kBufferSize;     
        }

        // Send buffer
        fin.read(buf.data(), nextSend);
        if (!fin.good()) {
            throw std::runtime_error("Error reading file");
        }
        sendall(socket, buf.c_str(), nextSend);  
    }

    fin.close();

    // Close socket
    iResult = shutdown(socket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(socket);
        WSACleanup();
        return;
    }
    closesocket(socket);
}

void Uploader::joinThread() {

}