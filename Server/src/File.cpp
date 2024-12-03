#include "File.h"

bool File::getFile(const std::string& filePath) {
    const std::string targetDirectory = "../build/files/";
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    // Chuyển đổi đường dẫn
    std::wstring sourceFilePathW, targetFilePathW;
    try {
        sourceFilePathW = converter.from_bytes(filePath);
        std::string fileName = filePath.substr(filePath.find_last_of("/\\") + 1);
        targetFilePathW = converter.from_bytes(targetDirectory + fileName);
    } catch (const std::exception& e) {
        std::cerr << "Invalid UTF-8 sequence: " << e.what() << std::endl;
        return false;
    }

    // Kiểm tra file nguồn
    if (GetFileAttributesW(sourceFilePathW.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::cerr << "Error: File does not exist at " << filePath << std::endl;
        return false;
    }

    // Tạo thư mục đích
    CreateDirectoryW(L"../build", NULL);
    CreateDirectoryW(L"../build/files", NULL);

    // Sao chép file
    if (!CopyFileW(sourceFilePathW.c_str(), targetFilePathW.c_str(), FALSE)) {
        std::cerr << "Error copying file. Code: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}

// Hàm hỗ trợ lấy dung lượng trống còn lại trên ổ đĩa
LONGLONG GetFreeDiskSpace(const std::string& directoryPath) {
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    std::wstring wideDirectoryPath(directoryPath.begin(), directoryPath.end());
    if (GetDiskFreeSpaceExW(wideDirectoryPath.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        return totalNumberOfFreeBytes.QuadPart;
    } else {
        return 0;
    }
}

std::string File::getFiles(const std::string& directoryPath) {
    // Chuyển đổi đường dẫn từ std::string sang std::wstring
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

    // Khởi tạo chuỗi kết quả
    std::ostringstream result;
    LARGE_INTEGER totalFileSize = {0}; // Tổng kích thước file
    int fileCount = 0, dirCount = 0;
    result << "\n";

    do {
        // Bỏ qua các mục "." và ".."
        if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0) {
            continue;
        }

        // Lấy thời gian chỉnh sửa file
        FILETIME ft = findFileData.ftLastWriteTime;
        SYSTEMTIME stUTC, stLocal;
        FileTimeToSystemTime(&ft, &stUTC);
        SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

        // Định dạng thời gian
        result << std::setw(2) << std::setfill('0') << stLocal.wDay << "/"
               << std::setw(2) << std::setfill('0') << stLocal.wMonth << "/"
               << stLocal.wYear << "  "
               << std::setw(2) << std::setfill('0') << stLocal.wHour << ":"
               << std::setw(2) << std::setfill('0') << stLocal.wMinute << "    ";

        // Kiểm tra xem là thư mục hay file
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            result << "<DIR>          ";
            dirCount++;
        } else {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findFileData.nFileSizeLow;
            fileSize.HighPart = findFileData.nFileSizeHigh;

            totalFileSize.QuadPart += fileSize.QuadPart;
            result << std::setw(15) << std::setfill(' ') << std::right << fileSize.QuadPart << " ";
            fileCount++;
        }

        // Chuyển đổi tên từ wchar_t* sang std::string
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::string itemName = converter.to_bytes(findFileData.cFileName);

        // Thêm tên file hoặc thư mục vào kết quả
        result << itemName << "\n";

    } while (FindNextFileW(hFind, &findFileData) != 0);

    DWORD error = GetLastError();
    if (error != ERROR_NO_MORE_FILES) {
        std::cerr << "Error occurred during directory iteration. Error: " << error << std::endl;
    }

    FindClose(hFind);

    // Thêm tổng kết
    result << "               " << fileCount << " File(s)      " << totalFileSize.QuadPart << " bytes\n";
    result << "               " << dirCount << " Dir(s)       " << GetFreeDiskSpace(directoryPath) << " bytes free\n";

    return result.str();
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



// Client
// {
//     "RequestId": "12345",
//     "Resume": "true"
// }
