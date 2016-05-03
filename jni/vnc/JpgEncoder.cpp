#include <stdexcept>

#include "JpgEncoder.hpp"
#include "droidvncserver.hpp"

JpgEncoder::JpgEncoder(unsigned int prePadding, unsigned int postPadding)
  : mTjHandle(tjInitCompress()),
    mSubsampling(TJSAMP_420),
    mPrePadding(prePadding),
    mPostPadding(postPadding),
    mMaxWidth(0),
    mMaxHeight(0),
    mEncodedData(NULL)    
{
}

JpgEncoder::~JpgEncoder() {
  tjFree(mEncodedData);
}

bool
JpgEncoder::encode(unsigned char *data, unsigned int width, unsigned int height, unsigned int stride, unsigned int bpp, Minicap::Format format, unsigned int quality) {
  unsigned char* offset = getEncodedData();

  return 0 == tjCompress2(
    mTjHandle,
    (unsigned char*) data,
    width,
    stride * bpp,
    height,
    convertFormat(format),
    &offset,
    &mEncodedSize,
    mSubsampling,
    quality,
    TJFLAG_FASTDCT | TJFLAG_NOREALLOC 
  );
}

int
JpgEncoder::getEncodedSize() {
  return mEncodedSize;
}

unsigned char*
JpgEncoder::getEncodedData() {
  return mEncodedData + mPrePadding;
}

bool
JpgEncoder::reserveData(uint32_t width, uint32_t height) {
  if (width == mMaxWidth && height == mMaxHeight) {
    return 0;
  }

  tjFree(mEncodedData);

  unsigned long maxSize = mPrePadding + mPostPadding + tjBufSize(
    width,
    height,
    mSubsampling
  );

  LOGD("Allocating %ld bytes for JPG encoder", maxSize);

  mEncodedData = tjAlloc(maxSize);

  if (mEncodedData == NULL) {
    return false;
  }

  mMaxWidth = width;
  mMaxHeight = height;

  return true;
}

int
JpgEncoder::convertFormat(Minicap::Format format) {
  switch (format) {
  case Minicap::FORMAT_RGBA_8888:
    return TJPF_RGBA;
  case Minicap::FORMAT_RGBX_8888:
    return TJPF_RGBX;
  case Minicap::FORMAT_RGB_888:
    return TJPF_RGB;
  case Minicap::FORMAT_BGRA_8888:
    return TJPF_BGRA;
  default:
    throw std::runtime_error("Unsupported pixel format");
  }
}

