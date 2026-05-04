// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <render/bcCompressor.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include <3d/dag_lockTexture.h>

#define GLOBAL_VARS_LIST          \
  VAR(src_tex)                    \
  VAR(src_tex_samplerstate)       \
  VAR(src_mip)                    \
  VAR(dst_mip)                    \
  VAR(dbg_sampleTex)              \
  VAR(dbg_sampleTex_samplerstate) \
  VAR(dbg_src_texelSize_uvOffset) \
  VAR(compress_rgba)              \
  VAR(src_face)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

bool BcCompressor::isAvailable(ECompressionType format)
{
  uint32_t texFmt = get_texture_compressed_format(format);
  G_ASSERTF_RETURN(texFmt, false, "Undefined bc format");

  return d3d::check_texformat(texFmt) && d3d::get_driver_desc().caps.hasResourceCopyConversion;
}

BcCompressor::ECompressionType get_texture_compression_type(uint32_t fmt)
{
  switch (fmt & TEXFMT_MASK)
  {
    case TEXFMT_DXT1: return BcCompressor::COMPRESSION_BC1;
    case TEXFMT_DXT5: return BcCompressor::COMPRESSION_BC3;
    case TEXFMT_ATI1N: return BcCompressor::COMPRESSION_BC4;
    case TEXFMT_ATI2N: return BcCompressor::COMPRESSION_BC5;
    case TEXFMT_BC6H: return BcCompressor::COMPRESSION_BC6H;
    case TEXFMT_ETC2_RG: return BcCompressor::COMPRESSION_ETC2_RG;
    case TEXFMT_ETC2_RGBA: return BcCompressor::COMPRESSION_ETC2_RGBA;
    case TEXFMT_ASTC4: return BcCompressor::COMPRESSION_ASTC4;
    default: return BcCompressor::COMPRESSION_ERR;
  }
}

uint32_t get_texture_compressed_format(BcCompressor::ECompressionType type)
{
  switch (type)
  {
    case BcCompressor::COMPRESSION_BC1: return TEXFMT_DXT1;
    case BcCompressor::COMPRESSION_BC3: return TEXFMT_DXT5;
    case BcCompressor::COMPRESSION_BC4: return TEXFMT_ATI1N;
    case BcCompressor::COMPRESSION_BC5: return TEXFMT_ATI2N;
    case BcCompressor::COMPRESSION_BC6H: return TEXFMT_BC6H;
    case BcCompressor::COMPRESSION_ETC2_RG: return TEXFMT_ETC2_RG;
    case BcCompressor::COMPRESSION_ETC2_RGBA: return TEXFMT_ETC2_RGBA;
    case BcCompressor::COMPRESSION_ASTC4: return TEXFMT_ASTC4;
    default: return 0;
  }
}

BcCompressor::ECompressionType get_texture_compression_type(Texture *tex)
{
  G_ASSERT(tex);

  TextureInfo texInfo;
  if (tex->getinfo(texInfo))
    return get_texture_compression_type(texInfo.cflg);
  else
    return BcCompressor::COMPRESSION_ERR;
}

static bool isMobileFormat(BcCompressor::ECompressionType type)
{
  return type == BcCompressor::ECompressionType::COMPRESSION_ETC2_RG ||
         type == BcCompressor::ECompressionType::COMPRESSION_ETC2_RGBA || type == BcCompressor::ECompressionType::COMPRESSION_ASTC4;
}

