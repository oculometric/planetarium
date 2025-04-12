#pragma once

#include <stdint.h>
#include <string>

#pragma pack(push)
#pragma pack(1)
struct PTBitmapHeader
{
    uint8_t sig_0 = 'B';
    uint8_t sig_1 = 'M';
    uint32_t size_bytes = 0;
    uint16_t res_0 = 'P';
    uint16_t res_1 = 'T';
    uint32_t data_offset = 0;
};

struct PTBitmapInfoHeader
{
    uint32_t header_size = 40;
    int32_t bitmap_width = 0;
    int32_t bitmap_height = 0;
    uint16_t planes = 1;
    uint16_t bits_per_pixel = 32;
    uint32_t compresion = 0;
    uint32_t data_length = 0;
    int32_t ppm_horizontal = 144;
    int32_t ppm_vertical = 144;
    uint32_t colour_palette_count = 0;
    uint32_t important_colours = 0;
};

bool writeRGBABitmap(std::string path, char* data, int32_t width, int32_t height);
bool readRGBABitmap(std::string path, char*& data, int32_t& width, int32_t& height); // TODO: improve the reading and writing of image files

#pragma pack(pop)