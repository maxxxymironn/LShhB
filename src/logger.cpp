#include "logger.hpp"

#include <iostream>

void Logger::printError(const char* str, const bool validationError) { 
    std::cout << "ERROR::" << (validationError ? "VALIDATION:" : "") << " " << str << "\n\n";
}

void Logger::printInfo(const char* str) { std::cout << str << "\n"; }

void Logger::printHelpInfo() {
    std::cout << 
    "LShhB <MODE> <ACTION> <DATA_TYPE> <SOURCE(_PATH)> <IMAGE_PATH> <optional:OUTPUT_PATH>\n\n"

    "Shh..\n\n"
    
    "MODE:\n"
    "1) simple - simple hiding algorithm.\n"
    "2) advanced - seed-based hiding algorithm. Password required.\n\n"

    "ACTION:\n"
    "1) help (--help, -h) - print help informaton.\n"
    "2) hide - hide information (string, any file, image) in image.\n"
    "3) read - read hiden information in image.\n\n"

    "DATA_TYPE:\n"
    "Uses with ACTION=hide/read.\n"
    "1) str - hide/read input string.\n"
    "2) file - hide/read file.\n"
    "3) image - hide/read image.\n\n"

    "SOURCE(_PATH):\n"
    "Uses with ACTION=hide.\n"
    "String or path to file/image you want to hide.\n\n"

    "IMAGE_PATH:\n"
    "Uses with ACTION=hide/read.\n"
    "Image will be loaded in memory and saved as new image with hidden information.\n"
    "New image will be saved as 'image_with_secret.png' where you run app from, if you does not use OUTPUT_PATH.\n"
    "Supports ONLY .png.\n\n"

    "OUTPUT_PATH:\n"
    "Uses with ACTION=hide/read.\n"
    "Result of action will be saved in this path.\n"
    "Filename must not include extension if ACTION=read & DATA_TYPE=image.\n\n"

    "For example:\n"
    "./LShhB advanced hide str \"This message will be hidden in container image with path "
        "= ~/Pictures/container.png as ~/mySecret.png\" ~/Pictures/container.png ~/mySecret.png\n"
    "./LShhB simple hide image ~/Pictures/source.jpeg ~/Pictures/container.png\n"
    "./LShhB simple read image ~/image_with_secret.png ~/myDirectory/secret_from_image\n\n";
}