#include <de3_genObjByMask.h>
#include <de3_genObjByDensGridMask.h>
#include <de3_genGrassByDensGridMask.h>

#include "landClassSlotsMgr.h"
#include "hmlPlugin.h"
#include <oldEditor/de_interface.h>
#include <oldEditor/de_workspace.h>
#include <de3_landClassData.h>
#include <de3_assetService.h>
#include <de3_hmapService.h>
#include <de3_hmapStorage.h>
#include <dllPluginCore/core.h>
#include <util/dag_hierBitMap2d.h>
#include <math/random/dag_random.h>
#include <debug/dag_debug.h>
#include <coolConsole/coolConsole.h>
#include <perfMon/dag_cpuFreq.h>
#include <stdio.h>

static IAssetService *assetSrv = NULL;
static IRendInstGenService *rigenSrv = NULL;
static int tiledByLandclassSubTypeId = -1;

#define EXPORT_AXIS_STEPS 4

#define PSEUDO_PLANE_SIZE         8192
#define PSEUDO_PLANE_GEN_CELL_SZ  1024
#define PSEUDO_PLANE_GRID_CELL_SZ 16

static bool __stdcall ri_land_get_height(Point3 &pos, Point3 *out_norm)
{
  if (!HmapLandPlugin::self)
    return false;
  return HmapLandPlugin::self->getHeight(Point2::xz(pos), pos.y, out_norm);
}

LandClassSlotsManager::LandClassSlotsManager(int layer_count)
{
  grid2world = 1.0;
  world0x = 0;
  world0y = 0;

  clear_and_resize(layers, layer_count);
  mem_set_0(layers);
  if (!assetSrv)
    assetSrv = DAGORED2->queryEditorInterface<IAssetService>();
  if (!rigenSrv)
    rigenSrv = DAGORED2->queryEditorInterface<IRendInstGenService>();
  tiledByLandclassSubTypeId = IDaEditor3Engine::get().registerEntitySubTypeId("mask_tile");

  if (rigenSrv)
    rigenSrv->setCustomGetHeight(&ri_land_get_height);
}


landclass::AssetData *LandClassSlotsManager::setLandClass(int layer_idx, int landclass_idx, const char *asset_name)
{
  if (layer_idx < 0 || layer_idx >= layers.size() || landclass_idx < 0)
    return NULL;

  if (!layers[layer_idx])
  {
    if (asset_name)
      layers[layer_idx] = new (midmem) PerLayer;
    else
      return NULL;
  }

  PerLayer &pl = *layers[layer_idx];
  if (landclass_idx >= pl.cls.size())
  {
    if (!asset_name)
      return NULL;

    int st = pl.cls.size();
    pl.cls.resize(landclass_idx + 1);
    for (int i = st; i < pl.cls.size(); i++)
      pl.cls[i] = NULL;
  }

  if (!pl.cls[landclass_idx])
  {
    if (!asset_name)
      return NULL;

    pl.cls[landclass_idx] = new (midmem) PerLayer::Rec(asset_name);
    pl.cls[landclass_idx]->reload();
    reinitRIGen();
    return pl.cls[landclass_idx]->landClass;
  }
  else if (!asset_name)
  {
    // debug("delete  slot %d/%d: %s", layer_idx, landclass_idx, (char*)pl.cls[landclass_idx]->name);
    del_it(pl.cls[landclass_idx]);
    reinitRIGen();
    DAGORED2->invalidateViewportCache();
    return NULL;
  }
  else
  {
    PerLayer::Rec &r = *pl.cls[landclass_idx];
    if (stricmp(r.name, asset_name) == 0)
      return r.landClass;

    r.name = asset_name;
    r.reload();
    reinitRIGen();
    return r.landClass;
  }
}

