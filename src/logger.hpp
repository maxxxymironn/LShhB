#pragma once

class Logger {
private:
    Logger() = default;

public:
    static Logger* getInstance() {
        static Logger instance;
        return &instance;
    }

    Logger(const Logger& other) = delete;
    Logger& operator=(const Logger& other) = delete;

    void printError(const char* str);
    void printInfo(const char* str);

    void printHelpInfo();
};