// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../av_plugin.h"
#include <assets/asset.h>
#include <libTools/renderUtil/dynRenderBuf.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <image/dag_texPixel.h>
#include <de3_bitMaskMgr.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>

#include <propPanel/control/container.h>
#include <propPanel/c_control_event_handler.h>


class TifMaskViewPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  TifMaskViewPlugin() : tex(NULL) { texId = register_managed_tex(":av_tif", NULL); }
  ~TifMaskViewPlugin() override { release_managed_tex_verified(texId, tex); }

  const char *getInternalName() const override { return "tifViewer"; }

  void registered() override {}
  void unregistered() override {}

  bool begin(DagorAsset *asset) override
  {
    IBitMaskImageMgr *bmImgMgr = EDITORCORE->queryEditorInterface<IBitMaskImageMgr>();
    if (!bmImgMgr)
      return false;

    IBitMaskImageMgr::BitmapMask img;
    if (!bmImgMgr->loadImage(img, NULL, asset->getTargetFilePath()))
      return false;

    TexImage32 *im = (TexImage32 *)tmpmem->alloc(sizeof(TexImage32) + img.getWidth() * img.getHeight() * 4);
    TexPixel32 *p = (TexPixel32 *)(im + 1);

    im->w = img.getWidth();
    im->h = img.getHeight();
    halfSz.y = im->w / 20.0;
    halfSz.x = im->h / 20.0;
    switch (img.getBitsPerPixel())
    {
      case 1:
        for (int y = 0; y < im->h; y++)
          for (int x = 0; x < im->w; x++, p++)
            p->r = p->g = p->b = p->a = img.getMaskPixel1(x, y) ? 255 : 0;
        break;
      case 2:
        for (int y = 0; y < im->h; y++)
          for (int x = 0; x < im->w; x++, p++)
            p->r = p->g = p->b = p->a = img.getMaskPixel2(x, y);
        break;
      case 4:
        for (int y = 0; y < im->h; y++)
          for (int x = 0; x < im->w; x++, p++)
            p->r = p->g = p->b = p->a = img.getMaskPixel4(x, y);
        break;
      case 8:
        for (int y = 0; y < im->h; y++)
          for (int x = 0; x < im->w; x++, p++)
            p->r = p->g = p->b = p->a = img.getMaskPixel8(x, y);
        break;
    }
    bmImgMgr->destroyImage(img);

    tex = d3d::create_tex(im, 0, 0, TEXFMT_A8R8G8B8, 1);
    tmpmem->free(im);

    change_managed_texture(texId, tex);
    if (EDITORCORE->getCurrentViewport())
    {
      BBox3 bbox;
      if (getSelectionBox(bbox))
        EDITORCORE->getCurrentViewport()->zoomAndCenter(bbox);
    }
    return true;
  }
  bool end() override
  {
    rbuf.addFaces(BAD_TEXTUREID);
    rbuf.clearBuf();
    halfSz.x = halfSz.y = 1;
    if (tex)
    {
      change_managed_texture(texId, NULL);
      del_d3dres(tex);
    }
    return true;
  }
  IGenEventHandler *getEventHandler() override { return NULL; }

  void clearObjects() override {}
  void onSaveLibrary() override {}
  void onLoadLibrary() override {}

  bool getSelectionBox(BBox3 &box) const override
  {
    box.lim[0] = Point3(-halfSz.x, 0, -halfSz.y);
    box.lim[1] = Point3(halfSz.x, 4, halfSz.y);
    return true;
  }

  void actObjects(float dt) override {}
  void beforeRenderObjects() override {}
  void renderObjects() override {}
  void renderTransObjects() override {}

  void renderGeometry(Stage stage) override
  {
    if (!tex || !getVisible())
      return;

    switch (stage)
    {
      case STG_RENDER_STATIC_TRANS:
        rbuf.drawQuad(Point3(-halfSz.x, 0.5, -halfSz.y), Point3(halfSz.x, 0.5, -halfSz.y), Point3(halfSz.x, 0.5, halfSz.y),
          Point3(-halfSz.x, 0.5, halfSz.y), E3DCOLOR(255, 255, 0, 255));
        rbuf.addFaces(texId);
        rbuf.flush();
        rbuf.clearBuf();
        break;

      default: break;
    }
  }

  bool supportAssetType(const DagorAsset &asset) const override { return strcmp(asset.getTypeStr(), "tifMask") == 0; }

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override { panel.setEventHandler(this); }
  void postFillPropPanel() override {}

private:
  Texture *tex;
  TEXTUREID texId;
  Point2 halfSz;
  DynRenderBuffer rbuf;
};

static InitOnDemand<TifMaskViewPlugin> plugin;


void init_plugin_tif_mask()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