void LandClassSlotsManager::reinitRIGen(bool reset_lc_cache)
{
  if (!rigenSrv || !HmapLandPlugin::self)
    return;
  if (!HmapLandPlugin::self->getlandClassMap().isFileOpened() && !HmapLandPlugin::self->isPseudoPlane())
    return;

  Tab<rendinst::gen::land::AssetData *> land_cls;

  rigenSrv->releaseRtRIGenData();
  for (int i = 0; i < layers.size(); i++)
  {
    if (!layers[i])
      continue;
    PerLayer &pl = *layers[i];
    for (int j = 0; j < pl.cls.size(); j++)
      if (pl.cls[j] && pl.cls[j]->rigenLand)
      {
        rigenSrv->releaseLandClassGameRes(pl.cls[j]->rigenLand);
        pl.cls[j]->rigenLand = NULL;
      }
  }
  if (reset_lc_cache)
    rigenSrv->resetLandClassCache();

  for (int i = 0; i < layers.size(); i++)
  {
    if (!layers[i])
      continue;
    PerLayer &pl = *layers[i];
    for (int j = 0; j < pl.cls.size(); j++)
      if (pl.cls[j])
      {
        pl.cls[j]->rigenLand = rigenSrv->getLandClassGameRes(pl.cls[j]->name);
        land_cls.push_back(pl.cls[j]->rigenLand);
      }
  }

  DataBlock appBlk(DAGORED2->getWorkspace().getAppPath());
  int lcmap_w = HmapLandPlugin::self->getlandClassMap().getMapSizeX(), lcmap_h = HmapLandPlugin::self->getlandClassMap().getMapSizeY();

  float gridCellSize = grid2world;
  float meshCellSize = HmapLandPlugin::self->getMeshCellSize();
  float w0x = world0x, w0y = world0y;
  float border_w = appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getReal("outerLandBorder", 1024.0f);
  outer_border = 0;
  if (HmapLandPlugin::self->isPseudoPlane())
  {
    lcmap_w = lcmap_h = PSEUDO_PLANE_SIZE / PSEUDO_PLANE_GRID_CELL_SZ;
    meshCellSize = PSEUDO_PLANE_GEN_CELL_SZ;
    G_ASSERTF(world0x == world0y && world0y == -PSEUDO_PLANE_SIZE / 2, "world0=%g,%g PSEUDO_PLANE_SIZE=%g", world0x, world0y,
      PSEUDO_PLANE_SIZE);
    G_ASSERTF(grid2world == PSEUDO_PLANE_GRID_CELL_SZ, "grid2world=%g PSEUDO_PLANE_GRID_CELL_SZ=%g", grid2world,
      PSEUDO_PLANE_GRID_CELL_SZ);
  }
  else if (border_w > 0)
  {
    outer_border = (int)ceilf(border_w / meshCellSize) * int(meshCellSize / gridCellSize);
    lcmap_w += outer_border * 2;
    lcmap_h += outer_border * 2;
    w0x -= outer_border * gridCellSize;
    w0y -= outer_border * gridCellSize;
  }

  float minGridCellSize = appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getReal("minGridCellSize", 32);
  int minGridCellCount = appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getInt("minGridCellCount", 4);

  int ri_cell_sz = meshCellSize / gridCellSize;
  while (ri_cell_sz > 1 && ri_cell_sz * gridCellSize >= minGridCellSize * 2 &&
         ((lcmap_w / ri_cell_sz) * (lcmap_h / ri_cell_sz) < minGridCellCount || ri_cell_sz * gridCellSize > 2048 ||
           (lcmap_w % ri_cell_sz) || (lcmap_h % ri_cell_sz)))
    ri_cell_sz /= 2;

  int msq_stride = ((lcmap_w + ri_cell_sz - 1) / ri_cell_sz), msq_plane = msq_stride * ((lcmap_h + ri_cell_sz - 1) / ri_cell_sz);
  debug("reinitRIGen: %d cls, riCell: %d (%.1f m), meshCell=%.1f m (number of ri cells: %d, %dx%d at %.1f,%.1f)"
        " minGridCellSize=%.0f minGridCellCount=%d",
    land_cls.size(), ri_cell_sz, ri_cell_sz * gridCellSize, meshCellSize, msq_plane, msq_stride, msq_plane / msq_stride, w0x, w0y,
    minGridCellSize, minGridCellCount);

  rigenSrv->createRtRIGenData(w0x, w0y, grid2world, ri_cell_sz, msq_stride, msq_plane / msq_stride, land_cls, world0x, world0y);
  for (int i = 0; i < layers.size(); i++)
  {
    if (!layers[i])
      continue;
    PerLayer &pl = *layers[i];
    for (int j = 0; j < pl.cls.size(); j++)
      if (pl.cls[j])
        pl.cls[j]->rigenBm = rigenSrv->getRIGenBitMask(pl.cls[j]->rigenLand);
  }

  for (int my = outer_border; my < lcmap_h - outer_border; my++)
    for (int mx = outer_border; mx < lcmap_w - outer_border; mx++)
    {
      unsigned w = HmapLandPlugin::self->getlandClassMap().getData(mx - outer_border, my - outer_border);

      for (int i = layers.size() - 1; i >= 0; i--)
      {
        if (!layers[i])
          continue;

        PerLayer &pl = *layers[i];
        int idx = landClsLayer[i].getLayerData(w);
        if (idx < pl.cls.size() && pl.cls[idx] && pl.cls[idx]->rigenBm)
          pl.cls[idx]->rigenBm->set(mx, my);
      }
    }
  rigenSrv->setSweepMask(objgenerator::lcmapExcl.bm, objgenerator::lcmapExcl.ox, objgenerator::lcmapExcl.oz,
    objgenerator::lcmapExcl.scale);
  rigenSrv->setReinitCallback(&reinitRIGenCallBack, this);
}