BcCompressor::BcCompressor(ECompressionType compr_type, unsigned int buffer_mips, unsigned int buffer_width,
  unsigned int buffer_height, int htiles, const char *bc_shader, bool use_own_buffer, const char *sqError_shader) :
  bufferMips(0), bufferWidth(0), bufferHeight(0), compressionType(compr_type), vbFiller(verts), useOwnBuffer(use_own_buffer)
{
  G_ASSERT(isValid());

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.mip_map_mode = d3d::MipMapMode::Point;
    ShaderGlobal::set_sampler(dbg_sampleTex_samplerstateVarId, d3d::request_sampler(smpInfo));
  }

  if (d3d::device_lost(NULL))
  {
    compressionType = COMPRESSION_ERR;
    return;
  }

  if (isMobileFormat(compressionType))
  {
    // compression using compute shader
    compressElemCompute = new_compute_shader(bc_shader);
    if (!compressElemCompute)
    {
      compressionType = COMPRESSION_ERR;
      return;
    }
  }
  else
  {
    compressMat = new_shader_material_by_name_optional(bc_shader, NULL);
    if (!compressMat || !compressMat.get())
    {
      compressionType = COMPRESSION_ERR;
      return;
    }
    compressElem = compressMat->make_elem();
    G_ASSERT(compressElem);
  }

  // accept empty buffer
  G_ASSERT((buffer_width > 0 && buffer_height > 0) || (buffer_width == 0 && buffer_height == 0));

  if (!resetBuffer(buffer_mips, buffer_width, buffer_height, htiles))
    compressionType = COMPRESSION_ERR;

  if (sqError_shader)
    errorShaderName = eastl::string(sqError_shader);
  else
    errorShaderName = eastl::string(eastl::string::CtorSprintf{}, "%s_sqError", bc_shader);
}

BcCompressor::BcCompressor(BcCompressor &&other) :
  useOwnBuffer(other.useOwnBuffer),
  bufferFlags(other.bufferFlags),
  bufferTex(eastl::move(other.bufferTex)),
  bufferMips(other.bufferMips),
  bufferWidth(other.bufferWidth),
  bufferHeight(other.bufferHeight),
  totalTiles(other.totalTiles),
  verts(eastl::move(other.verts)),
  vbFiller(verts),
  vb(eastl::move(other.vb)),
  compressElem(eastl::move(other.compressElem)),
  compressElemCompute(eastl::move(other.compressElemCompute)),
  compressMat(eastl::move(other.compressMat)),
  compressionType(other.compressionType),
  dbgSampleTex(eastl::move(other.dbgSampleTex)),
  errorTex(eastl::move(other.errorTex)),
  computeMSE(eastl::move(other.computeMSE)),
  errorShaderName(eastl::move(other.errorShaderName))
{
  vb->setReloadCallback(&vbFiller);
  other.releaseBuffer();
  other.closeErrorStats();
}

BcCompressor &BcCompressor::operator=(BcCompressor &&other)
{
  useOwnBuffer = other.useOwnBuffer;
  bufferFlags = other.bufferFlags;
  bufferTex = eastl::move(other.bufferTex);
  bufferMips = other.bufferMips;
  bufferWidth = other.bufferWidth;
  bufferHeight = other.bufferHeight;
  totalTiles = other.totalTiles;
  verts = eastl::move(other.verts);
  vb = eastl::move(other.vb);
  compressElem = eastl::move(other.compressElem);
  compressElemCompute = eastl::move(other.compressElemCompute);
  compressMat = eastl::move(other.compressMat);
  compressionType = other.compressionType;
  dbgSampleTex = eastl::move(other.dbgSampleTex);
  errorTex = eastl::move(other.errorTex);
  computeMSE = eastl::move(other.computeMSE);
  errorShaderName = eastl::move(other.errorShaderName);

  vb->setReloadCallback(&vbFiller);
  other.releaseBuffer();
  other.closeErrorStats();

  return *this;
}

BcCompressor::~BcCompressor() {}

void BcCompressor::VbFiller::reloadD3dRes(Sbuffer *sb) { sb->updateData(0, data_size(verts), verts.data(), VBLOCK_WRITEONLY); }

