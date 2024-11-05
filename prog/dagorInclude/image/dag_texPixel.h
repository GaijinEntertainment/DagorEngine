//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>

struct Color3;
class IMemAlloc;


// 32-bit BGRA pixels - compatible with E3DCOLOR
struct TexPixel32
{
  union
  {
#if defined(_TARGET_CPU_BE)
    struct
    {
      unsigned char a, r, g, b;
    };
#else
    struct
    {
      unsigned char b, g, r, a;
    };
#endif
    unsigned int u;
    unsigned int c;
  };
};

// 16-bit grayscale(luminance)+alpha
struct TexPixel8a
{
  union
  {
    struct
    {
      unsigned char l, a;
    };
    unsigned short u;
  };
};

// any type two-component pixel
template <typename T>
struct TexPixelRg
{
  T r, g;

  template <typename U>
  TexPixelRg &operator=(const TexPixelRg<U> &rhs)
  {
    r = rhs.r;
    g = rhs.g;
    return *this;
  }
};

// PixelRg - PixelRg operators

template <typename T, typename U>
auto operator*(const TexPixelRg<T> &lhs, const TexPixelRg<U> &rhs) -> TexPixelRg<decltype(eastl::declval<T>() * eastl::declval<U>())>
{
  return TexPixelRg<decltype(eastl::declval<T>() * eastl::declval<U>())>{lhs.r * rhs.r, lhs.g * rhs.g};
}

template <typename T, typename U>
auto operator/(const TexPixelRg<T> &lhs, const TexPixelRg<U> &rhs) -> TexPixelRg<decltype(eastl::declval<T>() / eastl::declval<U>())>
{
  return TexPixelRg<decltype(eastl::declval<T>() / eastl::declval<U>())>{lhs.r / rhs.r, lhs.g / rhs.g};
}

template <typename T, typename U>
auto operator+(const TexPixelRg<T> &lhs, const TexPixelRg<U> &rhs) -> TexPixelRg<decltype(eastl::declval<T>() + eastl::declval<U>())>
{
  return TexPixelRg<decltype(eastl::declval<T>() + eastl::declval<U>())>{lhs.r + rhs.r, lhs.g + rhs.g};
}

template <typename T, typename U>
auto operator-(const TexPixelRg<T> &lhs, const TexPixelRg<U> &rhs) -> TexPixelRg<decltype(eastl::declval<T>() - eastl::declval<U>())>
{
  return TexPixelRg<decltype(eastl::declval<T>() - eastl::declval<U>())>{lhs.r - rhs.r, lhs.g - rhs.g};
}

// PixelRg - any operators

template <typename T, typename U>
auto operator*(const TexPixelRg<T> &lhs, const U &rhs) -> TexPixelRg<decltype(eastl::declval<T>() * eastl::declval<U>())>
{
  return TexPixelRg<decltype(eastl::declval<T>() * eastl::declval<U>())>{lhs.r * rhs, lhs.g * rhs};
}

template <typename T, typename U>
auto operator/(const TexPixelRg<T> &lhs, const U &rhs) -> TexPixelRg<decltype(eastl::declval<T>() / eastl::declval<U>())>
{
  return TexPixelRg<decltype(eastl::declval<T>() / eastl::declval<U>())>{lhs.r / rhs, lhs.g / rhs};
}

template <typename T, typename U>
auto operator+(const TexPixelRg<T> &lhs, const U &rhs) -> TexPixelRg<decltype(eastl::declval<T>() + eastl::declval<U>())>
{
  return TexPixelRg<decltype(eastl::declval<T>() + eastl::declval<U>())>{lhs.r + rhs, lhs.g + rhs};
}

template <typename T, typename U>
auto operator-(const TexPixelRg<T> &lhs, const U &rhs) -> TexPixelRg<decltype(eastl::declval<T>() - eastl::declval<U>())>
{
  return TexPixelRg<decltype(eastl::declval<T>() - eastl::declval<U>())>{lhs.r - rhs, lhs.g - rhs};
}

// Tex image quasi-format
struct TexImage
{
  short w, h;
  // pixel data follows (depends on image format)
};


// Tex image formats for different pixel formats
struct TexImage32 : public TexImage
{
  inline TexPixel32 *getPixels() { return (TexPixel32 *)(this + 1); }
  static TexImage32 *create(int w, int h, IMemAlloc *mem = NULL);
  static TexImage32 *tryCreate(int w, int h, IMemAlloc *mem = NULL); // Returns nullptr if tryAlloc() failed
};

struct TexImage8a : public TexImage
{
  inline TexPixel8a *getPixels() { return (TexPixel8a *)(this + 1); }
  static TexImage8a *create(int w, int h, IMemAlloc *mem = NULL);
};

struct TexImage8 : public TexImage // uint8_t pixels
{
  inline unsigned char *getPixels() { return (unsigned char *)(this + 1); }
  static TexImage8 *create(int w, int h, IMemAlloc *mem = NULL);
};

struct TexImageR : public TexImage // real pixels
{
  inline float *getPixels() { return (float *)(this + 1); }
  static TexImageR *create(int w, int h, IMemAlloc *mem = NULL);
};

struct TexImageF : public TexImage // Color3 pixels
{
  inline Color3 *getPixels() { return (Color3 *)(this + 1); }
  static TexImageF *create(int w, int h, IMemAlloc *mem = NULL);
};

template <typename T>
struct TexImageAny : public TexImage // any pixel
{
  T *getPixels() { return (T *)(this + 1); }

  const T *getPixels() const { return (T *)(this + 1); }

  static TexImageAny<T> *create(int w, int h, IMemAlloc *mem = NULL)
  {
    if (!mem)
      mem = tmpmem;
    TexImageAny<T> *im = (TexImageAny<T> *)mem->alloc(w * h * sizeof(T) + sizeof(TexImageAny<T>));
    im->w = w;
    im->h = h;
    return im;
  }

  T &operator()(size_t x, size_t y) { return getPixels()[y * w + x]; }

  const T &operator()(size_t x, size_t y) const { return getPixels()[y * w + x]; }
};

// High-Dynamic-Range image info
struct HdrImageInfo
{
  float exposure;
  float gamma;
};
