// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <util/dag_string.h>
#include <3d/dag_resourceTags.h>
#include <EASTL/vector.h>
#include <math/dag_mathBase.h>
#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define STB_TRUETYPE_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 5262) // implicit fall-through in generated stb code
#include <imstb_truetype.h>
#pragma warning(pop)

static eastl::vector<uint8_t> load_file_to_buf(const char *path)
{
  FILE *f = fopen(path, "rb");
  if (!f)
    return {};
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (sz <= 0)
  {
    fclose(f);
    return {};
  }
  eastl::vector<uint8_t> buf((size_t)sz);
  const size_t read = fread(buf.data(), 1, (size_t)sz, f);
  fclose(f);
  if (read != (size_t)sz)
    return {};
  return buf;
}

static int utf8_next_codepoint(const char **p)
{
  const unsigned char *s = (const unsigned char *)*p;
  if (!*s)
    return -1;
  int cp;
  if ((*s & 0x80) == 0)
  {
    cp = *s++;
  }
  else if ((*s & 0xE0) == 0xC0)
  {
    cp = (*s++ & 0x1F) << 6;
    if (!*s)
      return -1;
    cp |= (*s++ & 0x3F);
  }
  else if ((*s & 0xF0) == 0xE0)
  {
    cp = (*s++ & 0x0F) << 12;
    if (!*s)
      return -1;
    cp |= (*s++ & 0x3F) << 6;
    if (!*s)
      return -1;
    cp |= (*s++ & 0x3F);
  }
  else
  {
    cp = '?';
    ++s;
  }
  *p = (const char *)s;
  return cp;
}

static class TextureGenRenderText : public TextureGenShader
{
public:
  virtual ~TextureGenRenderText() {}
  virtual void destroy() {}
  virtual const char *getName() const { return "render_text"; }
  virtual int getInputParametersCount() const { return 6; }
  virtual bool isInputParameterOptional(int) const { return true; }
  virtual int getRegCount(TShaderRegType tp) const { return (tp == TSHADER_REG_TYPE_OUTPUT) ? 1 : 0; }
  virtual bool isRegOptional(TShaderRegType tp, int /*reg*/) const { return tp != TSHADER_REG_TYPE_OUTPUT; }

