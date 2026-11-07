// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/random/dag_random.h>
#include <EASTL/optional.h>
#include <perfMon/dag_pix.h>

#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>
#include <textureGen/textureDataCache.h>

namespace
{
enum CacheStep
{
  CACHE_START_READBACK,
  CACHE_READ_TO_RECORD,
  CACHE_UPLOAD,
  CACHE_STEP_COUNT
};

size_t row_bytes(int32_t format, int w) { return static_cast<size_t>(w) * get_tex_format_desc(format).bytesPerElement; }

bool reads_back_async(const TextureInfo &info, int w, int h)
{
  return info.w == w && info.h == h && (info.cflg & (TEXCF_RTARGET | TEXCF_UNORDERED | TEXCF_UPDATE_DESTINATION));
}

// The driver hands a started readback to the next TEXLOCK_READ lock of the texture, however old it is.
void drop_readback(BaseTexture *src, int w, int h)
{
  TextureInfo info;
  src->getinfo(info);
  void *mapped = nullptr;
  int stride = 0;
  if (reads_back_async(info, w, h) && src->lockimg(&mapped, stride, 0, TEXLOCK_READ))
  {
    src->unlockimg();
  }
}

// A record is the texture format followed by the rows.
uint8_t *create_record_rows(const char *id, int32_t format, size_t bytes)
{
  uint8_t *record = static_cast<uint8_t *>(regcache::create_record(id, sizeof(format) + bytes));
  memcpy(record, &format, sizeof(format));
  return record + sizeof(format);
}

void copy_rows(uint8_t *dst, size_t dst_pitch, const uint8_t *src, size_t src_pitch, size_t row_size, int rows)
{
  for (int y = 0; y < rows; ++y)
  {
    memcpy(dst + dst_pitch * y, src + src_pitch * y, row_size);
  }
}

bool read_to_record(BaseTexture *src, const char *id, int w, int h)
{
  TextureInfo info;
  src->getinfo(info);
  const int32_t format = info.cflg & TEXFMT_MASK;
  const size_t rowBytes = row_bytes(format, w);
  if (info.w != w || info.h != h)
  {
    memset(create_record_rows(id, format, rowBytes * h), 0, rowBytes * h);
    return true;
  }

  void *mapped = nullptr;
  int stride = 0;
  if (!src->lockimg(&mapped, stride, 0, TEXLOCK_READ))
  {
    return false;
  }
  const bool readable = mapped && static_cast<size_t>(stride) >= rowBytes;
  if (readable)
  {
    copy_rows(create_record_rows(id, format, rowBytes * h), rowBytes, static_cast<const uint8_t *>(mapped),
      static_cast<size_t>(stride), rowBytes, h);
  }
  src->unlockimg();
  return readable;
}

bool upload_record(Texture *dest, const uint8_t *record, int w, int h)
{
  int32_t format = 0;
  memcpy(&format, record, sizeof(format));
  const size_t rowBytes = row_bytes(format, w);
  Texture *temp = d3d::create_tex(nullptr, w, h, format, 1, "temp_cache", RESTAG_TEXGEN);
  G_ASSERT_RETURN(temp, false);
  bool uploaded = false;
  void *mapped = nullptr;
  int stride = 0;
  if (temp->lockimg(&mapped, stride, 0, TEXLOCK_WRITE))
  {
    if (mapped && static_cast<size_t>(stride) >= rowBytes)
    {
      copy_rows(static_cast<uint8_t *>(mapped), static_cast<size_t>(stride), record + sizeof(format), rowBytes, rowBytes, h);
      uploaded = true;
    }
    temp->unlockimg();
  }
  if (uploaded)
  {
    uploaded = dest->update(temp) != 0;
  }
  del_d3dres(temp);
  return uploaded;
}
} // namespace

static class TextureGenCache : public TextureGenShader
{
public:
  virtual ~TextureGenCache() {}
  virtual void destroy() {}

  virtual const char *getName() const { return "cache_node"; }
  virtual bool mapsResourcesOnCpu(int sub_step) const { return sub_step == CACHE_READ_TO_RECORD; }
  virtual int getInputParametersCount() const { return 1; }
  virtual bool isInputParameterOptional(int) const { return false; }

  virtual int getRegCount(TShaderRegType tp) const
  {
    G_UNUSED(tp);
    return 1;
  }
  virtual bool isRegOptional(TShaderRegType tp, int reg) const
  {
    G_UNUSED(tp);
    G_UNUSED(reg);
    return false;
  }

  virtual void cancelSubSteps(TextureRegManager &, const TextureGenNodeData &data, int next_sub_step)
  {
    if (next_sub_step == CACHE_READ_TO_RECORD)
    {
      drop_readback(data.inputs[0].tex, data.nodeW, data.nodeH);
    }
  }

  virtual unsigned subSteps(const TextureGenNodeData &data) const
  {
    const bool fromCache = data.params.getBool("use_data_from_cache", false);
    return fromCache || data.inputs.size() < 1 || data.outputs.size() < 1 ? 1 : CACHE_STEP_COUNT;
  }

  virtual bool process(TextureGenerator &gen, TextureRegManager & /*regs*/, const TextureGenNodeData &data, int sub_step)
  {
    const int w = data.nodeW, h = data.nodeH;
    dag::ConstSpan<TextureInput> inputs = data.inputs;
    dag::ConstSpan<D3dResource *> outputs = data.outputs;
    if (inputs.size() < 1)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "not enough inputs for %s", getName()));
      return false;
    }
    if (outputs.size() < 1)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_REMARK, String(128, "not enough outputs for %s", getName()));
      return true;
    }
    if (!inputs[0].tex)
    {
      return false;
    }

    const char *id = data.params.getStr("preceding_subgraph_hash", "");
    if (!data.params.getBool("use_data_from_cache", false))
    {
      if (sub_step == CACHE_START_READBACK)
      {
        TextureInfo info;
        inputs[0].tex->getinfo(info);
        // A record holds one element per pixel, which a block compressed format does not have.
        const int32_t format = info.cflg & TEXFMT_MASK;
        if (get_tex_format_desc(format).isBlockFormat)
        {
          texgen_get_logger(&gen)->log(LOGLEVEL_ERR,
            String(128, "%s does not take a block compressed format 0x%X", getName(), format));
          return false;
        }
        if (reads_back_async(info, w, h))
        {
          int stride = 0;
          inputs[0].tex->lockimg(nullptr, stride, 0, TEXLOCK_READ);
        }
        return true;
      }
      if (sub_step == CACHE_READ_TO_RECORD)
      {
        return read_to_record(inputs[0].tex, id, w, h);
      }
    }

    const uint8_t *record = static_cast<const uint8_t *>(regcache::get_record_data_ptr(id));
    if (!record)
    {
      return false;
    }
    return !outputs[0] || upload_record(static_cast<Texture *>(outputs[0]), record, w, h);
  }
} texgen_cache;

void init_texgen_cache_shader(TextureGenerator *tex_gen) { texgen_add_shader(tex_gen, texgen_cache.getName(), &texgen_cache); }
