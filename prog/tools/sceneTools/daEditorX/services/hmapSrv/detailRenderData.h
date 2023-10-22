#pragma once

#include <3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <shaders/dag_shaderVar.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug3d.h>

static float full_vert[] = {-1, +1, -1, -3, +3, +1};

class DetailRenderData
{
public:
  struct NamedTex
  {
    Texture *tex;
    SimpleString name;
  };
  struct TexturePingPong
  {
    Texture *tex1, *tex2;
    TexturePingPong(Texture *t1, Texture *t2) : tex1(t1), tex2(t2) {}
    TexturePingPong() {}
  };
  static Tab<TexturePingPong> colorMapTexArr;
  static Tab<Texture *> detailMaskTexArr;
  static int copyProg;

  Texture *colorMapTex1, *colorMapTex2; // can be reused
  Texture *detailMaskTex2;              // can be reused

  Texture *colorMapTex, *detailMaskTex;
  TEXTUREID colorMapTexId, detailMaskTexId;
  Tab<NamedTex> maskTex;
  TEXTUREID defColorTexId, defDetailTexId;
  Texture *defColorTex, *defDetailTex;
  SimpleString colorMapName, detailMaskName;
  DataBlock actualLayers;

public:
  DetailRenderData(const DataBlock &layers, LandClassDetailInfo *detTex, dag::ConstSpan<HmapDetLayerProps> dlp)
  {
    DAEDITOR3.conNote("create DetailRenderData");
    G_ASSERT(detTex && detTex->editable);

    colorMapTex = colorMapTex1 = colorMapTex2 = detailMaskTex = detailMaskTex2 = NULL;
    colorMapTexId = detailMaskTexId = BAD_TEXTUREID;
    defColorTex = defDetailTex = NULL;
    defColorTexId = defDetailTexId = BAD_TEXTUREID;

    ShaderGlobal::reset_textures(true);
    unsigned usage = d3d::USAGE_RTARGET;
    if (detTex->colormapName[0] == '*')
    {
      colorMapName = detTex->colormapName.str() + 1;
      if (char *p = strchr(colorMapName.str(), ':'))
        *p = '\0';

      if (strstr(detTex->colormapName + 1, ":"))
        defColorTexId = getAuxTex(strstr(detTex->colormapName + 1, ":") + 1);
      if (defColorTexId != BAD_TEXTUREID)
        defColorTex = (Texture *)acquire_managed_tex(defColorTexId);
      colorMapTexId = get_managed_texture_id(detTex->colormapName);
      if (colorMapTexId < 0)
      {
        colorMapTex = d3d::create_tex(NULL, detTex->colorMapSize, detTex->colorMapSize,
          TEXFMT_A8R8G8B8 | TEXCF_SRGBWRITE | TEXCF_SRGBREAD | TEXCF_RTARGET, 1, detTex->colormapName);
        colorMapTexId = register_managed_tex(detTex->colormapName, colorMapTex);
      }
      else
        colorMapTex = (Texture *)acquire_managed_tex(colorMapTexId);

      int ping = 0;
      for (int ping = 0; ping < colorMapTexArr.size(); ++ping)
      {
        TextureInfo info;
        colorMapTexArr[ping].tex1->getinfo(info);
        if (info.w == detTex->colorMapSize && info.h == detTex->colorMapSize)
        {
          colorMapTex1 = colorMapTexArr[ping].tex1;
          colorMapTex2 = colorMapTexArr[ping].tex2;
          break;
        }
      }
      if (ping >= colorMapTexArr.size())
      {
        unsigned texFmt = TEXFMT_A8R8G8B8;
        if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16) & usage) == usage)
          texFmt = TEXFMT_A16B16G16R16;
        else if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & usage) == usage)
          texFmt = TEXFMT_A16B16G16R16F;

        colorMapTex1 = d3d::create_tex(NULL, detTex->colorMapSize, detTex->colorMapSize, texFmt | TEXCF_RTARGET, 1, "colormapping1");
        colorMapTex2 = d3d::create_tex(NULL, detTex->colorMapSize, detTex->colorMapSize, texFmt | TEXCF_RTARGET, 1, "colormapping2");
        colorMapTex1->texfilter(TEXFILTER_POINT);
        colorMapTex2->texfilter(TEXFILTER_POINT);
        colorMapTexArr.push_back(TexturePingPong(colorMapTex1, colorMapTex2));
      }
    }
    if (detTex->detailmapName[0] == '*')
    {
      detailMaskName = detTex->detailmapName.str() + 1;
      if (char *p = strchr(detailMaskName.str(), ':'))
        *p = '\0';

      if (strstr(detTex->detailmapName + 1, ":"))
        defColorTexId = getAuxTex(strstr(detTex->detailmapName + 1, ":") + 1);
      if (defDetailTexId != BAD_TEXTUREID)
        defDetailTex = (Texture *)acquire_managed_tex(defDetailTexId);
      detailMaskTexId = get_managed_texture_id(detTex->detailmapName);
      if (detailMaskTexId < 0)
      {
        detailMaskTex = d3d::create_tex(NULL, detTex->splattingMapSize, detTex->splattingMapSize, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1,
          detTex->detailmapName);
        detailMaskTexId = register_managed_tex(detTex->detailmapName, detailMaskTex);
      }
      else
        detailMaskTex = (Texture *)acquire_managed_tex(detailMaskTexId);

      int ping = 0;
      for (int ping = 0; ping < detailMaskTexArr.size(); ++ping)
      {
        TextureInfo info;
        detailMaskTexArr[ping]->getinfo(info);
        if (info.w == detTex->splattingMapSize && info.h == detTex->splattingMapSize)
        {
          detailMaskTex2 = detailMaskTexArr[ping];
          break;
        }
      }
      if (ping >= detailMaskTexArr.size())
      {
        detailMaskTex2 = d3d::create_tex(NULL, detTex->splattingMapSize, detTex->splattingMapSize, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1,
          "dettex2ping"); // can reuse between classes
        detailMaskTex2->texfilter(TEXFILTER_POINT);
        detailMaskTexArr.push_back(detailMaskTex2);
      }
    }

    for (int i = 0; i < layers.blockCount(); i++)
      if (*layers.getBlock(i)->getStr("lMask", ""))
      {
        if (getMaskTex(layers.getBlock(i)->getStr("lMask", "")))
          continue;
        NamedTex &nt = maskTex.push_back();
        nt.name = layers.getBlock(i)->getStr("lMask", "");
        nt.tex = d3d::create_tex(NULL, layers.getBlock(i)->getInt("lMaskW", detTex->splattingMapSize),
          layers.getBlock(i)->getInt("lMaskH", detTex->splattingMapSize), TEXFMT_L8 | TEXCF_READABLE | TEXCF_DYNAMIC, 1, nt.name);
      }
  }
  ~DetailRenderData()
  {
    DAEDITOR3.conNote("destroy DetailRenderData");
    ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(colorMapTexId, colorMapTex);
    ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(detailMaskTexId, detailMaskTex);
    if (defColorTexId != BAD_TEXTUREID)
    {
      release_managed_tex(defColorTexId);
      defColorTex = NULL;
    }
    if (defDetailTexId != BAD_TEXTUREID)
    {
      release_managed_tex(defDetailTexId);
      defDetailTex = NULL;
    }

    for (int i = 0; i < maskTex.size(); i++)
      del_d3dres(maskTex[i].tex);
    clear_and_shrink(maskTex);
  }

  void copyRender(Texture *src, Texture *dst)
  {
    d3d::set_render_target();
    d3d::set_render_target(dst, 0);
    d3d::set_program(copyProg);
    d3d::settex(0, src);
    d3d::draw_up(PRIM_TRILIST, 1, full_vert, 2 * sizeof(float));
  }

  void update(const DataBlock &layers, dag::ConstSpan<HmapDetLayerProps> dlp, const FastNameMap &detLayersMap)
  {
    if (!layers.blockCount())
      return;
    d3d::setablend(0);
    d3d::setsepablend(0);
    d3d::setzenable(0);
    d3d::setzwrite(0);
    d3d::setdepthbias(0.0f);
    d3d::setslopedepthbias(0.0f);
    d3d::writemask(WRITEMASK_ALL);
    d3d::setcull(CULL_NONE);

    static Tab<TEXTUREID> usedTexId(tmpmem);
    // DAEDITOR3.conNote("update DetailRenderData");

    Driver3dRenderTarget prevRt;
    d3d::get_render_target(prevRt);

    int i_reg[4] = {0, 0, 0, 0};
    float f_reg[4] = {0, 0, 0, 0};
    E3DCOLOR c;
    TEXTUREID texId;

    Texture *cmTex = colorMapTex2, *dmTex = detailMaskTex2;
    d3d::set_render_target();

    if (defColorTexId != BAD_TEXTUREID)
      copyRender(defColorTex, colorMapTex1);
    else
    {
      d3d::set_render_target(0, colorMapTex1, 0); // copy here instead!
      d3d::clearview(CLEAR_TARGET, 0x00000000, 1.0, 0);
    }
    if (defDetailTexId != BAD_TEXTUREID)
      copyRender(defDetailTex, detailMaskTex);
    else
    {
      d3d::set_render_target(0, detailMaskTex, 0); // copy here instead!
      d3d::clearview(CLEAR_TARGET, 0xFF000000, 1.0, 0);
    }

    usedTexId.clear();
    detailMaskTex->texfilter(TEXFILTER_POINT);
    for (int i = 0; i < layers.blockCount(); i++)
    {
      const DataBlock &b = *layers.getBlock(i);
      int idx = detLayersMap.getNameId(b.getStr("lName", ""));
      if (idx < 0)
        continue;

      Texture *cmTex2 = (cmTex == colorMapTex1) ? colorMapTex2 : colorMapTex1,
              *dmTex2 = (dmTex == detailMaskTex) ? detailMaskTex2 : detailMaskTex;

      d3d::settex(0, cmTex2);
      d3d::settex(1, NULL); //== hmap
      if (dlp[idx].needsMask)
        if (Texture *t = getMaskTex(b.getStr("lMask")))
        {
          d3d::settex(2, t);
        }
        else
          d3d::settex(2, NULL); //== create and mount 1-pix white tex
      d3d::settex(3, dmTex2);
      d3d::settex(4, NULL);

      for (int j = 0; j < dlp[idx].param.size(); j++)
        switch (dlp[idx].param[j].type)
        {
          case HmapDetLayerProps::Param::PT_int:
            f_reg[0] = b.getInt(dlp[idx].param[j].name, 0);
            d3d::set_ps_const(dlp[idx].param[j].regIdx, f_reg, 1);
            break;
          case HmapDetLayerProps::Param::PT_bool:
            i_reg[0] = b.getBool(dlp[idx].param[j].name, 0);
            d3d::set_ps_constb(dlp[idx].param[j].regIdx, i_reg, 1);
            break;
          case HmapDetLayerProps::Param::PT_float:
            f_reg[0] = b.getReal(dlp[idx].param[j].name, 0);
            d3d::set_ps_const(dlp[idx].param[j].regIdx, f_reg, 1);
            break;
          case HmapDetLayerProps::Param::PT_float2:
            *((Point2 *)f_reg) = b.getPoint2(dlp[idx].param[j].name, Point2(0, 0));
            d3d::set_ps_const(dlp[idx].param[j].regIdx, f_reg, 1);
            break;
          case HmapDetLayerProps::Param::PT_tex:
            texId = getAuxTex(b.getStr(dlp[idx].param[j].name, ""));
            if (BaseTexture *t = acquire_managed_tex(texId))
            {
              usedTexId.push_back(texId);
              d3d::settex(dlp[idx].param[j].regIdx, t);
            }
            else
              d3d::settex(dlp[idx].param[j].regIdx, NULL);
            break;
          case HmapDetLayerProps::Param::PT_color:
            c = b.getE3dcolor(dlp[idx].param[j].name, 0);
            f_reg[0] = c.r / 255.0;
            f_reg[1] = c.g / 255.0;
            f_reg[2] = c.b / 255.0;
            f_reg[3] = c.a / 255.0;
            d3d::set_ps_const(dlp[idx].param[j].regIdx, f_reg, 1);
            break;
        }
      d3d::set_render_target();
      d3d::set_render_target(0, cmTex, 0);

      if (dlp[idx].canOutSplatting && b.getBool("lOutSmap", true))
        d3d::set_render_target(1, dmTex, 0);

      d3d::set_program(dlp[idx].prog);
      d3d::draw_up(PRIM_TRILIST, 1, full_vert, 2 * sizeof(float));

      if (b.getBool("lOutCmap", true))
        cmTex = cmTex2;
      if (dlp[idx].canOutSplatting && b.getBool("lOutSmap", true))
        dmTex = dmTex2;
    }
    copyRender(cmTex == colorMapTex1 ? colorMapTex2 : colorMapTex1, colorMapTex);
    // d3d::stretch_rect(cmTex == colorMapTex1 ? colorMapTex2 : colorMapTex1, colorMapTex, NULL, NULL);
    if (dmTex != detailMaskTex2)
      d3d::stretch_rect(detailMaskTex2, detailMaskTex, NULL, NULL);
    detailMaskTex->texfilter(TEXFILTER_DEFAULT);
    d3d::set_render_target(prevRt);
    for (int i = 0; i < usedTexId.size(); i++)
      release_managed_tex(usedTexId[i]);
    ShaderElement::invalidate_cached_state_block();
  }

  Texture *getMaskTex(const char *name)
  {
    // DAEDITOR3.conNote("get DetailRenderData maskTex(%s)", name);
    for (int i = 0; i < maskTex.size(); i++)
      if (strcmp(name, maskTex[i].name) == 0)
        return maskTex[i].tex;
    return NULL;
  }
  TEXTUREID getAuxTex(const char *name)
  {
    // DAEDITOR3.conNote("get DetailRenderData auxTex(%s)", name);
    return name && *name ? get_managed_texture_id(String(0, "%s*", name)) : BAD_TEXTUREID;
  }

  void storeData(const char *prefix, bool store_cmap, bool store_smap)
  {
    // debug("storeData(%s, %d, %d) cmap=%s smask=%s", prefix, store_cmap, store_smap, colorMapName, detailMaskName);
    if (store_cmap && !colorMapName.isEmpty())
    {
      String fn(0, "%s/%s.tga", prefix, colorMapName);
      dd_mkpath(fn);
      save_rt_image_as_tga(colorMapTex, fn);
    }
    if (store_smap && !detailMaskName.isEmpty())
    {
      String fn(0, "%s/%s.tga", prefix, detailMaskName);
      dd_mkpath(fn);
      save_rt_image_as_tga(detailMaskTex, fn);
    }
  }
};
