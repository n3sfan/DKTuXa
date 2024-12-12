#include "AccountTable.h"

void processAndStore(const std::string& PCname_ip, std::map<std::string, std::string>& account_table) {
    // Tìm vị trí của dấu '-' để tách PC name và IP
    size_t delimiter_pos = PCname_ip.find_last_of('-');
    if (delimiter_pos == std::string::npos) {
        // Nếu không tìm thấy dấu '-', thông báo lỗi và thoát
        std::cerr << "Invalid format. Expected 'PC name-ip' format." << std::endl;
        return;
    }

    // Tách chuỗi thành PC name và IP
    std::string PCname = PCname_ip.substr(0, delimiter_pos);
    std::string ip = PCname_ip.substr(delimiter_pos + 1);
    if (PCname.empty() || ip.empty()) {
        std::cerr << "Invalid format. PCname or IP cannot be empty." << std::endl;
        return;
    }
    // Lưu dữ liệu vào map
    account_table[PCname] = ip;
}

bool checkPCname(const std::string& PCname, const std::map<std::string, std::string>& account_table) {
    // Sử dụng hàm find để kiểm tra xem PCname có trong map không
    auto it = account_table.find(PCname);
    if (it != account_table.end()) {
        return true;
    } 
    else {
        return false;
    }
}
