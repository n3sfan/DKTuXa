#include "Screenshot.h"

bool SaveBMPFile(const std::string filename, HBITMAP hBitmap, HDC hDC, int width, int height)
{
    BITMAPFILEHEADER bmpFileHeader;
    BITMAPINFOHEADER bmpInfoHeader;
    BITMAP bmp;
    DWORD dwBmpSize;
    DWORD dwBytesWritten;
    HANDLE hFile;
    char* lpBitmapData;

    // Lấy thông tin bitmap
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    // Tính kích thước ảnh
    dwBmpSize = ((bmp.bmWidth * bmp.bmBitsPixel + 31) / 32) * 4 * bmp.bmHeight;

    // Dữ liệu bitmap
    lpBitmapData = new char[dwBmpSize];
    ZeroMemory(lpBitmapData, dwBmpSize);

    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(BITMAPINFO));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height; // Hình ảnh lật ngược (âm để hiển thị đúng)
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24; // 24 bit màu
    bi.bmiHeader.biCompression = BI_RGB;

    // Lấy dữ liệu pixel từ bitmap
    GetDIBits(hDC, hBitmap, 0, height, lpBitmapData, &bi, DIB_RGB_COLORS);

    // Tạo file BMP
    hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        delete[] lpBitmapData;
        return false;
    }

    // Ghi header file
    bmpFileHeader.bfType = 0x4D42; // "BM"
    bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
    bmpFileHeader.bfReserved1 = 0;
    bmpFileHeader.bfReserved2 = 0;
    bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    WriteFile(hFile, &bmpFileHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);

    // Ghi header thông tin
    bmpInfoHeader = bi.bmiHeader;
    WriteFile(hFile, &bmpInfoHeader, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);

    // Ghi dữ liệu pixel
    WriteFile(hFile, lpBitmapData, dwBmpSize, &dwBytesWritten, NULL);

    // Đóng file
    CloseHandle(hFile);
    delete[] lpBitmapData;

    return true;
}

void CaptureScreen(const std::string filename)
{
    // Lấy thông tin màn hình
    HDC hScreenDC = GetDC(NULL); // DC màn hình chính
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Tạo bitmap tương thích
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    // Chụp màn hình
    BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);

    // Lưu bitmap thành file BMP
    if (SaveBMPFile(filename, hBitmap, hMemoryDC, screenWidth, screenHeight)) {
        std::cout << "Screenshot saved to " << filename << std::endl;
    } else {
        std::cerr << "Failed to save screenshot!" << std::endl;
    }

    // Dọn dẹp
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
}

void Screenshot::screenshot(const std::string filename){
    CaptureScreen(filename);
}
