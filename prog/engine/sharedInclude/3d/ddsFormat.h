// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// DDS format structures and bits for XBOX

/*==========================================================================;
 *
 *  Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddraw.h
 *  Content:    DirectDraw include file
 *
 ***************************************************************************/

#include <supp/_platform.h>
#include <util/dag_stdint.h>

#ifdef __GNUC__
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef int32_t LONG;
typedef void *LPVOID;
#endif

/*
 * DDPIXELFORMAT
 */
typedef struct _DDPIXELFORMAT
{
  DWORD dwSize;   // size of structure
  DWORD dwFlags;  // pixel format flags
  DWORD dwFourCC; // (FOURCC code)
  union
  {
    DWORD dwRGBBitCount;           // how many bits per pixel
    DWORD dwYUVBitCount;           // how many bits per pixel
    DWORD dwZBufferBitDepth;       // how many total bits/pixel in z buffer (including any stencil bits)
    DWORD dwAlphaBitDepth;         // how many bits for alpha channels
    DWORD dwLuminanceBitCount;     // how many bits per pixel
    DWORD dwBumpBitCount;          // how many bits per "buxel", total
    DWORD dwPrivateFormatBitCount; // Bits per pixel of private driver formats. Only valid in texture
                                   // format list and if DDPF_D3DFORMAT is set
  };
  union
  {
    DWORD dwRBitMask;         // mask for red bit
    DWORD dwYBitMask;         // mask for Y bits
    DWORD dwStencilBitDepth;  // how many stencil bits (note: dwZBufferBitDepth-dwStencilBitDepth is total Z-only bits)
    DWORD dwLuminanceBitMask; // mask for luminance bits
    DWORD dwBumpDuBitMask;    // mask for bump map U delta bits
    DWORD dwOperations;       // DDPF_D3DFORMAT Operations
  };
  union
  {
    DWORD dwGBitMask;      // mask for green bits
    DWORD dwUBitMask;      // mask for U bits
    DWORD dwZBitMask;      // mask for Z bits
    DWORD dwBumpDvBitMask; // mask for bump map V delta bits
    struct
    {
      WORD wFlipMSTypes; // Multisample methods supported via flip for this D3DFORMAT
      WORD wBltMSTypes;  // Multisample methods supported via blt for this D3DFORMAT
    } MultiSampleCaps;
  };
  union
  {
    DWORD dwBBitMask;             // mask for blue bits
    DWORD dwVBitMask;             // mask for V bits
    DWORD dwStencilBitMask;       // mask for stencil bits
    DWORD dwBumpLuminanceBitMask; // mask for luminance in bump map
  };
  union
  {
    DWORD dwRGBAlphaBitMask;       // mask for alpha channel
    DWORD dwYUVAlphaBitMask;       // mask for alpha channel
    DWORD dwLuminanceAlphaBitMask; // mask for alpha channel
    DWORD dwRGBZBitMask;           // mask for Z channel
    DWORD dwYUVZBitMask;           // mask for Z channel
  };
} DDPIXELFORMAT;


/*
 * DDCOLORKEY
 */
typedef struct _DDCOLORKEY
{
  DWORD dwColorSpaceLowValue;  // low boundary of color space that is to
                               // be treated as Color Key, inclusive
  DWORD dwColorSpaceHighValue; // high boundary of color space that is
                               // to be treated as Color Key, inclusive
} DDCOLORKEY;


/*
 * DDSCAPS2
 */
typedef struct _DDSCAPS2
{
  DWORD dwCaps; // capabilities of surface wanted
  DWORD dwCaps2;
  DWORD dwCaps3;
  union
  {
    DWORD dwCaps4;
    DWORD dwVolumeDepth;
  };
} DDSCAPS2;


/*
 * DDSURFACEDESC2
 */