bool BcCompressor::resetBuffer(unsigned int mips, unsigned int width, unsigned int height, int htiles)
{
  G_ASSERT(isValid());

  if (width == bufferWidth && height == bufferHeight)
    return true;

  G_ASSERT(mips > 0);
  G_ASSERT(width > 0 && height > 0);
  G_ASSERT((width & 3) == 0 && (height & 3) == 0);

  releaseBuffer();
  closeErrorStats();

  bufferMips = mips;
  bufferWidth = width;
  bufferHeight = height;
  totalTiles = htiles;

  String bufferTexName(32, "bc_compr_buf_%p", this);
  String vbufferName(32, "bc_compr_vbuf_%p", this);

  int textureFormat = 0;
  if (compressionType == COMPRESSION_BC1 || compressionType == COMPRESSION_BC4)
    textureFormat = TEXFMT_R32G32UI;
  else if (compressionType == COMPRESSION_BC3 || compressionType == COMPRESSION_BC5 || compressionType == COMPRESSION_BC6H ||
           isMobileFormat(compressionType))
    textureFormat = TEXFMT_A32B32G32R32UI;
  else
    G_ASSERT(0 && "Unknown compression scheme");

  uint32_t flags = isMobileFormat(compressionType) ? TEXCF_UNORDERED : TEXCF_RTARGET;
  bufferFlags = (uint32_t)textureFormat | flags;

  if (useOwnBuffer)
  {
    bufferTex = dag::create_tex(NULL, bufferWidth / 4, bufferHeight / 4, bufferFlags, bufferMips, bufferTexName, RESTAG_COMPRESS);
    d3d_err(bufferTex.getBaseTex());
  }
  if (useOwnBuffer && !bufferTex)
    return false;

  clear_and_resize(verts, 3 + (htiles - 1) * 4);
  verts[0] = Vertex(-1, +1, bufferWidth, bufferHeight);
  verts[1] = Vertex(-1, -3, bufferWidth, bufferHeight);
  verts[2] = Vertex(+3, +1, bufferWidth, bufferHeight);
  for (int i = 0; i < htiles - 1; ++i)
  {
    float right = -1 + (2.0 * (i + 1)) / htiles;
    verts[3 + i * 4 + 0] = Vertex(-1, -1, bufferWidth, bufferHeight);
    verts[3 + i * 4 + 1] = Vertex(right, -1, bufferWidth, bufferHeight);
    verts[3 + i * 4 + 2] = Vertex(-1, +1, bufferWidth, bufferHeight);
    verts[3 + i * 4 + 3] = Vertex(right, +1, bufferWidth, bufferHeight);
  }

  vb = dag::create_vb(data_size(verts), 0, vbufferName.c_str(), RESTAG_COMPRESS);
  d3d_err(vb.getBuf());
  if (!vb)
    return false;
  vbFiller.reloadD3dRes(vb.getBuf());
  vb->setReloadCallback(&vbFiller);
  return true;
}

Texture *BcCompressor::getBuffer() const
{
  G_ASSERT(useOwnBuffer);
  return bufferTex.getTex2D();
}

void BcCompressor::releaseBuffer()
{
  bufferWidth = bufferHeight = 0;

  bufferTex.close();
  vb.close();
}

void BcCompressor::update(TEXTUREID src_id, int tiles, Texture *external_buffer)
{
  updateFromMip(src_id, 0, 0, tiles, external_buffer);
}

void BcCompressor::updateFromMip(TEXTUREID src_id, int src_mip, int dst_mip, int tiles, Texture *external_buffer)
{
  updateFromFaceMip(src_id, -1, src_mip, dst_mip, tiles, external_buffer);
}

