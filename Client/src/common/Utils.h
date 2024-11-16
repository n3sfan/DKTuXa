#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <vector>

using namespace std;

string toLower(string s);
bool startsWith(string s, string prefix);
/**
 * Allow empty strings in the result
 */
vector<string> split(string s, string delim);
string trim(string s);
#endif