typedef struct _DDSURFACEDESC2
{
  DWORD dwSize;   // size of the DDSURFACEDESC structure
  DWORD dwFlags;  // determines what fields are valid
  DWORD dwHeight; // height of surface to be created
  DWORD dwWidth;  // width of input surface
  union
  {
    LONG lPitch;        // distance to start of next line (return value only)
    DWORD dwLinearSize; // Formless late-allocated optimized surface size
  };
  union
  {
    DWORD dwBackBufferCount; // number of back buffers requested
    DWORD dwDepth;           // the depth if this is a volume texture
  };
  union
  {
    DWORD dwMipMapCount; // number of mip-map levels requestde
                         // dwZBufferBitDepth removed, use ddpfPixelFormat one instead
    DWORD dwRefreshRate; // refresh rate (used when display mode is described)
    DWORD dwSrcVBHandle; // The source used in VB::Optimize
  };
  DWORD dwAlphaBitDepth;      // depth of alpha buffer requested
  DWORD dwReserved;           // reserved
  DWORD /*LPVOID*/ lpSurface; // pointer to the associated surface memory
  union
  {
    DDCOLORKEY ddckCKDestOverlay; // color key for destination overlay use
    DWORD dwEmptyFaceColor;       // Physical color for empty cubemap faces
  };
  DDCOLORKEY ddckCKDestBlt;    // color key for destination blt use
  DDCOLORKEY ddckCKSrcOverlay; // color key for source overlay use
  DDCOLORKEY ddckCKSrcBlt;     // color key for source blt use
  union
  {
    DDPIXELFORMAT ddpfPixelFormat; // pixel format description of the surface
    DWORD dwFVF;                   // vertex format description of vertex buffers
  };
  DDSCAPS2 ddsCaps;     // direct draw surface capabilities
  DWORD dwTextureStage; // stage in multitexture cascade
} DDSURFACEDESC2;

/*
 * ddsCaps field is valid.
 */
#define DDSD_CAPS 0x00000001l // default

/*
 * dwHeight field is valid.
 */
#define DDSD_HEIGHT 0x00000002l

/*
 * dwWidth field is valid.
 */
#define DDSD_WIDTH 0x00000004l

/*
 * lPitch is valid.
 */
#define DDSD_PITCH 0x00000008l

/*
 * dwBackBufferCount is valid.
 */
#define DDSD_BACKBUFFERCOUNT 0x00000020l

/*
 * dwZBufferBitDepth is valid.  (shouldnt be used in DDSURFACEDESC2)
 */
#define DDSD_ZBUFFERBITDEPTH 0x00000040l

/*
 * dwAlphaBitDepth is valid.
 */
#define DDSD_ALPHABITDEPTH 0x00000080l


/*
 * lpSurface is valid.
 */
#define DDSD_LPSURFACE 0x00000800l

/*
 * ddpfPixelFormat is valid.
 */
#define DDSD_PIXELFORMAT 0x00001000l

/*
 * ddckCKDestOverlay is valid.
 */
#define DDSD_CKDESTOVERLAY 0x00002000l

/*
 * ddckCKDestBlt is valid.
 */
#define DDSD_CKDESTBLT 0x00004000l

/*
 * ddckCKSrcOverlay is valid.
 */
#define DDSD_CKSRCOVERLAY 0x00008000l

/*
 * ddckCKSrcBlt is valid.
 */
#define DDSD_CKSRCBLT 0x00010000l

/*
 * dwMipMapCount is valid.
 */
#define DDSD_MIPMAPCOUNT 0x00020000l

/*
 * dwRefreshRate is valid
 */
#define DDSD_REFRESHRATE 0x00040000l

/*
 * dwLinearSize is valid
 */
#define DDSD_LINEARSIZE 0x00080000l

/*
 * dwTextureStage is valid
 */
#define DDSD_TEXTURESTAGE 0x00100000l
/*
 * dwFVF is valid
 */
#define DDSD_FVF          0x00200000l
/*
 * dwSrcVBHandle is valid
 */
#define DDSD_SRCVBHANDLE  0x00400000l

/*
 * dwDepth is valid
 */
#define DDSD_DEPTH 0x00800000l

