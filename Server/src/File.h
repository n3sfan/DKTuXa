#include <windows.h>
#include <iostream>

using namespace std;

class File{
    public:
        string readFile(const string& filePath);
        void deleteFile(const string& filePath);
};