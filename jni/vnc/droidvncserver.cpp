/*
  droid vnc server - Android VNC server
  Copyright (C) 2009 Jose Pereira <onaips@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "droidvncserver.hpp"
#include "rotation_watcher.hpp"
#include "update_screen.hpp"

#include "rfb/keysym.h"
#include "libvncserver/scale.h"
#include "Minicap.hpp"

#define ROT_0 (1 << 0)
#define ROT_90 (1 << 1)
#define ROT_180 (1 << 2)
#define ROT_270 (1 << 3)
#define ROT_ALL (ROT_0 | ROT_90 | ROT_180 | ROT_270)

// Configurables
static int serverPort = 5901; // Android already has 5900 bound natively in some devices.
static char serverPassword[256] = "";
static char serverPasswordFile[1024] = "";
static int allowedRotation = ROT_ALL;
static bool forcedRotation = false; // true if allowedRotation != ROT_ALL
static bool rotate180 = false;
static uint16_t scaling = 100;
static bool skipFrames = false;
static bool downgrade = false;

// Screen info
static fb_var_screeninfo screenInfo;
static int screenWidth = 0, screenHeight = 0;
static int screenRotation; // Current screen rotation
static int imageRotation; // Required frame rotation

// shared VNC buffers and screen
rfbScreenInfoPtr vncscr;
unsigned char *cmpbuf;
unsigned char *vncbuf;

// Reverse connection
static char *rhost = NULL;
static int rport = 5500;

// Minicap object
static Minicap *minicap = NULL;

// TODO: cleanup
static uint32_t idle = 0;
static uint32_t standby = 1;

void print(int logPriority, FILE* stream, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    if (logPriority != -1) __android_log_vprint(logPriority, LOG_TAG, format, args);
    va_end(args);
    fprintf(stream, "\n");
}

static void onClientGone(rfbClientPtr cl)
{
    LOGD("onClientGone: Client disconnected");
}

static rfbNewClientAction onClientConnect(rfbClientPtr cl)
{
    LOGD("onClientConnect: New client connection");
    if (scaling != 100) {
        unsigned int scaledWidth = (unsigned int) (vncscr->width * scaling / 100.0 + 0.5);
        unsigned int scaledHeight = (unsigned int) (vncscr->height * scaling / 100.0 + 0.5);
        LOGD("onClientConnect: Scaling to %dx%d", scaledWidth, scaledHeight);
        rfbScalingSetup(cl, scaledWidth, scaledHeight);
    }

    // cl->enableSupportedEncodings = TRUE;
    cl->clientGoneHook=(ClientGoneHookPtr) onClientGone;
    return RFB_CLIENT_ACCEPT;
}

static void onKeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl) {
}

static void onPointerEvent(int buttonMask, int x, int y, rfbClientPtr cl) {
}

static void reinitVncServer(int width, int height, int stride, int bpp, bool recreateBuffer) {
    // Rescale clients
    unsigned int scaledWidth, scaledHeight;
    if (forcedRotation) {
        if (imageRotation == 90 || imageRotation == 270) {
            if (vncscr->width == height && vncscr->height == stride) {
                LOGD("reinitVncServer: No action necessary!");
                return;
            }               
            scaledWidth = (unsigned int) (height * scaling / 100.0 + 0.5);
            scaledHeight = (unsigned int) (stride * scaling / 100.0 + 0.5);
        } else {
            if (vncscr->width == stride && vncscr->height == height) {
                LOGD("reinitVncServer: No action necessary! (2)");
                return;
            }
            scaledWidth = (unsigned int) (stride * scaling / 100.0 + 0.5);
            scaledHeight = (unsigned int) (height * scaling / 100.0 + 0.5);
        }
    } else {
        if (vncscr->width == width && vncscr->height == height) {
            LOGD("reinitVncServer: No action necessary! (3)");
            return;            
        }
        scaledWidth = (unsigned int) (width * scaling / 100.0 + 0.5);
        scaledHeight = (unsigned int) (height * scaling / 100.0 + 0.5);
    }

    LOGD("reinitVncServer: bpp: %d width: %d, height: %d, stride: %d, recreateBuffer: %d", bpp, width, height, stride, recreateBuffer);

    rfbClientIteratorPtr iterator = rfbGetClientIterator(vncscr);
    rfbClientPtr cl;
    while ((cl = rfbClientIteratorNext(iterator)) != NULL) {
        rfbScalingSetup(cl, scaledWidth, scaledHeight);
    }

    // Set new framebuffer size
    if (recreateBuffer) {
        unsigned char *oldBuffer = (unsigned char *) vncscr->frameBuffer;
        unsigned char *newBuffer = (unsigned char *) malloc(stride * height * bpp);
        unsigned char *newCmpBuffer = (unsigned char *) malloc(stride * height * bpp);
        if (forcedRotation) {
            if (imageRotation == 90 || imageRotation == 270) {
                rfbNewFramebuffer(vncscr, (char *)newBuffer, height, stride, 8, 3, bpp);
            } else {
                rfbNewFramebuffer(vncscr, (char *)newBuffer, stride, height, 8, 3, bpp);
            }
        } else {
            rfbNewFramebuffer(vncscr, (char *)newBuffer, width, height, 8, 3, bpp);
            vncscr->paddedWidthInBytes = stride * bpp;            
        }
        vncbuf = newBuffer;
        cmpbuf = newCmpBuffer;
        free(oldBuffer);
    } else {
        if (forcedRotation) {
            if (imageRotation == 90 || imageRotation == 270) {
                rfbNewFramebuffer(vncscr, (char *)vncscr->frameBuffer, height, stride, 8, 3, bpp);
            } else {
                rfbNewFramebuffer(vncscr, (char *)vncscr->frameBuffer, stride, height, 8, 3, bpp);
            }
        } else {
            rfbNewFramebuffer(vncscr, (char *)vncscr->frameBuffer, width, height, 8, 3, bpp);
            vncscr->paddedWidthInBytes = stride * bpp;            
        }
    }
    vncscr->bitsPerPixel = bpp * 8;
}

static void initVncServer(int argc, char **argv, unsigned int width, unsigned int height, unsigned int stride, unsigned int bpp)
{ 
    LOGD("initVncServer: bpp: %d width: %d, height: %d, stride: %d", bpp, width, height, stride);

    vncbuf = (unsigned char *) malloc(stride * height * bpp);
    cmpbuf = (unsigned char *) malloc(stride * height * bpp);

    if (forcedRotation) {
        if (imageRotation == 90 || imageRotation == 270) {
            vncscr = rfbGetScreen(&argc, argv, height, stride, 8, 3, bpp);
        } else {
            vncscr = rfbGetScreen(&argc, argv, stride, height, 8, 3, bpp);
        }
    } else {
        vncscr = rfbGetScreen(&argc, argv, width, height, 8, 3, bpp);
        vncscr->paddedWidthInBytes = stride * bpp;        
    }

    vncscr->bitsPerPixel = bpp * 8;
    vncscr->deferUpdateTime = 5; // 5ms
    vncscr->port = serverPort;
    vncscr->desktopName = (char *) "Android";
    vncscr->frameBuffer =(char *) vncbuf;
    vncscr->alwaysShared = TRUE;
    vncscr->kbdAddEvent = onKeyEvent;
    vncscr->ptrAddEvent = onPointerEvent;
    vncscr->httpDir = (char *) "webclients/";
    vncscr->newClientHook = (rfbNewClientHookPtr) onClientConnect;

    vncscr->serverFormat.trueColour = TRUE;
    vncscr->serverFormat.bitsPerPixel = bpp * 8;

    if (strcmp(serverPasswordFile, "") != 0) {
        LOGD("initVncServer: Using encrypted password file");
        vncscr->authPasswdData = serverPasswordFile;
    }
    else if (strcmp(serverPassword, "") != 0) {
        LOGD("initVncServer: Using plain text password");
        char **passwords = (char **)malloc(2 * sizeof(char **));
        passwords[0] = serverPassword;
        passwords[1] = NULL;
        vncscr->authPasswdData = passwords;
        vncscr->passwordCheck = rfbCheckPasswordByList;
    } 

    vncscr->sslcertfile = (char *) "self.pem";

    rfbInitServer(vncscr);
}

void cleanup(int exitCode)
{ 	
    LOGD("Cleaning up...");

    stop_rotation_watcher();

    if (minicap) {
        // Server hasn't been initialized if minicap isn't
        // Don't disconnect clients here else a reconnect can happen before exit
        rfbShutdownServer(vncscr, FALSE);
        minicap->setFrameAvailableListener(NULL);
        minicap_free(minicap);
    }

    exit(exitCode);
}

static int extractHostPort(char **destHost, int *destPort, char *str)
{
    int len = strlen(str);
    char *p;

    /* copy in to host */
    char *rhost = *destHost = (char *) malloc(len + 1);
    rhost = (char *) malloc(len+1);
    if (!rhost) {
        LOGE("extractHostPort: could not malloc string %d", len);
        return 1;
    }
    strncpy(rhost, str, len);
    rhost[len] = '\0';

    /* extract port, if any */
    if ((p = strrchr(rhost, ':')) != NULL) {
        int port = atoi(p+1);
        if (port < 0) {
            port = -port;
        } else if (rport < 20) {
            port = 5500 + rport;
        }
        *p = '\0';
        *destPort = port;
    }
    return 0;
}