landclass::AssetData *LandClassSlotsManager::getLandClass(int layer_idx, int landclass_idx)
{
  if (layer_idx < 0 || layer_idx >= layers.size() || !layers[layer_idx] || landclass_idx < 0)
    return NULL;

  PerLayer &pl = *layers[layer_idx];
  if (landclass_idx >= pl.cls.size() || !pl.cls[landclass_idx])
    return NULL;
  return pl.cls[landclass_idx]->landClass;
}

void LandClassSlotsManager::reloadLandClass(int layer_idx, int landclass_idx)
{
  if (layer_idx < 0 || layer_idx >= layers.size() || !layers[layer_idx] || landclass_idx < 0)
    return;

  PerLayer &pl = *layers[layer_idx];
  if (landclass_idx >= pl.cls.size() || !pl.cls[landclass_idx])
    return;
  pl.cls[landclass_idx]->reload();
}

void LandClassSlotsManager::clear()
{
  for (int i = layers.size() - 1; i >= 0; i--)
    del_it(layers[i]);
  clear_and_shrink(layers);
  outer_border = 0;
}

landclass::AssetData *LandClassSlotsManager::getAsset(const char *asset_name)
{
  return assetSrv ? assetSrv->getLandClassData(asset_name) : NULL;
}

void LandClassSlotsManager::releaseAsset(landclass::AssetData *asset)
{
  if (assetSrv && asset)
    assetSrv->releaseLandClassData(asset);
}
void LandClassSlotsManager::subscribeLandClassUpdateNotify(IAssetUpdateNotify *n)
{
  if (assetSrv)
    assetSrv->subscribeUpdateNotify(n, true, false);
}
void LandClassSlotsManager::unsubscribeLandClassUpdateNotify(IAssetUpdateNotify *n)
{
  if (assetSrv)
    assetSrv->unsubscribeUpdateNotify(n, true, false);
}


void LandClassSlotsManager::PerLayer::Rec::clearData()
{
  if (assetSrv && landClass)
    assetSrv->releaseLandClassData(landClass);
  landClass = NULL;
  if (rigenSrv)
    rigenSrv->releaseLandClassGameRes(rigenLand);
  rigenLand = NULL;
  rigenBm = NULL;
}

void LandClassSlotsManager::PerLayer::Rec::reload()
{
  clearData();

  // debug("reload %s", (char*)name);
  if (assetSrv)
    landClass = assetSrv->getLandClassData(name);
  entHolder.reset();
  changed = true;
  DAGORED2->invalidateViewportCache();
}

void LandClassSlotsManager::onLandSizeChanged(float gridStep, float ofs_x, float ofs_y,
  dag::ConstSpan<HmapBitLayerDesc> land_cls_layer)
{
  grid2world = gridStep;
  world0x = ofs_x;
  world0y = ofs_y;
  landClsLayer.set(land_cls_layer.data(), land_cls_layer.size());
  if (HmapLandPlugin::self && HmapLandPlugin::self->isPseudoPlane())
  {
    grid2world = PSEUDO_PLANE_GRID_CELL_SZ;
    world0x = world0y = -PSEUDO_PLANE_SIZE / 2;
  }

  tileSize = 256.0 / gridStep;
  if (tileSize < 1)
    tileSize = 1;

  // debug("onLandSizeChanged(%.3f, %.3f, %.3f) tileSize=%d", grid2world, world0x, world0y, tileSize);
  for (int i = layers.size() - 1; i >= 0; i--)
  {
    if (!layers[i])
      continue;

    PerLayer &pl = *layers[i];
    for (int j = pl.cls.size() - 1; j >= 0; j--)
      if (pl.cls[j])
        pl.cls[j]->entHolder.reset();
  }
  if (HmapLandPlugin::self)
    reinitRIGen();
  else if (rigenSrv)
    rigenSrv->clearRtRIGenData();
}

typedef HierBitMap2d<ConstSizeBitMap2d<3>> HierBitmap8;

