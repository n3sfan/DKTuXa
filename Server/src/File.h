#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <fstream>
#include <string>
#include <locale>
#include <codecvt>
#include <unordered_map>

#include <windows.h>



using namespace std;

class File{
    public:
        string getFile(const std::string& filePath);
        string getFiles(const std::string& directoryPath);
        void deleteFile(const string& filePath);
};