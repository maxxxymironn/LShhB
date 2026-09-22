#include "imageHandler.hpp"
#include "logger.hpp"
#include "enums.hpp"

#include <string_view>
#include <filesystem>
#include <string>
#include <format>
#include <iostream>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

struct InputData {
    std::string_view sourcePath;
    std::string_view imagePath;
    std::string_view outputPath;

    DataType sourceDataType;
    Action action;
    bool has4Channels;
    
    std::string secret;
    bool isAdvancedMode;
};

void validateData(char* argv[], const int argc, InputData& data, std::string& errorMsg);

void setEcho(const bool enable);

void sigIntHandler(int signum);

int main(int argc, char* argv[]) {
    std::string errorMsg;
    InputData data;
    
    validateData(argv, argc, data, errorMsg);
    
    if (errorMsg == "help") {
        Logger::getInstance()->printHelpInfo();
        return 0;
    } 
    else if (!errorMsg.empty()) {
        Logger::getInstance()->printError(errorMsg.c_str());
        return 1;
    }

    ImageHandler imgHandler(
        data.action == Action::HIDE ? data.imagePath : data.sourcePath,
        data.secret
    );
    if (!imgHandler.getStatus())
        return 1;

    if (data.action == Action::HIDE) {
        if (!imgHandler.hide(data.sourcePath, data.sourceDataType, data.has4Channels))
            return 1;

        bool isCustomPath = !data.outputPath.empty();
        std::string_view savePath = isCustomPath ? data.outputPath : data.sourcePath;
        if (!imgHandler.saveResult(isCustomPath, savePath, data.sourceDataType, data.action))
            return 1;
    }
    else {
        if (!imgHandler.read(data.sourceDataType))
            return 1;

        if (data.sourceDataType != DataType::STR) {
            bool isCustomPath = !data.outputPath.empty();
            std::string_view savePath = isCustomPath ? data.outputPath : data.sourcePath;
            if (!imgHandler.saveResult(isCustomPath, savePath, data.sourceDataType, data.action))
                return 1;
        }
    }

    return 0;
}

