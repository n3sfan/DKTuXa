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