void LandClassSlotsManager::onLandRegionChanged(int x0, int y0, int x1, int y1, MapStorage<uint32_t> &landClsMap, bool all_classes,
  bool dont_regenerate, bool finished)
{
  int tx0 = x0 / tileSize, ty0 = y0 / tileSize;
  int tx1 = (x1 + tileSize - 1) / tileSize, ty1 = (y1 + tileSize - 1) / tileSize;
  float box_sz = grid2world * tileSize;
  float world2grid = 1.0 / grid2world;
  Tab<HierBitmap8> bmps(tmpmem);
  Tab<PerLayer::Rec *> rec(tmpmem);
  int lcmap_w = landClsMap.getMapSizeX(), lcmap_h = landClsMap.getMapSizeY();
  // long update handling
  int64_t reft = ::ref_time_ticks();
  bool con_shown = false;
  int total_cnt = (ty1 - ty0) * (tx1 - tx0);
  int done_cnt = 0;
  CoolConsole &con = DAGORED2->getConsole();
  int rendInst_atype = DAEDITOR3.getAssetTypeId("rendInst");
  bool rigen_changed = false;

#define PROGRESS_PT(done)                             \
  if (con_shown)                                      \
    con.setDone(done);                                \
  else if (get_time_usec(reft) > 1500000)             \
  {                                                   \
    con_shown = true;                                 \
    con.startProgress();                              \
    con.setTotal(total_cnt);                          \
    con.setDone(done);                                \
    con.setActionDesc("Updating landclass tiles..."); \
  }

  int grass_attr = HmapLandPlugin::hmlService->getBitLayerAttrId(HmapLandPlugin::self->getLayersHandle(), "grass");
  for (int i = layers.size() - 1; i >= 0; i--)
  {
    if (!layers[i])
      continue;
    PerLayer &pl = *layers[i];
    for (int j = pl.cls.size() - 1; j >= 0; j--)
      if (pl.cls[j])
      {
        if ((all_classes || pl.cls[j]->changed) && pl.cls[j]->landClass)
        {
          pl.cls[j]->tmpBmpIdx = rec.size();
          rec.push_back(pl.cls[j]);
          pl.cls[j]->grassLayer =
            (grass_attr != -1 && HmapLandPlugin::hmlService->testBitLayerAttr(HmapLandPlugin::self->getLayersHandle(), i, grass_attr));
        }
        else
          pl.cls[j]->tmpBmpIdx = -1;
      }
  }
  bmps.resize(rec.size());
  for (int c = 0; c < bmps.size(); c++)
    bmps[c].resize(tileSize, tileSize);

  // debug("onLandRegionChanged(%d, %d, %d, %d), bmps=%d", x0, y0, x1, y1, bmps.size());
  if (!bmps.size())
    return;

  PROGRESS_PT(0);
  for (int ty = ty0; ty < ty1; ty++)
    for (int tx = tx0; tx < tx1; tx++, done_cnt++)
    {
      PROGRESS_PT(done_cnt);

      if (dont_regenerate)
      {
        bool need_gen = false;
        for (int c = 0; c < rec.size(); c++)
        {
          if (!rec[c]->landClass->tiled && !rec[c]->landClass->planted && !rec[c]->landClass->grass)
            continue;

          TiledEntityHolder::Tile *tile = rec[c]->entHolder.getTile(tx, ty);
          if (!tile || tile->discarded)
          {
            need_gen = true;
            break;
          }
        }
        if (!need_gen)
          continue;
      }

      for (int c = 0; c < bmps.size(); c++)
        bmps[c].reset();

      int tmx_end = lcmap_w - tx * tileSize;
      int tmy_end = lcmap_h - ty * tileSize;

      if (tmx_end > tileSize)
        tmx_end = tileSize;
      if (tmy_end > tileSize)
        tmy_end = tileSize;

      for (int my = ty * tileSize, tmy = 0; tmy < tmy_end; my++, tmy++)
        for (int mx = tx * tileSize, tmx = 0; tmx < tmx_end; mx++, tmx++)
        {
          unsigned w = landClsMap.getData(mx, my);

          for (int i = layers.size() - 1; i >= 0; i--)
          {
            if (!layers[i])
              continue;

            PerLayer &pl = *layers[i];
            int idx = landClsLayer[i].getLayerData(w);
            bool idx_valid = idx >= 0 && idx < pl.cls.size() && pl.cls[idx];
            if (idx_valid && pl.cls[idx]->tmpBmpIdx != -1)
              bmps[pl.cls[idx]->tmpBmpIdx].set(tmx, tmy);
            if (idx_valid && pl.cls[idx]->rigenBm && !pl.cls[idx]->rigenBm->get(mx + outer_border, my + outer_border))
            {
              rigen_changed = true;
              pl.cls[idx]->rigenBm->set(mx + outer_border, my + outer_border);
            }
            for (int k = 0; k < pl.cls.size(); k++)
              if (k != idx && pl.cls[k] && pl.cls[k]->rigenBm && (!idx_valid || pl.cls[idx]->rigenBm != pl.cls[k]->rigenBm) &&
                  pl.cls[k]->rigenBm->get(mx + outer_border, my + outer_border))
              {
                rigen_changed = true;
                pl.cls[k]->rigenBm->clr(mx + outer_border, my + outer_border);
              }
          }
        }

      for (int c = 0; c < bmps.size(); c++)
      {
        if (!rec[c]->landClass->tiled && !rec[c]->landClass->planted && !rec[c]->landClass->grass)
        {
          TiledEntityHolder::Tile *tile = rec[c]->entHolder.getTile(tx, ty);
          if (tile)
          {
            tile->clearAllObjects();
            tile->discarded = false;
          }
          continue;
        }

        TiledEntityHolder::Tile &tile = rec[c]->entHolder.createTile(tx, ty);
        if (bmps[c].isEmpty())
        {
          tile.clearAllObjects();
          tile.discarded = false;
          continue;
        }

        if (dont_regenerate && !tile.discarded)
          continue;

        if (!rec[c]->landClass->tiled || rec[c]->landClass->tiled->riOnly)
          tile.clearObjects(tile.TILED);
        else
        {
          // debug("rec[%d]=%p t=%d,%d", c, rec[c]->landClass->tiled, tx, ty);
          if (!tile.pool[tile.TILED].size())
            tile.resize(tile.TILED, rec[c]->landClass->tiled->data.size());

          tile.beginGenerate(tile.TILED);
          objgenerator::generateTiledEntitiesInMaskedRect(*rec[c]->landClass->tiled, tiledByLandclassSubTypeId, 62,
            HmapLandPlugin::self, tile.pool[tile.TILED], bmps[c], world2grid, tx * box_sz, ty * box_sz, box_sz, box_sz, world0x,
            world0y, rendInst_atype);
          tile.endGenerate(tile.TILED);
        }

        if (!rec[c]->landClass->planted || rec[c]->landClass->planted->riOnly)
          tile.clearObjects(tile.PLANTED);
        else
        {
          // debug("rec[%d]=%p t=%d,%d", c, rec[c]->landClass->planted, tx, ty);
          if (!tile.pool[tile.PLANTED].size())
            tile.resize(tile.PLANTED, rec[c]->landClass->planted->ent.size());

          tile.beginGenerate(tile.PLANTED);
          objgenerator::generatePlantedEntitiesInMaskedRect(*rec[c]->landClass->planted, tiledByLandclassSubTypeId, 62,
            HmapLandPlugin::self, tile.pool[tile.PLANTED], bmps[c], world2grid, tx * box_sz, ty * box_sz, box_sz, box_sz, world0x,
            world0y, 0, rendInst_atype);
          tile.endGenerate(tile.PLANTED);
        }

        if (!rec[c]->grassLayer || !rec[c]->landClass->grass)
          tile.clearGrass();
        else if (finished)
        {
          if (!tile.grass.size())
          {
            clear_and_resize(tile.grass, rec[c]->landClass->grass->data.size());
            for (int i = 0; i < tile.grass.size(); i++)
              tile.grass[i] = IDaEditor3Engine::get().cloneEntity(rec[c]->landClass->grass->data[i].entity);
          }

          for (int i = 0; i < tile.grass.size(); i++)
          {
            if (!tile.grass[i])
              continue;
            IGrassPlanting *gp = tile.grass[i]->queryInterface<IGrassPlanting>();
            if (!gp)
              continue;

            gp->startGrassPlanting((tx + 0.5) * box_sz + world0x, (ty + 0.5) * box_sz + world0y, 0.71 * box_sz);
            objgenerator::generateGrassInMaskedRect(rec[c]->landClass->grass->data[i], gp, bmps[c], world2grid, tx * box_sz,
              ty * box_sz, box_sz, box_sz, world0x, world0y);
            if (gp->finishGrassPlanting() == 0)
              tile.clearGrass();
          }
        }

        tile.discarded = false;
      }
    }
  PROGRESS_PT(done_cnt);
  DAGORED2->restoreEditorColliders();

  if (!dont_regenerate)
  {
    for (int c = 0; c < rec.size(); c++)
      rec[c]->changed = false;
    if (rigenSrv && HmapLandPlugin::self && (rigen_changed || all_classes))
      rigenSrv->discardRIGenRect(x0, y0, x1, y1);
  }
  if (con_shown)
    con.endProgress();
}
void LandClassSlotsManager::regenerateObjects(MapStorage<uint32_t> &landClsMap, bool only_changed)
{
  reinitRIGen();
  onLandRegionChanged(0, 0, landClsMap.getMapSizeX(), landClsMap.getMapSizeY(), landClsMap, !only_changed);

  if (HmapLandPlugin::self)
    HmapLandPlugin::self->resetLandmesh();
}