void BcCompressor::updateFromFaceMip(TEXTUREID src_id, int src_face, int src_mip, int dst_mip, int tiles, Texture *external_buffer)
{
  G_ASSERT_RETURN(isValid(), );
  G_ASSERT(dst_mip >= src_mip);
  G_ASSERT(useOwnBuffer != (bool)external_buffer);

  // render to buffer
  Driver3dRenderTarget prevTarget;
  d3d::get_render_target(prevTarget);

  Texture *buffer = useOwnBuffer ? bufferTex.getTex2D() : external_buffer;
  ShaderGlobal::set_float(src_mipVarId, src_mip);
  ShaderGlobal::set_float(dst_mipVarId, dst_mip);
  ShaderGlobal::set_int(src_faceVarId, src_face);
  if (src_id != BAD_D3DRESID)
    ShaderGlobal::set_texture(src_texVarId, src_id);

  if (isMobileFormat(compressionType))
  {
    int tiledWidth = tiles > 0 && tiles < totalTiles ? bufferWidth * tiles / totalTiles : bufferWidth;
#if DAGOR_DBGLEVEL > 0
    auto groupSizes = compressElemCompute->getThreadGroupSizes();
    G_ASSERT(((tiledWidth & (groupSizes[0] - 1)) == 0) && ((bufferHeight & (groupSizes[1] - 1)) == 0));
#endif
    ShaderGlobal::set_int(compress_rgbaVarId, compressionType == ECompressionType::COMPRESSION_ETC2_RGBA);
    compressElemCompute->setStates();
    d3d::set_rwtex(STAGE_CS, 2, buffer, 0, dst_mip);

    float cs_data[4] = {1.f / (bufferWidth >> src_mip), 1.f / (bufferHeight >> src_mip), (float)src_mip, 0.0f};
    d3d::set_cs_const(51, cs_data, 1);
    compressElemCompute->dispatchThreads(tiledWidth >> 2, bufferHeight >> 2, 1);

    d3d::set_rwtex(STAGE_CS, 2, NULL, 0, 0);
  }
  else
  {
    d3d::set_render_target({}, DepthAccess::RW, {{buffer, static_cast<uint32_t>(dst_mip), 0}});
    d3d::allow_render_pass_target_load();

    compressElem->setStates(0, true);
    if (tiles <= 0 || tiles > ((verts.size() - 3) >> 2))
    {
      d3d::setvsrc_ex(0, vb.getBuf(), 0, sizeof(Vertex));
      d3d::draw(PRIM_TRISTRIP, 0, 1);
    }
    else
    {
      d3d::setvsrc_ex(0, vb.getBuf(), 0, sizeof(Vertex));
      d3d::draw(PRIM_TRISTRIP, 3 + (tiles - 1) * 4, 2);
    }
    d3d::setvsrc_ex(0, nullptr, 0, 0);
  }

  if (src_id != BAD_D3DRESID)
    ShaderGlobal::set_texture(src_texVarId, BAD_TEXTUREID);

  d3d::set_render_target(prevTarget);
}

void BcCompressor::copyTo(Texture *dest_tex, int dest_x, int dest_y, int src_x, int src_y, int width, int height,
  Texture *external_buffer)
{
  copyToMip(dest_tex, 0, dest_x, dest_y, 0, src_x, src_y, width, height, external_buffer);
}

void BcCompressor::copyToMip(Texture *dest_tex, int dest_mip, int dest_x, int dest_y, int src_mip, int src_x, int src_y, int width,
  int height, Texture *external_buffer)
{
  G_ASSERT_RETURN(isValid(), );
  G_ASSERT(dest_tex && compressionType == get_texture_compression_type(dest_tex));
  G_ASSERT(useOwnBuffer != (bool)external_buffer);

  // copy to target
  Texture *buffer = useOwnBuffer ? bufferTex.getTex2D() : external_buffer;
  width = width < 0 ? (bufferWidth >> src_mip) : width;
  height = height < 0 ? (bufferHeight >> src_mip) : height;
  G_ASSERT(width + src_x <= max(1U, bufferWidth >> src_mip) && height + src_y <= max(1U, bufferHeight >> src_mip));
  dest_tex->updateSubRegion(buffer, src_mip, src_x / 4, src_y / 4, 0, max(1, width / 4), max(1, height / 4), 1, dest_mip, dest_x,
    dest_y, 0);
}

BcCompressor::ECompressionType BcCompressor::getCompressionType() const
{
  return compressMat.get() || compressElemCompute.get() ? compressionType : COMPRESSION_ERR;
}

bool BcCompressor::isValid() const { return compressionType != COMPRESSION_ERR; }

uint32_t BcCompressor::getBufferFlags() const
{
  G_ASSERT_RETURN(isValid(), 0);
  return bufferFlags;
}

