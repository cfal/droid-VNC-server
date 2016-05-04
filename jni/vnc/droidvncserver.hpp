#ifndef DROIDVNCSERVER_HPP
#define DROIDVNCSERVER_HPP

#define VERSION "1.0.6"
#define LOG_TAG "droidvncserver"
#define DEBUG 1

#include <fcntl.h>
#include <linux/fb.h>
#include <time.h>
#include <android/log.h>

#include <cmath>
#include <chrono>
#include <mutex>
#include <condition_variable>

#include "rfb/rfb.h"
#include "Minicap.hpp"

extern Minicap::Frame frame;
extern rfbScreenInfoPtr vncscr;
extern unsigned char *cmpbuf;
extern unsigned char *vncbuf;

const char *getImageFormatName();
void cleanup(int);
void print(int logPriority, FILE* stream, const char *format, ...);

#define P(format, args...) print(-1, stdout, format , ##args)
#define LOGI(format, args...) print(ANDROID_LOG_INFO, stdout, format , ##args)
#define LOGE(format, args...) print(ANDROID_LOG_ERROR, stderr, format , ##args)
#if DEBUG
#define LOGD(format, args...) print(ANDROID_LOG_DEBUG, stdout, format , ##args)
#else
#define LOGD(format, args...)
#endif
#define FATAL(format, args...) do { LOGE(format, ##args); cleanup(1); } while (0)

#endif
