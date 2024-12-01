#include "broadcastIP.h"

bool LAN::getWiFiIPAndSubnet() {
    std::string ipAddress;
    std::string subnetMask;
    ULONG outBufLen = sizeof(IP_ADAPTER_INFO);
    IP_ADAPTER_INFO* pAdapterInfo = (IP_ADAPTER_INFO*)malloc(outBufLen);

    // Lấy thông tin adapter
    if (GetAdaptersInfo(pAdapterInfo, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(outBufLen);
    }

    if (GetAdaptersInfo(pAdapterInfo, &outBufLen) == NO_ERROR) {
        IP_ADAPTER_INFO* pAdapter = pAdapterInfo;

        // Duyệt qua từng adapter
        while (pAdapter) {
            // Kiểm tra adapter thuộc loại WiFi
            if (pAdapter->Type == IF_TYPE_IEEE80211) { // Chỉ lấy adapter WiFi
                // Kiểm tra địa chỉ IP hợp lệ (không phải 0.0.0.0)
                if (pAdapter->IpAddressList.IpAddress.String[0] != '0') {
                    ipAddress = pAdapter->IpAddressList.IpAddress.String;
                    subnetMask = pAdapter->IpAddressList.IpMask.String;
                    setip(ipAddress);
                    setsubnetMask(subnetMask);
                    free(pAdapterInfo);
                    return true;
                }
            }
            pAdapter = pAdapter->Next;
        }
    }

    // Giải phóng bộ nhớ nếu không tìm thấy adapter WiFi
    if (pAdapterInfo)
        free(pAdapterInfo);

    return false;
}

std::vector<int> splitIP(const std::string& ip) {
    std::vector<int> parts;
    std::stringstream ss(ip);
    std::string segment;

    while (std::getline(ss, segment, '.')) {
        parts.push_back(std::stoi(segment)); // Chuyển từng phần tử từ string sang int
    }
    return parts;
}

// Hàm tính toán Broadcast IP
void LAN::calculateBroadcastIP() {
    const std::string ip = this->getip();
    const std::string subnetMask = this->getsubnetMask(); // Lấy giá trị subnet mask từ lớp

    // Tách IP và Subnet mask thành các phần tử số nguyên
    std::vector<int> ipParts = splitIP(ip);
    std::vector<int> subnetParts = splitIP(subnetMask);

    // Kiểm tra độ dài hợp lệ (cả IP và subnet mask đều phải có 4 phần)
    if (ipParts.size() != 4 || subnetParts.size() != 4) {
        std::cout << "Error: IP address and subnet mask are invalid!\n";
        return;
    }

    // Tính broadcast IP thủ công
    std::vector<int> broadcastParts(4);
    for (int i = 0; i < 4; ++i) {
        // Broadcast = (IP AND SubnetMask) | (~SubnetMask)
        broadcastParts[i] = (ipParts[i] & subnetParts[i]) | (~subnetParts[i] & 0xFF);
    }

    // Ghép các phần tử lại thành chuỗi địa chỉ IP
    std::string broadcastIP;
    for (int i = 0; i < 4; ++i) {
        broadcastIP += std::to_string(broadcastParts[i]);
        if (i < 3) broadcastIP += ".";
    }

    setbroadcastIP(broadcastIP); // Lưu giá trị Broadcast IP vào đối tượng
}


// int main(){
//     LAN a;
//     bool re = a.getWiFiIPAndSubnet();
//     a.calculateBroadcastIP();
//     std::string id = a.getip();
//     std::string subnet = a.getsubnetMask();
//     std::string broadcast = a.getbroadcastIP();
//     std::cout << id << std::endl;
//     std::cout << subnet << std::endl;
//     std::cout << broadcast << std::endl;
//     return 0;
// }