#include "imageHandler.hpp"
#include "logger.hpp"
#include "enums.hpp"

#include <string_view>
#include <filesystem>
#include <string>
#include <format>
#include <iostream>
#include <csignal>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

const unsigned char PNG_SIGN[8] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
const unsigned char JPG_SIGN[2] = { 0xff, 0xd8 };

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
    InputData data;
    
    {
        std::string errorMsg;
        validateData(argv, argc, data, errorMsg);
        
        if (errorMsg == "help") {
            Logger::getInstance()->printHelpInfo();
            return 0;
        } 
        else if (!errorMsg.empty()) {
            Logger::getInstance()->printError(errorMsg.c_str(), true);
            return 1;
        }
    }

    std::string_view savePath;
    if (!data.outputPath.empty())
        savePath = data.outputPath;

    ImageHandler imgHandler(data.imagePath, data.secret);
    if (!imgHandler.getStatus())
        return 1;

    if (data.action == Action::HIDE) {
        if (imgHandler.hide(data.sourcePath, data.sourceDataType, data.has4Channels))
            return 1;
    }
    else {
        if (imgHandler.read(data.sourceDataType))
            return 1;
    }

    if (imgHandler.saveResult(savePath, data.sourceDataType, data.action == Action::HIDE))
        return 1;

    return 0;
}

void validateData(char* argv[], const int argc, InputData& data, std::string& errMsg) {    
    const std::string_view mode = 1 < argc ? argv[1] : "";
    const std::string_view act = 2 < argc ? argv[2] : "";
    const std::string_view dType = 3 < argc ? argv[3] : "";
    const std::string_view src = 4 < argc ? argv[4] : "";
    const std::string_view img = 5 < argc ? argv[5] : "";
    const std::string_view out = 6 < argc ? argv[6] : "";

    if (argc > 7 || (act == "read" && argc > 6)) {
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

    // validate source_path
    if (data.action == Action::HIDE) {
        if (data.sourceDataType == DataType::STR) {
            if (src == "") {
                errMsg = "Source cannot be empty with DATA_TYPE=str.";
                return;
            }
        }
        else {
            if (!std::filesystem::exists(src)) {
                errMsg = std::format("Source with SOURCE_PATH='{}' not found", src);
                return;
            }

            if (std::filesystem::is_directory(src)) {
                errMsg = std::format("Source with SOURCE_PATH='{}' is not file", src);
                return;
            }

            std::fstream file(src.data(), std::ios::binary | std::ios::in);
            if (!file.is_open()) {
                errMsg = std::format("Open file with SOURCE_PATH='{}' failed", src);
                return;
            }

            if (data.sourceDataType == DataType::IMAGE) {
                const std::string readFailedStr(
                    std::format("Read file with SOURCE_PATH='{}' failed", src)
                );
                const std::string mustBeJpgPngStr(
                    std::format("File with SOURCE_PATH='{}' must be .jpg or .png", src)
                );

                char byte;
                if (!file.read(&byte, 1)) {
                    errMsg = readFailedStr;
                    return;
                }

                if (static_cast<unsigned char>(byte) == JPG_SIGN[0]) {
                    if (!file.read(&byte, 1)) {
                        errMsg = readFailedStr;
                        return;
                    }
                    if (static_cast<unsigned char>(byte) != JPG_SIGN[1]) {
                        errMsg = mustBeJpgPngStr;
                        return;
                    }
                    data.has4Channels = false;
                }
                else if (static_cast<unsigned char>(byte) == PNG_SIGN[0]) {
                    for (int i = 1; i < 8; ++i) {
                        if (!file.read(&byte, 1)) {
                            errMsg = readFailedStr;
                            return;
                        }
                        if (static_cast<unsigned char>(byte) != PNG_SIGN[i]) {
                            errMsg = mustBeJpgPngStr;
                            return;
                        }
                    }
                    data.has4Channels = true;
                }
                else {
                    errMsg = mustBeJpgPngStr;
                    return;
                }
            }
        }

        data.sourcePath = src;
    }

    // validate image_path
    const std::string_view imagePath = data.action == Action::READ
                                     ? src
                                     : img;
    if (!std::filesystem::exists(imagePath)) {
        errMsg = std::format("Image with IMAGE_PATH='{}' not found", imagePath);
        return;
    }
    if (std::filesystem::is_directory(imagePath)) {
        errMsg = std::format("Image with IMAGE_PATH='{}' is not file", imagePath);
        return;
    }

    std::fstream file(imagePath.data(), std::ios::binary | std::ios::in);
    if (!file.is_open()) { 
        errMsg = std::format("Open image with IMAGE_PATH='{}' failed", imagePath);
        return;
    }

    char byte;
    for (int i = 0; i < 8; ++i) {
        if (!file.read(&byte, 1)) {
            errMsg = std::format("Read image with IMAGE_PATH='{}' failed", imagePath);
            return;
        }

        if (static_cast<unsigned char>(byte) != PNG_SIGN[i]) {
            errMsg = std::format("Image with IMAGE_PATH='{}' must be .png", imagePath);
            return;
        }
    }

    data.imagePath = imagePath;

    // check output file
    const std::string_view outputPath = data.action == Action::READ
                                      ? img
                                      : out;
    if (outputPath != "") {       
        if (std::filesystem::is_directory(out)) {
            errMsg = std::format("OUTPUT_PATH='{}' cannot be a directory.", outputPath);
            return;
        }
        
        std::filesystem::path filePath = outputPath;

        if (filePath.has_parent_path() && !std::filesystem::exists(filePath.parent_path())) {
            errMsg = std::format("Directory of file with OUTPUT_PATH='{}' not exists", outputPath);
            return;
        }

        if (data.action == Action::HIDE) {
            if (!filePath.has_extension() || filePath.extension() != ".png") {
                errMsg = std::format(
                    "With ACTION=hide, file with OUTPUT_PATH='{}' must include .png extension.", outputPath
                );
                return;
            }
        }
        else if (data.sourceDataType == DataType::IMAGE) {
            if (filePath.has_extension()) {
                errMsg = std::format(
                    "With ACTION='read' and DATA_TYPE='IMAGE' "
                    "file with OUTPUT_PATH='{}' must not include any extension.", outputPath
                );
                return;
            }

            filePath.replace_extension(".png");
            if (std::filesystem::exists(filePath)) {
                errMsg = std::format(
                    "File with OUTPUT_PATH='{}' may replace existing file '{}.png', "
                    "if hidden image has extension .png", outputPath, outputPath
                );
                return;
            }

            filePath.replace_extension(".jpg");
            if (std::filesystem::exists(filePath)) {
                errMsg = std::format(
                    "File with OUTPUT_PATH='{}' may replace existing file '{}.jpg', "
                    "if hidden image has extension .jpg", outputPath, outputPath
                );
                return;
            }
        }

        if (std::filesystem::exists(outputPath)) {
            errMsg = std::format("File with OUTPUT_PATH='{}' already exists.", outputPath);
            return;
        }
        
        data.outputPath = outputPath;
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