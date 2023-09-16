//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class D3dResource;
class BaseTexture;
class Sbuffer;
typedef BaseTexture Texture;
typedef BaseTexture CubeTexture;
typedef BaseTexture VolTexture;
typedef BaseTexture ArrayTexture;

class D3DRESID
{
public:
  constexpr D3DRESID() = default;
  D3DRESID(const D3DRESID &rhs) = default;
  D3DRESID(D3DRESID &&rhs) = default;
  constexpr explicit D3DRESID(unsigned h) : handle(h) {}
  D3DRESID &operator=(const D3DRESID &) = default;
  explicit operator unsigned() const { return handle; }
  explicit operator bool() const { return handle != INVALID_ID; }
  bool operator==(const D3DRESID &rhs) const { return handle == rhs.handle; }
  bool operator!=(const D3DRESID &rhs) const { return handle != rhs.handle; }
  bool operator<(const D3DRESID &rhs) const { return handle < rhs.handle; }
  bool operator>(const D3DRESID &rhs) const { return handle > rhs.handle; }

  void reset() { handle = INVALID_ID; }
  unsigned index() const { return handle >> GENERATION_BITS; }
  unsigned generation() const { return handle & GENERATION_MASK; }
  bool checkMarkerBit() const { return generation() & 1; }

  enum : unsigned
  {
    INVALID_ID = 0u,
    INVALID_ID2 = ~1u,
    GENERATION_BITS = 12,
    GENERATION_MASK = (1 << GENERATION_BITS) - 1,
    GENERATION_BITS_START = 1,
    GENERATION_BITS_STEP = 2,
  };
  static inline D3DRESID make(unsigned index, unsigned gen) { return D3DRESID((index << GENERATION_BITS) | (gen & GENERATION_MASK)); }
  static D3DRESID fromIndex(unsigned index);

private:
  unsigned handle = INVALID_ID;
};

// alias for backward compatibility
typedef D3DRESID TEXTUREID;


#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
inline D3DRESID constexpr BAD_D3DRESID;
#else
static constexpr D3DRESID BAD_D3DRESID;  //-V1043
#endif

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
inline D3DRESID constexpr BAD_TEXTUREID;
#else
static constexpr D3DRESID BAD_TEXTUREID; //-V1043
#endif

template <typename T>
struct DebugConverter;
template <>
struct DebugConverter<D3DRESID>
{
  static unsigned getDebugType(const D3DRESID &v) { return unsigned(v); }
};
