#include "update_screen.hpp"
#include "droidvncserver.hpp"
#include "Minicap.hpp"

#include <cstdint>

static unsigned int x, y;

#define BYTES_PER_PIXEL 1
#define TYPE uint8_t
#include "update_screen_template.cpp"
#undef BYTES_PER_PIXEL
#undef TYPE

#define BYTES_PER_PIXEL 2
#define TYPE uint16_t
#include "update_screen_template.cpp"
#undef BYTES_PER_PIXEL
#undef TYPE

#define BYTES_PER_PIXEL 4
#define TYPE uint32_t
#include "update_screen_template.cpp"
#undef BYTES_PER_PIXEL
#undef TYPE

#define BYTES_PER_PIXEL 8
#define TYPE uint64_t
#include "update_screen_template.cpp"
#undef BYTES_PER_PIXEL
#undef TYPE

#define IN_BYTES_PER_PIXEL 4
#define OUT_BYTES_PER_PIXEL 2
#define IN_TYPE uint32_t
#define OUT_TYPE uint16_t

static OUT_TYPE convertFormat42(Minicap::Format format, IN_TYPE pixel) {
    if (format == Minicap::FORMAT_RGBA_8888) {
        /*int a = pixel >> 24;
        int b = (pixel >> 16) & 0xff;
        int g = (pixel >> 8) & 0xff;
        int r = pixel & 0xff;

        return (OUT_TYPE) pixel;*/
        // convert to RGB 565
        // int b = (pixel >> 16) & 0xff;
        // int g = (pixel >> 8) & 0xff;
        // int r = pixel & 0xff;
        int r = pixel >> 24;
        int g = (pixel >> 16) & 0xff;
        int b = (pixel >> 8) & 0xff;
        int a = pixel & 0xff;        
        return (((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3));
        
        // convert to RGB_888

        //        return (pixel >> 8);
        // return (OUT_TYPE) (pixel & 0xffffff);
    } else {
        // We don't know how to handle it.
        return (OUT_TYPE) pixel;
    }
    return 0;
}

#include "update_screen_downgrade_template.cpp"

#undef IN_BYTES_PER_PIXEL
#undef OUT_BYTES_PER_PIXEL
#undef IN_TYPE
#undef OUT_TYPE
