#include "droidvncserver.hpp"
#include "rotation_watcher.hpp"
#include <cstdlib>

static FILE* rotationWatcher = NULL;
static char rotateBuffer[40];

#define ROTATION_PKG_PATH "/data/local/tmp/rotate.apk"
#define ROTATION_PKG "jp.co.cyberagent.stf.rotationwatcher"
#define ROTATION_PKG_CLASS "jp.co.cyberagent.stf.rotationwatcher.RotationWatcher"

static int try_install_rotation_watcher() {
    FILE *f;
    char buf[1024];
    int i;
    
    if ((f = popen("pm install -r " ROTATION_PKG_PATH, "r")) == NULL) {
        LOGE("Could not initialize installation for rotation watcher");
        return 1;
    }

    memset(buf, 0x0, 1024);
    if (fgets(buf, 1023, f) == NULL) {
        LOGE("Could not install rotation watcher");
        return 1;
    }
    pclose(f);

    for (i = 0; i < 1024 && buf[i] && buf[i] != '\r' && buf[i] != '\n'; i++);
    buf[i] = 0;

    for (i = 0; i < 1024 && (buf[i] == '\r' || buf[i] == '\n'); i++);

    if (!strcmp(&buf[i], "Success")) {
        LOGD("Successfully installed rotation watcher.");
        return 0;
    } else {
        LOGE("Rotation watcher installation returned an error: %s", &buf[i]);
        return 1;
    }
}

static int _start_rotation_watcher(bool tryInstall) {
    int i, j;
    char buf[1024], cmd[4096];
    FILE *f;

    if (rotationWatcher) return 1;
    
    if ((f = popen("pm path " ROTATION_PKG, "r")) == NULL) {
        LOGE("Could not query rotation watcher path");
        return 1;
    }

    memset(buf, 0x0, 1024);
    memset(cmd, 0x0, 4096);

    fgets(buf, 1023, f);    
    pclose(f);
    
    for (i = 0; i < 1024 && buf[i] != ':'; i++);
    
    if (buf[i++] != ':' || i >= 1024) {
        if (tryInstall) {
            LOGD("Could not find rotation watcher path, trying install");
            if (try_install_rotation_watcher()) {
                LOGD("Install failed.");
                return 1;
            } else {
                return _start_rotation_watcher(false);
            }
        } else {
            LOGE("Could not find rotation watcher path");
            return 1;            
        }                
    }

    for (j = i; j < 1024 && buf[j] != '\0' && buf[j] != '\r' && buf[j] != '\n'; j++);
    if (j >= 1024) {
        LOGE("Could not parse rotation watcher path");
        return 1;
    }
    buf[j] = 0;

    LOGD("Found apk path: %s", &buf[i]);

    snprintf(cmd, 4095, "CLASSPATH=%s exec app_process /system/bin " ROTATION_PKG_CLASS, &buf[i]);
    if ((rotationWatcher = popen(cmd, "r")) == NULL) {
        LOGD("Could not start rotation watcher");
        return 1;
    }

    int fd = fileno(rotationWatcher);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

    memset(rotateBuffer, 0x0, 40);
    return 0;
}

int start_rotation_watcher() {
    return _start_rotation_watcher(true);
}

int stop_rotation_watcher() {
    if (!rotationWatcher) return 1;
    pclose(rotationWatcher);
    rotationWatcher = NULL;
    return 0;
}

bool check_rotation_change(int *rotation) {
    int i, j, len = strlen(rotateBuffer);
    bool changed = false;
    char *ret;
    
    while (true) {
        if (fgets(rotateBuffer + len, 39 - len, rotationWatcher) == NULL) {
            if (errno == EWOULDBLOCK) {
                return changed;
            }
            else {
                FATAL("Error reading rotation change");
            }
        }
        len = strlen(rotateBuffer);
        if (!rotateBuffer[0]) return changed;
        for (j = len - 1; j > 0 && rotateBuffer[j] != '\n'; j--);
        if (j <= 0) return changed;
        for (i = j - 1; i > 0 && rotateBuffer[i - 1] != '\n'; i--);
        rotateBuffer[j] = 0;
        j = atoi(&rotateBuffer[i]);
        if (j >= 0 && j <= 360) {
            *rotation = j;
            changed = true;
        }
        rotateBuffer[0] = 0;
        len = 0;
    }
    return changed;
}