  virtual bool process(TextureGenerator &gen, TextureRegManager & /*regs*/, const TextureGenNodeData &data, int)
  {
    if (data.outputs.size() < 1 || !data.outputs[0])
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_REMARK, String(128, "not enough outputs for %s", getName()));
      return true;
    }

    const int w = data.nodeW;
    const int h = data.nodeH;

    const char *text = data.params.getStr("text", "Hello World");
    const char *font_path = data.params.getStr("font_path", "");
    const float font_size = data.params.getReal("font_size", 48.0f);
    const float pos_x = data.params.getReal("pos_x", 0.5f);
    const float pos_y = data.params.getReal("pos_y", 0.5f);
    const float rotation = data.params.getReal("rotation", 0.0f);

    if (!text || !text[0])
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_REMARK, String(128, "render_text: text is empty"));
      return true;
    }
    if (!font_path || !font_path[0])
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "render_text: font_path is empty"));
      return false;
    }

    eastl::vector<uint8_t> fontBuf = load_file_to_buf(font_path);
    if (fontBuf.empty())
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(256, "render_text: cannot load font <%s>", font_path));
      return false;
    }

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, fontBuf.data(), stbtt_GetFontOffsetForIndex(fontBuf.data(), 0)))
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(256, "render_text: stbtt_InitFont failed for <%s>", font_path));
      return false;
    }

    const float scale = stbtt_ScaleForPixelHeight(&font, font_size);

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    const int textH = (int)((ascent - descent) * scale + 0.5f);

    int64_t textW = 0;
    {
      const char *p = text;
      int prevCp = -1;
      while (*p)
      {
        int cp = utf8_next_codepoint(&p);
        if (cp < 0)
          break;
        if (prevCp >= 0)
          textW += (int)(stbtt_GetCodepointKernAdvance(&font, prevCp, cp) * scale + 0.5f);
        int adv, lsb;
        stbtt_GetCodepointHMetrics(&font, cp, &adv, &lsb);
        textW += (int)(adv * scale + 0.5f);
        prevCp = cp;
      }
    }
    if (textW <= 0 || textH <= 0)
      return true;

    const int scaledAscent = (int)(ascent * scale + 0.5f);
    const int textWi = (int)textW;
    eastl::vector<uint8_t> textBmp((size_t)textWi * textH, 0);
    {
      const char *p = text;
      int64_t cx = 0;
      int prevCp = -1;
      while (*p)
      {
        int cp = utf8_next_codepoint(&p);
        if (cp < 0)
          break;
        if (prevCp >= 0)
          cx += (int)(stbtt_GetCodepointKernAdvance(&font, prevCp, cp) * scale + 0.5f);

        int adv, lsb;
        stbtt_GetCodepointHMetrics(&font, cp, &adv, &lsb);

        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&font, cp, scale, scale, &x0, &y0, &x1, &y1);
        const int gw = x1 - x0;
        const int gh = y1 - y0;

        if (gw > 0 && gh > 0)
        {
          eastl::vector<uint8_t> glyph((size_t)gw * gh);
          stbtt_MakeCodepointBitmap(&font, glyph.data(), gw, gh, gw, scale, scale, cp);

          const int bx = (int)cx + (int)(lsb * scale + 0.5f);
          const int by = scaledAscent + y0;

          for (int row = 0; row < gh; row++)
          {
            const int dy = by + row;
            if (dy < 0 || dy >= textH)
              continue;
            for (int col = 0; col < gw; col++)
            {
              const int dx = bx + col;
              if (dx < 0 || dx >= textWi)
                continue;
              uint8_t v = glyph[row * gw + col];
              uint8_t &dst = textBmp[dy * textWi + dx];
              if (v > dst)
                dst = v;
            }
          }
        }

        cx += (int)(adv * scale + 0.5f);
        prevCp = cp;
      }
    }

    const float cx_canvas = pos_x * (float)w;
    const float cy_canvas = pos_y * (float)h;
    const float rad = rotation * (PI / 180.0f);
    const float cosA = cosf(rad);
    const float sinA = sinf(rad);
    const float halfTW = (float)textWi * 0.5f;
    const float halfTH = textH * 0.5f;

    eastl::vector<uint8_t> canvasBmp((size_t)w * h, 0);
    for (int y = 0; y < h; y++)
    {
      for (int x = 0; x < w; x++)
      {
        const float dx = (float)x - cx_canvas;
        const float dy = (float)y - cy_canvas;
        const float rx = cosA * dx + sinA * dy + halfTW;
        const float ry = -sinA * dx + cosA * dy + halfTH;
        const int tx0 = (int)floorf(rx);
        const int ty0 = (int)floorf(ry);
        const int tx1 = tx0 + 1;
        const int ty1 = ty0 + 1;
        if (tx1 >= 0 && tx0 < textWi && ty1 >= 0 && ty0 < textH)
        {
          const float fx = rx - tx0;
          const float fy = ry - ty0;
          const auto sample = [&](int sx, int sy) -> float {
            return (sx >= 0 && sx < textWi && sy >= 0 && sy < textH) ? textBmp[sy * textWi + sx] : 0.0f;
          };
          const float v = sample(tx0, ty0) * (1 - fx) * (1 - fy) + sample(tx1, ty0) * fx * (1 - fy) +
                          sample(tx0, ty1) * (1 - fx) * fy + sample(tx1, ty1) * fx * fy;
          canvasBmp[y * w + x] = (uint8_t)(v + 0.5f);
        }
      }
    }

    Texture *dest = (Texture *)data.outputs[0];

    Texture *tempSys = d3d::create_tex(NULL, w, h, TEXFMT_R8 | TEXCF_SYSMEM, 1, "render_text_stage", RESTAG_TEXGEN);
    if (!tempSys)
    {
      texgen_get_logger(&gen)->log(LOGLEVEL_ERR, String(128, "render_text: failed to create staging texture"));
      return false;
    }

    void *mapData = nullptr;
    int stride = 0;
    if (tempSys->lockimg(&mapData, stride, 0, TEXLOCK_WRITE) && mapData)
    {
      for (int row = 0; row < h; row++)
        memcpy((uint8_t *)mapData + row * stride, canvasBmp.data() + row * w, (size_t)w);
      tempSys->unlockimg();
    }

    Texture *tempRT = d3d::create_tex(NULL, w, h, TEXFMT_R8 | TEXCF_RTARGET, 1, "render_text_rt", RESTAG_TEXGEN);
    if (!tempRT)
    {
      del_d3dres(tempSys);
      return false;
    }
    tempRT->update(tempSys);
    del_d3dres(tempSys);

    d3d::stretch_rect(tempRT, dest);
    del_d3dres(tempRT);
    return true;
  }
} texgen_render_text;

void init_texgen_render_text_shader(TextureGenerator *tex_gen)
{
  texgen_add_shader(tex_gen, texgen_render_text.getName(), &texgen_render_text);
}