#include <util/dag_roHugeHierBitMap2d.h>
#include <libTools/util/binDumpHierBitmap.h>

#define CHECK_ROHHB 0

#if CHECK_ROHHB
template <class Bitmap>
static void write_bm_tiff(const Bitmap &bm, const char *fn)
{
  int w = bm.getW(), h = bm.getH();
  IBitMaskImageMgr::BitmapMask img;
  HmapLandPlugin::bitMaskImgMgr->createBitMask(img, w, h, 1);

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      img.setMaskPixel1(x, y, bm.get(x, y) ? 128 : 0);

  HmapLandPlugin::bitMaskImgMgr->saveImage(img, ".", String(260, "%s.tif", fn));
  HmapLandPlugin::bitMaskImgMgr->destroyImage(img);
}
#endif

static void serialize_huge_bm(const objgenerator::HugeBitmask &bm, const char *fn, unsigned target_code)
{
  String base;
  DAGORED2->getProjectFolderPath(base);

#if CHECK_ROHHB
  write_bm_tiff(bm, fn);
#endif

  mkbindump::BinDumpSaveCB cwr(1 << 20, target_code, dagor_target_code_be(target_code));
  mkbindump::build_ro_hier_bitmap(cwr, bm, 4, 3);
  FullFileSaveCB fcwr(String(260, "%s/%s.bin", base.str(), fn));
  cwr.copyDataTo(fcwr);
  fcwr.close();

#if CHECK_ROHHB
  if (!dagor_target_code_be(target_code))
  {
    FullFileLoadCB crd(String(260, "%s/%s.bin", base.str(), fn));
    RoHugeHierBitMap2d<4, 3> *roBm = RoHugeHierBitMap2d<4, 3>::create(crd);
    if (roBm)
    {
      debug("%s %dx%d -> %dx%d", fn, bm.getW(), bm.getH(), roBm->getW(), roBm->getH());
      write_bm_tiff(*roBm, String(260, "%s_rep", fn));
      delete roBm;
    }
  }
#endif
}

