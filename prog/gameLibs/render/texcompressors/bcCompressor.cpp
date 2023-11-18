#include <3d/dag_drv3d.h>
#include <render/bcCompressor.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_computeShaders.h>

static int src_faceVarId = -1;

const char shader_var_src_tex[] = "src_tex";
const char shader_var_src_mip[] = "src_mip";
const char shader_var_dst_mip[] = "dst_mip";

int BcCompressor::srcTexVarId = -1;
int BcCompressor::srcMipVarId = -1;
int BcCompressor::dstMipVarId = -1;
int BcCompressor::etc2RGBAVarId = -1;

bool BcCompressor::isAvailable(ECompressionType format)
{
  // have problems with compression
  if (d3d::get_driver_code().is(d3d::apple && d3d::vulkan))
    return false;

  bool fmtAvailable = false;

  switch (format)
  {
    case BcCompressor::COMPRESSION_BC1: fmtAvailable = d3d::check_texformat(TEXFMT_DXT1); break;
    case BcCompressor::COMPRESSION_BC3: fmtAvailable = d3d::check_texformat(TEXFMT_DXT5); break;
    case BcCompressor::COMPRESSION_BC4: fmtAvailable = d3d::check_texformat(TEXFMT_ATI1N); break;
    case BcCompressor::COMPRESSION_BC5: fmtAvailable = d3d::check_texformat(TEXFMT_ATI2N); break;
    case BcCompressor::COMPRESSION_BC6H: fmtAvailable = d3d::check_texformat(TEXFMT_BC6H); break;
    case BcCompressor::COMPRESSION_ETC2_RG: fmtAvailable = d3d::check_texformat(TEXFMT_ETC2_RG); break;
    case BcCompressor::COMPRESSION_ETC2_RGBA: fmtAvailable = d3d::check_texformat(TEXFMT_ETC2_RGBA); break;
    default: G_ASSERTF(false, "Undefined bc format"); return false;
  }

  return fmtAvailable && d3d::get_driver_desc().caps.hasResourceCopyConversion;
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

    default: return BcCompressor::COMPRESSION_ERR;
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
  return type == BcCompressor::ECompressionType::COMPRESSION_ETC2_RG || type == BcCompressor::ECompressionType::COMPRESSION_ETC2_RGBA;
}

BcCompressor::BcCompressor(ECompressionType compr_type, unsigned int buffer_mips, unsigned int buffer_width,
  unsigned int buffer_height, int htiles, const char *bc_shader) :
  vb(NULL), bufferTex(NULL), bufferMips(0), bufferWidth(0), bufferHeight(0), compressionType(compr_type), vbFiller(verts)
{
  G_ASSERT(isValid());
  srcTexVarId = ::get_shader_variable_id(shader_var_src_tex, true);
  srcMipVarId = ::get_shader_variable_id(shader_var_src_mip, true);
  dstMipVarId = ::get_shader_variable_id(shader_var_dst_mip, true);
  src_faceVarId = ::get_shader_variable_id("src_face", true);
  etc2RGBAVarId = ::get_shader_variable_id("compress_rgba", true);

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
}

BcCompressor::~BcCompressor()
{
  compressElem = NULL;
  compressMat = NULL;
  releaseBuffer();
}

void BcCompressor::VbFiller::reloadD3dRes(Sbuffer *sb)
{
  Vertex *p;
  d3d_err(sb->lock(0, 0, (void **)&p, VBLOCK_WRITEONLY));
  if (!p)
    return;
  memcpy(p, verts.data(), data_size(verts));
  d3d_err(sb->unlock());
}

bool BcCompressor::resetBuffer(unsigned int mips, unsigned int width, unsigned int height, int htiles)
{
  G_ASSERT(isValid());

  if (width == bufferWidth && height == bufferHeight)
    return true;

  G_ASSERT(mips > 0);
  G_ASSERT(width > 0 && height > 0);
  G_ASSERT((width & 3) == 0 && (height & 3) == 0);

  releaseBuffer();

  bufferMips = mips;
  bufferWidth = width;
  bufferHeight = height;
  totalTiles = htiles;

  String bufferTexName(32, "bc_compr_buf_%p", this);

  int textureFormat = 0;
  if (compressionType == COMPRESSION_BC1 || compressionType == COMPRESSION_BC4)
    textureFormat = TEXFMT_R32G32UI;
  else if (compressionType == COMPRESSION_BC3 || compressionType == COMPRESSION_BC5 || compressionType == COMPRESSION_BC6H ||
           isMobileFormat(compressionType))
    textureFormat = TEXFMT_A32B32G32R32UI;
  else
    G_ASSERT(0 && "Unknown compression scheme");

  uint32_t flags = isMobileFormat(compressionType) ? TEXCF_UNORDERED : TEXCF_RTARGET;

  bufferTex = d3d::create_tex(NULL, bufferWidth / 4, bufferHeight / 4, textureFormat | flags, bufferMips, bufferTexName);
  d3d_err(bufferTex);
  if (!bufferTex)
    return false;

  bufferTex->texmipmap(TEXMIPMAP_NONE);
  bufferTex->texfilter(TEXFILTER_POINT);

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

#if !(_TARGET_C1 | _TARGET_C2)
  vb = d3d::create_vb(data_size(verts), 0, "bcCompr");
  d3d_err(vb);
  if (!vb)
    return false;
  vbFiller.reloadD3dRes(vb);
  vb->setReloadCallback(&vbFiller);
#endif
  return true;
}

