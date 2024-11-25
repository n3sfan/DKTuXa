#include "File.h"

string File::readFile(const string& filePath){
    // Chuyển đổi từ string sang wstring
    wstring wideFilePath(filePath.begin(), filePath.end());

    // Mở tệp để đọc
    HANDLE hFile = CreateFileW(
        wideFilePath.c_str(),         // Đường dẫn tệp
        GENERIC_READ,                 // Quyền truy cập đọc
        0,                            // Không chia sẻ
        NULL,                         // Không có thuộc tính bảo mật
        OPEN_EXISTING,                // Chỉ mở nếu tệp tồn tại
        FILE_ATTRIBUTE_NORMAL,        // Thuộc tính tệp bình thường
        NULL                          // Không có mẫu tệp
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        cerr << "Cannot open file: " << filePath << endl;
        return "";  // Trả về chuỗi rỗng nếu không mở được tệp
    }

    // Đọc dữ liệu từ tệp
    const DWORD bufferSize = 1024;
    char buffer[bufferSize];
    DWORD bytesRead;
    string fileContent;

    while (ReadFile(hFile, buffer, bufferSize - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';  // Đảm bảo buffer có ký tự kết thúc chuỗi
        fileContent += buffer;    // Thêm nội dung đọc được vào chuỗi kết quả
    }

    if (fileContent.empty()) {
        cerr << "Cannot read data from file or file is empty." << endl;
    }

    // Đóng tệp
    CloseHandle(hFile);

    // Trả về nội dung tệp
    return fileContent;
}

void File::deleteFile(const string& filePath){
    // Chuyển đổi từ string sang wstring
    wstring wideFilePath(filePath.begin(), filePath.end());

    // Gọi hàm DeleteFileW để xóa tệp
    if (DeleteFileW(wideFilePath.c_str())) {
        wcout << L"Success delete file: " << wideFilePath << endl;
    } else {
        wcerr << L"Cannot delete file: " << wideFilePath << L"\nError: " << GetLastError() << endl;
    }
}

// Tính checksum MD5
string File::calculateMD5(const string& filePath) {
    ifstream file(filePath, ios::binary);
    if (!file) {
        throw runtime_error("Cannot open file for MD5 calculation.");
    }

    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    char buffer[1024];
    while (file.read(buffer, sizeof(buffer))) {
        MD5_Update(&md5Context, buffer, file.gcount());
    }
    MD5_Update(&md5Context, buffer, file.gcount()); // For last remaining bytes

    unsigned char md5Result[MD5_DIGEST_LENGTH];
    MD5_Final(md5Result, &md5Context);

    ostringstream oss;
    for (unsigned char c : md5Result) {
        oss << hex << setw(2) << setfill('0') << (int)c;
    }
    return oss.str();
}

std::unordered_map<std::string, std::string> tempResponse; // Lưu trạng thái tạm

void File::saveTempResponse(const std::string& requestId, const std::string& responseData) {
    std::ofstream file("../temp/response_" + requestId + ".txt", std::ios::out);
    if (file.is_open()) {
        file << responseData;
        file.close();
    }
}

std::string File::loadTempResponse(const std::string& requestId) {
    std::ifstream file("../temp/response_" + requestId + ".txt", std::ios::in);
    std::string responseData, line;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            responseData += line + "\n";
        }
        file.close();
    }
    return responseData;
}

void File::clearTempResponse(const std::string& requestId) {
    std::remove(("../temp/response_" + requestId + ".txt").c_str());
}

// Client
// {
//     "RequestId": "12345",
//     "Resume": "true"
// }