void validateData(char* argv[], const int argc, InputData& data, std::string& errMsg) {    
    const std::string_view mode = 1 < argc ? argv[1] : "";
    const std::string_view act = 2 < argc ? argv[2] : "";
    const std::string_view dType = 3 < argc ? argv[3] : "";
    const std::string_view src = 4 < argc ? argv[4] : "";
    const std::string_view img = 5 < argc ? argv[5] : "";
    const std::string_view out = 6 < argc ? argv[6] : "";

    if (argc > 6 || act == "read" && argc > 5) {
        errMsg = std::format(
            "Got {} args.\nExpected up to 5 args for ACTION=read and up to 6 args for ACTION=hide.",
            argc
        );
        return;
    }

    if (argc == 1 || mode == "help" || mode == "--help" || mode == "-h") {
        errMsg = "help";
        return;
    }
    if (mode == "simple")
        data.isAdvancedMode = false;
    else if (mode == "advanced")
        data.isAdvancedMode = true;
    else {
        errMsg = std::format("Unknown MODE={}", mode);
        return;
    }

    if (act == "read")
        data.action = Action::READ;
    else if (act == "hide") 
        data.action = Action::HIDE;
    else {
        errMsg = std::format("Unknown ACTION={}", act);
        return;
    }

    if (dType == "str")
        data.sourceDataType = DataType::STR;
    else if (dType == "file")
        data.sourceDataType = DataType::FILE;
    else if (dType == "image")
        data.sourceDataType = DataType::IMAGE;
    else {
        errMsg = std::format("Unknown DATA_TYPE={}", dType);
        return;
    }

    std::filesystem::path filePath;
    // check source
    if (data.sourceDataType != DataType::STR || data.action != Action::HIDE) {
        if (!std::filesystem::exists(src)) {
            errMsg = std::format("Source with SOURCE_PATH='{}' not found", src);
            return;
        }

        if (std::filesystem::is_directory(src)) {
            errMsg = std::format("Source with SOURCE_PATH='{}' is not file", src);
            return;
        }

        if (data.sourceDataType == DataType::IMAGE) {
            filePath = src;

            if (!filePath.has_extension()) {
                errMsg = std::format(
                    "With DATA_TYPE={} source file with SOURCE_PATH='{}'"
                    "has not extension.\nSupports files with .png/.jpeg/.jpg",
                    dType, filePath.c_str()
                );
                return;
            }

            std::string_view ext = filePath.extension().c_str();
            if (data.action == Action::READ) {
                if (ext != ".png") {
                    errMsg = std::format(
                        "Source file with IMAGE_PATH='{}'"
                        "has unvailable extension.\nSupports only .png",
                        filePath.c_str()
                    );
                    return;
                }
            }
            else if (ext != ".png" && ext != ".jpeg" && ext != ".jpg") {
                errMsg = std::format(
                    "With DATA_TYPE={} source file with SOURCE_PATH='{}'"
                    "has unvailable extension.\nSupports files with .png/.jpeg/.jpg",
                    dType, filePath.c_str()
                );
                return;
            }

            data.has4Channels = ext == ".png";
        }
    }
    else if (src == "") {
        errMsg = "Source cannot be empty with DATA_TYPE=str.";
        return;
    }

    data.sourcePath = src;

    // check image
    if (data.action == Action::HIDE) {
        if (!std::filesystem::exists(img)) {
            errMsg = std::format("Image with IMAGE_PATH='{}' not found", img);
            return;
        }
        if (std::filesystem::is_directory(img)) {
            errMsg = std::format("Image with IMAGE_PATH='{}' is not file", src);
            return;
        }

        filePath = img;
        if (!filePath.has_extension()) {
            errMsg = std::format(
                "Image with IMAGE_PATH='{}'"
                "has not extension.\nSupports only .png",
                filePath.c_str()
            );
            return;
        }

        std::string_view ext = filePath.extension().c_str();
        if (ext != ".png") {
            errMsg = std::format(
                "Image with IMAGE_PATH='{}'"
                "has unvailable extension.\nSupports only .png",
                filePath.c_str()
            );
            return;
        }

        data.imagePath = img;
    }

    // check output file
    if (out != "") {
        filePath = out;
        if (std::filesystem::is_directory(filePath)) {
            errMsg = std::format("OUTPUT_PATH='{}' cannot be a directory.", out);
            return;
        }
        if (std::filesystem::exists(filePath)) {
            errMsg = std::format("File with OUTPUT_PATH='{}' already exists.", out);
            return;
        }

        if (filePath.has_parent_path() && !std::filesystem::exists(filePath.parent_path())) {
            errMsg = std::format("Directory of file with OUTPUT_PATH='{}' not exists", out);
            return;
        }
        
        data.outputPath = out;
    }

    if (data.isAdvancedMode) {
        std::signal(SIGINT, sigIntHandler);
        std::cout << "Enter password (seed): ";
    
        setEcho(false);
        getline(std::cin, data.secret);
        setEcho(true);
        std::cout << "\n";

        if (data.secret.empty()) {
            errMsg = "Password cannot be empty";
            return;
        }
        if (data.secret.size() < 8) {
            errMsg = "Password cannot be < 8";
            return;
        }
    }
}

#ifdef _WIN32
    void setEcho(const bool enable) {
        HANDLE stdInHandler = GetStdHandle(STD_INPUT_HANDLE); 
        DWORD mode;
        GetConsoleMode(stdInHandler, &mode);

        if (!enable)
            mode &= ~ENABLE_ECHO_INPUT;
        else
            mode |= ENABLE_ECHO_INPUT;

        SetConsoleMode(stdInHandler, mode);
    }
#else
    void setEcho(const bool enable) {
        struct termios tty;
        tcgetattr(STDIN_FILENO, &tty);

        if (enable)
            tty.c_lflag |= ECHO;
        else
            tty.c_lflag &= ~ECHO;

        tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    }    
#endif

void sigIntHandler(int signum) {
    setEcho(true);
    std::cout << "\n";
    exit(signum);
}