/*
 * All input fields are valid.
 */
#define DDSD_ALL 0x00fff9eel

/*
 * This bit is reserved
 */
#define DDSCAPS2_RESERVED4           0x00000002L
#define DDSCAPS2_HARDWAREDEINTERLACE 0x00000000L

/*
 * Indicates to the driver that this surface will be locked very frequently
 * (for procedural textures, dynamic lightmaps, etc). Surfaces with this cap
 * set must also have DDSCAPS_TEXTURE. This cap cannot be used with
 * DDSCAPS2_HINTSTATIC and DDSCAPS2_OPAQUE.
 */
#define DDSCAPS2_HINTDYNAMIC 0x00000004L

/*
 * Indicates to the driver that this surface can be re-ordered/retiled on
 * load. This operation will not change the size of the texture. It is
 * relatively fast and symmetrical, since the application may lock these
 * bits (although it will take a performance hit when doing so). Surfaces
 * with this cap set must also have DDSCAPS_TEXTURE. This cap cannot be
 * used with DDSCAPS2_HINTDYNAMIC and DDSCAPS2_OPAQUE.
 */
#define DDSCAPS2_HINTSTATIC 0x00000008L

/*
 * Indicates that the client would like this texture surface to be managed by the
 * DirectDraw/Direct3D runtime. Surfaces with this cap set must also have
 * DDSCAPS_TEXTURE set.
 */
#define DDSCAPS2_TEXTUREMANAGE 0x00000010L

/*
 * These bits are reserved for internal use */
#define DDSCAPS2_RESERVED1 0x00000020L
#define DDSCAPS2_RESERVED2 0x00000040L

/*
 * Indicates to the driver that this surface will never be locked again.
 * The driver is free to optimize this surface via retiling and actual compression.
 * All calls to Lock() or Blts from this surface will fail. Surfaces with this
 * cap set must also have DDSCAPS_TEXTURE. This cap cannot be used with
 * DDSCAPS2_HINTDYNAMIC and DDSCAPS2_HINTSTATIC.
 */
#define DDSCAPS2_OPAQUE 0x00000080L

/*
 * Applications should set this bit at CreateSurface time to indicate that they
 * intend to use antialiasing. Only valid if DDSCAPS_3DDEVICE is also set.
 */
#define DDSCAPS2_HINTANTIALIASING 0x00000100L


/*
 * This flag is used at CreateSurface time to indicate that this set of
 * surfaces is a cubic environment map
 */
#define DDSCAPS2_CUBEMAP 0x00000200L

/*
 * These flags preform two functions:
 * - At CreateSurface time, they define which of the six cube faces are
 *   required by the application.
 * - After creation, each face in the cubemap will have exactly one of these
 *   bits set.
 */
#define DDSCAPS2_CUBEMAP_POSITIVEX 0x00000400L
#define DDSCAPS2_CUBEMAP_NEGATIVEX 0x00000800L
#define DDSCAPS2_CUBEMAP_POSITIVEY 0x00001000L
#define DDSCAPS2_CUBEMAP_NEGATIVEY 0x00002000L
#define DDSCAPS2_CUBEMAP_POSITIVEZ 0x00004000L
#define DDSCAPS2_CUBEMAP_NEGATIVEZ 0x00008000L

/*
 * This macro may be used to specify all faces of a cube map at CreateSurface time
 */
#define DDSCAPS2_CUBEMAP_ALLFACES                                                                                      \
  (DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX | DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGATIVEY | \
    DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ)


/*
 * This flag is an additional flag which is present on mipmap sublevels from DX7 onwards
 * It enables easier use of GetAttachedSurface rather than EnumAttachedSurfaces for surface
 * constructs such as Cube Maps, wherein there are more than one mipmap surface attached
 * to the root surface.
 * This caps bit is ignored by CreateSurface
 */
#define DDSCAPS2_MIPMAPSUBLEVEL 0x00010000L

/* This flag indicates that the texture should be managed by D3D only */
#define DDSCAPS2_D3DTEXTUREMANAGE 0x00020000L

