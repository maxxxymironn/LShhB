#include "imageHandler.hpp"
#include "enums.hpp"
#include "logger.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../external/stb_image_write.h"

#include <algorithm>
#include <string>
#include <fstream>
#include <random>
#include <iostream>
#include <vector>

using uchar = unsigned char;
using uint = unsigned int;
using ullong = unsigned long long;

namespace {
    std::string errMsg;
    const uchar mask1 = 0b0000'0001;
    const uchar mask0 = 0b1111'1110;
    const std::string_view signature = "LShh..B";

    int correctSourceSize(uint srcSize, const uint containerCapacity) {
        if (srcSize * 8 > containerCapacity - 89) {
            float percent = 100.f * static_cast<float>(containerCapacity - 89) / (srcSize * 8);
            std::cout << "ONLY " << percent << "% of source WILL BE saved due target image capacity.\n"
                         "Do you want to continue? [y/N] ";

            std::string input;
            getline(std::cin, input);
            if (input.size() != 1 || (input[0] != 'Y' && input[0] != 'y'))
                return 0;

            srcSize = (containerCapacity - 89) / 8;
        }

        return srcSize;
    }

    std::vector<uint> getVecRandomPxs(const std::string_view secret, const uint imgSize) {
        std::vector<uint> imgPxs(imgSize);
        std::iota(imgPxs.begin(), imgPxs.end(), 0);

        uint hash = 5381;
        for (uchar c : secret)
            hash = ((hash << 5) + hash) + c;

        std::mt19937 gen(hash);
        std::shuffle(imgPxs.begin(), imgPxs.end(), gen);
        
        return imgPxs;
    }
}

ImageHandler::ImageHandler(const std::string_view imagePath, const std::string_view secret)
    : _px(0), 
      _subPx(0), 
      _advancedMode(!secret.empty())
{
    bool hasErrors = _openImage(imagePath);
    if (!hasErrors && _advancedMode)
        _imgPxs = getVecRandomPxs(secret, _imgWidth * _imgHeight);
}

ImageHandler::~ImageHandler() {
    if (_imgData) stbi_image_free(_imgData);
}

bool ImageHandler::_openImage(const std::string_view path) {
    _imgData = stbi_load(
        path.data(), 
        &_imgWidth, &_imgHeight, 
        &_imgChannels,  4        
    );
    if (!_imgData) {
        Logger::getInstance()->printError("Load source image failed");
        return 1;
    }

    // signature = 56 bit
    // header = 33 bit
    // data = >= 8 bit
    // minimum pixel capacity = (56 + 33 + 8) / 3 = 32.33 ~ 33
    if (_imgWidth * _imgHeight < 33) {
        Logger::getInstance()->printError(
            "Image size too small to hide/read information.\n"
        );
        return 1;
    }

    _imgCapacity = _imgWidth * _imgHeight * 3;

    return 0;
}

void ImageHandler::_hideSignature() {
    uint byte;

    for (const uchar symbol : signature) {
        for (int j = 7; j >= 0; --j, ++_subPx) {
            if (_advancedMode) {
                if (_subPx > 2) ++(_px), _subPx = 0;
                byte = _imgPxs[_px] * 4 + _subPx;
            }            
            else {
                if ((_subPx + 1) % 4 == 0) ++_subPx;
                byte = _subPx;
            }

            uchar bit = static_cast<uchar>(symbol >> j) & mask1;
            _imgData[byte] = bit ? _imgData[byte] | mask1
                                 : _imgData[byte] & mask0;
        }
    }
}

bool ImageHandler::_readSignature() {
    uint byte;
    uchar symbol;

    for (std::size_t i = 0; i < signature.size(); ++i) {
        symbol = 0;
        for (int j = 0; j < 8; ++j, ++_subPx) {
            if (_advancedMode) {
                if (_subPx > 2) ++(_px), _subPx = 0;
                byte = _imgPxs[_px] * 4 + _subPx;
            }
            else {
                if ((_subPx + 1) % 4 == 0) ++_subPx;
                byte = _subPx;
            }

            uchar bit = _imgData[byte] & mask1;
            symbol <<= 1;
            symbol += bit;
        }
        if (symbol != signature[i]) {
            Logger::getInstance()->printError("Signature did not match.");
            return 1;
        }
    }

    return 0;
}

void ImageHandler::_hideHeader(const uint size) {
    uint byte;

    for (int i = 31; i >= 0; --i, ++_subPx) {
        if (_advancedMode) {
            if (_subPx > 2) ++(_px), _subPx = 0;
            byte = _imgPxs[_px] * 4 + _subPx;
        }
        else {
            if ((_subPx + 1) % 4 == 0) ++_subPx;
            byte = _subPx;
        }

        uchar bit = static_cast<uchar>(size >> i) & mask1;
        _imgData[byte] = bit ? _imgData[byte] | mask1
                             : _imgData[byte] & mask0;
    }
}