void LandClassSlotsManager::exportEntityGenDataToFile(MapStorage<uint32_t> &land_cls_map, unsigned target_code)
{
  DataBlock appBlk(DAGORED2->getWorkspace().getAppPath());
  String base;
  DAGORED2->getProjectFolderPath(base);
  dd_mkdir(String(260, "%s/DoNotCommitMe", base.str()));

  DataBlock fileBlk;
  fileBlk.setBool("gen", true);
  fileBlk.setStr("target", mkbindump::get_target_str(target_code));

  for (int layer = 0; layer < rigenSrv->getRIGenLayers(); layer++)
    exportEntityGenDataLayer(land_cls_map, target_code, appBlk, base,
      layer == 0 ? fileBlk : *fileBlk.addNewBlock(String(0, "layer%d", layer)), layer);

  dblk::save_to_text_file_compact(fileBlk, String(260, "%s/DoNotCommitMe/sowedRi.blk", base.str()));
}
void LandClassSlotsManager::exportEntityGenDataLayer(MapStorage<uint32_t> &land_cls_map, unsigned target_code, const DataBlock &appBlk,
  const char *base, DataBlock &fileBlk, int layer_idx)
{
  Tab<objgenerator::HugeBitmask> bmps(tmpmem);
  Tab<PerLayer::Rec *> rec(tmpmem);
  Tab<int> rec_remap(tmpmem);
  int rec_used = 0;
  int lcmap_w = land_cls_map.getMapSizeX(), lcmap_h = land_cls_map.getMapSizeY();

  float gridCellSize = grid2world;
  float meshCellSize = HmapLandPlugin::self->getMeshCellSize();
  float minGridCellSize = appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getReal("minGridCellSize", 32);
  int minGridCellCount = appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getInt("minGridCellCount", 4);
  float w0x = world0x, w0y = world0y;
  float border_w = appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("riMgr")->getReal("outerLandBorder", 1024.0f);
  outer_border = 0;
  if (HmapLandPlugin::self->isPseudoPlane())
  {
    lcmap_w = lcmap_h = PSEUDO_PLANE_SIZE / PSEUDO_PLANE_GRID_CELL_SZ;
    meshCellSize = PSEUDO_PLANE_GEN_CELL_SZ;
    G_ASSERTF(world0x == world0y && world0y == -PSEUDO_PLANE_SIZE / 2, "world0=%g,%g PSEUDO_PLANE_SIZE=%g", world0x, world0y,
      PSEUDO_PLANE_SIZE);
    G_ASSERTF(grid2world == PSEUDO_PLANE_GRID_CELL_SZ, "grid2world=%g PSEUDO_PLANE_GRID_CELL_SZ=%g", grid2world,
      PSEUDO_PLANE_GRID_CELL_SZ);
  }
  else if (border_w > 0)
  {
    outer_border = (int)ceilf(border_w / meshCellSize) * int(meshCellSize / gridCellSize);
    lcmap_w += outer_border * 2;
    lcmap_h += outer_border * 2;
    w0x -= outer_border * gridCellSize;
    w0y -= outer_border * gridCellSize;
  }

  int ri_cell_sz = meshCellSize / gridCellSize;
  while (ri_cell_sz > 1 && ri_cell_sz * gridCellSize >= minGridCellSize * 2 &&
         ((lcmap_w / ri_cell_sz) * (lcmap_h / ri_cell_sz) < minGridCellCount || ri_cell_sz * gridCellSize > 2048 ||
           (lcmap_w % ri_cell_sz) || (lcmap_h % ri_cell_sz)))
    ri_cell_sz /= 2;
  if (int div = rigenSrv->getRIGenLayerCellSizeDivisor(layer_idx))
    ri_cell_sz = ri_cell_sz >= div ? ri_cell_sz / div : 1;

  Tab<int> msq(tmpmem);
  int msq_stride = (lcmap_w / ri_cell_sz), msq_plane = msq_stride * (lcmap_h / ri_cell_sz);
  debug("riCell: %d (%.1f m), meshCell=%.1f m (number of ri cells: %d) minGridCellSize=%.0f minGridCellCount=%d", ri_cell_sz,
    ri_cell_sz * gridCellSize, meshCellSize, msq_plane, minGridCellSize, minGridCellCount);

  for (int i = layers.size() - 1; i >= 0; i--)
  {
    if (!layers[i])
      continue;
    PerLayer &pl = *layers[i];
    for (int j = pl.cls.size() - 1; j >= 0; j--)
      if (pl.cls[j])
      {
        if (pl.cls[j]->landClass && pl.cls[j]->landClass->layerIdx == layer_idx &&
            (pl.cls[j]->landClass->tiled || pl.cls[j]->landClass->planted))
        {
          pl.cls[j]->tmpBmpIdx = rec.size();
          rec.push_back(pl.cls[j]);
        }
        else
          pl.cls[j]->tmpBmpIdx = -1;
      }
  }
  msq.resize(rec.size() * msq_plane);
  mem_set_0(msq);

  rec_remap.resize(rec.size());
  mem_set_ff(rec_remap);

  bmps.resize(rec.size());
  for (int c = 0; c < bmps.size(); c++)
    bmps[c].resize(lcmap_w, lcmap_h);
  for (int my = outer_border; my < lcmap_h - outer_border; my++)
    for (int mx = outer_border; mx < lcmap_w - outer_border; mx++)
    {
      unsigned w = land_cls_map.getData(mx - outer_border, my - outer_border);

      for (int i = layers.size() - 1; i >= 0; i--)
      {
        if (!layers[i])
          continue;

        PerLayer &pl = *layers[i];
        int idx = landClsLayer[i].getLayerData(w);
        if (idx < pl.cls.size() && pl.cls[idx] && pl.cls[idx]->tmpBmpIdx != -1)
        {
          int j = pl.cls[idx]->tmpBmpIdx;
          if (rec_remap[j] < 0)
            rec_remap[j] = rec_used++;
          bmps[j].set(mx, my);
          msq[msq_plane * j + (my / ri_cell_sz) * msq_stride + mx / ri_cell_sz]++;
        }
      }
    }

  Tab<int> mask_remap(tmpmem);
  int mask_cnt = 0;
  mask_remap.resize(rec.size());
  mem_set_ff(mask_remap);

  for (int i = layers.size() - 1; i >= 0; i--)
  {
    if (!layers[i])
      continue;
    PerLayer &pl = *layers[i];
    for (int j = pl.cls.size() - 1; j >= 0; j--)
    {
      if (!pl.cls[j] || pl.cls[j]->tmpBmpIdx < 0)
        continue;
      if (rec_remap[pl.cls[j]->tmpBmpIdx] < 0)
      {
        DAEDITOR3.conWarning("skip land <%s> from empty layer", pl.cls[j]->name);
        continue;
      }
      DataBlock *b = fileBlk.addNewBlock("mask");
      int idx = rec_remap[pl.cls[j]->tmpBmpIdx];
      b->setStr("mask", String(260, "%s/DoNotCommitMe/mask%d_%03d.bin", base, layer_idx, idx));
      b->setStr("land", pl.cls[j]->name);
      serialize_huge_bm(bmps[pl.cls[j]->tmpBmpIdx], String(260, "DoNotCommitMe/mask%d_%03d", layer_idx, idx), target_code);
      mask_remap[pl.cls[j]->tmpBmpIdx] = mask_cnt;
      int64_t total = 0;
      for (int k = 0; k < msq_plane; k++)
        total += msq[pl.cls[j]->tmpBmpIdx * msq_plane + k];
      b->setInt64(String(0, "m%d", mask_cnt), total);
      mask_cnt++;
    }
  }

  if (layer_idx == 0)
  {
    fileBlk.setStr("sweepMask", String(260, "%s/DoNotCommitMe/mask_sweep.bin", base));
    if (objgenerator::lcmapExcl.bm && !outer_border)
      serialize_huge_bm(*objgenerator::lcmapExcl.bm, "DoNotCommitMe/mask_sweep", target_code);
    else if (objgenerator::lcmapExcl.bm)
    {
      objgenerator::WorldHugeBitmask excl;
      float s = objgenerator::lcmapExcl.scale;
      excl.initExplicit(w0x, w0y, s, lcmap_w * gridCellSize * s, lcmap_h * gridCellSize * s);
      if (fmodf(outer_border * s, 1.0f) > 0)
      {
        DAEDITOR3.conError("Landscape/RI: exclusion map is converted with accuracy loss, scale=%g outer_border=%d, map=%dx%d", s,
          outer_border, land_cls_map.getMapSizeX(), land_cls_map.getMapSizeY());
        for (int my = 0, h = excl.bm->getH(); my < h; my++)
          for (int mx = 0, w = excl.bm->getW(); mx < w; mx++)
            if (objgenerator::lcmapExcl.isMarked(my / excl.scale + excl.ox, my / excl.scale + excl.oz))
              excl.bm->set(mx, my);
      }
      else
        for (int my = 0, h = objgenerator::lcmapExcl.bm->getH(), ofs = outer_border * gridCellSize * excl.scale; my < h; my++)
          for (int mx = 0, w = objgenerator::lcmapExcl.bm->getW(); mx < w; mx++)
            if (objgenerator::lcmapExcl.bm->get(mx, my))
              excl.bm->set(mx + ofs, my + ofs);
      serialize_huge_bm(*excl.bm, "DoNotCommitMe/mask_sweep", target_code);
    }
    else
    {
      objgenerator::HugeBitmask bm;
      bm.resize(128, 128);
      serialize_huge_bm(bm, "DoNotCommitMe/mask_sweep", target_code);
    }
  }

  fileBlk.setInt("maskNum", rec_used);
  fileBlk.setInt("riNumW", msq_stride);
  fileBlk.setInt("riNumH", msq_plane / msq_stride);
  fileBlk.setInt("riCellSz", ri_cell_sz);
  fileBlk.setReal("gridCell", gridCellSize);
  fileBlk.setReal("gridOfsX", w0x);
  fileBlk.setReal("gridOfsZ", w0y);
  fileBlk.setReal("densMapPivotX", world0x);
  fileBlk.setReal("densMapPivotZ", world0y);
  fileBlk.setReal("sweepMaskScale", objgenerator::lcmapExcl.bm ? objgenerator::lcmapExcl.scale : 0);

  for (int row = 0, num_rows = msq_plane / msq_stride; row < num_rows; row++)
  {
    DataBlock *b_row = fileBlk.addNewBlock("r");
    for (int col = 0; col < msq_stride; col++)
    {
      int i = row * msq_stride + col;
      DataBlock *b = b_row->addNewBlock("c");
      for (int j = 0; j < rec.size(); j++)
        if (msq[j * msq_plane + i] && mask_remap[j] >= 0)
          b->setInt(String(32, "m%d", mask_remap[j]), msq[j * msq_plane + i]);
    }
  }
}

