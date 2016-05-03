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

#define FUNCTION CONCAT2E(updateScreen, BYTES_PER_PIXEL)

void FUNCTION(int rotation) {
    unsigned int stride = frame.stride, height = frame.height, width = frame.width;
    TYPE* data = (TYPE*) frame.data;
    if (rotation == 0) {
        if (stride == width) {
            memcpy(vncbuf, data, frame.size);
        } else {
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    ((TYPE*)vncbuf)[y * width + x] = data[y * stride + x];
                }
            }
        }
    } else if (rotation == 90) {
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                ((TYPE*)vncbuf)[x * height + (height - y - 1)] = data[y * stride + x];
            }
        }                
    } else if (rotation == 180) {
        for (y = 0; y < height; y++) {        
            for (x = 0; x < width; x++) {
                ((TYPE*)vncbuf)[(height - y - 1) * width + (width - 1 - x)] = data[y * stride + x];
            }
        }
    } else { // if (rotation == 270)
        for (x = 0; x < width; x++) {
            for (y = 0; y < height; y++) {
                ((TYPE*)vncbuf)[(width - x - 1) * height + y] = data[y * stride + x];
            }
        }
    } 
}

#undef FUNCTION