static int try_get_dumpsys_display_info(int *width, int *height) {
    FILE *f;
    char buf[4096], *p, *q;
    int w, h;

    if ((f = popen("dumpsys window", "r")) == NULL) {
        LOGE("Could not run dumpsys");
        return 1;
    }

    if (fgets(buf, 4095, f) == NULL) {
        LOGE("Could not read dumpsys");
        return 1;
    }

    p = strstr(buf, " init=");
    if (!p) {
        LOGE("Unrecognized dumpsys output");
        return 1;
    }
    q = strstr(p, "x");
    if (!q) {
        LOGE("Unrecognized dumpsys output (2)");
        return 1;
    }

    p += 6;
    *q = 0;
    w = atoi(p);

    p = q + 1;
    q = strstr(p, " ");
    *q = 0;
    h = atoi(p);

    pclose(f);

    if (w <= 0 || h <= 0) {
        LOGE("Received invalid values from dumpsys");
        return 1;
    }

    *width = w;
    *height = h;

    return 0;
}

static int try_get_framebuffer_display_info(int *width, int *height) {
    int fd = open("/dev/graphics/fb0", O_RDONLY);
    if (fd < 0) {
        LOGE("Cannot open /dev/graphics/fb0");
        return -1;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &screenInfo) < 0) {
        close(fd);
        LOGE("Cannot get FBIOGET_VSCREENINFO of /dev/graphics/fb0");
        return -1;
    }
    *width = screenInfo.xres;
    *height = screenInfo.yres;
    close(fd);
    return 0;
}

