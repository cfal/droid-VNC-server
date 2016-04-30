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

void FUNCTION(Minicap::Frame *frame, int rotation) {
    LOGD("%s rotation=%d", __func__, rotation);
    unsigned int stride = frame->stride, height = frame->height;
    TYPE* data = (TYPE*) frame->data;
    if (rotation == 90) {
        for (x = 0; x < stride; x++) {
            for (y = 0; y < height; y++) {
                ((TYPE*)vncbuf)[(stride - x - 1) * height + y] = ((TYPE*)data)[y * stride + x];
            }
        }                
    } else if (rotation == 180) {
        for (x = 0; x < stride; x++) {
            for (y = 0; y < height; y++) {
                ((TYPE*)vncbuf)[y * stride + x] = ((TYPE*)data)[(height - 1 - y) * stride + (stride - 1 - x)];
            }
        }
    } else if (rotation == 270) {
        for (x = 0; x < stride; x++) {
            for (y = 0; y < height; y++) {
                int targetX = height - y - 1;
                int targetY = x;
                ((TYPE*)vncbuf)[targetY * height + targetX] = ((TYPE*)data)[y * stride + x];
            }
        }
    } else {
        memcpy(vncbuf, data, frame->size);
    }
    rfbMarkRectAsModified(vncscr, 0, 0, vncscr->width, vncscr->height);
}

#undef FUNCTION
