//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_texMgr.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_DObject.h>

// BC(DXT) texure compressor
// current format support:
//   BC1/DXT1, 64 bits per block, 4 bit per texel, no alpha
//   BC3/DXT5, 128 bits per block, 8 bit per texel, smooth alpha
//   BC5/ATI2N, 128 bits per block, 8 bit per texel, two components
// hot to use:
//   BcCompressor compr( BcCompressor::COMPRESSION_BC1, 128, 128, "bc_compressor" );
//   compr.update( src_tex_id, dest_tex, 0, 0 );
//   compr.update( src_tex_id, dest_tex, 128, 0 );
//   ...
class Sbuffer;
typedef Sbuffer Vbuffer;
class ShaderMaterial;
class ShaderElement;
class ComputeShaderElement;
class BaseTexture;
typedef BaseTexture Texture;

class BcCompressor
{
public:
  enum ECompressionType
  {
    COMPRESSION_BC1, // DXT1
    COMPRESSION_BC3, // DXT5
    COMPRESSION_BC4, // ATI1N
    COMPRESSION_BC5, // ATI2N
    COMPRESSION_BC6H,
    COMPRESSION_ETC2_RG,
    COMPRESSION_ETC2_RGBA,
    COMPRESSION_ERR, // unknown
  };

public:
  BcCompressor(ECompressionType compr_type, unsigned int buffer_mips, unsigned int buffer_width, unsigned int buffer_height,
    int htiles, const char *bc_shader);
  ~BcCompressor();

  bool resetBuffer(unsigned int mips, unsigned int width, unsigned int height, int htiles); // uncompressed dimensions. recreate buffer
                                                                                            // texture
  void update(TEXTUREID src_id, int htiles = -1);                                           // draw src to bc-target
  void updateFromMip(TEXTUREID src_id, int src_mip, int dst_mip, int htiles = -1);
  void updateFromFaceMip(TEXTUREID src_id, int src_face, int src_mip, int dst_mip, int htiles = -1);

  void copyTo(Texture *dest_tex, int dest_x, int dest_y, int src_x = 0, int src_y = 0, int width = -1, int height = -1);

  void copyToMip(Texture *dest_tex, int dest_mip, int dest_x, int dest_y, int src_mip = 0, int src_x = 0, int src_y = 0,
    int width = -1, int height = -1);
  void releaseBuffer(); // destroy buffer texture

  ECompressionType getCompressionType() const;
  bool isValid() const;

  static bool isAvailable(ECompressionType format);

private:
  Texture *bufferTex;

  unsigned int bufferMips;
  unsigned int bufferWidth;
  unsigned int bufferHeight;
  unsigned int totalTiles = 0;

  static int srcTexVarId;
  static int srcMipVarId;
  static int dstMipVarId;
  static int etc2RGBAVarId;
  struct Vertex
  {
    float x, y, z, w;
    Vertex() {}
    Vertex(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
  };
  SmallTab<Vertex, MidmemAlloc> verts;
  Vbuffer *vb;
  struct VbFiller : public Sbuffer::IReloadData
  {
    void reloadD3dRes(Sbuffer *sb);
    void destroySelf() {}
    const SmallTab<Vertex, MidmemAlloc> &verts;
    VbFiller(const SmallTab<Vertex, MidmemAlloc> &verts) : verts(verts) {}
  } vbFiller;
  Ptr<ShaderElement> compressElem;
  Ptr<ComputeShaderElement> compressElemCompute;
  Ptr<ShaderMaterial> compressMat;

  ECompressionType compressionType;
};

BcCompressor::ECompressionType get_texture_compression_type(uint32_t tex_fmt);
BcCompressor::ECompressionType get_texture_compression_type(Texture *tex);
