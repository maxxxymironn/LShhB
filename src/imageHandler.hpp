#pragma once

#include "enums.hpp"

#include <string_view>
#include <string>
#include <vector>

class ImageHandler {
    int _imgWidth;
    int _imgHeight;
    int _imgChannels;
    unsigned int _imgCapacity;
    unsigned char* _imgData;

    unsigned int _px;
    unsigned int _subPx;
    std::vector<unsigned int> _imgPxs;
    std::string textData;

    bool _advancedMode;

    void _hideSignature();
    bool _readSignature();

    void _hideHeader(const unsigned int size);
    unsigned int _readHeader();

    void _hideImageHeader(const unsigned int width, const unsigned int height, const bool has4Channels);
    void _readImageHeader(unsigned int& width, unsigned int& height, bool& has4Channels);

    void _hidePayload(const unsigned char* const srcData, const unsigned int size);
    void _readPayload(unsigned char* const srcData, const unsigned int size);
    // void _readImagePayload(unsigned char* const srcData, const unsigned int srcSize);

    bool _hideImage(const std::string_view source, const bool has4Channels);
    void _readImage();

    bool _hideStr(const std::string_view source);
    void _readStr();

    bool _openImage(const std::string_view path);

    bool _saveFile(const std::string& data);

public:
    ImageHandler(const std::string_view imagePath, const std::string_view secret);
    ~ImageHandler();

    bool getStatus() { return _imgData; }

    bool saveResult(
        const std::string_view savePath, 
        const DataType dataType, bool hide
    );

    bool hide(
        const std::string_view source, 
        const DataType dataType, 
        const bool has4Channels=true
    );
    bool read(const DataType dataType);
};