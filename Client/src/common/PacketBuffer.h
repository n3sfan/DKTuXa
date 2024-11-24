#ifndef SOCKET_BUFFER_H
#define SOCKET_BUFFER_H

#include <string>

#include <winsock2.h>

/**
 * Helper for reading/writing packet
 * Only read or write at one time.
 */
class PacketBuffer {
    private:
        /**
         * Connection socket
         */
        SOCKET socket;
        /**
         * Headers
         */
        int packetSize;

        bool readMode;
        int packetPos;

        int kBufferSize = 8192;
        std::string buffer = "";

        bool checkPos(int len);
    public:
        PacketBuffer(SOCKET socket, bool readMode);
        int getPacketSize();
        void setPacketSize(int packetSize);
        int getPacketPos();
        std::string& getBuffer();

        void flush();
        bool read(char *buf, int len);
        bool write(const char *buf, int len);
        int readInt();
        void writeInt(int val);
        std::string readString();
        void writeString(const std::string &str);
};

#endif // SOCKET_BUFFER_H