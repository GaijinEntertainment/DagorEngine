#include "../av_plugin.h"
#include <assets/asset.h>
#include <libTools/renderUtil/dynRenderBuf.h>
#include <3d/dag_drv3d.h>
#include <image/dag_texPixel.h>
#include <de3_bitMaskMgr.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>

#include <propPanel2/c_panel_base.h>
#include <propPanel2/c_control_event_handler.h>


class TifMaskViewPlugin : public IGenEditorPlugin, public ControlEventHandler
{
public:
  TifMaskViewPlugin() : tex(NULL) { texId = register_managed_tex(":av_tif", NULL); }
  ~TifMaskViewPlugin() { release_managed_tex_verified(texId, tex); }

  virtual const char *getInternalName() const { return "tifViewer"; }

  virtual void registered() {}
  virtual void unregistered() {}

  virtual bool begin(DagorAsset *asset)
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
  virtual bool end()
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
  virtual IGenEventHandler *getEventHandler() { return NULL; }

  virtual void clearObjects() {}
  virtual void onSaveLibrary() {}
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const
  {
    box.lim[0] = Point3(-halfSz.x, 0, -halfSz.y);
    box.lim[1] = Point3(halfSz.x, 4, halfSz.y);
    return true;
  }

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects() {}
  virtual void renderObjects() {}
  virtual void renderTransObjects() {}

  virtual void renderGeometry(Stage stage)
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
    }
  }

  virtual bool supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "tifMask") == 0; }

  virtual void fillPropPanel(PropertyContainerControlBase &panel) { panel.setEventHandler(this); }
  virtual void postFillPropPanel() {}

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
