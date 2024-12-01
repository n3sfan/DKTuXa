#include "Utils.h"

#include <algorithm>
#include <cctype>

using namespace std;

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return tolower(c); });
    return s;
}

bool startsWith(string s, string prefix) {
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

vector<string> split(string s, string delim) {
    vector<string> res;
    int pos = 0, nxt = s.size();
    while ((nxt = s.find(delim, pos)) != s.npos) {
        res.push_back(s.substr(pos, nxt - pos)); 
        pos = nxt + delim.size();
    }
    res.push_back(s.substr(pos));
    return res; 
}

string trim(string s) {
    int i = 0;
    while (i < s.size() && isspace(s[i]))
        ++i;
    int j = (int)s.size() - 1;
    while (j >= 0 && isspace(s[j]))
        --j;
    
    if (i > j) 
        return "";
    return s.substr(i, j - i + 1);
}