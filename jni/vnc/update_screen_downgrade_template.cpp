#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#endif

#ifndef CONCAT2E
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#ifndef CONCAT3
#define CONCAT3(a,b,c) a##b##c
#endif

#ifndef CONCAT3E
#define CONCAT3E(a,b,c) CONCAT3(a,b,c)
#endif

#define FUNCTION CONCAT3E(updateScreen, IN_BYTES_PER_PIXEL, OUT_BYTES_PER_PIXEL)

void FUNCTION(int rotation) {
    unsigned int stride = frame.stride, height = frame.height, width = frame.width;
    IN_TYPE *data = (IN_TYPE *)frame.data;
    if (rotation == 0) {
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                ((OUT_TYPE*)vncbuf)[y * width + x] = CONVERT(data[y * stride + x]);
            }
        }
    } else if (rotation == 90) {
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                ((OUT_TYPE*)vncbuf)[x * height + (height - y - 1)] = CONVERT(data[y * stride + x]);
            }
        }                
    } else if (rotation == 180) {
        for (y = 0; y < height; y++) {        
            for (x = 0; x < width; x++) {
                ((OUT_TYPE*)vncbuf)[(height - y - 1) * width + (width - 1 - x)] = CONVERT(data[y * stride + x]);
            }
        }
    } else if (rotation == 270) {
        for (x = 0; x < width; x++) {
            for (y = 0; y < height; y++) {
                ((OUT_TYPE*)vncbuf)[(width - x - 1) * height + y] = CONVERT(data[y * stride + x]);
            }
        }
    }
    rfbMarkRectAsModified(vncscr, 0, 0, vncscr->width, vncscr->height);
}

#undef FUNCTION
