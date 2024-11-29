#include "File.h"

std::string File::getFiles(const std::string& directoryPath) {
    std::wstring wideDirectoryPath(directoryPath.begin(), directoryPath.end());
    if (wideDirectoryPath.back() != L'\\') {
        wideDirectoryPath += L'\\'; // Đảm bảo đường dẫn kết thúc bằng dấu '\'
    }

    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW((wideDirectoryPath + L"*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Cannot open directory: " << directoryPath << "\nError: " << GetLastError() << std::endl;
        return ""; // Trả về chuỗi rỗng nếu không thể mở thư mục
    }

    std::string fileList;
    do {
        // Bỏ qua các mục "." và ".."
        if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0) {
            continue;
        }

        // Chuyển đổi tên file từ wchar_t* sang std::string
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::string fileName = converter.to_bytes(findFileData.cFileName);

        // Thêm tên file vào danh sách
        fileList += fileName + "\n";

    } while (FindNextFileW(hFind, &findFileData) != 0);

    DWORD error = GetLastError();
    if (error != ERROR_NO_MORE_FILES) {
        std::cerr << "Error occurred during directory iteration. Error: " << error << std::endl;
    }

    FindClose(hFind);
    return fileList;
}


void File::deleteFile(const std::string& filePath) {
    // Chuyển đổi từ std::string sang std::wstring
    std::wstring wideFilePath(filePath.begin(), filePath.end());

    // Kiểm tra xem tệp có tồn tại không trước khi xóa
    DWORD fileAttributes = GetFileAttributesW(wideFilePath.c_str());
    if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
        std::wcerr << L"Error: File does not exist or cannot access: " << wideFilePath << L"\n";
        return;
    }

    // Kiểm tra xem đó có phải là tệp không
    if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        std::wcerr << L"Error: The path is a directory, not a file: " << wideFilePath << L"\n";
        return;
    }

    // Thử xóa tệp
    if (DeleteFileW(wideFilePath.c_str())) {
        std::wcout << L"File successfully deleted: " << wideFilePath << std::endl;
    } else {
        DWORD error = GetLastError();
        std::wcerr << L"Cannot delete file: " << wideFilePath << L"\nError code: " << error << std::endl;

        // Hiển thị lỗi cụ thể hơn nếu cần
        switch (error) {
            case ERROR_ACCESS_DENIED:
                std::wcerr << L"Error: Access denied. Check file permissions or if the file is in use.\n";
                break;
            case ERROR_FILE_NOT_FOUND:
                std::wcerr << L"Error: File not found.\n";
                break;
            case ERROR_SHARING_VIOLATION:
                std::wcerr << L"Error: The file is being used by another process.\n";
                break;
            default:
                std::wcerr << L"Error: Unknown error occurred.\n";
                break;
        }
    }
}


// // Tính checksum MD5
// string File::calculateMD5(const string& filePath) {
//     ifstream file(filePath, ios::binary);
//     if (!file) {
//         throw runtime_error("Cannot open file for MD5 calculation.");
//     }

//     MD5_CTX md5Context;
//     MD5_Init(&md5Context);

//     char buffer[1024];
//     while (file.read(buffer, sizeof(buffer))) {
//         MD5_Update(&md5Context, buffer, file.gcount());
//     }
//     MD5_Update(&md5Context, buffer, file.gcount()); // For last remaining bytes

//     unsigned char md5Result[MD5_DIGEST_LENGTH];
//     MD5_Final(md5Result, &md5Context);

//     ostringstream oss;
//     for (unsigned char c : md5Result) {
//         oss << hex << setw(2) << setfill('0') << (int)c;
//     }
//     return oss.str();
// }

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