class FrameWaiter: public Minicap::FrameAvailableListener {
public:
    FrameWaiter()
        : mTimeout(std::chrono::milliseconds(100)),
          mPendingFrames(0),
          mStopped(false) {
    }

    int
    waitForFrame() {
        std::unique_lock<std::mutex> lock(mMutex);

        while (!mStopped) {
            if (mCondition.wait_for(lock, mTimeout, [this]{return mPendingFrames > 0;})) {
                return mPendingFrames--;
            }
        }

        return 0;
    }

    void
    reportExtraConsumption(int count) {
        std::unique_lock<std::mutex> lock(mMutex);
        mPendingFrames -= count;
    }

    void
    onFrameAvailable() {
        std::unique_lock<std::mutex> lock(mMutex);
        mPendingFrames += 1;
        mCondition.notify_one();
    }

    int
    getPendingFrames() {
        std::unique_lock<std::mutex> lock(mMutex);
        if (!mPendingFrames) return 0;
        return mPendingFrames--;
    }

    void
    stop() {
        mStopped = true;
    }

    bool
    isStopped() {
        return mStopped;
    }

private:
    std::mutex mMutex;
    std::condition_variable mCondition;
    std::chrono::milliseconds mTimeout;
    int mPendingFrames;
    bool mStopped;
};

static void resetMinicapOrientation() {
    Minicap::DisplayInfo desiredInfo, realInfo;
    realInfo.width = desiredInfo.width = screenWidth;
    realInfo.height = desiredInfo.height = screenHeight;
    desiredInfo.orientation = screenRotation / 90;

    minicap->setRealInfo(realInfo);

    if (minicap->setDesiredInfo(desiredInfo) != 0)
        FATAL("Could not set desired display info");

    if (minicap->applyConfigChanges() != 0)
        FATAL("Could not apply config changes");
}

static unsigned int getImageRotation() {
    unsigned int s = screenRotation / 90, rot;
    if ((allowedRotation & (1 << s)) != 0) {
        rot = 0;
    } else if ((allowedRotation & (1 << ((s + 2) & 3))) != 0) {
        rot = 180;
    } else if ((allowedRotation & (1 << ((s + 1) & 3))) != 0) {
        rot = 90;
    } else { // ((allowedRotation & (1 << ((s + 3) & 3))) != 0)
        rot = 270;
    }
    return rotate180 ? (rot + 180) % 360 : rot;
}

