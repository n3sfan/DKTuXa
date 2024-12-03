#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <vector>

std::string toLower(std::string s);
bool startsWith(std::string s, std::string prefix);
/**
 * Allow empty std::std::strings in the result
 */
std::vector<std::string> split(std::string s, std::string delim);
std::string trim(std::string s);
#endif