/* This flag indicates that the managed surface can be safely lost */
#define DDSCAPS2_DONOTPERSIST 0x00040000L

/* indicates that this surface is part of a stereo flipping chain */
#define DDSCAPS2_STEREOSURFACELEFT 0x00080000L


/*
 * Indicates that the surface is a volume.
 * Can be combined with DDSCAPS_MIPMAP to indicate a multi-level volume
 */
#define DDSCAPS2_VOLUME 0x00200000L

/*
 * Indicates that the surface may be locked multiple times by the application.
 * This cap cannot be used with DDSCAPS2_OPAQUE.
 */
#define DDSCAPS2_NOTUSERLOCKABLE 0x00400000L

/*
 * Indicates that the vertex buffer data can be used to render points and
 * point sprites.
 */
#define DDSCAPS2_POINTS 0x00800000L

/*
 * Indicates that the vertex buffer data can be used to render rt pactches.
 */
#define DDSCAPS2_RTPATCHES 0x01000000L

/*
 * Indicates that the vertex buffer data can be used to render n patches.
 */
#define DDSCAPS2_NPATCHES 0x02000000L

/*
 * This bit is reserved for internal use
 */
#define DDSCAPS2_RESERVED3 0x04000000L


/*
 * Indicates that the contents of the backbuffer do not have to be preserved
 * the contents of the backbuffer after they are presented.
 */
#define DDSCAPS2_DISCARDBACKBUFFER 0x10000000L

/*
 * Indicates that all surfaces in this creation chain should be given an alpha channel.
 * This flag will be set on primary surface chains that may have no explicit pixel format
 * (and thus take on the format of the current display mode).
 * The driver should infer that all these surfaces have a format having an alpha channel.
 * (e.g. assume D3DFMT_A8R8G8B8 if the display mode is x888.)
 */
#define DDSCAPS2_ENABLEALPHACHANNEL 0x20000000L

/*
 * Indicates that all surfaces in this creation chain is extended primary surface format.
 * This flag will be set on extended primary surface chains that always have explicit pixel
 * format and the pixel format is typically GDI (Graphics Device Interface) couldn't handle,
 * thus only used with fullscreen application. (e.g. D3DFMT_A2R10G10B10 format)
 */
#define DDSCAPS2_EXTENDEDFORMATPRIMARY 0x40000000L

/*
 * Indicates that all surfaces in this creation chain is additional primary surface.
 * This flag will be set on primary surface chains which must present on the adapter
 * id provided on dwCaps4. Typically this will be used to create secondary primary surface
 * on DualView display adapter.
 */
#define DDSCAPS2_ADDITIONALPRIMARY 0x80000000L


/****************************************************************************
 *
 * DIRECTDRAW PIXELFORMAT FLAGS
 *
 ****************************************************************************/

/*
 * The surface has alpha channel information in the pixel format.
 */
#define DDPF_ALPHAPIXELS 0x00000001l

/*
 * The pixel format contains alpha only information
 */
#define DDPF_ALPHA 0x00000002l

/*
 * The FourCC code is valid.
 */
#define DDPF_FOURCC 0x00000004l

/*
 * The surface is 4-bit color indexed.
 */
#define DDPF_PALETTEINDEXED4 0x00000008l

/*
 * The surface is indexed into a palette which stores indices
 * into the destination surface's 8-bit palette.
 */
#define DDPF_PALETTEINDEXEDTO8 0x00000010l

/*
 * The surface is 8-bit color indexed.
 */
#define DDPF_PALETTEINDEXED8 0x00000020l

/*
 * The RGB data in the pixel format structure is valid.
 */
#define DDPF_RGB 0x00000040l

/*
 * The surface will accept pixel data in the format specified
 * and compress it during the write.
 */
#define DDPF_COMPRESSED 0x00000080l

/*
 * The surface will accept RGB data and translate it during
 * the write to YUV data.  The format of the data to be written
 * will be contained in the pixel format structure.  The DDPF_RGB
 * flag will be set.
 */
