#ifndef SCREENSHOT_H_
#define SCREENSHOT_H_

#include <windows.h>
#include <iostream>
#include <filesystem>
#include <string>
#include "Server.h"

class Screenshot
{
    private:
        
    public:
        void screenshot(const std::string filename);
};

#endif // SCREENSHOT_H_