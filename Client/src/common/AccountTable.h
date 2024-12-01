#include <map>
#include <iostream>
#include <string>

// std::map<std::string, std::string> account_table;

void processAndStore(const std::string& PCname_ip, std::map<std::string, std::string>& account_table);
bool checkPCname(const std::string& PCname, const std::map<std::string, std::string>& account_table);
