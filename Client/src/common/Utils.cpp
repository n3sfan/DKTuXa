#include "Utils.h"

#include <algorithm>
#include <cctype>

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return tolower(c); });
    return s;
}

vector<string> split(string s, string delim) {
    vector<string> res;
    int pos = 0, nxt = s.size();
    while ((nxt = s.find(delim, pos)) != s.npos) {
        res.push_back(s.substr(pos, nxt - pos)); 
        pos = nxt + delim.size();
    }
    res.push_back(s.substr(pos, nxt - pos));
    return res; 
}

string trim(string s) {
    int i = 0;
    while (i < s.size() && s[i] == ' ')
        ++i;
    int j = (int)s.size() - 1;
    while (j >= 0 && s[j] == ' ')
        --j;
    
    if (i > j) 
        return "";
    return s.substr(i, j - i + 1);
}