void LandClassSlotsManager::TiledEntityHolder::Tile::clearGrass()
{
  for (int i = 0; i < grass.size(); i++)
    destroy_it(grass[i]);
  clear_and_shrink(grass);
}
void LandClassSlotsManager::TiledEntityHolder::Tile::clear(int pool_type)
{
  dag::Span<landclass::SingleEntityPool> &p = pool[pool_type];
  for (int i = 0; i < p.size(); i++)
    p[i].landclass::SingleEntityPool::~SingleEntityPool();
  p.reset();
}

void LandClassSlotsManager::TiledEntityHolder::Tile::resize(int pool_type, int count)
{
  clear(pool_type);

  dag::Span<landclass::SingleEntityPool> &p = pool[pool_type];
  p.set(nullptr, count);
  p.set((landclass::SingleEntityPool *)memalloc(data_size(p), midmem), count);
  for (int i = 0; i < p.size(); i++)
    new (&p[i], _NEW_INPLACE) landclass::SingleEntityPool;
}

void LandClassSlotsManager::TiledEntityHolder::Tile::beginGenerate(int pool_type)
{
  for (int gi = 0; gi < pool[pool_type].size(); gi++)
    pool[pool_type][gi].resetUsedEntities();
}

void LandClassSlotsManager::TiledEntityHolder::Tile::endGenerate(int pool_type)
{
  for (int gi = 0; gi < pool[pool_type].size(); gi++)
    pool[pool_type][gi].deleteUnusedEntities();
}

bool LandClassSlotsManager::TiledEntityHolder::Tile::isPoolEmpty(int pool_type)
{
  for (int gi = 0; gi < pool[pool_type].size(); gi++)
    if (pool[pool_type][gi].entUsed)
      return false;
  return true;
}

LandClassSlotsManager::TiledEntityHolder::TiledEntityHolder() : tiles(5) {}