static void printUsage(char **argv)
{
    P("\nUsage: %s [options]\n\n"
      "Server options:\n"
      "  -p <password>\t- Password to access server\n"
      "  -e <path to encrypted password file>\t- path to encrypted password file to access server\n"
      "  -R <host:port>\t- Host for reverse connection\n\n"
      "Display options:\n"
      "  -w <width>\t- Screen width\n"
      "  -h <height>\t- Screen height\n"      
      "  -r <rotation>\t- Force screen rotation (degrees) (0,90,180,270)\n"
      "  -o <orientation>\t- Force screen orientation (landscape, portrait)\n"
      "  -z\t\t- Rotate display another 180 degrees (for zte compatibility)\n"
      "  -s <scale>\t- Scale percentage (20,30,50,100,150)\n"
      "  -d\t\t- Downgrade color when possible\n"
      "  -f\t\t- Enable frame skipping\n\n"
      "Other options:\n"
      "-v\t\t- Output version\n"
      "-?\t\t- Print this help\n", argv[0]);
}

int main(int argc, char **argv)
{
    //pipe signals
    signal(SIGINT, cleanup);
    signal(SIGKILL, cleanup);
    signal(SIGILL, cleanup);

    if(argc > 1) {
        int i = 1, r;
        while(i < argc) {
            if(*argv[i] == '-') {
                switch(*(argv[i] + 1)) {
                case '?':
                    printUsage(argv);
                    return 0;
                case 'v':
                    P("androidvncserver version " VERSION);
                    return 0;
                case 'w':
                    if (++i >= argc) FATAL("No screen width provided");
                    screenWidth = atoi(argv[i]);
                    break;
                case 'h':
                    if (++i >= argc) FATAL("No screen height provided");
                    screenHeight = atoi(argv[i]);
                    break;
                case 'p':
                    if (++i >= argc) FATAL("No password provided");
                    strncpy(serverPassword,argv[i], 255);
                    break;
                case 'e':
                    if (++i >= argc) FATAL("No password file provided");
                    strncpy(serverPasswordFile, argv[i], 1023);
                    LOGD("Using %s", serverPasswordFile);
                    strcpy(serverPassword, "");
                    break;
                case 'd':
                    i++;
                    downgrade = true;
                    break;
                case 'z': 
                    i++;
                    rotate180 = true;
                    break;
                case 'P':
                    if (++i >= argc) FATAL("No server port provided");
                    serverPort = atoi(argv[i]);
                    break;
                case 's':
                    if (++i >= argc) FATAL("No scaling value provided");
                    r = atoi(argv[i]); 
                    if (r >= 1 && r <= 150) {
                        scaling = r;
                    } else {
                        FATAL("Invalid scaling value: %d%%", r);
                    }
                    LOGD("Scaling to %d%%\n",scaling);
                    break;
                case 'R':
                    if (++i >= argc) FATAL("No reverse host/port provided");
                    if (extractHostPort(&rhost, &rport, argv[i]))
                        FATAL("Could not read reverse host/port");
                    break;
                case 'r':
                    if (allowedRotation != ROT_ALL)
                        FATAL("Cannot specify -r and -o together");

                    if (++i >= argc) FATAL("No rotation provided");                    
                    r = atoi(argv[i]);
                    switch (r) {
                    case 90:
                        allowedRotation = ROT_90;
                        break;
                    case 180:
                        allowedRotation = ROT_180;
                        break;
                    case 270:
                        allowedRotation = ROT_270;
                        break;
                    case 360:
                        r = 0;
                    case 0:
                        allowedRotation = ROT_0;
                        break;
                    default: FATAL("Unknown rotation value: %d", r);
                    }
                    LOGD("Forcing %d degree screen rotation", r);
                    break;
                case 'o':
                    if (allowedRotation != ROT_ALL)
                        FATAL("Cannot specify -o and -r together");

                    if (++i >= argc) FATAL("No orientation provided");
                    if (!strcmp(argv[i], "portrait")) {
                        allowedRotation = ROT_0 | ROT_180;
                        LOGD("Forcing portrait orientation");
                    } else if (!strcmp(argv[i], "landscape")) {
                        allowedRotation = ROT_90 | ROT_270;
                        LOGD("Forcing landscape orientation");
                    } else {
                        FATAL("Unknown orientation: %s", argv[i]);
                    }
                    break;
                case 'f':
                    i++;
                    skipFrames = true;
                    LOGD("Enabled frame skipping");
                }
            }
            i++;
        }
    }

    LOGD("Starting rotation watcher");

    if (start_rotation_watcher())
        FATAL("Could not start rotation watcher");

    LOGD("Starting thread pool");

    minicap_start_thread_pool();

    // Get the screen dimensions if not provided
    if (screenWidth <= 0 || screenHeight <= 0) {
        LOGD("Querying display info (1)");
        if (try_get_dumpsys_display_info(&screenWidth, &screenHeight) != 0) {
            LOGD("Querying display info (2)");
            if (try_get_framebuffer_display_info(&screenWidth, &screenHeight) != 0) {
                LOGD("Querying display info (3)");
                Minicap::DisplayInfo fbInfo;
                if (!minicap_try_get_display_info(0, &fbInfo)) {
                    screenWidth = fbInfo.width;
                    screenHeight = fbInfo.height;
                } else  {
                    FATAL("Unable to get display info");
                }
            }
        }
    }

    forcedRotation = allowedRotation != ROT_ALL;

    if ((minicap = minicap_create(0)) == NULL) {
        FATAL("Could not create minicap object");
    }

    switch (minicap->getCaptureMethod()) {
    case Minicap::METHOD_FRAMEBUFFER:
        LOGD("Display method: framebuffer");
        break;
    case Minicap::METHOD_SCREENSHOT:
        LOGD("Display method: screenshot");
        break;
    case Minicap::METHOD_VIRTUAL_DISPLAY:
        LOGD("Display method: virtual display");
        break;
    }

    Minicap::DisplayInfo realInfo;
    realInfo.width = screenWidth;
    realInfo.height = screenHeight;
    if (minicap->setRealInfo(realInfo) != 0)
        FATAL("Could not set real display info");        

    // Get and set original orientation
    while (!check_rotation_change(&screenRotation));
    LOGD("Original rotation: %d", screenRotation);
    imageRotation = getImageRotation();    

    FrameWaiter *gWaiter = new FrameWaiter;
    minicap->setFrameAvailableListener(gWaiter);

    resetMinicapOrientation();

    Minicap::Frame frame;    

    // Grab the first frame so we can check its properties
    if (gWaiter->isStopped())
        FATAL("Frame waiter not started");

    gWaiter->waitForFrame();
    if (minicap->consumePendingFrame(&frame) != 0)
        FATAL("Could not read first frame");

    LOGD("Bytes per pixel: %d", frame.bpp);
    switch (frame.format) {
    case Minicap::FORMAT_NONE:
        LOGD("Image format: none");
        break;
    case Minicap::FORMAT_CUSTOM:
        LOGD("Image format: Custom");
        break;
    case Minicap::FORMAT_TRANSLUCENT:
        LOGD("Image format: Translucent");
        break;
    case Minicap::FORMAT_TRANSPARENT:
        LOGD("Image format: Transparent");
        break;
    case Minicap::FORMAT_OPAQUE:
        LOGD("Image format: Opaque");
        break;        
    case Minicap::FORMAT_RGBA_8888:
        LOGD("Image format: RGBA 8888");
        break;
    case Minicap::FORMAT_RGBX_8888:
        LOGD("Image format: RGBX 8888");
        break;
    case Minicap::FORMAT_RGB_888:
        LOGD("Image format: RGB 888");
        break;
    case Minicap::FORMAT_RGB_565:
        LOGD("Image format: RGB 565");
        break;
    case Minicap::FORMAT_BGRA_8888:
        LOGD("Image format: BGRA 8888");
        break;
    case Minicap::FORMAT_RGBA_5551:
        LOGD("Image format: RGBA 5551");
        break;
    case Minicap::FORMAT_RGBA_4444:
        LOGD("Image format: RGBA 4444");
        break;
    case Minicap::FORMAT_UNKNOWN:
        LOGD("Image format: Unknown");
        break;
    default:
        FATAL("Unknown image format: %d", frame.format);
    }

    unsigned int targetBpp = frame.bpp;
    void (*updateScreenFn)(Minicap::Frame *, int);

    if (downgrade) {
        // Only support 4 -> 2 for now
        switch (frame.bpp) {
        case 1: updateScreenFn = &updateScreen1; break;
        case 2: updateScreenFn = &updateScreen2; break;
        case 4:
            updateScreenFn = &updateScreen42;
            targetBpp = 2;
            LOGD("Downgrading from 32bit to 16bit color");
            break;
        case 8: updateScreenFn = &updateScreen8; break;
        default:
            FATAL("Unsupported bpp: %d", frame.bpp);
        }
    } else {
        switch (frame.bpp) {
        case 1: updateScreenFn = &updateScreen1; break;
        case 2: updateScreenFn = &updateScreen2; break;
        case 4: updateScreenFn = &updateScreen4; break;
        case 8: updateScreenFn = &updateScreen8; break;
        default:
            FATAL("Unsupported bpp: %d", frame.bpp);
        }
    }

    initVncServer(argc, argv, frame.width, frame.height, frame.stride, targetBpp);

    unsigned int expectedFrameSize = frame.stride * frame.height * frame.bpp;
    if (expectedFrameSize != frame.size)
        FATAL("Unexpected frame size %d, expected %d", frame.size, expectedFrameSize);

    // Send the first frame
    (*updateScreenFn)(&frame, imageRotation);
    minicap->releaseConsumedFrame(&frame);

    LOGD("VNC server initialized");

    if (rhost) {
        rfbClientPtr cl;
        cl = rfbReverseConnection(vncscr, rhost, rport);
        if (cl == NULL) {
            LOGD("Couldn't connect to remote host");
        } else {
            LOGD("Starting on hold client");
            cl->onHold = FALSE;
            rfbStartOnHoldClient(cl);
        }
    }

    // long usec;
    rfbRunEventLoop(vncscr, -1, TRUE);

    int x, y, pending, err;

    while (1) {
        if (check_rotation_change(&screenRotation)) {
            LOGD("Screen rotation changed: %d", screenRotation);

            imageRotation = getImageRotation();

            // Stop old minicap instance
            minicap->setFrameAvailableListener(NULL);
            gWaiter->stop();
            minicap_free(minicap);
            delete gWaiter;

            // Wait for 0.1ms
            usleep(100000);

            // Create new minicap instance
            minicap = minicap_create(0);
            gWaiter = new FrameWaiter;
            minicap->setFrameAvailableListener(gWaiter);
            resetMinicapOrientation();            
            gWaiter->waitForFrame();
            minicap->consumePendingFrame(&frame);

            LOGD("Got first re-oriented frame: width=%d height=%d stride=%d bpp=%d", frame.width, frame.height, frame.stride, frame.bpp);

            unsigned int newFrameSize = frame.stride * frame.height * frame.bpp;
            if (frame.size != newFrameSize)
                FATAL("Unexpected frame size %d, expected %d", frame.size, newFrameSize);

            // Reinitialize VNC screen and frame buffer
            reinitVncServer(frame.width, frame.height, frame.stride, targetBpp, newFrameSize != expectedFrameSize);
            expectedFrameSize = newFrameSize;

            // Send the first frame
            (*updateScreenFn)(&frame, imageRotation);
            minicap->releaseConsumedFrame(&frame);
        }

        while (!gWaiter->isStopped() && (pending = gWaiter->getPendingFrames()) > 0) {
            if (skipFrames && pending > 1) {
                // Skip frames if we have too many. Not particularly thread safe,
                // but this loop should be the only consumer anyway (i.e. nothing
                // else decreases the frame count).
                gWaiter->reportExtraConsumption(pending - 1);

                while (--pending >= 1) {
                    if ((err = minicap->consumePendingFrame(&frame)) != 0) {
                        if (err == -EINTR) {
                            LOGE("Frame consumption interrupted by EINTR");
                        }
                        else {
                            LOGE("Unable to skip pending frame");
                        }
                    }
                    minicap->releaseConsumedFrame(&frame);
                }
            }

            if ((err = minicap->consumePendingFrame(&frame)) == 0) {
                (*updateScreenFn)(&frame, imageRotation);
            } else {
                if (err == -EINTR) {
                    LOGD("Frame consumption interrupted by EINTR");
                }
                else {
                    LOGE("Unable to consume pending frame");
                }
            }
            minicap->releaseConsumedFrame(&frame);
        }
    }

    LOGD("Finishing..");
    cleanup(0);
}

