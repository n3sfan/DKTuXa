#include "File.h"

std::string File::getFile(const std::string& filePath) {
    // Đường dẫn tới thư mục đích trong dự án
    const std::string targetDirectory = "../build/files/";
    
    // Lấy tên file từ đường dẫn nguồn
    std::string fileName = filePath.substr(filePath.find_last_of("/\\") + 1);

    // Tạo đường dẫn đích với tên file
    std::string targetFilePath = targetDirectory + fileName;

    // Chuyển đổi std::string sang std::wstring (Windows API yêu cầu wide string)
    std::wstring sourceFilePathW(filePath.begin(), filePath.end());
    std::wstring targetFilePathW(targetFilePath.begin(), targetFilePath.end());

    // Kiểm tra file nguồn có tồn tại hay không
    if (GetFileAttributesW(sourceFilePathW.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::cerr << "Error: File does not exist at the specified source path: " << filePath << std::endl;
        return "false";
    }

    // Kiểm tra và tạo thư mục đích nếu chưa tồn tại
    CreateDirectoryW(L"../build", NULL); // Tạo thư mục build nếu chưa có
    CreateDirectoryW(L"../build/files", NULL); // Tạo thư mục files nếu chưa có

    // Sao chép file từ nguồn tới đích
    if (!CopyFileW(sourceFilePathW.c_str(), targetFilePathW.c_str(), FALSE)) {
        DWORD error = GetLastError();
        std::cerr << "Error copying file. Code: " << error << std::endl;
        return "false";
    }

    std::cout << "File copied successfully to: " << targetFilePath << std::endl;
    return fileName;
}


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

        // Chuyển đổi tên từ wchar_t* sang std::string
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::string itemName = converter.to_bytes(findFileData.cFileName);

        // Kiểm tra xem mục này có phải là thư mục hay không
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            fileList += itemName + " [Dir]\n"; // Nếu là thư mục, thêm chú thích "[Dir]"
        } else {
            fileList += itemName + "\n"; // Nếu là tệp, chỉ thêm tên
        }

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



// Client
// {
//     "RequestId": "12345",
//     "Resume": "true"
// }
