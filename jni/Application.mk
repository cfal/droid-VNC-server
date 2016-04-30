NDK_TOOLCHAIN_VERSION=4.9
APP_CPPFLAGS += -std=c++11 -fexceptions
APP_STL := gnustl_static

APP_CFLAGS += \
	-Ofast \
	-funroll-loops \
	-fno-strict-aliasing

# APP_LDFLAGS += -lm_hard		

APP_CFLAGS += \
	-march=armv7-a \
	-mfpu=neon \
	-mfloat-abi=softfp \
	-marm \
	-fprefetch-loop-arrays \
	-DHAVE_NEON=1

APP_ABI:=armeabi-v7a 
APP_OPTIM := debug
APP_PLATFORM := android-18
#APP_MODULES:= jpeg libpng libcrypto_static libssl_static androidvncserver
