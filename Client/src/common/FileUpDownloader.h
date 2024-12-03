#ifndef DOWNLOADER_H_
#define DOWNLOADER_H_

#include <string>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "PacketBuffer.h"

void setSockOptions(SOCKET socket);

bool recvall(SOCKET ConnectSocket, char *buf, int len);
bool sendall(SOCKET ConnectSocket, const char *buf, int len);

/**
 * Parallel downloading, multiple files at the same time.
 * Checksum.
 * Buffering each chunk.
 */
class Downloader {
    const int kBufferSize = 8192;
    public:
        void downloadFile(std::string host, std::string port, const std::string &filename);
        void joinThread();
};

class Uploader {
    const int kBufferSize = 8192;
    public:
        void uploadFile(SOCKET socket, const std::string &filename);
        void joinThread();
};

#endif // DOWNLOADER_H_