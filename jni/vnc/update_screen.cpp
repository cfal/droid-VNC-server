#include "update_screen.hpp"
#include "droidvncserver.hpp"
#include "Minicap.hpp"

#include <cstdint>

static unsigned int x, y;

#define BYTES_PER_PIXEL 1
#define TYPE uint8_t

void setupScreen1() {
    // default to RGB_332
    LOGD("Setup for 8-bit color");
    vncscr->serverFormat.redShift = 0;
    vncscr->serverFormat.greenShift = 3;
    vncscr->serverFormat.blueShift = 6;
    vncscr->serverFormat.redMax = 7;
    vncscr->serverFormat.greenMax = 7;
    vncscr->serverFormat.blueMax = 3;
}

#include "update_screen_template.cpp"
#undef BYTES_PER_PIXEL
#undef TYPE

#define BYTES_PER_PIXEL 2
#define TYPE uint16_t

void setupScreen2() {
    LOGD("Setup for 16-bit color");    
    switch (frame.format) {
    case Minicap::FORMAT_TRANSLUCENT:
    case Minicap::FORMAT_RGBA_4444:
        vncscr->serverFormat.redShift = 0;
        vncscr->serverFormat.greenShift = 4;
        vncscr->serverFormat.blueShift = 8;
        vncscr->serverFormat.redMax = 15;
        vncscr->serverFormat.greenMax = 15;
        vncscr->serverFormat.blueMax = 15;
        break;
    case Minicap::FORMAT_TRANSPARENT:
    case Minicap::FORMAT_RGBA_5551:
        vncscr->serverFormat.redShift = 0;
        vncscr->serverFormat.greenShift = 5;
        vncscr->serverFormat.blueShift = 10;
        vncscr->serverFormat.redMax = 31;
        vncscr->serverFormat.greenMax = 31;
        vncscr->serverFormat.blueMax = 31;
        break;
    case Minicap::FORMAT_RGB_565:
        vncscr->serverFormat.redShift = 0;
        vncscr->serverFormat.greenShift = 5;
        vncscr->serverFormat.blueShift = 11;
        vncscr->serverFormat.redMax = 31;
        vncscr->serverFormat.greenMax = 63;
        vncscr->serverFormat.blueMax = 31;
        break;
    default:
        LOGE("Unknown image format: %s", getImageFormatName());
        break;        
    }
}

#include "update_screen_template.cpp"
#undef BYTES_PER_PIXEL
#undef TYPE

#define BYTES_PER_PIXEL 4
#define TYPE uint32_t

void setupScreen4() {
    LOGD("Setup for 32-bit color");    
    switch (frame.format) {
    case Minicap::FORMAT_BGRA_8888:
        vncscr->serverFormat.redShift = 16;
        vncscr->serverFormat.greenShift = 8;
        vncscr->serverFormat.blueShift = 0;
        vncscr->serverFormat.redMax = 255;
        vncscr->serverFormat.greenMax = 255;
        vncscr->serverFormat.blueMax = 255;
        break;
    case Minicap::FORMAT_TRANSLUCENT:
    case Minicap::FORMAT_TRANSPARENT:
    case Minicap::FORMAT_RGBA_8888:
    case Minicap::FORMAT_RGBX_8888:
        vncscr->serverFormat.redShift = 0;
        vncscr->serverFormat.greenShift = 8;
        vncscr->serverFormat.blueShift = 16;
        vncscr->serverFormat.redMax = 255;
        vncscr->serverFormat.greenMax = 255;
        vncscr->serverFormat.blueMax = 255;
        break;
    default:
        LOGE("Unknown image format: %d", frame.format);
        break;
    }
}

#include "update_screen_template.cpp"
#undef BYTES_PER_PIXEL
#undef TYPE

#define BYTES_PER_PIXEL 8
#define TYPE uint64_t

void setupScreen8() {
    LOGD("Setup for 64-bit color");    
    vncscr->serverFormat.redShift = 0;
    vncscr->serverFormat.greenShift = 16;
    vncscr->serverFormat.blueShift = 32;
    vncscr->serverFormat.redMax = 65535;
    vncscr->serverFormat.greenMax = 65535;
    vncscr->serverFormat.blueMax = 65535;
}

#include "update_screen_template.cpp"
#undef BYTES_PER_PIXEL
#undef TYPE

#define IN_BYTES_PER_PIXEL 4
#define OUT_BYTES_PER_PIXEL 2
#define IN_TYPE uint32_t
#define OUT_TYPE uint16_t

void setupScreen42() {
    LOGD("Downgrade to 16-bit color");
    // Convert to RGB_565
    vncscr->serverFormat.redShift = 0;
    vncscr->serverFormat.greenShift = 5;
    vncscr->serverFormat.blueShift = 11;
    vncscr->serverFormat.redMax = 31;
    vncscr->serverFormat.greenMax = 63;
    vncscr->serverFormat.blueMax = 31;    
}

// static OUT_TYPE convertFormat42(IN_TYPE pixel) {
//     //    unsigned int b, g, r;
//     if (frame.format == Minicap::FORMAT_BGRA_8888) {
//         // r = ((pixel >> 19) & 31); // >> 16, >> 3
//         // g = ((pixel >> 10) & 63); // >> 8, >> 2
//         // b = ((pixel >> 3) & 31); // >> 0, >> 3
//         return ((pixel >> 19) & 0x1f) | // r
//             ((pixel >> 5) & 0x7e0) | // g
//             ((pixel << 8) & 0xf800); // b
//     } else {
//         // else assume RGB*
//         // b = ((pixel >> 19) & 31); // >> 16, >> 3
//         // g = ((pixel >> 10) & 63); // >> 8, >> 2
//         // r = ((pixel >> 3) & 31); // >> 0, >> 3
//         return ((pixel >> 8) & 0xf800) | // b
//             ((pixel >> 5) & 0x7e0) | // g
//             ((pixel >> 3) & 0x1f); // r
//     }
//     //    return ((b << 11) | (g << 5) | r);
// }

#define CONVERT(p) (frame.format == Minicap::FORMAT_BGRA_8888 ? \
                    (((p >> 19) & 0x1f) | ((p >> 5) & 0x7e0) | ((p << 8) & 0xf800)) : \
                    (((p >> 8) & 0xf800) | ((p >> 5) & 0x7e0) | ((p >> 3) & 0x1f)));

#include "update_screen_downgrade_template.cpp"

#undef CONVERT
#undef IN_BYTES_PER_PIXEL
#undef OUT_BYTES_PER_PIXEL
#undef IN_TYPE
#undef OUT_TYPE