#define DDPF_RGBTOYUV 0x00000100l

/*
 * pixel format is YUV - YUV data in pixel format struct is valid
 */
#define DDPF_YUV 0x00000200l

/*
 * pixel format is a z buffer only surface
 */
#define DDPF_ZBUFFER 0x00000400l

/*
 * The surface is 1-bit color indexed.
 */
#define DDPF_PALETTEINDEXED1 0x00000800l

/*
 * The surface is 2-bit color indexed.
 */
#define DDPF_PALETTEINDEXED2 0x00001000l

/*
 * The surface contains Z information in the pixels
 */
#define DDPF_ZPIXELS 0x00002000l

/*
 * The surface contains stencil information along with Z
 */
#define DDPF_STENCILBUFFER 0x00004000l

/*
 * Premultiplied alpha format -- the color components have been
 * premultiplied by the alpha component.
 */
#define DDPF_ALPHAPREMULT 0x00008000l


/*
 * Luminance data in the pixel format is valid.
 * Use this flag for luminance-only or luminance+alpha surfaces,
 * the bit depth is then ddpf.dwLuminanceBitCount.
 */
#define DDPF_LUMINANCE 0x00020000l

/*
 * Luminance data in the pixel format is valid.
 * Use this flag when hanging luminance off bumpmap surfaces,
 * the bit mask for the luminance portion of the pixel is then
 * ddpf.dwBumpLuminanceBitMask
 */
#define DDPF_BUMPLUMINANCE 0x00040000l

/*
 * Bump map dUdV data in the pixel format is valid.
 */
#define DDPF_BUMPDUDV 0x00080000l


/*
 * Indicates that this surface can be used as a 3D texture.  It does not
 * indicate whether or not the surface is being used for that purpose.
 */
#define DDSCAPS_TEXTURE 0x00001000l


#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
  ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#endif /* defined(MAKEFOURCC) */

#define FOURCC_DXT1  (MAKEFOURCC('D', 'X', 'T', '1'))
#define FOURCC_DXT2  (MAKEFOURCC('D', 'X', 'T', '2'))
#define FOURCC_DXT3  (MAKEFOURCC('D', 'X', 'T', '3'))
#define FOURCC_DXT4  (MAKEFOURCC('D', 'X', 'T', '4'))
#define FOURCC_DXT5  (MAKEFOURCC('D', 'X', 'T', '5'))
#define FOURCC_ATI1N (MAKEFOURCC('A', 'T', 'I', '1'))
#define FOURCC_ATI2N (MAKEFOURCC('A', 'T', 'I', '2'))

#define FOURCC_DX10 (MAKEFOURCC('D', 'X', '1', '0'))

typedef struct _DDSHDR_DXT10
{
  enum
  {
    RESOURCE_DIMENSION_UNKNOWN = 0,
    RESOURCE_DIMENSION_BUFFER = 1,
    RESOURCE_DIMENSION_TEXTURE1D = 2,
    RESOURCE_DIMENSION_TEXTURE2D = 3,
    RESOURCE_DIMENSION_TEXTURE3D = 4
  };

  DWORD dxgiFormat;
  DWORD resourceDimension;
  DWORD miscFlag;
  DWORD arraySize;
  DWORD miscFlags2;
} DDSHDR_DXT10;

