#include <ioSys/dag_dataBlock.h>
#include <3d/dag_drv3d.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/random/dag_random.h>
#include <EASTL/vector.h>
#include <EASTL/optional.h>
#include <perfMon/dag_pix.h>

#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>
#include <textureGen/textureDataCache.h>


static class TextureGenCache : public TextureGenShader
{
private:
  int h, w;

  void readTextureFromGPU(BaseTexture *src_texture, eastl::vector<uint8_t> &dest_array)
  {
    TextureInfo texInfo;
    src_texture->getinfo(texInfo);
    int32_t format = texInfo.cflg & TEXFMT_MASK;
    const TextureFormatDesc &desc = get_tex_format_desc(format);
    int bytesPerPixel = (desc.r.bits + desc.g.bits + desc.b.bits + desc.a.bits) / 8;
    size_t map_byte_size = w * h * bytesPerPixel;

    dest_array.resize(sizeof(format) + map_byte_size);

    if (texInfo.h != h || texInfo.w != w)
    {
      memset(dest_array.data(), 0, sizeof(format) + map_byte_size);
      return;
    }
    void *mapData = nullptr;
    int stride = 0;
    if (!src_texture->lockimg(&mapData, stride, 0, TEXLOCK_READ) && mapData)
      return;

    memcpy(dest_array.data() + sizeof(format), mapData, map_byte_size);
    memcpy(dest_array.data(), &format, sizeof(format));
    src_texture->unlockimg();
  }

  void writeTextureToGPU(Texture *dest_texture, const eastl::vector<uint8_t> &src_array, Texture *temp_tex, int format)
  {
    if (!dest_texture)
      return;
    TextureInfo info;
    dest_texture->getinfo(info);
    void *mapData = nullptr;
    int stride = 0;
    if (!temp_tex->lockimg(&mapData, stride, 0, TEXLOCK_WRITE) || !mapData)
      return;

    const TextureFormatDesc &desc = get_tex_format_desc(format);
    int bytesPerPixel = (desc.r.bits + desc.g.bits + desc.b.bits + desc.a.bits) / 8;

    memcpy(mapData, src_array.data() + sizeof(int32_t), w * h * bytesPerPixel);
    temp_tex->unlockimg();
    dest_texture->update(temp_tex);
  }

  void cpu_algorithm(dag::ConstSpan<TextureInput> &inputs, dag::ConstSpan<D3dResource *> &outputs, const char *id,
    bool read_data_from_cache)
  {
    eastl::vector<uint8_t> textureData;

    if (!read_data_from_cache)
    {
      readTextureFromGPU(inputs[0].tex, textureData);
      regcache::set_record_data(id, textureData.data(), textureData.size());
    }


    void *cached_data = regcache::get_record_data_ptr(id);

    int32_t format = 0;
    memcpy(&format, cached_data, sizeof(format));

    const TextureFormatDesc &desc = get_tex_format_desc(format);
    int bytesPerPixel = (desc.r.bits + desc.g.bits + desc.b.bits + desc.a.bits) / 8;
    size_t map_byte_size = w * h * bytesPerPixel;
    textureData.resize(sizeof(format) + map_byte_size);
    memcpy(textureData.data(), cached_data, textureData.size());

    Texture *temp_tex = d3d::create_tex(NULL, w, h, format, 1, "temp_cache");
    G_ASSERT(temp_tex);

    if (temp_tex)
    {
      writeTextureToGPU((Texture *)outputs[0], textureData, temp_tex, format);
      del_d3dres(temp_tex);
    }
  }


public:
  virtual ~TextureGenCache() {}
  virtual void destroy() {}

  virtual const char *getName() const { return "cache_node"; }
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

  virtual bool process(TextureGenerator &gen, TextureRegManager & /*regs*/, const TextureGenNodeData &data, int)
  {
    w = data.nodeW, h = data.nodeH;
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

    const char *id = data.params.getStr("preceding_subgraph_hash", "");
    bool useDataFromCache = data.params.getBool("use_data_from_cache", false);

    cpu_algorithm(inputs, outputs, id, useDataFromCache);
    return true;
  }
} texgen_cache;

void init_texgen_cache_shader(TextureGenerator *tex_gen) { texgen_add_shader(tex_gen, texgen_cache.getName(), &texgen_cache); }
