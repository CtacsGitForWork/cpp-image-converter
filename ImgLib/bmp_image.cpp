#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {    
    char signature[2] = {'B', 'M'};
    uint32_t file_size;
    uint32_t reserved = 0;
    uint32_t data_offset;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {    
    uint32_t header_size = 40;
    int32_t width;
    int32_t height;
    uint16_t planes = 1;
    uint16_t bits_per_pixel = 24;
    uint32_t compression = 0;
    uint32_t image_size;
    int32_t x_pixels_per_meter = 11811;
    int32_t y_pixels_per_meter = 11811;
    uint32_t colors_used = 0;
    uint32_t important_colors = 0x1000000;
}
PACKED_STRUCT_END

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);
    if (!out) {
        return false;
    }

    const int width = image.GetWidth();
    const int height = image.GetHeight();
    const int stride = GetBMPStride(width);

    BitmapFileHeader file_header;
    file_header.file_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + stride * height;
    file_header.data_offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);

    BitmapInfoHeader info_header;
    info_header.width = width;
    info_header.height = height;
    info_header.image_size = stride * height;

    out.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
    out.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));

    vector<char> buffer(stride);
    for (int y = height - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < width; ++x) {
            buffer[x * 3 + 0] = static_cast<char>(line[x].b);
            buffer[x * 3 + 1] = static_cast<char>(line[x].g);
            buffer[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        out.write(buffer.data(), stride);
    }

    return out.good();
}

Image LoadBMP(const Path& file) {
    ifstream in(file, ios::binary);
    if (!in) {
        return {};
    }

    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    in.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    in.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    if (file_header.signature[0] != 'B' || file_header.signature[1] != 'M' ||
        info_header.bits_per_pixel != 24 || info_header.compression != 0) {
        return {};
    }

    const int width = info_header.width;
    const int height = info_header.height;
    const int stride = GetBMPStride(width);

    Image result(width, height, Color::Black());
    vector<char> buffer(stride);

    in.seekg(file_header.data_offset);

    for (int y = height - 1; y >= 0; --y) {
        in.read(buffer.data(), stride);
        if (!in) {
            return {};
        }

        Color* line = result.GetLine(y);
        for (int x = 0; x < width; ++x) {
            line[x].b = static_cast<byte>(buffer[x * 3 + 0]);
            line[x].g = static_cast<byte>(buffer[x * 3 + 1]);
            line[x].r = static_cast<byte>(buffer[x * 3 + 2]);
            line[x].a = byte{255};
        }
    }

    return result;
}

}  // namespace img_lib
