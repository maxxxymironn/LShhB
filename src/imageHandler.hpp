#pragma once

#include "enums.hpp"

#include <string_view>
#include <filesystem>
#include <vector>

class ImageHandler {
    struct Info {
        unsigned int width;
        unsigned int height;
        bool has4Channels;
    };

    int _imgWidth;
    int _imgHeight;
    int _imgChannels;
    unsigned int _imgCapacity;
    unsigned char* _imgData;

    unsigned int _px;
    unsigned int _subPx;
    std::vector<unsigned int> _imgPxs;

    std::filesystem::path _imgPath;
    std::string_view _secret;
    bool _advancedMode;

    void _hideSignature();
    bool _readSignature();

    void _hideHeader(const unsigned int size);
    unsigned int _readHeader();

    void _hideImageHeader(const unsigned int width, const unsigned int height, const bool has4Channels);
    Info _readImageHeader();

    void _hidePayload(const unsigned char* const srcData, const unsigned int size);
    bool _readPayload(const unsigned int size);
    void _readImagePayload(unsigned char* const srcData, const unsigned int srcSize);

    bool _hideImage(const std::string_view source, const bool has4Channels);
    void _readImage();

    bool _openImage(const std::string_view path);

public:
    ImageHandler(const std::string_view imagePath, const std::string_view secret);
    ~ImageHandler();

    bool getStatus() { return _imgData; }

    void setSecret(const std::string_view secret) { _secret = secret; }
    bool saveResult(
        const bool isCustomPath, const std::string_view savePath, 
        const DataType dataType, const Action action
    );

    bool hide(
        const std::string_view source, 
        const DataType dataType, 
        const bool has4Channels=true
    );
    bool read(const DataType dataType);
};