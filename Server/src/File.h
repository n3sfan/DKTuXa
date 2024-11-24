#include <iostream>
#include <windows.h>

class File{
    public:
        std::string readFile(const std::string& filePath);
        void deleteFile(const std::string& filePath);
};