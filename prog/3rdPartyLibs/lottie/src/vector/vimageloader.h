#ifndef VIMAGELOADER_H
#define VIMAGELOADER_H

#include "vbitmap.h"

V_BEGIN_NAMESPACE

class VImageLoader
{
public:
    static VImageLoader& instance()
    {
         static VImageLoader singleton;
         return singleton;
    }

    VBitmap load(const char *fileName);
    VBitmap load(const char *data, size_t len);
    ~VImageLoader();
private:
    VImageLoader();
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

V_END_NAMESPACE

#endif // VIMAGELOADER_H