BcCompressor::ErrorStats::Entry::Entry(float MSE_) : MSE(MSE_), PSNR(-10.f * log10(MSE)) {}

BcCompressor::ErrorStats::ErrorStats(Point4 MSE) : r(MSE.x), g(MSE.y), b(MSE.z), a(MSE.w) {}

BcCompressor::ErrorStats::Entry BcCompressor::ErrorStats::getRGB() const { return {(r.MSE + g.MSE + b.MSE) / 3.f}; }

BcCompressor::ErrorStats::Entry BcCompressor::ErrorStats::getRGBA() const { return {(r.MSE + g.MSE + b.MSE + a.MSE) / 4.f}; }

BcCompressor::ErrorStats BcCompressor::getErrorStats(TEXTUREID src_id, int src_face, int src_mip, int dst_mip, int tile,
  Texture *external_buffer)
{
  G_ASSERT(isValid());
  G_ASSERT(useOwnBuffer != (bool)external_buffer);

  initErrorStats(); // lazily initialization, debug purpose.
  if (!computeMSE.getMat())
    return ErrorStats();

  int tileWidth = (bufferWidth / totalTiles) >> dst_mip;
  int tileHeight = bufferHeight >> dst_mip;
  int copySrcX = tileWidth * tile;
  // By passing external_buffer we cover both cases of useOwnBuffer.
  copyToMip(dbgSampleTex.getTex2D(), dst_mip, 0, 0, dst_mip, copySrcX, 0, tileWidth, tileHeight, external_buffer);

  SCOPE_RENDER_TARGET;

  ShaderGlobal::set_float(src_mipVarId, src_mip);
  ShaderGlobal::set_float(dst_mipVarId, dst_mip);
  ShaderGlobal::set_int(src_faceVarId, src_face);
  if (src_id != BAD_D3DRESID)
    ShaderGlobal::set_texture(src_texVarId, src_id);
  ShaderGlobal::set_texture(dbg_sampleTexVarId, dbgSampleTex.getTexId());
  ShaderGlobal::set_float4(dbg_src_texelSize_uvOffsetVarId,
    Point4(1.f / float(bufferWidth), 1.f / float(bufferHeight), float(tile) / float(totalTiles), 0));

  d3d::set_render_target({}, DepthAccess::RW, {{errorTex.getTex2D(), (uint32_t)dst_mip}});
  computeMSE.render();

  if (src_id != BAD_D3DRESID)
    ShaderGlobal::set_texture(src_texVarId, BAD_TEXTUREID);

  Point4 sqError = Point4::ZERO;
  int count = 0;
  if (auto lockedTex = lock_texture<const Point4>(errorTex.getTex2D(), dst_mip, TEXLOCK_READ))
  {
    for (Point4 texError : lockedTex)
    {
      sqError += texError;
      count++;
    }
  }

  Point4 MSE = (count > 0) ? sqError / float(count) : Point4::ZERO;

  return ErrorStats(MSE);
}

void BcCompressor::initErrorStats()
{
  if (errorTex)
    return;

  int tileWidth = bufferWidth / totalTiles;

  eastl::string dbgSampleTexName(eastl::string::CtorSprintf{}, "bcCompressor_dbgSampletex_%p", this);
  dbgSampleTex = dag::create_tex(NULL, tileWidth, bufferHeight,
    get_texture_compressed_format(compressionType) | TEXCF_UPDATE_DESTINATION, bufferMips, dbgSampleTexName.c_str(), RESTAG_COMPRESS);

  eastl::string errorTexName(eastl::string::CtorSprintf{}, "bcCompressor_MSEtex_%p", this);
  errorTex = dag::create_tex(NULL, tileWidth / 4, bufferHeight / 4, TEXCF_RTARGET | TEXFMT_A32B32G32R32F, bufferMips,
    errorTexName.c_str(), RESTAG_COMPRESS);

  computeMSE.init(errorShaderName.c_str());
}

void BcCompressor::closeErrorStats()
{
  dbgSampleTex.close();
  errorTex.close();
  computeMSE.clear();
}
#undef GLOBAL_VARS_LIST
