#include "bitmap.h"

#include <fstream>

using namespace std;

bool writeRGBABitmap(string path, void* data, int32_t width, int32_t height)
{
    if (width <= 0 || height <= 0 || data == nullptr)
        return false;

    ofstream file(path, ios::binary);
    if (!file.is_open())
        return false;

    PTBitmapHeader header;
    header.data_offset = sizeof(PTBitmapHeader) + sizeof(PTBitmapInfoHeader);
    header.size_bytes = header.data_offset + (width * height * 4);

    PTBitmapInfoHeader info_header;
    info_header.bitmap_width = width;
    info_header.bitmap_height = height;
    info_header.bits_per_pixel = 32;
    info_header.compresion = 0;
    info_header.data_length = (width * height * 4);
    
    file.write((char*)&header, sizeof(PTBitmapHeader));
    file.write((char*)&info_header, sizeof(PTBitmapInfoHeader));
    file.write((char*)data, info_header.data_length);

    file.clear();

    return true;
}