uint ImageHandler::_readHeader() {
    uint size = 0;
    uint byte;

    for (int i = 0; i < 32; ++i, ++_subPx) {
        if (_advancedMode) {
            if (_subPx > 2) ++(_px), _subPx = 0;
            byte = _imgPxs[_px] * 4 + _subPx;
        }
        else {
            if ((_subPx + 1) % 4 == 0) ++_subPx;
            byte = _subPx;
        }
        
        uchar bit = _imgData[byte] & mask1;
        size <<= 1;
        size += bit;
    }

    return size;
}

void ImageHandler::_hideImageHeader(const uint width, const uint height, const bool has4Channels) {
    uint byte;

    for (int i = 15; i >= 0; --i, ++_subPx) {
        if (_advancedMode) {
            if (_subPx > 2) ++(_px), _subPx = 0;
            byte = _imgPxs[_px] * 4 + _subPx;
        }
        else {
            if ((_subPx + 1) % 4 == 0) ++_subPx;
            byte = _subPx;
        }

        uchar bit = static_cast<uchar>(width >> i) & mask1;
        _imgData[byte] = bit ? _imgData[byte] | mask1
                             : _imgData[byte] & mask0;
    }
    
    for (int i = 15; i >= 0; --i, ++_subPx) {
        if (_advancedMode) {
            if (_subPx > 2) ++(_px), _subPx = 0;
            byte = _imgPxs[_px] * 4 + _subPx;
        }
        else {
            if ((_subPx + 1) % 4 == 0) ++_subPx;
            byte = _subPx;
        }

        uchar bit = static_cast<uchar>(height >> i) & mask1;
        _imgData[byte] = bit ? _imgData[byte] | mask1
                            : _imgData[byte] & mask0;
    }

    if (_advancedMode) {
        if (_subPx > 2) ++(_px), _subPx = 0;
        byte = _imgPxs[_px] * 4 + _subPx++;
    }
    else {
        if ((_subPx + 1) % 4 == 0) ++_subPx;
        byte = _subPx++;
    }
    _imgData[byte] = has4Channels ? _imgData[byte] | mask1
                                  : _imgData[byte] & mask0;
}

void ImageHandler::_readImageHeader(unsigned int& width, unsigned int& height, bool& has4Channels) {
    uint byte;

    for (int i = 0; i < 16; ++i, ++_subPx) {
        if (_advancedMode) {
            if (_subPx > 2) ++(_px), _subPx = 0;
            byte = _imgPxs[_px] * 4 + _subPx;
        }
        else {
            if ((_subPx + 1) % 4 == 0) ++_subPx;
            byte = _subPx;
        }

        uchar bit = _imgData[byte] & mask1;
        width <<= 1;
        width += bit;
    }

    for (int i = 0; i < 16; ++i, ++_subPx) {
        if (_advancedMode) {
            if (_subPx > 2) ++(_px), _subPx = 0;
            byte = _imgPxs[_px] * 4 + _subPx;
        }
        else {
            if ((_subPx + 1) % 4 == 0) ++_subPx;
            byte = _subPx;
        }

        uchar bit = _imgData[byte] & mask1;
        height <<= 1;
        height += bit;
    }

    if (_advancedMode) {
        if (_subPx > 2) ++(_px), _subPx = 0;
        byte = _imgPxs[_px] * 4 + _subPx++;
    }
    else {
        if ((_subPx + 1) % 4 == 0) ++_subPx;
        byte = _subPx++;
    }
    has4Channels = _imgData[byte] & mask1;
}

void ImageHandler::_hidePayload(const uchar* const srcData, const uint srcSize) {
    uint bytePos;

    for (uint i = 0; i < srcSize; ++i) {
        for (int j = 7; j >= 0; --j, ++_subPx) {
            if (_advancedMode) {
                if (_subPx > 2) ++(_px), _subPx = 0;
                bytePos = _imgPxs[_px] * 4 + _subPx;
            }            
            else {
                if ((_subPx + 1) % 4 == 0) ++_subPx;
                bytePos = _subPx;
            }

            uchar bit = static_cast<uchar>(srcData[i] >> j) & mask1;
            _imgData[bytePos] = bit ? _imgData[bytePos] | mask1
                                    : _imgData[bytePos] & mask0;
        }
    }
}

void ImageHandler::_readPayload(unsigned char* const srcData, const unsigned int size) {
    uint bytePos;

    for (uint i = 0; i < size; ++i) {
        uchar byte = 0;
        for (int j = 0; j < 8; ++j, ++_subPx) {
            if (_advancedMode) {
                if (_subPx > 2) ++(_px), _subPx = 0;
                bytePos = _imgPxs[_px] * 4 + _subPx;
            }
            else {
                if ((_subPx + 1) % 4 == 0) ++_subPx;
                bytePos = _subPx;
            }        

            uchar bit = _imgData[bytePos] & mask1;
            byte <<= 1;
            byte += bit;
        }
        srcData[i] = byte;
    }
}

