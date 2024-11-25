#include <iostream>
#include <openssl/md5.h>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <fstream>
#include <string>
#include <unordered_map>

using namespace std;

class File{
    public:
        string readFile(const string& filePath);
        void deleteFile(const string& filePath);
        
        string calculateMD5(const string& filePath);
        void saveTempResponse(const std::string& requestId, const std::string& responseData);
        string loadTempResponse(const std::string& requestId);
        void clearTempResponse(const std::string& requestId);
};