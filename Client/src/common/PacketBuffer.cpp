#include "PacketBuffer.h"

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sstream>

PacketBuffer::PacketBuffer(SOCKET socket, bool readMode): socket(socket), readMode(readMode), packetSize(4), packetPos(0) {
    isUDP = false;

    if (readMode) {
        std::string recvbuf;
        recvbuf.resize(kBufferSize);

        int received = 0;
        int iResult;
        while ((iResult = recv(socket, recvbuf.data(), kBufferSize, 0))) {
            if (iResult < 0) { 
                std::cout << "DEBUG: recvall error: " <<  WSAGetLastError() << "\n";
                // NOTIFY
                throw std::runtime_error("Error in readMode");
                break;
            }
        
            buffer += std::string(recvbuf.c_str(), iResult);
            received += iResult;
        }
        // std::cout << "DEBUG: " << buffer << "\n";
    }
}

PacketBuffer::PacketBuffer(SOCKET socket, bool readMode, sockaddr_in *destAddress): socket(socket), readMode(readMode), packetSize(4), packetPos(0), destAddress(destAddress) {   
    isUDP = true;

    if (readMode) {
        std::cout << "DEBUG: UDP readMode\n";

        std::string recvbuf;
        recvbuf.resize(kBufferSize);
        int destAddressSize = sizeof(*destAddress);

        int received = 0;
        int iResult;
        if ((iResult = recvfrom(socket, recvbuf.data(), kBufferSize, 0, (sockaddr*) destAddress, &destAddressSize))) {
            if (iResult < 0) { 
                std::cout << "DEBUG: UDP recvall error: " <<  WSAGetLastError() << "\n";
                // NOTIFY
                throw std::runtime_error("UDP Error in readMode");
            }
            buffer += std::string(recvbuf.c_str(), iResult);
            received += iResult;
        }

        std::cout << "DEBUG: received " << received << " bytes. UDP: " << isUDP << ". Buffer: " << buffer.c_str() << "\n";
        if (iResult < 0) std::cout << WSAGetLastError() << " error\n";
    }
}

bool PacketBuffer::checkPos(int len) {
    if (packetPos + len > packetSize) {
        std::stringstream ss;
        ss << "PacketBuffer: Data exceeded size of packet! " << packetPos << " " << len << " " << packetSize;
        throw std::invalid_argument(ss.str());
    }
    packetPos += len;
    return true;
}

int PacketBuffer::getPacketSize() {
    return packetSize;
}

void PacketBuffer::setPacketSize(int packetSize) {
    this->packetSize = packetSize;
}

int PacketBuffer::getPacketPos() {
    return packetPos;
}

std::string &PacketBuffer::getBuffer() {
    return buffer;
}

void PacketBuffer::flush() { 
    if (isUDP) {
        int sent = 0;
        int iResult;
        int destAddressSize = sizeof(*destAddress);
        int n = 4;
        while (n--){
            iResult = sendto(socket, buffer.data() + sent, buffer.size() - sent, 0, (sockaddr*)destAddress, destAddressSize);

            if (iResult < 0) { 
                std::cout << "DEBUG: sendall error: " <<  WSAGetLastError() << "\n";
            } else {
                sent += iResult;
            }
            // sleep(1);
        }
    } else {
        int sent = 0;
        int iResult;
        do {
            iResult = send(socket, buffer.data() + sent, buffer.size() - sent, 0);

            if (iResult < 0) { 
                std::cout << "DEBUG: sendall error: " <<  WSAGetLastError() << "\n";
            } else {
                sent += iResult;
            }
        } while (sent < buffer.size());
    }
}

bool PacketBuffer::read(char *buf, int len) {
    checkPos(len);
    std::memcpy(buf, buffer.data() + packetPos - len, len);
    // int received = 0;
    // int iResult;
    // while (received < len || (iResult = recv(socket, buf + received, len - received, 0)) > 0) {
    //     if (iResult < 0) { 
    //         std::cout << "DEBUG: recvall error: " <<  WSAGetLastError() << "\n";
    //         // NOTIFY
    //         return false;
    //     }
    //     received += iResult;
    // }
    return true;
}

bool PacketBuffer::write(const char *buf, int len) {
    // std::cout << buf[0] << " buf[0]\n";
    checkPos(len);
    buffer += std::string(buf, len);
    
    // int next = 0;
    // while (next < len) {
    //     if (buffer.size() + len <= kBufferSize) {
    //         next = len;
    //         buffer += std::string(buf, len);      
    //     } else {
    //         next = kBufferSize - buffer.size();
    //         buffer += std::string(buf, next);
    //     }

    //     if (buffer.size() < kBufferSize)
    //         break;

    //     // Send buffer
    //     int sent = 0;
    //     int iResult;
    //     while (sent < kBufferSize && (iResult = send(socket, buf + sent, kBufferSize - sent, 0)) > 0) {
    //         if (iResult < 0) { 
    //             std::cout << "DEBUG: sendall error: " <<  WSAGetLastError() << "\n";
    //             // NOTIFY
    //             return false;
    //         }
    //         sent += iResult;
    //     }
    // }
    return true;
}

int PacketBuffer::readInt() {
    int res = 0;
    read((char*)&res, 4);
    return res;
}

void PacketBuffer::writeInt(int val) {
    write((char*)&val, 4);
}

std::string PacketBuffer::readString() {
    int len = readInt();
    std::string res;
    res.resize(len);
    read(res.data(), len);
    return res;
}

void PacketBuffer::writeString(const std::string &str) {
    writeInt(str.size());
    write(str.c_str(), str.size());
}