bool ImageHandler::_hideImage(const std::string_view source, const bool has4Channels) {
    int srcImgWidth;
        int srcImgHeight;
        int srcImgChannels;
        unsigned char* srcImgData = nullptr;

        srcImgData = stbi_load(
            source.data(),
            &srcImgWidth, &srcImgHeight,
            &srcImgChannels, has4Channels ? 4 : 3
        );
        if (!srcImgData) {
            Logger::getInstance()->printError("Load source image failed");
            return 1;
        }

        _hideImageHeader(srcImgWidth, srcImgHeight, srcImgChannels == 4);

        const uint srcSize = srcImgWidth * srcImgHeight * srcImgChannels;
        uint srcImgBytes = correctSourceSize(srcSize, _imgCapacity);
        if (!srcImgBytes)
            return 1;

        _hidePayload(srcImgData, srcImgBytes);

        if (srcImgData) stbi_image_free(srcImgData);
        return 0;
}

void ImageHandler::_readImage() {
    uint width = 0;
    uint height = 0;
    bool has4Channels = false;
    _readImageHeader(width, height, has4Channels);

    const uint size = width * height * (has4Channels ? 4 : 3);
    uchar* hiddenImgData = new uchar[size];

    const uint savedSize = size * 8 > _imgCapacity - 89 
                         ? (_imgCapacity - 89) / 8
                         : size;
    
    _readPayload(hiddenImgData, savedSize);
    for (uint i = savedSize; i < size; ++i)
        hiddenImgData[i] = 0;

    stbi_image_free(_imgData);
    _imgData = hiddenImgData;
    _imgWidth = width;
    _imgHeight = height;
    _imgChannels = has4Channels ? 4 : 3;
}

bool ImageHandler::_hideStr(const std::string_view source) {
    const uint size = source.size();
    _hideHeader(size);

    uint srcBytes = correctSourceSize(size, _imgCapacity);
    if (!srcBytes)
        return 1;

    _hidePayload(reinterpret_cast<const uchar*>(source.data()), srcBytes);

    return 0;
}

void ImageHandler::_readStr() {
    const uint size = _readHeader();
    const uint savedSize = size * 8 > _imgCapacity - 89 
                         ? (_imgCapacity - 89) / 8
                         : size;

    textData.resize(savedSize);
    
    _readPayload(reinterpret_cast<uchar*>(textData.data()), savedSize);
}

bool ImageHandler::saveResult(
    const std::string_view savePath, const DataType dataType, const bool hide
) {
    constexpr const char* DEFAULT_IMAGE_NAME = "image_with_secret.png";
    constexpr const char* DEFAULT_RESULT_NAME = "secret_from_image";

    bool success = true;
    std::string outputFileName;

    if (hide) {
        outputFileName = !savePath.empty()
                       ? savePath
                       : DEFAULT_IMAGE_NAME;
        
        success = stbi_write_png(
            outputFileName.c_str(),
            _imgWidth, _imgHeight,
            _imgChannels, _imgData,
            _imgWidth * _imgChannels
        );
    }
    else {
        outputFileName = !savePath.empty()
                       ? savePath
                       : DEFAULT_RESULT_NAME;
        
        if (dataType == DataType::IMAGE) {
            if (_imgChannels == 4) {
                outputFileName += ".png";
                success = stbi_write_png(
                    outputFileName.c_str(),
                    _imgWidth, _imgHeight,
                    _imgChannels, _imgData,
                    _imgWidth * _imgChannels
                );
            }
            else {
                outputFileName += ".jpg";
                success = stbi_write_jpg(
                    outputFileName.c_str(),
                    _imgWidth, _imgHeight, 
                    _imgChannels, _imgData, 85
                );
            }
        }
        else {
            std::ofstream file(outputFileName);
            if (file.is_open()) {
                if (!file.write(textData.data(), textData.size()))
                    success = false;
            }
            else success = false;
        }
    }

    if (!success) {
        Logger::getInstance()->printError("Failed to save result");
        return 1;
    }
    return 0;
}

bool ImageHandler::read(const DataType dataType) {
    if (_readSignature())
        return 1;

    switch (dataType) {
        case DataType::STR: _readStr(); break;
        case DataType::IMAGE: _readImage(); break;
        case DataType::FILE: {
            Logger::getInstance()->printInfo("Work with file in progress.");
            return 1;
        }
    }
    return 0;
}

bool ImageHandler::hide(const std::string_view source, const DataType dataType, const bool has4Channels) {
    _hideSignature();

    switch (dataType) {
        case DataType::STR: return _hideStr(source);
        case DataType::IMAGE: return _hideImage(source, has4Channels);
        case DataType::FILE: {
            Logger::getInstance()->printInfo("Work with file in progress.");
            return 1;
        }
    }

    return 0;
}