#include "Broadcast.h"

// Hàm lấy tên máy tính
std::string getPCName() {
    char computerName[256];
    DWORD size = sizeof(computerName);
    if (GetComputerNameA(computerName, &size)) {
        return std::string(computerName);
    } else {
        return "Unknown PC";
    }
}

// Hàm lấy địa chỉ IP của máy tính
std::string getIPAddress() {
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
                    free(pAdapterInfo);
                    return ipAddress;
                }
            }
            pAdapter = pAdapter->Next;
        }
    }

    // Giải phóng bộ nhớ nếu không tìm thấy adapter WiFi
    if (pAdapterInfo)
        free(pAdapterInfo);

    return "Unknown_IP";
}

// Hàm tổng hợp
std::string getPCInfo() {
    std::string pcName = getPCName();
    std::string ip = getIPAddress();
    return pcName + "-" + ip;
}
