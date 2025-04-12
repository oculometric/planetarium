#include "bitmap.h"

#include <fstream>

using namespace std;

bool writeRGBABitmap(string path, char* data, int32_t width, int32_t height)
{
    // if the image params are nonsense, don't do anything
    if (width <= 0 || height <= 0 || data == nullptr)
        return false;

    // if we can't open the file, don't do anything
    ofstream file(path, ios::binary);
    if (!file.is_open())
        return false;

    // create a bitmap header
    PTBitmapHeader header;
    header.data_offset = sizeof(PTBitmapHeader) + sizeof(PTBitmapInfoHeader);
    header.size_bytes = header.data_offset + (width * height * 4);

    // create bitmapinfo header
    PTBitmapInfoHeader info_header;
    info_header.bitmap_width = width;
    info_header.bitmap_height = height;
    info_header.bits_per_pixel = 32;
    info_header.compresion = 0;
    info_header.data_length = (width * height * 4);
    
    // write header, followed by info header, followed by data
    file.write((char*)&header, sizeof(PTBitmapHeader));
    file.write((char*)&info_header, sizeof(PTBitmapInfoHeader));

    // reorganise data RGBA -> BGRA
    char* other_buf = new char[width * height * 4];
    size_t length = width * height;
    for (size_t i = 0; i < length; i++)
    {
        other_buf[(i * 4) + 0] = data[(i * 4) + 2];
        other_buf[(i * 4) + 1] = data[(i * 4) + 1];
        other_buf[(i * 4) + 2] = data[(i * 4) + 0];
        other_buf[(i * 4) + 3] = data[(i * 4) + 3];
    }

    file.write((char*)other_buf, info_header.data_length);
    delete[] other_buf;

    file.close();

    return true;
}

bool readRGBABitmap(string path, char*& data, int32_t& width, int32_t& height)
{
    // if we can't open the file, don't do anything
    ifstream file(path, ios::binary | ios::ate);
    if (!file.is_open())
        return false;

    // grab the file size
    size_t size = file.tellg();
    file.seekg(0);

    if (size < sizeof(PTBitmapHeader) + sizeof(PTBitmapInfoHeader))
    {
        file.close();
        return false;
    }

    // grab the bitmap header
    PTBitmapHeader header;
    file.read((char*)(&header), sizeof(PTBitmapHeader));

    // check the validity of the header
    PTBitmapHeader sample;
    if (header.sig_0 != sample.sig_0 || header.sig_1 != sample.sig_1)
    {
        file.close();
        return false;
    }

    if (size < header.size_bytes || header.data_offset < (sizeof(PTBitmapHeader) + sizeof(PTBitmapInfoHeader)))
    {
        file.close();
        return false;
    }

    // grab the bitmap info header
    PTBitmapInfoHeader info_header;
    file.read((char*)(&info_header), sizeof(PTBitmapInfoHeader));

    // check the validity of the info header
    PTBitmapInfoHeader info_sample;
    if (info_header.header_size != info_sample.header_size)
    {
        file.close();
        return false;
    }

    if (size < info_header.data_length + header.data_offset)
    {
        file.close();
        return false;
    }

    if (info_header.compresion != 0)
    {
        file.close();
        return false;
    }

    char* buffer = new char[info_header.data_length];
    file.read(buffer, info_header.data_length);

    file.close();

    width = info_header.bitmap_width;
    height = info_header.bitmap_height;
    if (info_header.bits_per_pixel == 32)
    {
        data = buffer;
        return true;
    }
    else if (info_header.bits_per_pixel == 24)
    {
        char* other_buf = new char[width * height * 4];
        size_t length = width * height;
        for (size_t i = 0; i < length; i++)
        {
            other_buf[(i * 4) + 0] = buffer[(i * 3) + 2];
            other_buf[(i * 4) + 1] = buffer[(i * 3) + 1];
            other_buf[(i * 4) + 2] = buffer[(i * 3) + 0];
            other_buf[(i * 4) + 3] = 255;
        }

        data = other_buf;
        delete[] buffer;

        return true;
    }
    else return false;
}
