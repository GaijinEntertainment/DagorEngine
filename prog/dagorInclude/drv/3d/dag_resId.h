//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

//! \cond DETAIL
class D3dResource;
class BaseTexture;
class Sbuffer;
typedef BaseTexture Texture;
typedef BaseTexture CubeTexture;
typedef BaseTexture VolTexture;
typedef BaseTexture ArrayTexture;
//! \endcond

//! \brief An indirect identifier for a resource
//! \details Resource IDs are used to indirectly identify resources in the engine.
//! The indirection is then used for streaming and stubbing out resources which are not yet loaded.
//! E.g. binding SRVs to shaders is usually done through the shader var system, which in turn uses
//! resource IDs and not raw texture object pointers.
//! \note Consists of an index for an entry inside the resource manager and a generation counter
//! to distinguish between different resources stored within the same slot.
//! For most uses these details are of little importance.
class D3DRESID
{
public:
  //! \brief Constructs an invalid resource ID
  constexpr D3DRESID() = default;

  D3DRESID(const D3DRESID &rhs) = default;
  D3DRESID(D3DRESID &&rhs) = default;
  D3DRESID &operator=(const D3DRESID &) = default;
  D3DRESID &operator=(D3DRESID &&) = default;

  //! \brief Conversion from the underlying representation
  constexpr explicit D3DRESID(unsigned h) : handle(h) {}
  //! \brief Conversion to the underlying representation
  explicit operator unsigned() const { return handle; }
  //! \brief Checks validity of the handle (it still might be broken though)
  explicit operator bool() const { return handle != INVALID_ID; }


  //! \name Comparison operators
  //! \brief IDs are totally ordered w.r.t. the underlying representation to allow for use in ordered containers.
  //! No semantic meaning is attached to the ordering.
  //!@{
  bool operator==(const D3DRESID &rhs) const { return handle == rhs.handle; }
  bool operator!=(const D3DRESID &rhs) const { return handle != rhs.handle; }
  bool operator<(const D3DRESID &rhs) const { return handle < rhs.handle; }
  bool operator>(const D3DRESID &rhs) const { return handle > rhs.handle; }
  //!@}

  //! \brief Resets the resource ID to an invalid state
  void reset() { handle = INVALID_ID; }
  //! \brief Gets the entry index for this ID
  unsigned index() const { return handle >> GENERATION_BITS; }
  //! \brief Gets the entry value generation for this ID
  unsigned generation() const { return handle & GENERATION_MASK; }
  //! \brief Checks whether the ID is not broken (generation MUST be odd)
  bool checkMarkerBit() const { return generation() & 1; }

  //! \cond DETAIL
  enum : unsigned
  {
    INVALID_ID = 0u,
    INVALID_ID2 = ~1u,
    GENERATION_BITS = 12,
    GENERATION_MASK = (1 << GENERATION_BITS) - 1,
    GENERATION_BITS_START = 1,
    GENERATION_BITS_STEP = 2,
  };
  //! \endcond

  //! \brief Creates a new resource ID from an index and a generation
  //! \param index The resource manager entry index
  //! \param gen The generation counter for the resource
  //! \returns A new resource ID
  static inline D3DRESID make(unsigned index, unsigned gen) { return D3DRESID((index << GENERATION_BITS) | (gen & GENERATION_MASK)); }
  //! \brief Creates a new resource ID from an index
  //! \details The generation counter is acquired from the resource manager entry corresponding to the \p index.
  //! \param index The resource manager entry index
  //! \returns A new resource ID
  static D3DRESID fromIndex(unsigned index);

private:
  unsigned handle = INVALID_ID;
};

//! Alias for backward compatibility and readability
typedef D3DRESID TEXTUREID;


//! An invalid resource ID constant
#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
inline D3DRESID constexpr BAD_D3DRESID;
#else
static constexpr D3DRESID BAD_D3DRESID; //-V1043
#endif

//! Alias for backwards compatibility and readability
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
