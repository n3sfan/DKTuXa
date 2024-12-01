#ifndef BROADCAST_IP_H_
#define BROADCAST_IP_H_

#include <iostream>
#include <string>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <sstream>
#include <vector>

class LAN{
    private:
        std::string ip, broadcastIP, subnetMask;
    public:
        std::string getip(){
            return this->ip;
        }
        std::string getbroadcastIP(){
            return this->broadcastIP;
        }
        std::string getsubnetMask(){
            return this->subnetMask;
        }
        void setip(std::string IpAddress){
            this->ip = IpAddress;
        }
        void setbroadcastIP(std::string BroadcastIP){
            this->broadcastIP = BroadcastIP;
        }
        void setsubnetMask(std::string Subnetmask){
            this->subnetMask = Subnetmask;
        }
        bool getWiFiIPAndSubnet();
        void calculateBroadcastIP();
};

#endif