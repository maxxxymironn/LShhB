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
    "1) simple - simple hidding algorithm.\n"
    "2) advanced - uses a password (seed) for better hidding.\n\n"

    "ACTION:\n"
    "1) help (--help, -h) - print help informaton.\n"
    "2) hide - hide information (string, .txt file, image) into image.\n"
    "3) read - read hidden information from image.\n\n"

    "DATA_TYPE:\n"
    "Uses with ACTION=hide/read.\n"
    "1) str - hide/read input string.\n"
    "2) file - hide/read file.\n"
    "3) image - hide/read image.\n\n"

    "SOURCE(_PATH):\n"
    "Uses with ACTION=hide.\n"
    "Contains string or path to all format file/image you want to hide.\n\n"

    "IMAGE_PATH:\n"
    "Uses with ACTION=hide/read.\n"
    "Image path uses for coping image to create new image with hidden information. Supports .png only \n"
    "Result image will be saved in this path if you not point where save image\n\n"

    "OUTPUT_PATH:\n"
    "Uses with ACTION=hide/read.\n"
    "Path where will be saved result image. Must include image name without file extension.\n"

    "For example:\n"
    "LShhB advanced hide str \"This message will be hidden in image with path = ~/Pictures/container.png\" ~/Pictures/container.png\n"
    "LshhB simple hide image ~/Pictures/source.jpeg ~/Pictures/container.png ~/Pictures/imageWithSecret.png\n"
    "LshhB simple read image ~/Pictures/imageWithSecret.png /home/myDirectory/secret_from_image\n\n";
}