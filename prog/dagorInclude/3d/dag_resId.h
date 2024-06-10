//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

/**
 * @brief The file determines a class for creating and managing resource handle.
 */
#pragma once

class D3dResource;
class BaseTexture;
class Sbuffer;
typedef BaseTexture Texture;
typedef BaseTexture CubeTexture;
typedef BaseTexture VolTexture;
typedef BaseTexture ArrayTexture;

/**
 * @brief Encapsulates handle to a resource.
 */
class D3DRESID
{
public:
  /**
   * @brief Default constructor.
   */
  constexpr D3DRESID() = default;

  /**
   * @brief Copy constructor.
   * 
   * @param [in] rhs    The object to copy from.
   */
  D3DRESID(const D3DRESID &rhs) = default;

  /**
   * @brief Move constructor.
   * 
   * @param [in] The object to move from.
   */
  D3DRESID(D3DRESID &&rhs) = default;

  /**
   * @brief Initialization constructor.
   * 
   * @param [in] h  A handle to the resource
   */
  constexpr explicit D3DRESID(unsigned h) : handle(h) {}

  /**
   * @brief Copy-assignment operator.
   */
  D3DRESID &operator=(const D3DRESID &) = default;

  /**
   * @brief Returns the resource handle.
   * 
   * @return Handle to the resource.
   */
  explicit operator unsigned() const { return handle; }

  /**
   * @brief Checks whether the resource handle is valid.
   * 
   * @return \c true if the handle to the resource is valid, \c false otherwise.
   */
  explicit operator bool() const { return handle != INVALID_ID; }

  /**
   * @brief Compares two resource handles.
   * 
   * @param [in] rhs    Resource handle to compare with.
   * @return \c true if handles are identical, \c false otherwise.
   */
  bool operator==(const D3DRESID &rhs) const { return handle == rhs.handle; }

 /**
   * @brief Compares two resource handles.
   *
   * @param [in] rhs    Resource handle to compare with.
   * @return \c true if handles are different, \c false otherwise.
   */
  bool operator!=(const D3DRESID &rhs) const { return handle != rhs.handle; }

 /**
   * @brief Compares two resource handles.
   *
   * @param [in] rhs    Resource handle to compare with.
   * @return \c true if \c handle precedes \c rhs.handle, \c false otherwise.
   */
  bool operator<(const D3DRESID &rhs) const { return handle < rhs.handle; }

 /**
   * @brief Compares two resource handles.
   *
   * @param [in] rhs    Resource handle to compare with.
   * @return \c true if \c rhs.handle precedes \c handle, \c false otherwise.
   */
  bool operator>(const D3DRESID &rhs) const { return handle > rhs.handle; }

  /**
   * @brief Resets the resource handle.
   */
  void reset() { handle = INVALID_ID; }

  /**
   * @brief Retrives the resource index.
   * 
   * @return Index of the resource.
   */
  unsigned index() const { return handle >> GENERATION_BITS; }

  /**
   * @brief Retrives the resource generation.
   *
   * @return Generation of the resource.
   * 
   * Generation is used to differentiate between resources with identical ids. 
   * Only a single generation is valid for a given id.
   */
  unsigned generation() const { return handle & GENERATION_MASK; }
  bool checkMarkerBit() const { return generation() & 1; }

  /**
   * @brief Various compile-time constant for managing resource handles.
   */
  enum : unsigned
  {
    INVALID_ID = 0u,
    INVALID_ID2 = ~1u,
    GENERATION_BITS = 12,
    GENERATION_MASK = (1 << GENERATION_BITS) - 1,
    GENERATION_BITS_START = 1,
    GENERATION_BITS_STEP = 2,
  };

  /**
   * @brief Constructs \ref D3DRESID object (a handle to the resource) from by a given index and a generation.
   * 
   * @param [in] index      Resource index.
   * @param [in] generation Resource generation.
   * 
   * Generation is used to differentiate between resources with identical ids. 
   * Only a single generation is valid for a given id.
   */
  static inline D3DRESID make(unsigned index, unsigned gen) { return D3DRESID((index << GENERATION_BITS) | (gen & GENERATION_MASK)); }

  /**
   * 
   * @brief Constructs \ref D3DRESID object (a handle to the resource) by a given index.
   * 
   * @param [in] index Resource index.
   * 
   * The handle is constructed from the currently valid resource generation.
   */
  static D3DRESID fromIndex(unsigned index);

private:

   /**
    * @brief Resource handle.
    */
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