void BcCompressor::releaseBuffer()
{
  bufferWidth = bufferHeight = 0;

  if (bufferTex)
    del_d3dres(bufferTex);
  if (vb)
    del_d3dres(vb);
}

void BcCompressor::update(TEXTUREID src_id, int tiles) { updateFromMip(src_id, 0, 0, tiles); }

void BcCompressor::updateFromMip(TEXTUREID src_id, int src_mip, int dst_mip, int tiles)
{
  updateFromFaceMip(src_id, -1, src_mip, dst_mip, tiles);
}

void BcCompressor::updateFromFaceMip(TEXTUREID src_id, int src_face, int src_mip, int dst_mip, int tiles)
{
  G_ASSERT(isValid());
  G_ASSERT(src_id != BAD_TEXTUREID);
  G_ASSERT(dst_mip >= src_mip);
  // render to buffer
  Driver3dRenderTarget prevTarget;
  d3d::get_render_target(prevTarget);

  if (isMobileFormat(compressionType))
  {
    int tiledWidth = tiles > 0 && tiles < totalTiles ? bufferWidth * tiles / totalTiles : bufferWidth;
#if DAGOR_DBGLEVEL > 0
    auto groupSizes = compressElemCompute->getThreadGroupSizes();
    G_ASSERT(((tiledWidth & (groupSizes[0] - 1)) == 0) && ((bufferHeight & (groupSizes[1] - 1)) == 0));
#endif
    ShaderGlobal::set_real(srcMipVarId, src_mip);
    ShaderGlobal::set_real(dstMipVarId, dst_mip);
    ShaderGlobal::set_texture(srcTexVarId, src_id);
    ShaderGlobal::set_int(etc2RGBAVarId, compressionType == ECompressionType::COMPRESSION_ETC2_RGBA);
    compressElemCompute->setStates();
    d3d::set_rwtex(STAGE_CS, 2, bufferTex, 0, dst_mip);

    float cs_data[4] = {1.f / (bufferWidth >> src_mip), 1.f / (bufferHeight >> src_mip), (float)src_mip, 0.0f};
    d3d::set_cs_const(51, cs_data, 1);
    compressElemCompute->dispatchThreads(tiledWidth, bufferHeight, 1);
    d3d::resource_barrier({bufferTex, RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    d3d::set_rwtex(STAGE_CS, 0, bufferTex, 0, 0);
  }
  else
  {
    d3d::set_render_target();
    d3d::set_render_target(0, bufferTex, dst_mip);

    ShaderGlobal::set_real(srcMipVarId, src_mip);
    ShaderGlobal::set_real(dstMipVarId, dst_mip);
    ShaderGlobal::set_int(src_faceVarId, src_face);
    ShaderGlobal::set_texture(srcTexVarId, src_id);

    compressElem->setStates(0, true);
    if (tiles <= 0 || tiles > ((verts.size() - 3) >> 2))
    {
#if _TARGET_C1 | _TARGET_C2

#else
      d3d::setvsrc_ex(0, vb, 0, sizeof(Vertex));
      d3d::draw(PRIM_TRISTRIP, 0, 1);
#endif
    }
    else
    {
#if _TARGET_C1 | _TARGET_C2

#else
      d3d::setvsrc_ex(0, vb, 0, sizeof(Vertex));
      d3d::draw(PRIM_TRISTRIP, 3 + (tiles - 1) * 4, 2);
#endif
    }

    d3d::resource_barrier({bufferTex, RB_RO_SRV | RB_RO_COPY_SOURCE, (unsigned)dst_mip, 1});
  }

  ShaderGlobal::set_texture(srcTexVarId, BAD_TEXTUREID);

  d3d::set_render_target(prevTarget);
}

void BcCompressor::copyTo(Texture *dest_tex, int dest_x, int dest_y, int src_x, int src_y, int width, int height)
{
  copyToMip(dest_tex, 0, dest_x, dest_y, 0, src_x, src_y, width, height);
}

void BcCompressor::copyToMip(Texture *dest_tex, int dest_mip, int dest_x, int dest_y, int src_mip, int src_x, int src_y, int width,
  int height)
{
  G_ASSERT(isValid());
  G_ASSERT(dest_tex && compressionType == get_texture_compression_type(dest_tex));
  // copy to target
  width = width < 0 ? (bufferWidth >> src_mip) : width;
  height = height < 0 ? (bufferHeight >> src_mip) : height;
  G_ASSERT(width + src_x <= max(1U, bufferWidth >> src_mip) && height + src_y <= max(1U, bufferHeight >> src_mip));
  dest_tex->updateSubRegion(bufferTex, src_mip, src_x / 4, src_y / 4, 0, max(1, width / 4), max(1, height / 4), 1, dest_mip, dest_x,
    dest_y, 0);
}

BcCompressor::ECompressionType BcCompressor::getCompressionType() const
{
  return compressMat.get() || compressElemCompute.get() ? compressionType : COMPRESSION_ERR;
}

bool BcCompressor::isValid() const { return compressionType != COMPRESSION_ERR; }