#if _TARGET_PC_WIN
#include <d3d9types.h>
#include <DXGIFormat.h>
#else
#if _TARGET_XBOX
#include <DXGIFormat.h>
#endif
enum D3DFORMAT
{
  D3DFMT_UNKNOWN = 0,
  D3DFMT_R8G8B8 = 20,
  D3DFMT_A8R8G8B8 = 21,
  D3DFMT_X8R8G8B8 = 22,
  D3DFMT_R5G6B5 = 23,
  D3DFMT_X1R5G5B5 = 24,
  D3DFMT_A1R5G5B5 = 25,
  D3DFMT_A4R4G4B4 = 26,
  D3DFMT_R3G3B2 = 27,
  D3DFMT_A8 = 28,
  D3DFMT_A8R3G3B2 = 29,
  D3DFMT_X4R4G4B4 = 30,
  D3DFMT_A2B10G10R10 = 31,
  D3DFMT_A8B8G8R8 = 32,
  D3DFMT_X8B8G8R8 = 33,
  D3DFMT_G16R16 = 34,
  D3DFMT_A2R10G10B10 = 35,
  D3DFMT_A16B16G16R16 = 36,
  D3DFMT_L8 = 50,
  D3DFMT_A8L8 = 51,
  D3DFMT_V8U8 = 60,
  D3DFMT_DXT1 = FOURCC_DXT1,
  D3DFMT_DXT2 = FOURCC_DXT2,
  D3DFMT_DXT3 = FOURCC_DXT3,
  D3DFMT_DXT4 = FOURCC_DXT4,
  D3DFMT_DXT5 = FOURCC_DXT5,
  D3DFMT_L16 = 81,
  D3DFMT_R16F = 111,
  D3DFMT_G16R16F = 112,
  D3DFMT_A16B16G16R16F = 113,
  D3DFMT_R32F = 114,
  D3DFMT_G32R32F = 115,
  D3DFMT_A32B32G32R32F = 116,

#if !_TARGET_XBOX
  DXGI_FORMAT_UNKNOWN = 0,
  DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
  DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
  DXGI_FORMAT_R32G32B32A32_UINT = 3,
  DXGI_FORMAT_R32G32B32A32_SINT = 4,
  DXGI_FORMAT_R32G32B32_TYPELESS = 5,
  DXGI_FORMAT_R32G32B32_FLOAT = 6,
  DXGI_FORMAT_R32G32B32_UINT = 7,
  DXGI_FORMAT_R32G32B32_SINT = 8,
  DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
  DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
  DXGI_FORMAT_R16G16B16A16_UNORM = 11,
  DXGI_FORMAT_R16G16B16A16_UINT = 12,
  DXGI_FORMAT_R16G16B16A16_SNORM = 13,
  DXGI_FORMAT_R16G16B16A16_SINT = 14,
  DXGI_FORMAT_R32G32_TYPELESS = 15,
  DXGI_FORMAT_R32G32_FLOAT = 16,
  DXGI_FORMAT_R32G32_UINT = 17,
  DXGI_FORMAT_R32G32_SINT = 18,
  DXGI_FORMAT_R32G8X24_TYPELESS = 19,
  DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
  DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
  DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
  DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
  DXGI_FORMAT_R10G10B10A2_UNORM = 24,
  DXGI_FORMAT_R10G10B10A2_UINT = 25,
  DXGI_FORMAT_R11G11B10_FLOAT = 26,
  DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
  DXGI_FORMAT_R8G8B8A8_UNORM = 28,
  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
  DXGI_FORMAT_R8G8B8A8_UINT = 30,
  DXGI_FORMAT_R8G8B8A8_SNORM = 31,
  DXGI_FORMAT_R8G8B8A8_SINT = 32,
  DXGI_FORMAT_R16G16_TYPELESS = 33,
  DXGI_FORMAT_R16G16_FLOAT = 34,
  DXGI_FORMAT_R16G16_UNORM = 35,
  DXGI_FORMAT_R16G16_UINT = 36,
  DXGI_FORMAT_R16G16_SNORM = 37,
  DXGI_FORMAT_R16G16_SINT = 38,
  DXGI_FORMAT_R32_TYPELESS = 39,
  DXGI_FORMAT_D32_FLOAT = 40,
  DXGI_FORMAT_R32_FLOAT = 41,
  DXGI_FORMAT_R32_UINT = 42,
  DXGI_FORMAT_R32_SINT = 43,
  DXGI_FORMAT_R24G8_TYPELESS = 44,
  DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
  DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
  DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
  DXGI_FORMAT_R8G8_TYPELESS = 48,
  DXGI_FORMAT_R8G8_UNORM = 49,
  DXGI_FORMAT_R8G8_UINT = 50,
  DXGI_FORMAT_R8G8_SNORM = 51,
  DXGI_FORMAT_R8G8_SINT = 52,
  DXGI_FORMAT_R16_TYPELESS = 53,
  DXGI_FORMAT_R16_FLOAT = 54,
  DXGI_FORMAT_D16_UNORM = 55,
  DXGI_FORMAT_R16_UNORM = 56,
  DXGI_FORMAT_R16_UINT = 57,
  DXGI_FORMAT_R16_SNORM = 58,
  DXGI_FORMAT_R16_SINT = 59,
  DXGI_FORMAT_R8_TYPELESS = 60,
  DXGI_FORMAT_R8_UNORM = 61,
  DXGI_FORMAT_R8_UINT = 62,
  DXGI_FORMAT_R8_SNORM = 63,
  DXGI_FORMAT_R8_SINT = 64,
  DXGI_FORMAT_A8_UNORM = 65,
  DXGI_FORMAT_R1_UNORM = 66,
  DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
  DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
  DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
  DXGI_FORMAT_BC1_TYPELESS = 70,
  DXGI_FORMAT_BC1_UNORM = 71,
  DXGI_FORMAT_BC1_UNORM_SRGB = 72,
  DXGI_FORMAT_BC2_TYPELESS = 73,
  DXGI_FORMAT_BC2_UNORM = 74,
  DXGI_FORMAT_BC2_UNORM_SRGB = 75,
  DXGI_FORMAT_BC3_TYPELESS = 76,
  DXGI_FORMAT_BC3_UNORM = 77,
  DXGI_FORMAT_BC3_UNORM_SRGB = 78,
  DXGI_FORMAT_BC4_TYPELESS = 79,
  DXGI_FORMAT_BC4_UNORM = 80,
  DXGI_FORMAT_BC4_SNORM = 81,
  DXGI_FORMAT_BC5_TYPELESS = 82,
  DXGI_FORMAT_BC5_UNORM = 83,
  DXGI_FORMAT_BC5_SNORM = 84,
  DXGI_FORMAT_B5G6R5_UNORM = 85,
  DXGI_FORMAT_B5G5R5A1_UNORM = 86,
  DXGI_FORMAT_B8G8R8A8_UNORM = 87,
  DXGI_FORMAT_B8G8R8X8_UNORM = 88,
  DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
  DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
  DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
  DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
  DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
  DXGI_FORMAT_BC6H_TYPELESS = 94,
  DXGI_FORMAT_BC6H_UF16 = 95,
  DXGI_FORMAT_BC6H_SF16 = 96,
  DXGI_FORMAT_BC7_TYPELESS = 97,
  DXGI_FORMAT_BC7_UNORM = 98,
  DXGI_FORMAT_BC7_UNORM_SRGB = 99,
  DXGI_FORMAT_AYUV = 100,
  DXGI_FORMAT_Y410 = 101,
  DXGI_FORMAT_Y416 = 102,
  DXGI_FORMAT_NV12 = 103,
  DXGI_FORMAT_P010 = 104,
  DXGI_FORMAT_P016 = 105,
  DXGI_FORMAT_420_OPAQUE = 106,
  DXGI_FORMAT_YUY2 = 107,
  DXGI_FORMAT_Y210 = 108,
  DXGI_FORMAT_Y216 = 109,
  DXGI_FORMAT_NV11 = 110,
  DXGI_FORMAT_AI44 = 111,
  DXGI_FORMAT_IA44 = 112,
  DXGI_FORMAT_P8 = 113,
  DXGI_FORMAT_A8P8 = 114,
  DXGI_FORMAT_B4G4R4A4_UNORM = 115,
#endif
};
#endif

enum
{
  D3DFMT_R4G4B4A4 = D3DFMT_A4R4G4B4 + 0x80,
  D3DFMT_R5G5B5A1 = D3DFMT_A1R5G5B5 + 0x80,
};
