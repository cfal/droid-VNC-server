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
#define CONVERT CONCAT3E(convertFormat, IN_BYTES_PER_PIXEL, OUT_BYTES_PER_PIXEL)

void FUNCTION(Minicap::Frame *frame, int rotation) {
    unsigned int stride = frame->stride, height = frame->height;
    IN_TYPE *data = (IN_TYPE *)frame->data;
    Minicap::Format format = frame->format;
    
    if (rotation == 90) {
        for (x = 0; x < stride; x++) {
            for (y = 0; y < height; y++) {
                int targetX = y;
                int targetY = stride - x - 1;                        
                ((OUT_TYPE*)vncbuf)[targetY * height + targetX] = CONVERT(format, data[y * stride + x]);
            }
        }                
    } else if (rotation == 180) {
        for (x = 0; x < stride; x++) {
            for (y = 0; y < height; y++) {
                ((OUT_TYPE*)vncbuf)[y * stride + x] = CONVERT(format, data[(height - 1 - y) * stride + (stride - 1 - x)]);
            }
        }
    } else if (rotation == 270) {
        for (x = 0; x < stride; x++) {
            for (y = 0; y < height; y++) {
                int targetX = height - y - 1;
                int targetY = x;
                ((OUT_TYPE*)vncbuf)[targetY * height + targetX] = CONVERT(format, data[y * stride + x]);
            }
        }
    } else {
        for (x = 0; x < stride; x++) {
            for (y = 0; y < height; y++) {
                ((OUT_TYPE*)vncbuf)[y * stride + x] = CONVERT(format, data[y * stride + x]);
            }
        }
    }
    rfbMarkRectAsModified(vncscr, 0, 0, vncscr->width, vncscr->height);
}

#undef FUNCTION
