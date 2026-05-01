// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <de3_landMeshData.h>
#include <de3_hmapService.h>
#include <de3_hmapStorage.h>
#include <de3_genHmapData.h>
#include <de3_entityFilter.h>
#include <de3_objEntity.h>
#include <de3_hmapDetLayerProps.h>
#include "../../de_appwnd.h"
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_overrideStates.h>

#include <oldEditor/de_workspace.h>

#include <oldEditor/de_interface.h>
#include <oldEditor/de_util.h>

#include <landMesh/lmeshRenderer.h>
#include <landMesh/clipMap.h>
#include <landMesh/lastClip.h>
#include <heightmap/lodGrid.h>
#include <heightmap/simpleHeightmapRenderer.h>
#include <heightmap/heightmapCulling.h>
#include <heightmap/heightmapMetricsCalc.h>
#include <render/grassTranslucency.h>
#include <scene/dag_physMat.h>
#include <scene/dag_physMatId.h>

#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_platform_pc.h>
#include <3d/dag_render.h>
#include <3d/dag_texMgrTags.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>
#include <render/dag_cur_view.h>
#include <util/dag_bitArray.h>
#include <libTools/util/progressInd.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_directUtils.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_ioUtils.h>
#include <util/dag_simpleString.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_fastPtrList.h>
#include <debug/dag_debug.h>
#include <assets/asset.h>
#include <math/dag_mesh.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <3d/dag_texPackMgr2.h>
#include <coolConsole/coolConsole.h>
#include <perfMon/dag_cpuFreq.h>

#include <shaders/dag_shaderBlock.h>
#include <libTools/shaderResBuilder/meshDataSave.h>
#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <libTools/shaderResBuilder/globalVertexDataConnector.h>
#include <libTools/meshOptimize/quadrics.h>
#include <libTools/util/strUtil.h>
#include <libTools/staticGeom/geomObject.h>
#include <sceneBuilder/nsb_export_lt.h>
#include <sceneBuilder/nsb_LightmappedScene.h>
#include <sceneBuilder/nsb_LightingProvider.h>
#include <sceneBuilder/nsb_StdTonemapper.h>
#include <EditorCore/ec_IEditorCore.h>
#include <ctype.h>
#include <debug/dag_debug3d.h>
#include <render/toroidalHeightmap.h>
#include <render/toroidalPuddleMap.h>
#include <3d/dag_resPtr.h>
#include <de3_editorEvents.h>
#include <de3_splineGenSrv.h>
#include <physMap/physMatSwRendererRT.h>
#include <physMap/physMatHtTex.h>
#include <landMesh/biomeQuery.h>
#include <heightmap/heightmapHandler.h>

/*#include "detailRenderData.h"
Tab<DetailRenderData::TexturePingPong> DetailRenderData::colorMapTexArr;
Tab<Texture *> DetailRenderData::detailMaskTexArr;
int DetailRenderData::copyProg = BAD_PROGRAM;
static const char *vs_hlsl =
  "struct VSInput {float2 pos:POSITION; };\n"
  "struct VSOutput{float4 pos:POSITION; float2 tc:TEXCOORD0; };\n"
  "VSOutput vs_main(VSInput input)\n"
  "{\n"
  "VSOutput output;\n"
  "output.pos = float4(input.pos.xy, 0, 1);\n"
  "output.tc  = input.pos*float2(0.5, -0.5)+float2(0.50001,0.50001);\n"
  "return output;\n"
  "}\n";

static const char *vs_hlsl11 =
  "struct VSInput {float2 pos:POSITION; };\n"
  "struct VSOutput{float4 pos:SV_POSITION; float2 tc:TEXCOORD0; };\n"
  "VSOutput vs_main(VSInput input)\n"
  "{\n"
  "VSOutput output;\n"
  "output.pos = float4(input.pos.xy, 0, 1);\n"
  "output.tc  = input.pos*float2(0.5, -0.5)+float2(0.50001,0.50001);\n"
  "return output;\n"
  "}\n";*/

using editorcore_extapi::dagTools;

extern const char *filter_class_name;
static void optimizeMesh(Mesh &m);
extern TEXTUREID load_land_micro_details(const DataBlock &blk);
extern void close_land_micro_details(TEXTUREID &id);

#define CALL_LMESH_TYPED_RENDER(X, p) X.render(p, X.RENDER_WITH_CLIPMAP, false)


static int hmap_stor_mismatch_cnt = 0;

static RenderDecalMaterialsWithRT<256, 256> decalRenderer;

static int render_with_normalmapVarId = -1;
static int should_use_clipmapVarId = -1, bake_landmesh_combineVarId = -1;
static int global_frame_blockid = -1;

static void get_clipmap_rendering_services(IRenderingService *&hmap, Tab<IRenderingService *> &rend_srv)
{
  rend_srv.clear();
  hmap = NULL;
  for (int i = 0, plugin_cnt = IEditorCoreEngine::get()->getPluginCount(); i < plugin_cnt; ++i)
  {
    IGenEditorPlugin *p = IEditorCoreEngine::get()->getPlugin(i);
    IRenderingService *iface = p->queryInterface<IRenderingService>();
    if (stricmp(p->getInternalName(), "heightmapLand") == 0)
    {
      hmap = iface;
    }
    else if (strcmp(p->getInternalName(), "_riEntMgr") == 0 || strcmp(p->getInternalName(), "_invalidEntMgr") == 0)
      continue; // skip
    else
    {
      if (p->getVisible())
      {
        if (iface)
          rend_srv.push_back(iface);
      }
    }
  }
}

static int decal_ent_mask = -1, decal3d_ent_mask = -1;
static void get_subtype_mask_for_clipmap_rendering(unsigned &old_st_mask, unsigned &new_st_mask)
{
  static int polygonSubtypeMask = -1, splineSubtypeMask = -1, roadsSubtypeMask = -1, hmapSubtypeMask = -1, lmeshSubtypeMask = -1,
             rend_ent_geomMask = -1;

  if (polygonSubtypeMask == -1)
    polygonSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("poly_tile");
  if (splineSubtypeMask == -1)
    splineSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("spline_cls");
  if (roadsSubtypeMask == -1)
    roadsSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("road_obj");
  if (hmapSubtypeMask == -1)
    hmapSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("hmap_obj");

  if (rend_ent_geomMask == -1)
    rend_ent_geomMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_ent_geom");
  if (decal_ent_mask == -1)
    decal_ent_mask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_decal_geom");
  if (decal3d_ent_mask == -1)
    decal3d_ent_mask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_decal3d_geom");

  if (lmeshSubtypeMask == -1)
    lmeshSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("lmesh_obj");

  old_st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
  new_st_mask = old_st_mask | roadsSubtypeMask | splineSubtypeMask | polygonSubtypeMask | rend_ent_geomMask;
  new_st_mask &= ~(lmeshSubtypeMask | hmapSubtypeMask | (1 << IObjEntity::ST_NOT_COLLIDABLE));
}

static void render_decals_to_clipmap(IRenderingService *hmap, const Tab<IRenderingService *> &rend_srv, unsigned old_st_mask,
  unsigned new_st_mask)
{
  filter_class_name = "land_mesh";
  DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, new_st_mask);
  hmap->renderGeometry(IRenderingService::STG_RENDER_TO_CLIPMAP);

  DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, old_st_mask & ~(decal_ent_mask | decal3d_ent_mask));
  for (int i = 0; i < rend_srv.size(); i++)
    rend_srv[i]->renderGeometry(IRenderingService::STG_RENDER_TO_CLIPMAP);

  DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, decal_ent_mask | decal3d_ent_mask);
  for (int i = 0; i < rend_srv.size(); i++)
    rend_srv[i]->renderGeometry(IRenderingService::STG_RENDER_TO_CLIPMAP);
  DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, new_st_mask);
  hmap->renderGeometry(IRenderingService::STG_RENDER_TO_CLIPMAP_LATE);
  for (int i = 0; i < rend_srv.size(); i++)
    rend_srv[i]->renderGeometry(IRenderingService::STG_RENDER_TO_CLIPMAP_LATE);
  DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, old_st_mask);

  filter_class_name = NULL;

  d3d::settm(TM_WORLD, TMatrix::IDENT);
}

HmapDetLayerProps::~HmapDetLayerProps()
{
  d3d::delete_program(prog);
  d3d::delete_vertex_shader(vs);
  d3d::delete_pixel_shader(ps);
  d3d::delete_vdecl(vd);
}

class GenericHeightMapService : public IHmapService
{
  // Safe atomic publish of <tmpPath> onto <finalPath>: snapshots the existing
  // <finalPath> into <finalPath>.old, then renames <tmpPath> onto <finalPath>,
  // and removes <finalPath>.old on success. If the publish rename fails we
  // roll back by renaming the backup back to <finalPath> so the on-disk
  // state stays consistent.
  //
  // The reader path (openFile / mapStorageFileExists) falls back to
  // <finalPath>.old when <finalPath> is missing, so if both the publish AND
  // the rollback rename fail -- or if the process dies between the two
  // renames -- the backup file is still discoverable as the last-known-good
  // copy and no data appears lost on next load. Any stale .old from a prior
  // crashed save is therefore safe to overwrite: the reader either already
  // promoted it to finalPath, or finalPath itself is still present and the
  // .old is dead weight.
  //
  // label is a short prefix used in error logs so both MapStorageImpl and
  // R32ZstFloatMapStorage callers can share this helper.
  static bool atomic_publish_with_backup(const char *label, const String &tmpPath, const String &finalPath)
  {
    String oldPath(0, "%s.old", finalPath.c_str());

    if (dd_file_exist(finalPath))
    {
      if (dd_file_exist(oldPath))
        dd_erase(oldPath);
      if (!dd_rename(finalPath, oldPath))
      {
        DAEDITOR3.conError("%s: backup rename '%s' -> '%s' failed; leaving existing file in place. New data stays at '%s'.", label,
          finalPath.c_str(), oldPath.c_str(), tmpPath.c_str());
        return false;
      }
    }

    if (!dd_rename(tmpPath, finalPath))
    {
      // Publish rename failed. Try to put the backup back so the on-disk
      // state looks untouched. If rollback also fails, .old remains as the
      // last-known-good copy that openFile/mapStorageFileExists will pick up.
      if (dd_file_exist(oldPath) && dd_rename(oldPath, finalPath))
      {
        DAEDITOR3.conError(
          "%s: publish rename '%s' -> '%s' failed; rolled back to previous good file. New data is still available at '%s'.", label,
          tmpPath.c_str(), finalPath.c_str(), tmpPath.c_str());
      }
      else
      {
        DAEDITOR3.conError("%s: publish rename '%s' -> '%s' failed AND rollback failed; last-known-good data is at '%s' (reader "
                           "falls back to it). New data stays at '%s'.",
          label, tmpPath.c_str(), finalPath.c_str(), oldPath.c_str(), tmpPath.c_str());
      }
      return false;
    }

    if (dd_file_exist(oldPath))
      dd_erase(oldPath);
    return true;
  }

  // size_t-safe IGenLoad that streams a const memory buffer of arbitrary
  // length. Used as the source for zstd_compress_data so 4 GiB+ pixel
  // buffers compress correctly; InPlaceMemLoadCB took an int datasize
  // that silently truncated, producing empty/corrupt .zst files.
  class LargeMemLoadCB : public IBaseLoad
  {
  public:
    LargeMemLoadCB(const void *data, size_t data_sz) :
      start((const char *)data), cur((const char *)data), end((const char *)data + data_sz)
    {}

    void read(void *dst, int size) override
    {
      if (tryRead(dst, size) != size)
        DAGOR_THROW(LoadException("LargeMemLoadCB: unexpected EOF", 0));
    }

    int tryRead(void *dst, int size) override
    {
      if (cur >= end || size <= 0)
        return 0;
      size_t take = min(size_t(size), size_t(end - cur));
      memcpy(dst, cur, take);
      cur += take;
      return int(take);
    }

    // tell()/seek*() are int-capped by the IGenLoad contract; streaming zstd
    // only calls tryRead/read, so the clamp is defensive.
    int tell() override
    {
      int64_t ofs = int64_t(cur - start);
      return ofs > int64_t(INT_MAX) ? INT_MAX : int(ofs);
    }
    void seekto(int) override { G_ASSERTF(0, "LargeMemLoadCB::seekto unsupported"); }
    void seekrel(int) override { G_ASSERTF(0, "LargeMemLoadCB::seekrel unsupported"); }
    const char *getTargetName() override { return "(mem-src)"; }
    int64_t getTargetDataSize() override { return int64_t(end - start); }

  private:
    const char *start;
    const char *cur;
    const char *end;
  };

  // size_t-safe IGenSave that writes decompressed bytes straight into a
  // pre-sized memory buffer. Counterpart of LargeMemLoadCB: the old
  // DynamicMemGeneralSaveCB scratch + int-capped copy path silently
  // truncated 4 GiB payloads to zero.
  class LargeMemSaveCB : public IBaseSave
  {
  public:
    LargeMemSaveCB(void *data, size_t data_sz) : start((char *)data), cur((char *)data), end((char *)data + data_sz) {}

    void write(const void *src, int size) override
    {
      if (size <= 0)
        return;
      size_t take = min(size_t(size), size_t(end - cur));
      memcpy(cur, src, take);
      cur += take;
      if (take < size_t(size))
        overflow = true;
    }

    int tell() override
    {
      int64_t ofs = int64_t(cur - start);
      return ofs > int64_t(INT_MAX) ? INT_MAX : int(ofs);
    }
    void seekto(int) override { G_ASSERTF(0, "LargeMemSaveCB::seekto unsupported"); }
    void seektoend(int = 0) override { G_ASSERTF(0, "LargeMemSaveCB::seektoend unsupported"); }
    const char *getTargetName() override { return "(mem-dst)"; }
    void flush() override {}

    bool hasOverflow() const { return overflow; }

  private:
    char *start;
    char *cur;
    char *end;
    bool overflow = false;
  };

  // Flat-memory persistent MapStorage impl. The base class holds a single
  // Tab<T> pixels[]; this subclass adds atomic zstd-compressed .dat
  // persistence for float / uint16/32/64 / E3DCOLOR maps.
  //
  // In the current editor no caller actually persists a map through this
  // impl: landClsMap/colorMap/detTex*/lightMapScaled are RAM-only derived
  // outputs, and the heightmaps use R32ZstFloatMapStorage below. The
  // persistence path is kept functional (and size_t-safe via LargeMemLoad/
  // SaveCB) so a new caller that needs a disk-backed uint* / color map can
  // use it without re-deriving the compression layer.
  //
  // Disk format (little-endian):
  //   [u32 magic = 'FMS1']
  //   [u32 version = 1]
  //   [i32 w][i32 h]
  //   [u32 typeSize]        // sanity check against sizeof(T)
  //   [u64 defVal]          // default pixel value (low bytes hold T)
  //   [zstd-compressed raw T[w*h]]
  // Writes go to <fname>.tmp then atomic rename onto <fname>.
  //
  // Backward-compat: the pre-flat paged format lived in "<fname>/<fname>.blk"
  // plus "<fname>/XXXX-YYYY.<fname>.dat" siblings. This impl does NOT read
  // that legacy format -- projects that still have paged lightmap/water maps
  // need to regenerate after upgrading. The heightmap keeps a legacy reader
  // in R32ZstFloatMapStorage below since heightmap data is not trivially
  // regenerable.
  template <class T>
  class MapStorageImpl : public MapStorage<T>
  {
    using MapStorage<T>::mapSizeX;
    using MapStorage<T>::mapSizeY;
    using MapStorage<T>::defaultValue;
    using MapStorage<T>::pixels;

  public:
    static const uint32_t FILE_MAGIC = _MAKE4C('FMS1');
    static const uint32_t FILE_VERSION = 1;

    MapStorageImpl(int sx, int sy, const T &defval) { MapStorage<T>::reset(sx, sy, defval); }
    ~MapStorageImpl() override { closeFile(); }

    bool createFile(const char *file_name) override
    {
      // Set up the file path state; actual on-disk write happens on flushData.
      if (!file_name || !*file_name)
        return false;
      fname = file_name;
      return true;
    }

    bool openFile(const char *file_name) override
    {
      if (!file_name || !*file_name)
        return false;
      fname = file_name;

      // Primary path is fname; if it's missing, fall back to fname.old,
      // which atomic_publish_with_backup leaves behind when a save crashed
      // or the publish rename failed. Treating .old as the last-known-good
      // copy turns a transient save failure into just "loaded the previous
      // version", not apparent data loss. If we do fall back, promote .old
      // to fname so the next save starts from a clean state.
      String loadPath = fname;
      if (!dd_file_exist(loadPath))
      {
        String oldPath(0, "%s.old", fname.c_str());
        if (!dd_file_exist(oldPath))
          return true; // fresh file: keep the reset-initialized defaults
        if (dd_rename(oldPath, fname))
          DAEDITOR3.conNote("MapStorageImpl: recovered '%s' from backup '%s' (previous save did not complete)", fname.c_str(),
            oldPath.c_str());
        else
          loadPath = oldPath; // couldn't promote -- read .old in place
      }

      FullFileLoadCB reader(loadPath);
      if (!reader.fileHandle)
        return false;

      DAGOR_TRY
      {
        uint32_t magic = reader.readInt();
        if (magic != FILE_MAGIC)
        {
          DAEDITOR3.conError("MapStorageImpl: bad magic 0x%08X in '%s'", magic, fname.c_str());
          return false;
        }
        uint32_t ver = reader.readInt();
        if (ver != FILE_VERSION)
        {
          DAEDITOR3.conError("MapStorageImpl: unsupported version %u in '%s'", ver, fname.c_str());
          return false;
        }
        int32_t w = reader.readInt();
        int32_t h = reader.readInt();
        uint32_t typeSize = reader.readInt();
        if (typeSize != sizeof(T))
        {
          DAEDITOR3.conError("MapStorageImpl: type size mismatch in '%s': file=%u expected=%u", fname.c_str(), typeSize,
            uint32_t(sizeof(T)));
          return false;
        }
        int64_t dvRaw = 0;
        reader.read(&dvRaw, sizeof(dvRaw));
        T def;
        memcpy(&def, &dvRaw, sizeof(T));
        if (w <= 0 || h <= 0)
        {
          DAEDITOR3.conError("MapStorageImpl: bad dimensions %dx%d in '%s'", w, h, fname.c_str());
          return false;
        }

        MapStorage<T>::reset(w, h, def);
        const size_t rawBytes = size_t(w) * size_t(h) * sizeof(T);

        // Stream decompression straight into pixels.data(). Avoids the
        // int-capped DynamicMemGeneralSaveCB scratch + memcpy path that
        // silently truncated >2 GiB payloads on the compress side.
        LargeMemSaveCB dst(pixels.data(), rawBytes);
        int64_t unpacked = zstd_decompress_data(dst, reader);
        if (unpacked <= 0 || size_t(unpacked) != rawBytes)
        {
          DAEDITOR3.conError("MapStorageImpl: zstd payload size mismatch in '%s': got %lld need %lld", fname.c_str(),
            (long long)unpacked, (long long)rawBytes);
          return false;
        }
      }
      DAGOR_CATCH(const IGenLoad::LoadException &)
      {
        DAEDITOR3.conError("MapStorageImpl: I/O exception reading '%s'", fname.c_str());
        return false;
      }
      return true;
    }

    void closeFile(bool finalize = false) override
    {
      if (finalize)
        fname = "";
    }

    void eraseFile() override
    {
      if (!fname.empty() && dd_file_exist(fname))
        dd_erase(fname);
      fname = "";
      // Reset the pixel buffer so a follow-up save on a different path does
      // not emit stale data. Keep the size/defaultValue as the caller left
      // them so the map stays usable in memory.
      MapStorage<T>::reset(mapSizeX, mapSizeY, defaultValue);
    }

    const char *getFileName() const override { return fname; }
    bool isFileOpened() const override { return !fname.empty(); }
    bool canBeReverted() const override { return !fname.empty() && dd_file_exist(fname); }
    bool revertChanges() override
    {
      if (fname.empty())
        return false;
      String keep = fname;
      return openFile(keep);
    }

    bool flushData() override
    {
      if (fname.empty())
        return true;

      String tmpPath(0, "%s.tmp", fname.c_str());
      {
        FullFileSaveCB writer(tmpPath);
        if (!writer.fileHandle)
        {
          DAEDITOR3.conError("MapStorageImpl: cannot open '%s' for writing", tmpPath.c_str());
          return false;
        }

        DAGOR_TRY
        {
          writer.writeInt(int(FILE_MAGIC));
          writer.writeInt(int(FILE_VERSION));
          writer.writeInt(mapSizeX);
          writer.writeInt(mapSizeY);
          writer.writeInt(int(sizeof(T)));
          int64_t dvRaw = 0;
          memcpy(&dvRaw, &defaultValue, sizeof(T));
          writer.write(&dvRaw, sizeof(dvRaw));

          const size_t rawBytes = size_t(mapSizeX) * size_t(mapSizeY) * sizeof(T);
          // Use the size_t-safe LargeMemLoadCB instead of InPlaceMemLoadCB;
          // the latter took an int datasize and silently truncated >2 GiB
          // pixel buffers to zero, producing 16-byte corrupt .dat files.
          LargeMemLoadCB in(pixels.data(), rawBytes);
          int64_t packed = zstd_compress_data(writer, in, rawBytes, /*solid_threshold*/ 1 << 20, /*compressionLevel*/ 16);
          if (packed <= 0)
          {
            DAEDITOR3.conError("MapStorageImpl: zstd_compress_data failed for '%s'", tmpPath.c_str());
            return false;
          }
        }
        DAGOR_CATCH(const IGenSave::SaveException &)
        {
          DAEDITOR3.conError("MapStorageImpl: I/O exception writing '%s'", tmpPath.c_str());
          return false;
        }
      }

      return atomic_publish_with_backup("MapStorageImpl", tmpPath, fname);
    }

  protected:
    String fname;
  };

  // Reads the pre-flat paged .blk + per-subMap .dat files into a MapStorage<T>
  // that the caller has already pre-sized via resetStored(). Rectangle-copies
  // the world-coord overlap of the legacy submap bbox and the caller's stored
  // rect; legacy pixels outside the caller's rect are discarded. This is what
  // tightens det-hmap from the 2048-aligned legacy bbox down to exactly
  // detRect on first save.
  //
  // Format reminder (produced by the now-deleted SubMapStorageImpl):
  //   <dir>/<base>.blk            -- DataBlock with "w", "h", "defVal"
  //   <dir>/XXXX-YYYY.<base>.dat  -- per-submap (2048x2048 region, 8x8 tiles
  //                                  of 256x256 each). Header:
  //     [u32 ver (bit 31 = compressed)][u32 subW][u32 subH][u32 elemSize]
  //     then compressed (or raw if uncompressed) row-major tile data:
  //     tile index = ty*numTilesX + tx; each tile = elemSize*elemSize pixels.
  // The compressed body uses LZMA in recent saves.
  // Returns true if the legacy .blk was found and parsed (missing .dat
  // siblings leave those regions at caller's defaultValue). Returns false if
  // no .blk is present -- the caller should treat that as "no legacy data".
  template <class T>
  static bool read_legacy_paged_into_stored(MapStorage<T> &out, const char *legacy_dir, const char *basename)
  {
    static const int SUBMAP_SZ = 2048;
    static const int ELEM_SZ = 256;

    String blkPath(260, "%s%s.blk", legacy_dir, basename);
    if (!dd_file_exist(blkPath))
      return false;

    DataBlock blk;
    if (!blk.load(blkPath))
      return false;

    const int logicalW = blk.getInt("w", 0);
    const int logicalH = blk.getInt("h", 0);
    // defVal and logicalW/H are advisory: caller has already pre-sized `out`
    // from heightmapLand.plugin.blk. Legacy logicalW/H are used only to bound
    // the submap iteration and to stop the per-pixel loop at submap edges.
    if (logicalW <= 0 || logicalH <= 0)
      return false;

    const int callerOx = out.storedOfsX;
    const int callerOy = out.storedOfsY;
    const int callerSw = out.storedW;
    const int callerSh = out.storedH;
    if (callerSw <= 0 || callerSh <= 0)
      return true; // nothing to fill
    const int callerExX = callerOx + callerSw;
    const int callerExY = callerOy + callerSh;

    const int numSubMapsX = (logicalW + SUBMAP_SZ - 1) / SUBMAP_SZ;
    const int numSubMapsY = (logicalH + SUBMAP_SZ - 1) / SUBMAP_SZ;

    for (int sy = 0; sy < numSubMapsY; sy++)
      for (int sx = 0; sx < numSubMapsX; sx++)
      {
        // Early-out: skip submaps that don't overlap the caller's stored
        // rect. The legacy bbox may be much larger than detRect, so most
        // submaps are pure waste to read for a tight detRect.
        const int subOffX = sx * SUBMAP_SZ;
        const int subOffY = sy * SUBMAP_SZ;
        const int subExX = subOffX + SUBMAP_SZ;
        const int subExY = subOffY + SUBMAP_SZ;
        if (subExX <= callerOx || subOffX >= callerExX || subExY <= callerOy || subOffY >= callerExY)
          continue;

        String subPath(260, "%s%04d-%04d.%s.dat", legacy_dir, sx, sy, basename);
        if (!dd_file_exist(subPath))
          continue;

        FullFileLoadCB reader(subPath);
        if (!reader.fileHandle)
          continue;

        int ver = reader.readInt();
        const bool compressed = (uint32_t(ver) & 0x80000000u) != 0;
        if ((ver & 0x7FFFFFFF) != 1)
        {
          logerr("legacy paged submap '%s': bad version 0x%08X", subPath.c_str(), ver);
          continue;
        }
        const int subW = reader.readInt();
        const int subH = reader.readInt();
        const int esz = reader.readInt();
        if (esz != ELEM_SZ)
        {
          logerr("legacy paged submap '%s': unexpected elemSize %d", subPath.c_str(), esz);
          continue;
        }

        const size_t subBytes = size_t(subW) * size_t(subH) * sizeof(T);
        SmallTab<T, TmpmemAlloc> subBuf;
        clear_and_resize(subBuf, subW * subH);

        if (compressed)
        {
          const int bodyOff = reader.tell();
          const int bodyLen = df_length(reader.fileHandle) - bodyOff;
          uint8_t peek[2] = {0, 0};
          reader.read(peek, 2);
          reader.seekto(bodyOff);

          LzmaLoadCB z_crd(reader, bodyLen);
          z_crd.read(subBuf.data(), int(subBytes));
          z_crd.close();
        }
        else
          reader.read(subBuf.data(), int(subBytes));

        const int numTilesX = (subW + ELEM_SZ - 1) / ELEM_SZ;
        const int numTilesY = (subH + ELEM_SZ - 1) / ELEM_SZ;
        for (int ty = 0; ty < numTilesY; ty++)
          for (int tx = 0; tx < numTilesX; tx++)
          {
            const T *tileData = &subBuf[(ty * numTilesX + tx) * ELEM_SZ * ELEM_SZ];
            for (int py = 0; py < ELEM_SZ; py++)
            {
              const int wy = subOffY + ty * ELEM_SZ + py;
              if (wy >= logicalH)
                break;
              const int localY = wy - callerOy;
              if (unsigned(localY) >= unsigned(callerSh))
                continue;
              for (int px = 0; px < ELEM_SZ; px++)
              {
                const int wx = subOffX + tx * ELEM_SZ + px;
                if (wx >= logicalW)
                  break;
                const int localX = wx - callerOx;
                if (unsigned(localX) >= unsigned(callerSw))
                  continue;
                out.pixels[intptr_t(localY) * callerSw + localX] = tileData[py * ELEM_SZ + px];
              }
            }
          }
      }
    return true;
  }

  // Float MapStorage persisted as a single file:
  //   <dir>/<base>.r32.zst   -- zstd-compressed raw floats, row-major, stored
  //                             rect only (storedW x storedH pixels)
  //
  // No sidecar: mapSize/storedOfs/storedW/storedH/defVal are all derivable
  // from heightmapLand.plugin.blk (heightMapSizeX/Y, detDivisor, detRect0,
  // detRect1) and heightmap defVal is always 0. The caller is responsible
  // for calling resetStored() with the correct extent BEFORE openFile();
  // openFile just decompresses bytes into pixels.data().
  //
  // openFile picks the format automatically:
  //   * .r32.zst present                -> decompress into caller buffer
  //   * .r32.zst.old (crash recovery)   -> promote and decompress
  //   * legacy paged <base>/<base>.blk  -> read 2048-submap tree, rectangle-
  //                                        copy world-coord overlap into the
  //                                        caller's pre-sized buffer (data
  //                                        outside caller's stored rect is
  //                                        discarded). Next flushData wipes
  //                                        the legacy dir -- this tightens
  //                                        the on-disk footprint from the
  //                                        legacy 2048-aligned bbox down to
  //                                        exactly detRect on first save.
  //   * none of the above               -> fresh file; caller's pixels hold
  //                                        defaultValue from resetStored.
  class R32ZstFloatMapStorage : public MapStorage<float>
  {
  public:
    R32ZstFloatMapStorage(int sx, int sy, float defval) { MapStorage<float>::reset(sx, sy, defval); }
    ~R32ZstFloatMapStorage() override { closeFile(); }

    bool createFile(const char *file_name) override
    {
      // Only initializes path/fname; the .r32.zst itself is written by flushData
      // into the parent dir, which already exists. The legacy paged-format
      // directory is read-only fallback (see openFile) and must not be created.
      return buildPathes(file_name);
    }

    bool openFile(const char *file_name) override
    {
      if (!buildPathes(file_name))
        return false;

      String compressedPath = buildR32ZstPath();
      if (dd_file_exist(compressedPath))
        return readR32Zst(compressedPath);

      // Crash-recovery fallback: atomic_publish_with_backup leaves the
      // previous good .r32.zst at <compressedPath>.old when a save fails or
      // the process dies between the backup and publish renames. Promote .old
      // to the final name so subsequent saves start clean.
      String oldCompressedPath(0, "%s.old", compressedPath.c_str());
      if (dd_file_exist(oldCompressedPath))
      {
        if (dd_rename(oldCompressedPath, compressedPath))
        {
          DAEDITOR3.conNote("R32ZstFloatMapStorage: recovered '%s' from backup '%s' (previous save did not complete)",
            compressedPath.c_str(), oldCompressedPath.c_str());
          return readR32Zst(compressedPath);
        }
        return readR32Zst(oldCompressedPath);
      }

      // Legacy paged fallback: read the old .blk + .dat submap tree and
      // rectangle-copy world-coord overlap into the caller-preset buffer.
      // Data outside the caller's stored rect is discarded (tightens to
      // detRect). Next flushData wipes the legacy dir.
      String legacyBlk(260, "%s%s.blk", path.str(), fname.str());
      if (dd_file_exist(legacyBlk))
        return read_legacy_paged_into_stored<float>(*this, path.c_str(), fname.c_str());

      // Fresh file -- keep caller's reset-initialized defaults.
      return true;
    }

    void closeFile(bool finalize = false) override
    {
      if (finalize)
      {
        fname = "";
        path = "";
      }
    }

    void eraseFile() override
    {
      // Guard against eraseFile after closeFile(true) (or before any
      // createFile/openFile) -- buildSiblingPath with empty fname returns a
      // CWD-relative ".r32.zst" path that could match an unrelated file.
      if (fname.empty())
        return;
      String compressedPath = buildR32ZstPath();
      if (dd_file_exist(compressedPath))
        dd_erase(compressedPath);
      if (!path.empty() && dd_dir_exists(path))
        dag::remove_dirtree(path);
      // Preserve the stored rect (don't fall back to full-extent alloc), so a
      // det-heightmap at 32768x32768 logical doesn't suddenly balloon to a
      // 4 GiB allocation on erase. Callers that want to extend the edit area
      // must explicitly resetStored to a larger bbox.
      MapStorage<float>::resetStored(mapSizeX, mapSizeY, storedOfsX, storedOfsY, storedW, storedH, defaultValue);
    }

    const char *getFileName() const override { return path; }
    bool isFileOpened() const override { return !fname.empty(); }
    bool canBeReverted() const override { return !fname.empty() && dd_file_exist(buildR32ZstPath()); }
    bool revertChanges() override
    {
      if (fname.empty())
        return false;
      String compressedPath = buildR32ZstPath();
      if (!dd_file_exist(compressedPath))
        return false;
      return readR32Zst(compressedPath);
    }

    bool flushData() override
    {
      if (fname.empty())
        return true;

      String r32Path = buildR32ZstPath();
      String r32Tmp(0, "%s.tmp", r32Path.c_str());

      // Raw-float payload: zstd of storedW * storedH row-major floats, no
      // header. Streaming from pixels.data() through a size_t-safe reader
      // avoids the int-sized API cap on InPlaceMemLoadCB that silently
      // truncated 4 GiB payloads to 0.
      const size_t storedBytes = size_t(storedW) * size_t(storedH) * sizeof(float);
      {
        FullFileSaveCB writer(r32Tmp);
        if (!writer.fileHandle)
        {
          DAEDITOR3.conError("R32ZstFloatMapStorage: cannot open '%s' for writing", r32Tmp.c_str());
          return false;
        }
        if (storedBytes > 0)
        {
          LargeMemLoadCB src(pixels.data(), storedBytes);
          int64_t packed = zstd_compress_data(writer, src, storedBytes, /*solid_threshold*/ 1 << 20, /*compressionLevel*/ 11);
          if (packed <= 0)
          {
            DAEDITOR3.conError("R32ZstFloatMapStorage: zstd_compress_data failed for '%s'", r32Tmp.c_str());
            return false;
          }
        }
        // Empty stored rect -> zero-byte .r32.zst; load path notices
        // storedW*storedH == 0 and skips the decompress.
      }
      if (!atomic_publish_with_backup("R32ZstFloatMapStorage", r32Tmp, r32Path))
        return false;

      // One-shot cleanup of the legacy paged dir after the first successful
      // .r32.zst write. If the dir still exists and contains the old .blk
      // descriptor, wipe it.
      if (dd_dir_exists(path))
      {
        String legacyBlk(260, "%s%s.blk", path.str(), fname.str());
        if (dd_file_exist(legacyBlk))
        {
          dag::remove_dirtree(path);
          DAEDITOR3.conNote("R32ZstFloatMapStorage: migrated legacy paged dir '%s' to '%s'", path.c_str(), r32Path.c_str());
        }
      }
      return true;
    }

  private:
    bool buildPathes(const char *file_name)
    {
      path = "";
      fname = "";
      if (!file_name || !*file_name)
        return false;
      const char *fn = dd_get_fname(file_name);
      const char *ext = dd_get_fname_ext(file_name);
      if (!fn || !ext)
        return false;
      path.printf(260, "%.*s/", ext - file_name, file_name);
      dd_simplify_fname_c(path);
      path.resize(strlen(path) + 1);
      fname.printf(260, "%.*s", ext - fn, fn);
      return true;
    }

    String buildR32ZstPath() const
    {
      // path is "<dir>/<fname>/" and fname is "<fname>". The .r32.zst lives
      // next to the legacy dir as "<dir>/<fname>.r32.zst". Tab<char>::resize
      // only lowers the used-count and doesn't null-terminate, so slicing
      // path via c_str()+resize would leave the old tail readable and build
      // an inside-dir path. Use printf with an explicit length instead.
      const int suffixLen = int(fname.length()) + 1;
      const int dirLen = max(int(path.length()) - suffixLen, 0);
      return String(0, "%.*s%s.r32.zst", dirLen, path.c_str(), fname.c_str());
    }

    // Decompress straight into caller-preset pixels.data(). Decoded byte
    // count must equal storedW*storedH*sizeof(float); mismatch (caller's
    // stored rect disagrees with what was saved) -> conError and fail.
    bool readR32Zst(const String &compressedPath)
    {
      const size_t storedBytes = size_t(storedW) * size_t(storedH) * sizeof(float);
      if (storedBytes == 0)
        return true; // caller pre-sized to empty rect; nothing to read.

      FullFileLoadCB reader(compressedPath);
      if (!reader.fileHandle)
      {
        DAEDITOR3.conError("R32ZstFloatMapStorage: cannot open '%s' for reading", compressedPath.c_str());
        return false;
      }
      LargeMemSaveCB dst(pixels.data(), storedBytes);
      int64_t unpacked = zstd_decompress_data(dst, reader);
      if (unpacked <= 0 || size_t(unpacked) != storedBytes)
      {
        DAEDITOR3.conError("R32ZstFloatMapStorage: zstd payload size mismatch in '%s': got %lld need %lld", compressedPath.c_str(),
          (long long)unpacked, (long long)storedBytes);
        return false;
      }
      return true;
    }

    String path, fname;
  };

  class HmapStorage : public HeightMapStorage
  {
  public:
    void destroy()
    {
      if (hasDistinctInitialAndFinalMap())
        del_it(hmFinal);
      del_it(hmInitial);

      ctorClear();
    }
    void alloc(int sx, int sy, bool sep_final)
    {
      ctorClear();
      hmInitial = new R32ZstFloatMapStorage(sx, sy, 0);
      if (sep_final)
        hmFinal = new R32ZstFloatMapStorage(sx, sy, 0);
      else
        hmFinal = hmInitial;
    }
  };

  class BitLayersList
  {
  public:
    BitLayersList(const DataBlock &desc) : layers(midmem), layerAttr(midmem)
    {
      int total_bits = 0;
      int layer_nid = desc.getNameId("layer");
      int attr_nid = desc.getNameId("attr");

      for (int i = 0; i < desc.blockCount(); i++)
        if (desc.getBlock(i)->getBlockNameId() == layer_nid)
        {
          const DataBlock &blk = *desc.getBlock(i);
          const char *name = blk.getStr("name", NULL);
          int bits = blk.getInt("bits", 0);

          if (!name)
          {
            DEBUG_CTX("unnamed layer, skipped");
            continue;
          }

          int id = layerNames.getNameId(name);
          if (id != -1)
          {
            DEBUG_CTX("duplicate layer: %s, skipped", name);
            continue;
          }
          if (bits <= 0 || bits > 32)
          {
            DEBUG_CTX("invalid layer bit width: %s - %d bits, skipped", name, bits);
            continue;
          }
          if (total_bits + bits > 32)
          {
            DEBUG_CTX("too big layer: %s - %d bits, previous layers already use %d bits, skipped", name, bits, total_bits);
            continue;
          }

          HmapBitLayerDesc &d = layers.push_back();
          d.wordMask = (0xFFFFFFFFU << (32 - bits)) >> (32 - total_bits - bits);
          d.shift = total_bits;
          d.bitCount = bits;

          layerNames.addNameId(name);
          total_bits += bits;

          unsigned attr = 0;
          for (int j = 0; j < blk.paramCount(); j++)
            if (blk.getParamType(j) == DataBlock::TYPE_STRING && blk.getParamNameId(j) == attr_nid)
            {
              const char *attr_name = blk.getStr(j);
              int attr_id = attrNames.addNameId(attr_name);
              if (attr_id >= 32)
              {
                DEBUG_CTX("too many (max=32) different attributes; %s skipped", attr_name);
                continue;
              }
              attr |= 1U << attr_id;
            }
          layerAttr.push_back(attr);

          debug("added layer %d: %s, mask=%p shift=%d bits=%d", layers.size() - 1, name, d.wordMask, d.shift, d.bitCount);
        }

      // debug("created %d layers (%d bits)", layers.size(), total_bits);
    }

    Tab<HmapBitLayerDesc> layers;
    Tab<unsigned> layerAttr;

    FastNameMapEx layerNames;
    FastNameMap attrNames;
  };


  // support maximum 16 gen hmap records
  static const int MAX_GENHMAP = 16;
  FastNameMapEx genHmapNames;
  GenHmapData genHmap[MAX_GENHMAP];
  int genHmapReg;

  Tab<HmapDetLayerProps> detLayers;
  FastNameMap detLayersMap;
  FastPtrList handlesToUpdate;

  int lastClipTexSz;
  UniqueTexHolder last_clip;
  d3d::SamplerInfo last_clip_sampler;
  bool rebuilLastClip;
  bool preparingClipmap;

  bool shouldShowPhysMatColors;
  bool shouldUpdatePhysMatHtTex;

  Texture *physMatHtTex;
  TEXTUREID physMatHtTexId;

  bool enableTrackDirt;

  struct HeightMap2
  {
    LodGrid lodGrid;
    HeightmapRenderer rend;
    float sx = 1, sy = 1, ax = 1, ay = 1;
    TEXTUREID texMainId = BAD_TEXTUREID, texDetId = BAD_TEXTUREID;
    BBox2 bboxMain, bboxDet;

    MetricsErrors *metrics = nullptr;
    SimpleHeightmapRenderer *metricsRenderer = nullptr;
    float hMin = 0, hMax = 1;

    ~HeightMap2()
    {
      del_it(metrics);
      del_it(metricsRenderer);
    }
  } hm2;

  GrassTranslucency *grassTranslucency;
  float grassTranslucencyHalfSize;

  float puddlesPowerScale = 1.0f;
  float puddlesNoiseInfluence = 1.0f;
  float puddlesSeed = 0.0f;

  int biomeQuery = -1;
  int biomeQueryResult = -1;

  int desiredHmapUpscale = 1;
  int hmapUpscale = 1;
  int hmapUpscaleAlgo = 0; // BICUBIC / LANCZOS / HERMITE

public:
  GenericHeightMapService()
  {
    memset(genHmap, 0, sizeof(genHmap));
    grassTranslucency = NULL;
    grassTranslucencyHalfSize = 0;
    genHmapReg = 0;
    clipmap = NULL;
    shouldUseClipmap = false;
    preparingClipmap = false;
    shouldShowPhysMatColors = false;
    shouldUpdatePhysMatHtTex = false;
    rebuilLastClip = true;
    lastClipTexSz = 4096;
  }
  void init()
  {
    DataBlock appblk(DAGORED2->getWorkspace().getAppBlkPath());

    auto &drvDesc = d3d::get_driver_desc();
    lastClipTexSz = min(drvDesc.maxtexw, drvDesc.maxtexh);

    shouldUseClipmap = appblk.getBlockByName("clipmap") != NULL;
    clipmap = NULL;

    should_use_clipmapVarId = ::get_shader_glob_var_id("should_use_clipmap", true);
    bake_landmesh_combineVarId = ::get_shader_glob_var_id("bake_landmesh_combine", true);
    render_with_normalmapVarId = ::get_shader_glob_var_id("render_with_normalmap", true);

    global_frame_blockid = ShaderGlobal::getBlockId("global_frame");

    if (const DataBlock *b = appblk.getBlockByName("dynamicDeferred"))
      if (b->getBool("grassTranslucency", true))
      {
        grassTranslucency = new GrassTranslucency;
        grassTranslucency->init(appblk.getBlockByNameEx("dynamicDeferred")->getInt("grassTranslucencyMapSize", 512));
        grassTranslucencyHalfSize = appblk.getBlockByNameEx("dynamicDeferred")->getReal("grassTranslucencyHalfSize", 8192);
      }

    /*static VSDTYPE dcl[] = { VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT2), VSD_END };
    VPROG vs = d3d::create_vertex_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? vs_hlsl11 : vs_hlsl, -1, "vs_main",
      d3d::get_driver_code().is(d3d::dx11) ? "vs_4_0" : "vs_3_0");
    if (vs == BAD_VPROG)
      DAEDITOR3.conError("Copy.VS: %s\n%s", d3d::get_last_error(), vs_hlsl);

    static const char *ps_hlsl11 =
      "struct VSOutput{float4 pos:SV_POSITION; float2 tc:TEXCOORD0; };\n"
      "Texture2D tex : register(t0);\n"
      "SamplerState tex_samplerstate : register(s0);\n"
      "float4 ps_main(VSOutput input):SV_Target0\n"
      "{return tex.Sample(tex_samplerstate, input.tc);}\n";
    static const char *ps_hlsl =
      "struct VSOutput{float4 pos:POSITION; float2 tc:TEXCOORD0; };\n"
      "sampler2D tex : register(s0);\n"
      "float4 ps_main(VSOutput input):COLOR0\n"
      "{return tex2D(tex, input.tc);}\n";
    FSHADER ps = d3d::create_pixel_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? ps_hlsl11 : ps_hlsl, -1, "ps_main",
      d3d::get_driver_code().is(d3d:dx11) ? "ps_4_0" : "ps_3_0");
    if (ps == BAD_FSHADER)
      DAEDITOR3.conError("Copy.PS: %s\n%s", d3d::get_last_error(), ps_hlsl);

    if (ps != BAD_FSHADER && vs != BAD_VPROG)
      DetailRenderData::copyProg = d3d::create_program(vs, ps, d3d::create_vdecl(dcl));*/

    shaders::OverrideState flipCullState;
    flipCullState.set(shaders::OverrideState::FLIP_CULL);
    flipCullStateId = shaders::overrides::create(flipCullState);

    shaders::OverrideState blendOpMaxState;
    blendOpMaxState.set(shaders::OverrideState::BLEND_OP);
    blendOpMaxState.blendOp = BLENDOP_MAX;
    blendOpMaxStateId = shaders::overrides::create(blendOpMaxState);

    shaders::OverrideState disableDepthClipState;
    disableDepthClipState.set(shaders::OverrideState::Z_CLAMP_ENABLED);
    disableDepthClipStateId = shaders::overrides::create(disableDepthClipState);
  }
  ~GenericHeightMapService()
  {
    del_it(clipmap);
    closeFixedClip();
    close_land_micro_details(landMicrodetailsId);
    del_it(grassTranslucency);

    if (physMatHtTex)
      release_managed_tex_verified(physMatHtTexId, physMatHtTex);
  }

  DataBlock microDetails;
  TEXTUREID landMicrodetailsId = BAD_TEXTUREID;

  DataBlock grassBlk;
  SimpleString paintDetailsTextureName;

  void loadLandMicroDetails(const DataBlock &blk)
  {
    if (microDetails != blk)
    {
      debug("load land micro details (%d params, %d blocks)", blk.paramCount(), blk.blockCount());
      ShaderGlobal::reset_from_vars(landMicrodetailsId);
      release_managed_tex(landMicrodetailsId);
      landMicrodetailsId = load_land_micro_details(blk);
      microDetails = blk;
    }
  }

  void onLevelBlkLoaded(const DataBlock &level_blk) override
  {
    loadLandMicroDetails(*level_blk.getBlockByNameEx("micro_details"));
    grassBlk = *level_blk.getBlockByNameEx("randomGrass");
    paintDetailsTextureName = level_blk.getStr("global_paint_details_tex", "assets_color_global_tex_palette*");
  }

  void setGrassBlk(const DataBlock *blk) override { grassBlk = *blk; }

  const char *getGlobalPaintDetailsTexName() const override { return paintDetailsTextureName.str(); }

  void initClipmap()
  {
    DataBlock appblk(DAGORED2->getWorkspace().getAppBlkPath());
    const DataBlock *clipBlk = appblk.getBlockByName("clipmap");
    G_ASSERT(shouldUseClipmap);
    G_ASSERT(clipBlk != NULL);

    float clipmapScale = safediv(1.0f, sqrtf(clamp(clipBlk->getReal("clipmapScale", 1.f), 0.5f, 4.0f)));
    float texel_sz = clamp(clipBlk->getReal("texelSize", 0.015) * clipmapScale, 0.0025f, 1.0f);
    int cache_cnt = clipBlk->getInt("cacheCnt", 0);
    int buf_cnt = clipBlk->getInt("bufferCnt", 0);

    bool useToroidalHeightmap = clipBlk->getBool("useToroidalHeightmap", false);
    bool useToroidalPuddles = clipBlk->getBool("useToroidalPuddles", false);

    clipmap = new Clipmap(clipBlk->getBool("useUAVFeedback", false));
    clipmap->init(texel_sz, Clipmap::CPU_HW_FEEDBACK, Clipmap::getDefaultFeedbackProperties(), clipBlk->getInt("texMips", 7));
    clipmap->initVirtualTexture(clipBlk->getInt("cacheWidth", 4096), clipBlk->getInt("cacheHeight", 8192), 256, 1,
      ::dgs_tex_anisotropy, float(2 * 1920 * 1080));
    if (useToroidalHeightmap)
    {
      toroidalHeightmap.reset(new ToroidalHeightmap());
      toroidalHeightmap->init(2048, 64.0f, 256.0f, TEXFMT_R8, 128);
    }
    if (useToroidalPuddles)
    {
      toroidalPuddles.reset(new ToroidalPuddles());
      toroidalPuddles->init(1024, 64.0f, 256.0f, 128);
    }
    heightmapPatchesRenderer.reset(new HeightmapPatchesRenderer(1024));

    carray<uint32_t, 4> bufFormats;
    int bufCnt = min<int>(bufFormats.size(), buf_cnt);
    carray<uint32_t, 4> formats;
    int cacheCnt = min<int>(formats.size(), cache_cnt);
    for (int i = 0; i < bufCnt; ++i)
      bufFormats[i] = parse_tex_format(clipBlk->getStr(String(32, "buf_tex%d", i), "NONE"), TEXFMT_DEFAULT);
    for (int i = 0; i < cacheCnt; ++i)
      formats[i] = parse_tex_format(clipBlk->getStr(String(32, "cache_tex%d", i), "NONE"), TEXFMT_DEFAULT);
    clipmap->createCaches(formats.data(), formats.data(), cacheCnt, bufFormats.data(), bufCnt);
    clipmap->setMaxTileUpdateCount(TEX_DEFAULT_CLIPMAP_UPDATES_PER_FRAME);

    lastClipTexSz = min(clipBlk->getInt("lastClipTexSz", 4096), lastClipTexSz);
    G_ASSERT(clipmap != NULL);
    DAEDITOR3.conNote("hmapSrv: clipmap %s inited on-demand (texelSz=%.4f cacheCnt=%d lastClipSz=%d)",
      clipBlk->getBool("argb32bit", false) ? "ARGB8" : "RGB565", texel_sz, cache_cnt, lastClipTexSz);
    microDetails.setBool("__not_inited", true);
  }

  bool mapStorageFileExists(const char *fname) const override
  {
    const char *fn = dd_get_fname(fname);
    const char *ext = dd_get_fname_ext(fname);
    String blk_fn(260, "%.*s/%.*s.blk", ext - fname, fname, ext - fn, fn);
    if (dd_file_exist(blk_fn))
    {
      DataBlock blk;
      return blk.load(blk_fn) && blk.paramCount();
    }
    if (dd_file_exist(fname))
      return true;
    // New-format projects store the flat .r32.zst next to the legacy paged
    // dir at "<dir>/<basename>.r32.zst" (no .dat extension) -- see
    // R32ZstFloatMapStorage::buildR32ZstPath. A naive "%s.r32.zst" would
    // probe "<dir>/<basename>.dat.r32.zst" and miss the real migrated file,
    // making loadMapFile skip openFile and load a zero-sized heightmap.
    String zstPath(0, "%.*s.r32.zst", int(ext - fname), fname);
    if (dd_file_exist(zstPath))
      return true;
    // atomic_publish_with_backup may leave the previous good file at
    // <fname>.old when a save fails mid-swap. Treat that as "exists" so
    // callers (e.g. eager-regen guards) don't mistake a transient save
    // failure for a fresh/empty project. Also check the .r32.zst.old sibling
    // (same extension-stripping rule as above) for the new flat-file layout.
    String oldPath(0, "%s.old", fname);
    if (dd_file_exist(oldPath))
      return true;
    String zstOldPath(0, "%.*s.r32.zst.old", int(ext - fname), fname);
    return dd_file_exist(zstOldPath);
  }

  bool createStorage(HeightMapStorage &_hm, int sx, int sy, bool sep_final) override
  {
    HmapStorage &hm = (HmapStorage &)_hm;
    hm.alloc(sx, sy, sep_final);
    return true;
  }
  void destroyStorage(HeightMapStorage &_hm) override
  {
    HmapStorage &hm = (HmapStorage &)_hm;
    hm.destroy();
  }

  FloatMapStorage *createFloatMapStorage(int sx, int sy, float defval) override { return new MapStorageImpl<float>(sx, sy, defval); }
  Uint64MapStorage *createUint64MapStorage(int sx, int sy, uint64_t defval) override
  {
    return new MapStorageImpl<uint64_t>(sx, sy, defval);
  }
  Uint32MapStorage *createUint32MapStorage(int sx, int sy, unsigned defval) override
  {
    return new MapStorageImpl<uint32_t>(sx, sy, defval);
  }
  Uint16MapStorage *createUint16MapStorage(int sx, int sy, unsigned defval) override
  {
    return new MapStorageImpl<uint16_t>(sx, sy, defval);
  }
  ColorMapStorage *createColorMapStorage(int sx, int sy, E3DCOLOR defval) override
  {
    return new MapStorageImpl<E3DCOLOR>(sx, sy, defval);
  }


  void *createBitLayersList(const DataBlock &desc) override { return new (midmem) BitLayersList(desc); }
  void destroyBitLayersList(void *handle) override
  {
    BitLayersList *bll = (BitLayersList *)handle;
    del_it(bll);
  }
  dag::Span<HmapBitLayerDesc> getBitLayersList(void *handle) override
  {
    BitLayersList *bll = (BitLayersList *)handle;
    return bll ? make_span(bll->layers) : dag::Span<HmapBitLayerDesc>();
  }
  int getBitLayerIndexByName(void *handle, const char *layer_name) override
  {
    const BitLayersList *bll = (BitLayersList *)handle;
    return bll ? bll->layerNames.getNameId(layer_name) : -1;
  }
  const char *getBitLayerName(void *handle, int layer_idx) override
  {
    const BitLayersList *bll = (BitLayersList *)handle;
    return bll ? bll->layerNames.getName(layer_idx) : NULL;
  }
  int getBitLayerAttrId(void *handle, const char *attr_name) override
  {
    const BitLayersList *bll = (BitLayersList *)handle;
    return bll ? bll->attrNames.getNameId(attr_name) : -1;
  }
  bool testBitLayerAttr(void *handle, int layer_idx, int attr_id) override
  {
    if (!handle || layer_idx < 0 || attr_id < 0 || attr_id >= 32)
      return false;

    const BitLayersList *bll = (BitLayersList *)handle;
    return (bll->layerAttr[layer_idx] & (1U << attr_id)) ? true : false;
  }
  int findBitLayerByAttr(void *handle, int attr_id, int start_with_layer) override
  {
    if (!handle || start_with_layer < 0 || attr_id < 0 || attr_id >= 32)
      return -1;

    const BitLayersList *bll = (BitLayersList *)handle;
    int attr_mask = 1U << attr_id;
    for (int l = start_with_layer; l < bll->layerAttr.size(); l++)
      if (bll->layerAttr[l] & attr_mask)
        return l;
    return -1;
  }

  /*void createDetLayerClassList(const DataBlock &blk) override
  {
    static VSDTYPE dcl[] = { VSD_STREAM(0), VSD_REG(VSDR_POS, VSDT_FLOAT2), VSD_END };

    String hlsl;
    int nid_layer = blk.getNameId("layer");
    int nid_hlsl = blk.getNameId("hlsl");

    detLayers.clear();
    for (int i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == nid_layer)
      {
        const DataBlock &b = *blk.getBlock(i);
        HmapDetLayerProps &dl = detLayers.push_back();

        dl.vs = dl.ps = dl.vd = dl.prog = -1;
        dl.name = b.getStr("name");
        if (detLayersMap.addNameId(dl.name) != detLayers.size()-1)
        {
          DAEDITOR3.conError("duplicate layer <%s> in block #%d", dl.name, i);
          detLayers.pop_back();
          continue;
        }

        dl.canOutSplatting = b.getBool("can_output_splatting", false);
        dl.needsMask = b.getBool("require_input_mask", true);

        int psf_idx = 0, psb_idx = 0, smp_idx = 5;
        for (int j = 0; j < b.blockCount(); j++)
        {
          HmapDetLayerProps::Param &p = dl.param.push_back();
          p.name = b.getBlock(j)->getStr("name", "");
          p.pmin = p.pmax = 0;
          if (strcmp(b.getBlock(j)->getBlockName(), "input_int") == 0)
          {
            p.type = HmapDetLayerProps::Param::PT_int;
            p.pmin = b.getBlock(j)->getIPoint2("range", IPoint2(0,1)).x;
            p.pmax = b.getBlock(j)->getIPoint2("range", IPoint2(0,1)).y;
            p.defValI[0] = b.getBlock(j)->getInt("default", (p.pmin+p.pmax)/2);
            p.regIdx = psf_idx++;
          }
          else if (strcmp(b.getBlock(j)->getBlockName(), "input_float") == 0)
          {
            p.type = HmapDetLayerProps::Param::PT_float;
            p.pmin = b.getBlock(j)->getPoint2("range", Point2(0,1)).x;
            p.pmax = b.getBlock(j)->getPoint2("range", Point2(0,1)).y;
            p.defValF[0] = b.getBlock(j)->getReal("default", (p.pmin+p.pmax)/2);
            p.regIdx = psf_idx++;
          }
          else if (strcmp(b.getBlock(j)->getBlockName(), "input_float2") == 0)
          {
            p.type = HmapDetLayerProps::Param::PT_float2;
            p.pmin = b.getBlock(j)->getPoint2("range", Point2(0,1)).x;
            p.pmax = b.getBlock(j)->getPoint2("range", Point2(0,1)).y;
            *((Point2*)p.defValF) = b.getBlock(j)->getPoint2("default", Point2(p.pmin, p.pmax));
            p.regIdx = psf_idx++;
          }
          else if (strcmp(b.getBlock(j)->getBlockName(), "input_color") == 0)
          {
            *((Point4*)p.defValF) = b.getBlock(j)->getPoint4("default", Point4(1,1,1,1));
            p.type = HmapDetLayerProps::Param::PT_color;
            p.regIdx = psf_idx++;
          }
          else if (strcmp(b.getBlock(j)->getBlockName(), "input_texture") == 0)
          {
            p.type = HmapDetLayerProps::Param::PT_tex;
            p.texType = b.getBlock(j)->getStr("type", "");
            p.regIdx = smp_idx++;
          }
          else
            p.name = NULL;

          if (p.name.empty())
            dl.param.pop_back();
        }
        hlsl = d3d::get_driver_code().is(d3d::dx11) ?
          "struct VSOutput{float4 pos:SV_POSITION; float2 tc:TEXCOORD0; };\n"
          "Texture2D current_colormap_tex: register(t0);\n"
          "Texture2D heightmap_tex: register(t1);\n"
          "Texture2D mask_tex: register(t2);\n"
          "SamplerState current_colormap_tex_samplerstate: register(s0);\n"
          "SamplerState heightmap_tex_samplerstate: register(s1);\n"
          "SamplerState mask_tex_samplerstate: register(s2);\n"
          :
          "struct VSOutput{float4 pos:POSITION; float2 tc:TEXCOORD0; };\n"
          "sampler2D current_colormap_tex: register(s0);\n"
          "sampler2D heightmap_tex: register(s1);\n"
          "sampler2D mask_tex: register(s2);\n";

        if (d3d::get_driver_code().is(d3d::dx11))
        {
          hlsl +=  "#define hardware_dx11\n";
          hlsl +=  "#define tex2D(a, uv) a.Sample(a##_samplerstate, uv)\n";
        }

        String nm;
        for (int j = 0; j < dl.param.size(); j ++)
        {
          nm = dl.param[j].name;
          for (char *p = nm.data(); *p; p++)
            if (!isalnum(*p) && *p !='_')
              *p = '_';

          switch (dl.param[j].type)
          {
            case HmapDetLayerProps::Param::PT_bool:
              hlsl.aprintf(0, "bool %s: register(b%d);\n", nm, dl.param[j].regIdx);
              break;
            case HmapDetLayerProps::Param::PT_int:
              hlsl.aprintf(0, "float %s: register(c%d);\n", nm, dl.param[j].regIdx);
              break;
            case HmapDetLayerProps::Param::PT_float:
              hlsl.aprintf(0, "float %s: register(c%d);\n", nm, dl.param[j].regIdx);
              break;
            case HmapDetLayerProps::Param::PT_float2:
              hlsl.aprintf(0, "float2 %s: register(c%d);\n", nm, dl.param[j].regIdx);
              break;
            case HmapDetLayerProps::Param::PT_color:
              hlsl.aprintf(0, "float4 %s: register(c%d);\n", nm, dl.param[j].regIdx);
              break;
            case HmapDetLayerProps::Param::PT_tex:
              if (d3d::get_driver_code().is(d3d::dx11))
              {
                hlsl.aprintf(0, "Texture2D %s: register(t%d);\n", nm, dl.param[j].regIdx);
                hlsl.aprintf(0, "SamplerState %s_samplerstate: register(s%d);\n", nm, dl.param[j].regIdx);
              }
              else
                hlsl.aprintf(0, "sampler2D %s: register(s%d);\n", nm, dl.param[j].regIdx);
              break;
          }
        }

        if (dl.canOutSplatting)
        {
          hlsl += d3d::get_driver_code().is(d3d::dx11) ?
            "struct MRT_OUTPUT{float4 colormap:SV_Target0; float4 splattingmap1:SV_Target1; float4 splattingmap2:SV_Target2;};\n"
            "Texture2D current_splattingmap1_tex: register(t3);\n"
            "Texture2D current_splattingmap2_tex: register(t4);\n"
            "SamplerState current_splattingmap1_tex_samplerstate: register(s3);\n"
            "SamplerState current_splattingmap2_tex_samplerstate: register(s4);\n"
            :
            "struct MRT_OUTPUT{float4 colormap:COLOR0; float4 splattingmap1:COLOR1; float4 splattingmap2:COLOR2;};\n"
            "sampler2D current_splattingmap1_tex: register(s3);\n"
            "sampler2D current_splattingmap2_tex: register(s4);\n";
        } else
          hlsl += d3d::get_driver_code().is(d3d::dx11) ?
            "struct MRT_OUTPUT{float4 colormap:SV_Target0; };\n" :
            "struct MRT_OUTPUT{float4 colormap:COLOR0; };\n";


        hlsl +=
          "MRT_OUTPUT ps_main(VSOutput input) {\n"
          "MRT_OUTPUT output;\n"
          "float4 current_colormap = tex2D(current_colormap_tex, input.tc);\n"
          "float heightmap = tex2D(heightmap_tex, input.tc).x;\n"
          "float mask = tex2D(mask_tex, input.tc).x;\n"
        ;
        if (dl.canOutSplatting)
          hlsl +=
            "float4 splattingmap1 = tex2D(current_splattingmap1_tex, input.tc);\n"
            "float4 splattingmap2 = tex2D(current_splattingmap2_tex, input.tc);\n";

        for (int j = 0; j < b.paramCount(); j++)
          if (b.getParamNameId(j) == nid_hlsl && b.getParamType(j) == b.TYPE_STRING)
            hlsl.aprintf(0, "%s\n", b.getStr(j));
        hlsl += "return output;}\n";

        dl.hlslCode = hlsl;
        dl.vs = d3d::create_vertex_shader_hlsl(d3d::get_driver_code().is(d3d::dx11) ? vs_hlsl11 : vs_hlsl, -1, "vs_main",
          d3d::get_driver_code().is(d3d::dx11) ? "vs_4_0" : "vs_3_0");
        if (dl.vs < 0)
          DAEDITOR3.conError("DetLayer.VS: %s\n%s", d3d::get_last_error(), d3d::get_driver_code().is(d3d::dx11) ? vs_hlsl11 :
  vs_hlsl); dl.ps = d3d::create_pixel_shader_hlsl(dl.hlslCode, -1, "ps_main", d3d::get_driver_code().is(d3d::dx11) ? "ps_4_0" :
  "ps_3_0"); if (dl.ps < 0) DAEDITOR3.conError("DetLayer.PS: %s\n%s", d3d::get_last_error(), dl.hlslCode); dl.vd =
  d3d::create_vdecl(dcl); dl.prog = d3d::create_program(dl.vs, dl.ps, dl.vd);

        DAEDITOR3.conNote("DetLayer[%d] %s: (%d,%d,%d)->%d", detLayers.size(), dl.name, dl.vs, dl.ps, dl.vd, dl.prog);
      }
  }
  dag::Span<HmapDetLayerProps> getDetLayerClassList() override { return make_span(detLayers); }
  int resolveDetLayerClassName(const char *nm) override { return detLayersMap.getNameId(nm); }
  void *createDetailRenderData(const DataBlock &layers, LandClassDetailInfo *detTex) override
  {
    void *h = new DetailRenderData(layers, detTex, detLayers);
    updateDetailRenderData(h, layers);
    return h;
  }
  void destroyDetailRenderData(void *handle) override
  {
    if (handle)
    {
      handlesToUpdate.delPtr(handle);
      delete (DetailRenderData*)handle;
    }
  }
  void updateDetailRenderData(void *handle, const DataBlock &layers) override
  {
    if (handle)
    {
      ((DetailRenderData*)handle)->actualLayers = layers;
      handlesToUpdate.addPtr(handle);
    }
  }
  void updatePendingDetailRenderData()
  {
    dag::ConstSpan<void*> h = handlesToUpdate.getList();
    handlesToUpdate.reset();
    for (int i = 0; i < h.size(); i ++)
    {
      ((DetailRenderData*)h[i])->update(((DetailRenderData*)h[i])->actualLayers, detLayers, detLayersMap);
      ((DetailRenderData*)h[i])->actualLayers.reset();
    }
  }

  Texture *getDetailRenderDataMaskTex(void *handle, const char *name) override
  {
    return handle ? ((DetailRenderData*)handle)->getMaskTex(name) : NULL;
  }
  TEXTUREID getDetailRenderDataTex(void *handle, const char *name) override
  {
    return handle ? ((DetailRenderData*)handle)->getAuxTex(name) : BAD_TEXTUREID;
  }
  void storeDetailRenderData(void *handle, const char *prefix, bool store_cmap, bool store_smap) override
  {
    if (handle)
      ((DetailRenderData*)handle)->storeData(prefix, store_cmap, store_smap);
  }
  */

  const char *getLandPhysMatName(int pmatId) override { return pmatId >= 0 ? PhysMat::getMaterial(pmatId).name.str() : NULL; }

  int getPhysMatId(const char *name) override { return PhysMat::getMaterialId(name); }

  void beforeRender() override
  {
    biome_query::update();

    if (is_managed_textures_streaming_load_on_demand())
    {
      static int land_gen = 0, clipmap_gen = 0;
      const auto &info0 = textag_get_info(TEXTAG_LAND);
      const auto &info1 = textag_get_info(TEXTAG_USER_START + 0);
      int cur_land_gen = interlocked_relaxed_load(info0.loadGeneration);
      int cur_clipmap_gen = interlocked_relaxed_load(info1.loadGeneration);

      if (land_gen != cur_land_gen)
        debug("some of %d land textures updated (gen=%d -> %d), invalidate clipmap", interlocked_relaxed_load(info0.texCount),
          land_gen, cur_land_gen);
      if (clipmap_gen != cur_clipmap_gen)
        debug("some of %d clipmap textures updated (gen=%d -> %d), invalidate clipmap", interlocked_relaxed_load(info1.texCount),
          clipmap_gen, cur_clipmap_gen);

      if (land_gen != cur_land_gen || clipmap_gen != cur_clipmap_gen)
      {
        land_gen = cur_land_gen;
        clipmap_gen = cur_clipmap_gen;
        invalidateClipmap(false, true);
      }
    }
  }

  int getBiomeIndices(const Point3 &p) override
  {
    d3d::GpuAutoLock lock;
    if (biomeQuery == -1)
    {
      biomeQuery = biome_query::query(p, 10.);
      return biomeQueryResult;
    }
    BiomeQueryResult queryResult;
    if (biome_query::get_query_result(biomeQuery, queryResult) == GpuReadbackResultState::SUCCEEDED)
    {
      if (queryResult.mostFrequentBiomeGroupWeight > queryResult.secondMostFrequentBiomeGroupWeight)
        biomeQueryResult = queryResult.mostFrequentBiomeGroupIndex | (queryResult.secondMostFrequentBiomeGroupIndex << 8);
      else
        biomeQueryResult = (queryResult.mostFrequentBiomeGroupIndex << 8) | queryResult.secondMostFrequentBiomeGroupIndex;
      biomeQuery = -1;
    }
    return biomeQueryResult;
  }

  bool getIsSolidByPhysMatName(const char *name) override
  {
    int pmatId = PhysMat::getMaterialId(name);
    return pmatId >= 0 ? PhysMat::getMaterial(pmatId).isSolid : false;
  }

  void setDesiredHmapUpscale(int scale) override { desiredHmapUpscale = scale; }

  int getDesiredHmapUpscale() const override { return desiredHmapUpscale; }

  void setHmapUpscale(int scale) override { hmapUpscale = scale; }

  int getHmapUpscale() const override { return hmapUpscale; }

  void setHmapUpscaleAlgo(int algo) override { hmapUpscaleAlgo = algo; }

  int getHmapUpscaleAlgo() const override { return hmapUpscaleAlgo; }

  GenHmapData *registerGenHmap(const char *name, HeightMapStorage *hm, Uint32MapStorage *landclsmap,
    dag::ConstSpan<HmapBitLayerDesc> lndclass_layers, ColorMapStorage *colormap, Uint32MapStorage *ltmap, const Point2 &hmap_ofs,
    float cell_size) override
  {
    int id = genHmapNames.getNameId(name);
    if (id != -1 && (genHmapReg & (1 << id)))
      return NULL;
    if (id == -1 && genHmapNames.nameCount() >= MAX_GENHMAP)
      return NULL;

    if (id == -1)
      id = genHmapNames.addNameId(name);
    G_ASSERT(id >= 0 && id < MAX_GENHMAP);
    genHmapReg |= 1 << id;

    GenHmapData &ghd = genHmap[id];
    ghd.hmap = hm;
    ghd.landClassMap = landclsmap;
    memcpy(&ghd.landClassLayers, &lndclass_layers, sizeof(ghd.landClassLayers));
    ghd.colorMap = colormap;
    ghd.ltMap = ltmap;
    ghd.offset = hmap_ofs;
    ghd.cellSize = cell_size;
    ghd.skyColor = Color3(0, 0, 0);
    ghd.sunColor = Color3(0, 0, 0);
    ghd.altCollider = NULL;

    return &ghd;
  }
  bool unregisterGenHmap(const char *name) override
  {
    int id = genHmapNames.getNameId(name);
    if (id == -1 || !(genHmapReg & (1 << id)))
      return false;

    memset(&genHmap[id], 0, sizeof(genHmap[id]));
    genHmapReg &= ~(1 << id);
    if (!genHmapReg)
      genHmapNames.reset();
    return true;
  }
  GenHmapData *findGenHmap(const char *name) override
  {
    int id = genHmapNames.getNameId(name);
    return (id != -1 && (genHmapReg & (1 << id))) ? &genHmap[id] : NULL;
  }

  void initLodGridHm2(const DataBlock &blk) override
  {
    static const int MAX_LOD_CNT = 16;
    int nid = blk.getNameId("lodRad");
    int lod_cnt = clamp(blk.getInt("lodCount", 8), 3, (int)MAX_LOD_CNT);
    int lod0Rad = blk.getInt("lod0Rad", 1);
    int tesselation = blk.getInt("tesselation", 0);
    int lodRadMul = blk.getInt("lodRadMul", 1);
    hm2.lodGrid.init(lod_cnt, lod0Rad, tesselation, lodRadMul);
    Ptr<ShaderMaterial> hm2_mat = new_shader_material_by_name("heightmap");
    if (hm2_mat)
      hm2.rend.init("heightmap", "", "hmap_tess_factor", true);
    DAEDITOR3.conNote("initLodGridHm2(%d, %d, %d, %d) hm2_mat=%p", lod_cnt, lod0Rad, tesselation, lodRadMul, hm2_mat.get());
  }

  void updateMetricsHm2(bool use_metrics, const HeightMapStorage &htStorage, bool show_final_hmap, float cellSize,
    float waterLevel) override
  {
    if (!use_metrics)
    {
      if (hm2.metrics)
        DAEDITOR3.conNote("updateMetricsHm2(use_metrics=%d)", use_metrics);
      del_it(hm2.metrics);
      del_it(hm2.metricsRenderer);
      return;
    }
    int time0 = dagTools->getTimeMsec();

    const MapStorage<float> &hmap = show_final_hmap ? htStorage.getFinalMap() : htStorage.getInitialMap();
    float hScale = htStorage.heightScale, hOfs = htStorage.heightOffset;
    auto get_ht = [&hmap, hScale, hOfs](int x, int y) { return hmap.getData(x, y) * hScale + hOfs; };

    int w = htStorage.getMapSizeX();
    int h = htStorage.getMapSizeY();
    Tab<uint16_t> r16;
    r16.resize(w * h);
    float hMin = get_ht(0, 0);
    float hMax = hMin;

    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; x++)
      {
        float v = get_ht(x, y);
        hMin = min(v, hMin);
        hMax = max(v, hMax);
      }

    hMax = max(hMax, hMin + 1.f);

    float scale = 65535.f / (hMax - hMin);
    auto to = r16.data();

    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x, ++to)
        *to = clamp<int>((get_ht(x, y) - hMin) * scale + 0.5f, 0, 65535);

    HeightmapHandler handler;
    handler.initRaw(r16.data(), cellSize, w, h, hMin, hMax - hMin, -0.5 * cellSize * Point2(w, h));

    hm2.hMin = hMin;
    hm2.hMax = hMax;
    del_it(hm2.metrics);
    hm2.metrics = new MetricsErrors();
    const int metrics_dim = 3;
    const int metrics_maxCalcLevel = 14;
    const int metrics_minCalcLevel = 2;
    hm2.metrics->calc_lod_errors(handler, metrics_minCalcLevel, metrics_maxCalcLevel, 1 << metrics_dim, waterLevel);
    if (!hm2.metricsRenderer)
    {
      hm2.metricsRenderer = new SimpleHeightmapRenderer;
      hm2.metricsRenderer->init("heightmap", true, metrics_dim);
    }

    DAEDITOR3.conNote("updateMetricsHm2(final=%d, cellSize=%g, waterLevel=%g) htRange=(%g, %g)  for %.2f sec", //
      show_final_hmap, cellSize, waterLevel, hMin, hMax, (dagTools->getTimeMsec() - time0) / 1e3f);
  }
  bool areMetricsUsedForHm2() const override { return hm2.metrics != nullptr; }

  void setupRenderHm2(float sx, float sy, float ax, float ay, Texture *htTexMain, TEXTUREID htTexIdMain, float mx0, float my0,
    float mw, float mh, Texture *htTexDet, TEXTUREID htTexIdDet, float dx0, float dy0, float dw, float dh, const Point3 &vpos) override
  {
    static int w2hm_main_gvid = ::get_shader_glob_var_id("world_to_hmap_low", true);
    static int w2hm_det_gvid = ::get_shader_glob_var_id("world_to_hmap_high", true);
    static int tex_main_gvid = ::get_shader_glob_var_id("tex_hmap_low", true);
    static int tex_main_samplerstate_gvid = ::get_shader_glob_var_id("tex_hmap_low_samplerstate", true);
    static int tex_det_gvid = ::get_shader_glob_var_id("tex_hmap_high", true);
    static int tex_det_samplerstate_gvid = ::get_shader_glob_var_id("tex_hmap_high_samplerstate", true);
    static int tex_hmap_inv_sizes_gvid = ::get_shader_glob_var_id("tex_hmap_inv_sizes", true);

    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      d3d::SamplerHandle smp = d3d::request_sampler(smpInfo);
      ShaderGlobal::set_sampler(tex_main_samplerstate_gvid, smp);
      ShaderGlobal::set_sampler(tex_det_samplerstate_gvid, smp);
    }

    hm2.sx = sx;
    hm2.sy = sy;
    hm2.ax = ax;
    hm2.ay = ay;
    hm2.texMainId = htTexMain ? htTexIdMain : BAD_TEXTUREID;
    hm2.texDetId = htTexDet ? htTexIdDet : BAD_TEXTUREID;

    TextureInfo ti;
    Color4 tisz(1.0f / 4096, 1.0f / 4096, 1.0f / 4096, 1.0f / 4096);
    Color4 noHmapMapping(Color4(10e+3, 10e+3, 10e+10, 10e+10)); // 1mm^2 at (-10000Km, -10000Km)
    if (htTexMain)
    {
      if (!htTexMain->getinfo(ti))
        ti.w = ti.h = 4096;
      tisz.r = 1.0f / ti.w;
      tisz.g = 1.0f / ti.h;
      ShaderGlobal::set_float4(w2hm_main_gvid, Color4(1.f / mw, 1.f / mh, -mx0 / mw, -my0 / mh));
      ShaderGlobal::set_texture_fast(tex_main_gvid, htTexIdMain);
      hm2.bboxMain[0].x = mx0;
      hm2.bboxMain[0].y = my0;
      hm2.bboxMain[1].x = mx0 + mw;
      hm2.bboxMain[1].y = my0 + mh;
    }
    else
    {
      ShaderGlobal::set_texture_fast(tex_main_gvid, BAD_TEXTUREID);
      ShaderGlobal::set_float4(w2hm_main_gvid, noHmapMapping);
    }

    if (htTexDet)
    {
      if (!htTexDet->getinfo(ti))
        ti.w = ti.h = 4096;
      tisz.b = 1.0f / ti.w;
      tisz.a = 1.0f / ti.h;
      ShaderGlobal::set_float4(w2hm_det_gvid, Color4(1.f / dw, 1.f / dh, -dx0 / dw, -dy0 / dh));
      ShaderGlobal::set_texture_fast(tex_det_gvid, htTexIdDet);
      if (!htTexMain)
      {
        ShaderGlobal::set_float4(w2hm_main_gvid, Color4(1.f / dw, 1.f / dh, -dx0 / dw, -dy0 / dh));
        ShaderGlobal::set_texture_fast(tex_main_gvid, htTexIdDet);
        tisz.r = 1.0f / ti.w;
        tisz.g = 1.0f / ti.h;
      }
      hm2.bboxDet[0].x = dx0;
      hm2.bboxDet[0].y = dy0;
      hm2.bboxDet[1].x = dx0 + dw;
      hm2.bboxDet[1].y = dy0 + dh;
    }
    else
    {
      ShaderGlobal::set_texture_fast(tex_det_gvid, BAD_TEXTUREID);
      ShaderGlobal::set_float4(w2hm_det_gvid, noHmapMapping);
    }

    ShaderGlobal::set_float4(tex_hmap_inv_sizes_gvid, tisz);
  }

  void startUAVFeedback() const override
  {
    if (clipmap)
      clipmap->startUAVFeedback();
  }

  void endUAVFeedback() const override
  {
    if (clipmap)
      clipmap->endUAVFeedback();
  }

  void copyUAVFeedback() const override
  {
    if (clipmap)
      clipmap->copyUAVFeedback();
  }

  BBox3 getLMeshBBoxWithHMapWBBox(LandMeshManager &p) const override { return p.getBBoxWithHMapWBBox(); }

  void renderHm2(LandMeshRenderer &r, const Point3 &vpos, bool infinite, bool render_hm) const override
  {
    if (hm2.texMainId == BAD_TEXTUREID && hm2.texDetId == BAD_TEXTUREID)
      return;

    static LodGridCullData cullData;
    LMeshRenderingMode oldMode;
    if (render_hm)
      oldMode = r.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_HEIGHTMAP);

    d3d::settm(TM_WORLD, TMatrix::IDENT);
    const BBox2 *clip = hm2.texMainId != BAD_TEXTUREID ? (infinite ? NULL : &hm2.bboxMain) : &hm2.bboxDet;
    mat44f globtm;
    d3d::getglobtm(globtm);
    Frustum frustum;
    frustum.construct(globtm);
    if (hm2.metrics)
    {
      static int heightmap_region_gvid = ::get_shader_glob_var_id("heightmap_region", true);
      TMatrix4 proj;
      d3d::gettm(TM_PROJ, &proj);
      HeightmapMetricsQuality hq = {proj_to_distance_scale(proj)};
      if (hq.distanceScale == 0)
        hq.maxRelativeTexelTess = 0;

      const float maxUpwardDisplacement = 0.5f;
      const float maxDownwardDisplacement = 0.f;
      const bool mirror = infinite;
      ShaderGlobal::set_float4(heightmap_region_gvid,
        mirror ? Color4(-(128 << 20), -(128 << 20), 128 << 20, 128 << 20)
               : Color4(hm2.bboxMain[0].x, hm2.bboxMain[0].y, hm2.bboxMain[1].x, hm2.bboxMain[1].y));
      {
        static int tex_main_samplerstate_gvid = ::get_shader_glob_var_id("tex_hmap_low_samplerstate", true);
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w =
          mirror ? d3d::AddressMode::Mirror : d3d::AddressMode::Clamp;
        ShaderGlobal::set_sampler(tex_main_samplerstate_gvid, d3d::request_sampler(smpInfo));
      }
      cullData.eraseAll();
      cull_lod_grid3(*hm2.metrics, cullData, frustum, nullptr, hm2.hMin, hm2.hMax, hm2.bboxMain, maxUpwardDisplacement,
        maxDownwardDisplacement, mirror, v_ldu(&vpos.x), nullptr, hq);

      if (cullData.hasPatches())
      {
        MetricsErrors &errors = *hm2.metrics;
        G_ASSERT(errors.dim == hm2.metricsRenderer->getDim());
        hm2.metricsRenderer->render(cullData, hm2.metricsRenderer->getShElem(), hm2.metricsRenderer->getDimBits());
      }
    }
    else
    {
      hm2.rend.setRenderClip(clip);
      cullData.frustum = frustum;
      Point2 clippedVpos = Point2::xz(vpos);
      if (clip)
      {
        clippedVpos.x = min(max(clip->left(), clippedVpos.x), clip->right());
        clippedVpos.y = min(max(clip->top(), clippedVpos.y), clip->bottom());
      }
      int lod = (length(clippedVpos - Point2::xz(vpos)) / hm2.rend.getDim()) * 0.5f;
      lod += max(0.f, vpos.y) * 0.001;
      lod = clamp(lod, 0, hm2.lodGrid.lodsCount - 1);
      float lodScale = (1 << lod);
      Point2 cellSize(hm2.sx * lodScale, hm2.sy * lodScale);
      float lod0AreaSize = 0.f;
      cull_lod_grid(hm2.lodGrid, hm2.lodGrid.lodsCount - lod, clippedVpos.x, clippedVpos.y, cellSize.x, cellSize.y, hm2.ax * lodScale,
        hm2.ay * lodScale, -10000, 10000, &frustum, clip, cullData, NULL, lod0AreaSize, hm2.rend.get_hmap_tess_factorVarId());
      hm2.rend.render(hm2.lodGrid, cullData);
    }

    if (render_hm)
      r.setLMeshRenderingMode(oldMode);
    cullData.frustum.reset();
  }
  void renderHm2VSM(LandMeshRenderer &r, const Point3 &vpos) const override
  {
    if (hm2.texMainId == BAD_TEXTUREID && hm2.texDetId == BAD_TEXTUREID)
      return;
    LMeshRenderingMode oldMode = r.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_VSM);
    renderHm2(r, vpos, true, false);
    r.setLMeshRenderingMode(oldMode);
  }

  static bool renderHm2ToFeedback(LandMeshRenderer &r);

  class HeightmapPatchesRenderer
  {
    UniqueTexHolder hmapPatchesDepthTex, hmapPatchesTex;
    PostFxRenderer processHmapPatchesDepth;
    int texSize;
    ToroidalHelper hmapPatchesData;
    int world_to_hmap_patches_tex_ofsVarId;
    int world_to_hmap_patches_ofsVarId;

    struct HmapPatchesCallback
    {
      LandMeshManager &provider;
      LandMeshRenderer &renderer;
      ViewProjMatrixContainer oviewproj;
      float texelSize;
      int texSize;
      IPoint2 newOrigin;
      float minZ, maxZ;
      LMeshRenderingMode prevLandmeshRenderingMode;
      Tab<IRenderingService *> rendSrv;

      HmapPatchesCallback(float texel, LandMeshRenderer &lm_renderer, LandMeshManager &lm_mgr, const IPoint2 &new_origin, int tex_size,
        float minz, float maxz) :
        renderer(lm_renderer),
        provider(lm_mgr),
        oviewproj(),
        texelSize(texel),
        texSize(tex_size),
        newOrigin(new_origin),
        minZ(minz),
        maxZ(maxz),
        prevLandmeshRenderingMode(LMeshRenderingMode::RENDERING_LANDMESH)
      {
        int plugin_cnt = IEditorCoreEngine::get()->getPluginCount();
        for (int i = 0; i < plugin_cnt; ++i)
        {
          IGenEditorPlugin *p = IEditorCoreEngine::get()->getPlugin(i);
          IRenderingService *iface = p->queryInterface<IRenderingService>();
          if (p->getVisible())
          {
            if (iface)
              rendSrv.push_back(iface);
          }
        }
      }
      void start(const IPoint2 &)
      {
        d3d_get_view_proj(oviewproj);
        TMatrix4 vtm = TMatrix4::IDENT;
        vtm.setcol(0, 1, 0, 0, 0);
        vtm.setcol(1, 0, 0, 1, 0);
        vtm.setcol(2, 0, 1, 0, 0);
        d3d::settm(TM_VIEW, &vtm);
        ShaderGlobal::set_float4(get_shader_variable_id("hmap_patches_min_max_z", true), minZ, maxZ, 0.0, 1.0);
        filter_class_name = "land_mesh";
        prevLandmeshRenderingMode = renderer.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_CLIPMAP);
        static int land_mesh_prepare_clipmap_blockid = ShaderGlobal::getBlockId("land_mesh_prepare_clipmap");
        ShaderGlobal::setBlock(land_mesh_prepare_clipmap_blockid, ShaderGlobal::LAYER_SCENE);
        shaders::OverrideState flipCullState;
        flipCullState.set(shaders::OverrideState::FLIP_CULL);
        shaders::OverrideStateId flipCullStateId = shaders::overrides::create(flipCullState);
        shaders::overrides::set(flipCullStateId);
      }
      void renderQuad(const IPoint2 &lt, const IPoint2 &wd, const IPoint2 &texelsFrom)
      {
        d3d::setview(lt.x, lt.y, wd.x, wd.y, 0, 1);
        BBox2 box(point2(texelsFrom) * texelSize, point2(texelsFrom + wd) * texelSize);
        SCOPE_VIEW_PROJ_MATRIX;
        d3d::clearview(CLEAR_ZBUFFER, E3DCOLOR(), 0, 0);
        TMatrix4 proj = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].y, box[0].y, minZ, maxZ);
        d3d::settm(TM_PROJ, &proj);
        for (int i = 0; i < rendSrv.size(); i++)
          rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_HEIGHT_PATCH);
        if (ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>())
        {
          ISplineGenService::LayerIndexList loft_layers(0);
          splSrv->gatherLoftLayers(loft_layers, true);

          d3d::settm(TM_WORLD, TMatrix::IDENT);
          TMatrix4 globtm;
          d3d::getglobtm(globtm);
          Frustum frustum;
          frustum.construct(globtm);
          loft_layers.iterate_layers([&](unsigned ll) { splSrv->renderGeneratedGeom(ll, true, frustum, -1, true); });
        }
      }
      void end()
      {
        d3d_set_view_proj(oviewproj);
        ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
        renderer.setLMeshRenderingMode(prevLandmeshRenderingMode);
        filter_class_name = NULL;
        shaders::overrides::reset();
      }
    };

  public:
    explicit HeightmapPatchesRenderer(int resolution) : texSize(resolution)
    {
      hmapPatchesDepthTex = dag::create_tex(NULL, texSize, texSize, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "hmap_patches_depth_tex");
      hmapPatchesTex = dag::create_tex(NULL, texSize, texSize, TEXCF_RTARGET | TEXFMT_L16, 1, "hmap_patches_tex");
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
        smpInfo.filter_mode = d3d::FilterMode::Linear;
        ShaderGlobal::set_sampler(get_shader_variable_id("hmap_patches_tex_samplerstate", true), d3d::request_sampler(smpInfo));
      }
      processHmapPatchesDepth.init("process_hmap_patches_depth");
      hmapPatchesData.curOrigin = IPoint2(-1000000, 1000000);
      hmapPatchesData.texSize = texSize;
      world_to_hmap_patches_tex_ofsVarId = ::get_shader_glob_var_id("world_to_hmap_patches_tex_ofs", true);
      world_to_hmap_patches_ofsVarId = ::get_shader_glob_var_id("world_to_hmap_patches_ofs", true);
    }
    void invalidate() { hmapPatchesData.curOrigin = IPoint2(-1000000, 1000000); }
    void render(LandMeshManager &provider, LandMeshRenderer &renderer, const Point3 &origin, float distance)
    {
      Point2 alignedOrigin = Point2::xz(origin);
      float fullDistance = 2 * distance;
      float texelSize = (fullDistance / hmapPatchesData.texSize);
      static const int TEXEL_ALIGN = 4;
      IPoint2 newTexelsOrigin = (ipoint2(floor(alignedOrigin / (texelSize))) + IPoint2(TEXEL_ALIGN / 2, TEXEL_ALIGN / 2));
      newTexelsOrigin = newTexelsOrigin - (newTexelsOrigin % TEXEL_ALIGN);
      static const int THRESHOLD = TEXEL_ALIGN * 4;
      IPoint2 move = abs(hmapPatchesData.curOrigin - newTexelsOrigin);
      if (move.x >= THRESHOLD || move.y >= THRESHOLD)
      {
        const float fullUpdateThreshold = 0.45;
        const int fullUpdateThresholdTexels = fullUpdateThreshold * hmapPatchesData.texSize;
        // if distance travelled is too big, there is no need to update movement in two steps
        if (max(move.x, move.y) < fullUpdateThresholdTexels)
        {
          if (move.x < move.y)
            newTexelsOrigin.x = hmapPatchesData.curOrigin.x;
          else
            newTexelsOrigin.y = hmapPatchesData.curOrigin.y;
        }
        SCOPE_RENDER_TARGET;
        d3d::set_render_target((Texture *)nullptr, 0);
        d3d::set_depth(hmapPatchesDepthTex.getTex2D(), DepthAccess::RW);
        BBox3 landBox = provider.getBBox();
        float minZ = landBox[0].y - 100;
        float maxZ = landBox[1].y + 500;
        HmapPatchesCallback patchCb(texelSize, renderer, provider, newTexelsOrigin, hmapPatchesData.texSize, minZ, maxZ);
        toroidal_update(newTexelsOrigin, hmapPatchesData, fullUpdateThresholdTexels, patchCb);

        d3d::resource_barrier({hmapPatchesDepthTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
        d3d::set_render_target(0, hmapPatchesTex.getTex2D(), 0);
        d3d::set_depth((Texture *)nullptr, DepthAccess::RW);
        processHmapPatchesDepth.render();
        d3d::resource_barrier({hmapPatchesTex.getTex2D(), RB_RO_SRV | RB_STAGE_VERTEX | RB_STAGE_COMPUTE, 0, 0});
        Point2 ofs =
          point2((hmapPatchesData.mainOrigin - hmapPatchesData.curOrigin) % hmapPatchesData.texSize) / hmapPatchesData.texSize;

        ShaderGlobal::set_float4(world_to_hmap_patches_tex_ofsVarId, ofs.x, ofs.y, 0, 0);
        alignedOrigin = point2(hmapPatchesData.curOrigin) * texelSize;
        ShaderGlobal::set_float4(world_to_hmap_patches_ofsVarId, 1.0f / fullDistance, 1.0f / fullDistance,
          -alignedOrigin.x / fullDistance + 0.5, -alignedOrigin.y / fullDistance + 0.5);
      }
    }
  };
  class LandmeshCMRenderer : public ClipmapRenderer, public ToroidalHeightmapRenderer, public ToroidalPuddlesRenderer
  {
  public:
    LandMeshRenderer &renderer;
    LandMeshManager &provider;
    Tab<IRenderingService *> rendSrv;
    IRenderingService *hmap;
    float minZ, maxZ;
    float hRel;
    LMeshRenderingMode omode;
    TMatrix4 ovtm, oproj;
    bool perspvalid;
    Driver3dPerspective persp;
    unsigned old_st_mask;
    unsigned new_st_mask;
    shaders::OverrideStateId flipCullStateId;
    bool shouldShowPhysMatColors;

    LandmeshCMRenderer(LandMeshRenderer &r, LandMeshManager &p, bool shouldRenderPhysmats) :
      rendSrv(tmpmem),
      renderer(r),
      provider(p),
      shouldShowPhysMatColors(shouldRenderPhysmats),
      hmap(0),
      omode(LMeshRenderingMode::RENDERING_LANDMESH),
      old_st_mask(0),
      new_st_mask(0)
    {
      shaders::OverrideState flipCullState;
      flipCullState.set(shaders::OverrideState::FLIP_CULL);
      flipCullStateId = shaders::overrides::create(flipCullState);
    }
    void startRender() { get_clipmap_rendering_services(hmap, rendSrv); }

    void startRenderTiles(const Point2 &center) override
    {
      Point3 pos(center.x, hRel, center.y);
      BBox3 landBox = provider.getBBox();
      minZ = landBox[0].y - 100;
      maxZ = landBox[1].y + 500;

      omode = renderer.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_CLIPMAP);
      static int land_mesh_prepare_clipmap_blockid = ShaderGlobal::getBlockId("land_mesh_prepare_clipmap");
      ShaderGlobal::setBlock(land_mesh_prepare_clipmap_blockid, ShaderGlobal::LAYER_SCENE);
      renderer.prepare(provider, pos, pos.y);
      TMatrix vtm = TMatrix::IDENT;
      d3d::gettm(TM_VIEW, &ovtm);
      d3d::gettm(TM_PROJ, &oproj);
      perspvalid = d3d::getpersp(persp);
      vtm.setcol(0, 1, 0, 0);
      vtm.setcol(1, 0, 0, 1);
      vtm.setcol(2, 0, 1, 0);
      shaders::overrides::set(flipCullStateId);
      d3d::settm(TM_VIEW, vtm);

      get_subtype_mask_for_clipmap_rendering(old_st_mask, new_st_mask);
    }
    void endRenderTiles() override
    {
      renderer.setRenderInBBox(BBox3());
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
      renderer.setLMeshRenderingMode(omode);
      d3d::settm(TM_VIEW, &ovtm);
      d3d::settm(TM_PROJ, &oproj);
      if (perspvalid)
        d3d::setpersp(persp);
      shaders::overrides::reset();
    }

    void renderTile(const BBox2 &region) override
    {
      d3d::settm(TM_WORLD, TMatrix::IDENT);
      d3d::clearview(CLEAR_TARGET, 0x00000000, 1.0f, 0);


      TMatrix4 proj;
      proj = matrix_ortho_off_center_lh(region[0].x, region[1].x, region[1].y, region[0].y, minZ, maxZ);
      d3d::settm(TM_PROJ, &proj);

      BBox3 landBox = provider.getBBox();
      renderer.setRenderInBBox(
        BBox3(Point3(region[0].x, landBox[0].y - 10, region[0].y), Point3(region[1].x, landBox[1].y + 10, region[1].y)));
      renderer.render(provider, renderer.RENDER_CLIPMAP, ::grs_cur_view.pos);
      render_decals_to_clipmap(hmap, rendSrv, old_st_mask, new_st_mask);
      if (renderer.physMap && shouldShowPhysMatColors)
      {
        Driver3dRenderTarget curRt;
        d3d::get_render_target(curRt);

        int x, y, w, h;
        float minz, maxz;
        d3d::getview(x, y, w, h, minz, maxz);

        RectInt destRect;
        destRect.left = x;
        destRect.top = y;
        destRect.right = x + w;
        destRect.bottom = y + h;

        decalRenderer.renderPhysMap(*renderer.physMap, region, true);
        decalRenderer.updateRT(*renderer.physMap, true);
        d3d::stretch_rect(decalRenderer.getRT(), curRt.getColor(0).tex, NULL, &destRect);
      }
    }
    void renderFeedback(const TMatrix4 &globtm) override
    {
      d3d::settm(TM_WORLD, TMatrix::IDENT);

      LMeshRenderingMode oldMode = renderer.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_FEEDBACK);
      if (!renderHm2ToFeedback(renderer))
        renderer.render(provider, renderer.RENDER_DEPTH, ::grs_cur_view.pos);

      static int land_mesh_render_depth_blockid = ShaderGlobal::getBlockId("land_mesh_render_depth");
      ShaderGlobal::setBlock(land_mesh_render_depth_blockid, ShaderGlobal::LAYER_SCENE);

      static int decal3d_ent_mask = -1;
      if (decal3d_ent_mask == -1)
        decal3d_ent_mask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("rend_decal3d_geom");

      int old_st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
      DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, decal3d_ent_mask);
      for (int i = 0; i < rendSrv.size(); i++)
        rendSrv[i]->renderGeometry(IRenderingService::STG_RENDER_DYNAMIC_OPAQUE);
      DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER, old_st_mask);

      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
      renderer.setLMeshRenderingMode(oldMode);
    }
  };

  Clipmap *clipmap;
  eastl::unique_ptr<ToroidalHeightmap> toroidalHeightmap;
  eastl::unique_ptr<ToroidalPuddles> toroidalPuddles;
  eastl::unique_ptr<HeightmapPatchesRenderer> heightmapPatchesRenderer;
  bool shouldUseClipmap;
  shaders::UniqueOverrideStateId flipCullStateId;
  shaders::UniqueOverrideStateId blendOpMaxStateId;
  shaders::UniqueOverrideStateId disableDepthClipStateId;

  LandMeshManager *createLandMeshManager(IGenLoad &crd) override
  {
    LandMeshManager *p = new LandMeshManager(true);
    p->resetRenderDataNeeded(LC_SWAP_VERTICAL_DETAIL);
    p->setGrassMaskBlk(grassBlk);
    biome_query::init();
    if (p->loadDump(crd))
      return p;
    delete p;
    return NULL;
  }

  PhysMap *loadPhysMap(LandMeshManager *landMeshManager, IGenLoad &crd) override { return landMeshManager->loadPhysMap(crd, true); }

  void destroyLandMeshManager(LandMeshManager *&p) const override
  {
    if (!p)
      return;
    biome_query::close();
    del_it(p);
  }

  LandMeshRenderer *createLandMeshRenderer(LandMeshManager &p) override
  {
    Ptr<ShaderMaterial> mat = new_shader_material_by_name("land_mesh", NULL);
    if (!mat.get())
      return NULL;
    DAEDITOR3.setFatalHandler();
    LandMeshRenderer *r = p.createRenderer();

    DAEDITOR3.popFatalHandler();
    return (LandMeshRenderer *)r;
  }

  void destroyLandMeshRenderer(LandMeshRenderer *&r) const override
  {
    if (!r)
      return;
    del_it(r);
  }

  BBox3 getBBoxWithHMapWBBox(LandMeshManager &p) const override { return p.getBBoxWithHMapWBBox(); }

  void prepareLandMesh(LandMeshRenderer &r, LandMeshManager &p, const Point3 &pos) const override { r.prepare(p, pos, pos.y); }

  void invalidatePhysMatHtTex() override { shouldUpdatePhysMatHtTex = true; }

  void setShouldShowPhysMatColors(bool val) override
  {
    if (shouldShowPhysMatColors != val)
      invalidateClipmap(true, true);
    shouldShowPhysMatColors = val;
  }

  void invalidateClipmap(bool force_redraw, bool rebuild_last_clip) override
  {
    if (!clipmap)
      return;
    clipmap->invalidate(force_redraw);

    invalidatePhysMatHtTex();

    if (toroidalHeightmap)
      toroidalHeightmap->invalidate();
    if (toroidalPuddles)
      toroidalPuddles->invalidate();
    if (rebuild_last_clip)
      rebuilLastClip = true;
    if (heightmapPatchesRenderer)
      heightmapPatchesRenderer->invalidate();
  }
  void prepareClipmap(LandMeshRenderer &r, LandMeshManager &p, float ht_rel) override
  {
    if (!clipmap)
    {
      if (shouldUseClipmap)
        initClipmap();
      else
        return;
    }
    if (preparingClipmap) // avoid recursion
      return;
    preparingClipmap = true;
    // updatePendingDetailRenderData();
    if (grassTranslucency)
      grassTranslucency->invalidate();

    if (rebuilLastClip)
    {
      rebuilLastClip = false;
      prepareFixedClip(&p, &r, lastClipTexSz);
      setFixedClipToShader(&p);
    }

    if (enableTrackDirt && shouldUpdatePhysMatHtTex && r.physMap)
    {
      static int hmap_ofs_thicknessVarId = ::get_shader_glob_var_id("hmap_ofs_thickness", true);
      static int hmap_ofs_thickness_mapVarId = ::get_shader_glob_var_id("hmap_ofs_thickness_map", true);
      static int hmap_ofs_tex_sizeVarId = ::get_shader_glob_var_id("hmap_ofs_tex_size", true);
      if (physMatHtTex)
      {
        ShaderGlobal::set_texture(hmap_ofs_thicknessVarId, BAD_TEXTUREID);
        release_managed_tex_verified(physMatHtTexId, physMatHtTex);
      }

      float thickness = 0;

      BBox2 hmapBox2D = hm2.bboxDet;

      PhysMapTexData *physMapTexData = render_phys_map_ht_data(*r.physMap, hmapBox2D, thickness, 200 / 255.0f, false);

      if (thickness < 0.01f || !physMapTexData)
      {
        DAEDITOR3.conRemark("Can't create physmat height texture with %.1f thickness", thickness);
      }
      else
      {
        physMatHtTex = create_phys_map_ht_tex(physMapTexData);
        physMatHtTexId = register_managed_tex("physMatHtTex", physMatHtTex);
        ShaderGlobal::set_texture(hmap_ofs_thicknessVarId, physMatHtTexId);

        ShaderGlobal::set_float4(hmap_ofs_thickness_mapVarId,
          Color4(1.f / hmapBox2D.width().x, 1.f / hmapBox2D.width().y, -hmapBox2D[0].x / hmapBox2D.width().x,
            -hmapBox2D[0].y / hmapBox2D.width().y));

        float maxThickness = thickness * 255.0f / 200.0f;
        ShaderGlobal::set_float4(hmap_ofs_tex_sizeVarId, Color4(1.0f / 1024.0f, maxThickness, 0, 0));
        destroy_phys_map_tex_data(physMapTexData);
      }
      shouldUpdatePhysMatHtTex = false;
    }

    int64_t reft = ref_time_ticks();
    DagorCurView savedView = ::grs_cur_view;
    LandmeshCMRenderer landmeshCMRenderer(r, p, shouldShowPhysMatColors);
    landmeshCMRenderer.hRel = ht_rel;

    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    d3d::settm(TM_WORLD, TMatrix::IDENT);
    TMatrix4 gtm;
    d3d::getglobtm(gtm);
    Driver3dPerspective persp;
    d3d::getpersp(persp);
    if (::grs_draw_wire)
      d3d::setwire(0);
    ShaderGlobal::set_int_fast(render_with_normalmapVarId, 1);
    landmeshCMRenderer.startRender();
    clipmap->finalizeFeedback();

    int w, h;
    d3d::get_target_size(w, h);
    clipmap->setTargetSize(w, h, 0.f);
    clipmap->prepareRender(landmeshCMRenderer);
    clipmap->prepareFeedback(::grs_cur_view.pos, gtm, nullptr, {ht_rel, persp.wk});
    clipmap->renderFallbackFeedback(landmeshCMRenderer, gtm);

    ShaderGlobal::set_int_fast(render_with_normalmapVarId, 2);
    if (toroidalHeightmap != NULL)
    {
      // render with parallax
      toroidalHeightmap->updateHeightmap(landmeshCMRenderer, ::grs_cur_view.pos, 0.0f, 0.5f);
    }

    if (toroidalPuddles != NULL)
    {
      static int displacement_output_typeVarId = ::get_shader_glob_var_id("displacement_output_type", true);
      ShaderGlobal::set_int_fast(displacement_output_typeVarId, 1);
      toroidalPuddles->updatePuddles(landmeshCMRenderer, ::grs_cur_view.pos, 0.5f);
      ShaderGlobal::set_int_fast(displacement_output_typeVarId, 0);

      static int heightmap_puddlesVarId = ::get_shader_glob_var_id("heightmap_puddles", true);
      ShaderGlobal::set_int_fast(heightmap_puddlesVarId, 1);
    }

    heightmapPatchesRenderer->render(p, r, ::grs_cur_view.pos, 16.0f);

    preparingClipmap = false;

    ::grs_cur_view = savedView;


    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    setFixedClipToShader(&p);
    if (::grs_draw_wire)
      d3d::setwire(1);
    if (get_time_usec(reft) > 50000)
      debug("prepareClipmap in %dus", get_time_usec(reft));
  }

  static void render_decals_cb(const BBox3 &)
  {
    Tab<IRenderingService *> rendSrv(tmpmem);
    IRenderingService *hmap = NULL;
    unsigned old_st_mask, new_st_mask;
    get_clipmap_rendering_services(hmap, rendSrv);
    get_subtype_mask_for_clipmap_rendering(old_st_mask, new_st_mask);
    render_decals_to_clipmap(hmap, rendSrv, old_st_mask, new_st_mask);
  }
  void prepareFixedClip(LandMeshManager *lmMgr, LandMeshRenderer *lmRend, int texture_size)
  {
    int64_t reft = ref_time_ticks();

    closeFixedClip();
    if (!lmMgr || !lmRend)
      return;

    LandMeshData data;
    data.lmeshMgr = lmMgr;
    data.lmeshRenderer = lmRend;
    data.texture_size = texture_size;
    data.use_dxt = false; // true;
    data.decals_cb = render_decals_cb;
    data.global_frame_id = ShaderGlobal::getBlockId("global_frame");
    data.flipCullStateId = flipCullStateId.get();
    data.land_mesh_prepare_clipmap_blockid = ShaderGlobal::getBlockId("land_mesh_prepare_clipmap");
    LMeshRenderingMode oldm = lmRend->getLMeshRenderingMode();
    ShaderGlobal::enableAutoBlockChange(false);

    prepare_fixed_clip(last_clip, last_clip_sampler, data, false, ::grs_cur_view.pos);
    lmRend->setLMeshRenderingMode(oldm);
    ShaderGlobal::enableAutoBlockChange(true);

    if (get_time_usec(reft) > 20000)
      debug("create last clip of size %dx%d in %dus", texture_size, texture_size, get_time_usec(reft));
  }

  void setFixedClipToShader(const LandMeshManager *lmMgr) { last_clip.setVar(); }

  void closeFixedClip() { last_clip.close(); }

  void renderLandMesh(LandMeshRenderer &r, LandMeshManager &p) const override
  {
    if (preparingClipmap) // avoid invalid render landmesh
      return;
    TIME_D3D_PROFILE(renderLandMesh);
    DagorCurView savedView = ::grs_cur_view;
    d3d::settm(TM_WORLD, TMatrix::IDENT);
    r.render(p, r.RENDER_WITH_CLIPMAP, ::grs_cur_view.pos);
    ::grs_cur_view = savedView;
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
  }

  void renderLandMeshClipmap(LandMeshRenderer &r, LandMeshManager &p) const override
  {
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    DagorCurView savedView = ::grs_cur_view;
    d3d::settm(TM_WORLD, TMatrix::IDENT);
    LMeshRenderingMode oldMode = r.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_CLIPMAP);
    ShaderGlobal::set_int_fast(render_with_normalmapVarId, 1);
    r.render(p, r.RENDER_CLIPMAP, ::grs_cur_view.pos);
    render_decals_cb(BBox3());
    ShaderGlobal::set_int_fast(render_with_normalmapVarId, 2);
    r.setLMeshRenderingMode(oldMode);
    ::grs_cur_view = savedView;
  }

  void renderLandMeshVSM(LandMeshRenderer &r, LandMeshManager &p) const override
  {
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    DagorCurView savedView = ::grs_cur_view;
    r.forceLowQuality(true);
    LMeshRenderingMode oldMode = r.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_VSM);
    r.render(p, r.RENDER_ONE_SHADER, ::grs_cur_view.pos);
    r.forceLowQuality(false);
    r.setLMeshRenderingMode(oldMode);
    ::grs_cur_view = savedView;
  }
  void renderLandMeshDepth(LandMeshRenderer &r, LandMeshManager &p) const override
  {
    DagorCurView savedView = ::grs_cur_view;
    LMeshRenderingMode oldMode = r.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_DEPTH);
    r.render(p, r.RENDER_DEPTH, ::grs_cur_view.pos);
    r.setLMeshRenderingMode(oldMode);
    ::grs_cur_view = savedView;
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
  }


  void renderLandMeshHeight(LandMeshRenderer &r, LandMeshManager &p) const override
  {
    DagorCurView savedView = ::grs_cur_view;
    LMeshRenderingMode oldMode = r.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_HEIGHTMAP);
    BBox3 landBox;
    landBox[0].x = p.getCellOrigin().x * p.getLandCellSize() + p.getOffset().x;
    landBox[0].y = -8192;
    landBox[0].z = p.getCellOrigin().y * p.getLandCellSize() + p.getOffset().z;
    landBox[1].x = (p.getNumCellsX() + p.getCellOrigin().x) * p.getLandCellSize() - p.getGridCellSize() + p.getOffset().x;
    landBox[1].y = 8192;
    landBox[1].z = (p.getNumCellsY() + p.getCellOrigin().y) * p.getLandCellSize() - p.getGridCellSize() + p.getOffset().z;

    r.setRenderInBBox(landBox);
    float oldInvGeomDist = r.getInvGeomLodDist();
    r.setInvGeomLodDist(0.5 / landBox.width().length());
    shaders::overrides::set(blendOpMaxStateId.get());
    r.render(p, r.RENDER_ONE_SHADER, ::grs_cur_view.pos);
    r.setLMeshRenderingMode(oldMode);
    ::grs_cur_view = savedView;
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    shaders::overrides::reset();
    r.setRenderInBBox(BBox3());
    r.setInvGeomLodDist(oldInvGeomDist);
  }

  void renderDecals(LandMeshRenderer &r, LandMeshManager &p) const override
  {
    BBox3 levelBox = BBox3();

    levelBox = p.getBBox();
    levelBox.lim[0].x = p.getCellOrigin().x * p.getLandCellSize();
    levelBox.lim[0].z = p.getCellOrigin().y * p.getLandCellSize();
    levelBox.lim[1].x = (p.getNumCellsX() + p.getCellOrigin().x) * p.getLandCellSize() - p.getGridCellSize();
    levelBox.lim[1].z = (p.getNumCellsY() + p.getCellOrigin().y) * p.getLandCellSize() - p.getGridCellSize();
    r.setRenderInBBox(levelBox);

    TMatrix4 globtm;
    d3d::getglobtm(globtm);
    LMeshRenderingMode omode = r.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_LANDMESH);
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    r.renderDecals(p, LandMeshRenderer::RENDER_WITH_CLIPMAP, globtm, false, false);
    r.setLMeshRenderingMode(omode);

    r.setRenderInBBox(BBox3());
  }

  LMeshRenderingMode setGrassMaskRenderingMode(LandMeshRenderer &r) override
  {
    LMeshRenderingMode oldMode = r.setLMeshRenderingMode(LMeshRenderingMode::GRASS_MASK);
    return oldMode;
  }

  void restoreRenderingMode(LandMeshRenderer &r, LMeshRenderingMode oldMode) override
  {
    r.setLMeshRenderingMode(oldMode);
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
  }

  void renderLandMeshGrassMask(LandMeshRenderer &r, LandMeshManager &p) const override
  {
    DagorCurView savedView = ::grs_cur_view;
    LMeshRenderingMode oldMode = r.setLMeshRenderingMode(LMeshRenderingMode::GRASS_MASK);
    r.render(p, r.RENDER_GRASS_MASK, ::grs_cur_view.pos, LandMeshRenderer::RENDER_FOR_GRASS);
    r.setLMeshRenderingMode(oldMode);
    ::grs_cur_view = savedView;
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
  }

  void updateGrassTranslucency(LandMeshRenderer &r, LandMeshManager &p) const override
  {
    if (!grassTranslucency)
      return;

    class MyGrassTranlucencyCB : public GrassTranlucencyCB
    {
    public:
      LandMeshRenderer &r;
      LandMeshManager &p;
      LMeshRenderingMode omode;
      uint32_t oldflags;

      MyGrassTranlucencyCB(LandMeshRenderer &lmr, LandMeshManager &lmp) : r(lmr), p(lmp) {}
      void start(const BBox3 &box) override
      {
        if (::grs_draw_wire)
          d3d::setwire(0);
        oldflags = LandMeshRenderer::lmesh_render_flags;
        omode = r.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_CLIPMAP);
        r.setRenderInBBox(box);
      }
      void renderTranslucencyColor() override
      {
        LandMeshRenderer::lmesh_render_flags &= ~(LandMeshRenderer::RENDER_DECALS | LandMeshRenderer::RENDER_COMBINED);
        r.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_CLIPMAP);
        r.render(p, LandMeshRenderer::RENDER_CLIPMAP, ::grs_cur_view.pos);
      }
      void renderTranslucencyMask() override
      {
        LandMeshRenderer::lmesh_render_flags = oldflags;
        r.setLMeshRenderingMode(LMeshRenderingMode::GRASS_MASK);
        r.render(p, LandMeshRenderer::RENDER_GRASS_MASK, ::grs_cur_view.pos);
      }
      void finish() override
      {
        LandMeshRenderer::lmesh_render_flags = oldflags;
        r.setLMeshRenderingMode(omode);
        r.setRenderInBBox(BBox3());
        d3d::setwire(::grs_draw_wire);
      }
    };
    if (!is_managed_textures_streaming_load_on_demand())
      ddsx::tex_pack2_perform_delayed_data_loading();

    MyGrassTranlucencyCB cb(r, p);
    r.prepare(p, ::grs_cur_view.pos, ::grs_cur_view.pos.y);
    grassTranslucency->update(::grs_cur_view.pos, grassTranslucencyHalfSize, cb);
  }

  void updatePropertiesFromLevelBlk(const DataBlock &level_blk) override
  {
    if (grassTranslucency)
    {
      int grassAmount = GrassTranslucency::GRASSY;
      const char *translucencyStr = level_blk.getBlockByNameEx("shader_vars")->getStr("land_translucency", "grassy");
      if (stricmp(translucencyStr, "grassy") == 0)
        grassAmount = GrassTranslucency::GRASSY;
      else if (stricmp(translucencyStr, "some") == 0)
        grassAmount = GrassTranslucency::SOME;
      else if (stricmp(translucencyStr, "no") == 0)
        grassAmount = GrassTranslucency::NO;
      else
        DAEDITOR3.conError("land_translucency is <%s> and should be one of: grassy, some, no", translucencyStr);
      grassTranslucency->setGrassAmount(grassAmount);
    }

    puddlesPowerScale = level_blk.getReal("puddlesPowerScale", 0.0f);
    puddlesNoiseInfluence = level_blk.getReal("puddlesNoiseInfluence", 1.0f);
    puddlesSeed = level_blk.getReal("puddlesSeed", 0.0f);
    enableTrackDirt = level_blk.getBool("trackDirt", true);
  }

  void getPuddlesParams(float &power_scale, float &seed, float &noise_influence) override
  {
    power_scale = puddlesPowerScale;
    seed = puddlesSeed;
    noise_influence = puddlesNoiseInfluence;
  }

  void resetTexturesLandMesh(LandMeshRenderer &r) const override { r.resetTextures(); }

  bool exportToGameLandMesh(mkbindump::BinDumpSaveCB &cwr, dag::ConstSpan<landmesh::Cell> cells,
    dag::ConstSpan<MaterialData> materials, int lod1TrisDensity, bool tools_internal) const override
  {
    enum
    {
      BOTTOM_ABOVE = 0,
      BOTTOM_BELOW = 1,
      BOTTOM_COUNT
    };
    ShaderMaterialsSaver matSaver;
    ShaderTexturesSaver texSaver;
    CoolConsole &con = DAGORED2->getConsole();

    ShaderMeshData::buildForTargetCode = cwr.getTarget();
    int objSzpos, lmCatPos, obj_sz;
    Tab<ShaderMeshData> land_objects(tmpmem);
    land_objects.resize(cells.size());
    Tab<ShaderMeshData> decal_objects(tmpmem);
    decal_objects.resize(cells.size());
    Tab<ShaderMeshData> combined_objects(tmpmem);
    combined_objects.resize(cells.size());
    bool hasHmapPaches = false;
    Tab<ShaderMeshData> patches_objects(tmpmem);
    patches_objects.resize(cells.size());
    int time0 = dagTools->getTimeMsec();
    con.startLog();
    con.setTotal(cells.size());
    con.setActionDesc("optimizing landmeshes...");
    con.startProgress();

    for (int cellNo = 0; cellNo < cells.size(); ++cellNo)
    {
      ::optimizeMesh(*cells[cellNo].land_mesh);
      if (cells[cellNo].decal_mesh)
        ::optimizeMesh(*cells[cellNo].decal_mesh);
      if (cells[cellNo].combined_mesh)
        ::optimizeMesh(*cells[cellNo].combined_mesh);
      if (cells[cellNo].patches_mesh)
        ::optimizeMesh(*cells[cellNo].patches_mesh);
      con.incDone();
    }
    con.endProgress();
    DAEDITOR3.conRemark("lmesh optimized in %g", (dagTools->getTimeMsec() - time0) / 1000.0);
    time0 = dagTools->getTimeMsec();

    PtrTab<ShaderMaterial> shmat(tmpmem);

    int numMat = materials.size();
    shmat.resize(numMat);
    for (int i = 0; i < numMat; ++i)
    {
      ShaderMaterial *m = ::new_shader_material(materials[i], true);
      if (!m)
      {
        DAEDITOR3.conError("Cannot create landmesh shader material: %s, shader <%s>", materials[i].matName.str(),
          materials[i].className.str());
        ShaderMeshData::buildForTargetCode = _MAKE4C('PC');
        return false;
      }
      G_ASSERT(m);
      shmat[i] = m;
    }

    con.setActionDesc("building land shadermeshes...");
    con.startProgress();
    con.setTotal(land_objects.size());
    for (int i = 0; i < land_objects.size(); ++i)
    {
      ShaderMeshData &md = land_objects[i];
      if (!numMat)
      {
        *cells[i].land_mesh = Mesh();
        if (cells[i].decal_mesh)
          *cells[i].decal_mesh = Mesh();
        if (cells[i].combined_mesh)
          *cells[i].combined_mesh = Mesh();
        if (cells[i].patches_mesh)
          *cells[i].patches_mesh = Mesh();
      }
      land_objects[i].build(*cells[i].land_mesh, (ShaderMaterial **)&shmat[0], shmat.size(), IdentColorConvert::object);
      if (cells[i].decal_mesh)
        decal_objects[i].build(*cells[i].decal_mesh, (ShaderMaterial **)&shmat[0], shmat.size(), IdentColorConvert::object);
      if (cells[i].combined_mesh)
        combined_objects[i].build(*cells[i].combined_mesh, (ShaderMaterial **)&shmat[0], shmat.size(), IdentColorConvert::object);
      if (cells[i].patches_mesh)
      {
        patches_objects[i].build(*cells[i].patches_mesh, (ShaderMaterial **)&shmat[0], shmat.size(), IdentColorConvert::object);
        hasHmapPaches = true;
      }

      con.incDone();
    }
    con.endProgress();
    DAEDITOR3.conRemark("shader lmesh built in %g", (dagTools->getTimeMsec() - time0) / 1000.0);

    time0 = dagTools->getTimeMsec();

    Tab<ShaderMeshData> lodobjects(tmpmem);
    if (!tools_internal)
      lodobjects.resize(cells.size());
    con.setActionDesc("building lods...");
    con.startProgress();
    con.setTotal(lodobjects.size());
    for (int i = 0; i < lodobjects.size(); ++i)
    {
      ShaderMeshData &md = lodobjects[i];
      Mesh lodmesh = *cells[i].land_mesh;
      try
      {
        Tab<meshopt::Vsplit> vsplits(tmpmem);

        int lodFaceCount = int(int64_t(lodmesh.face.size()) * lod1TrisDensity / 100);
        if (lodFaceCount == 0)
          lodFaceCount = lodmesh.face.size();
        meshopt::make_vsplits((const uint8_t *)lodmesh.getFace().data(), elem_size(lodmesh.getFace()), false, lodmesh.getFace().size(),
          NULL, lodmesh.getVert().data(), lodmesh.getVert().size(), vsplits, lodFaceCount, true);
        meshopt::progressive_optimize(lodmesh, vsplits.data(), vsplits.size(), true);
        lodmesh.kill_unused_verts();
      }
      catch (...)
      {
        DAEDITOR3.conError("cell %d lod wasn't made", i);
        debug_flush(false);
        ShaderMeshData::buildForTargetCode = _MAKE4C('PC');
        return false;
      }

      md.build(lodmesh, (ShaderMaterial **)&shmat[0], shmat.size(), IdentColorConvert::object);

      con.incDone();
    }
    con.endProgress();
    DAEDITOR3.conRemark("lmesh lod built in %g", (dagTools->getTimeMsec() - time0) / 1000.0);

    GlobalVertexDataConnector land_gvd, gvd_lod, decal_gvd, combined_gvd, patches_gvd;
    for (int i = 0; i < land_objects.size(); ++i)
    {
      BSphere3 bsph;
      bsph = cells[i].box;
      land_gvd.addMeshData(&land_objects[i], bsph.c, bsph.r);
    }
    land_gvd.connectData(false, NULL);
    for (int i = 0; i < lodobjects.size(); ++i)
    {
      BSphere3 bsph;
      bsph = cells[i].box;
      gvd_lod.addMeshData(&lodobjects[i], bsph.c, bsph.r);
    }
    gvd_lod.connectData(true, NULL);

    for (int i = 0; i < decal_objects.size(); ++i)
    {
      if (!cells[i].decal_mesh)
        continue;
      BSphere3 bsph;
      bsph = cells[i].box;
      decal_gvd.addMeshData(&decal_objects[i], bsph.c, bsph.r);
    }
    decal_gvd.connectData(false, NULL);

    for (int i = 0; i < combined_objects.size(); ++i)
    {
      if (!cells[i].combined_mesh)
        continue;
      BSphere3 bsph;
      bsph = cells[i].box;
      combined_gvd.addMeshData(&combined_objects[i], bsph.c, bsph.r);
    }
    combined_gvd.connectData(false, NULL);

    if (hasHmapPaches)
    {
      for (int i = 0; i < patches_objects.size(); ++i)
      {
        if (!cells[i].patches_mesh)
          continue;
        BSphere3 bsph;
        bsph = cells[i].box;
        patches_gvd.addMeshData(&patches_objects[i], bsph.c, bsph.r);
      }
      patches_gvd.connectData(false, NULL);
    }

    con.startProgress();
    con.setTotal(land_objects.size() + lodobjects.size() + combined_objects.size() + decal_objects.size() +
                 (hasHmapPaches ? patches_objects.size() : 0));
    for (int i = 0; i < land_objects.size(); ++i)
    {
      ShaderMeshData &md = land_objects[i];
      if (!tools_internal)
        land_objects[i].optimizeForCache(false);

      for (int j = 0; j < md.elems.size(); ++j)
        matSaver.addMaterial(md.elems[j].mat, texSaver, NULL);
      md.enumVertexData(matSaver);
      con.incDone();
    }

    for (int i = 0; i < lodobjects.size(); ++i)
    {
      ShaderMeshData &md = lodobjects[i];
      if (!tools_internal)
        lodobjects[i].optimizeForCache(false);

      md.enumVertexData(matSaver);
      con.incDone();
    }

    for (int i = 0; i < decal_objects.size(); ++i)
    {
      ShaderMeshData &md = decal_objects[i];
      if (!tools_internal)
        decal_objects[i].optimizeForCache(false);

      for (int j = 0; j < md.elems.size(); ++j)
        matSaver.addMaterial(md.elems[j].mat, texSaver, NULL);
      md.enumVertexData(matSaver);
      con.incDone();
    }

    for (int i = 0; i < combined_objects.size(); ++i)
    {
      ShaderMeshData &md = combined_objects[i];
      if (!tools_internal)
        combined_objects[i].optimizeForCache(false);

      for (int j = 0; j < md.elems.size(); ++j)
        matSaver.addMaterial(md.elems[j].mat, texSaver, NULL);
      md.enumVertexData(matSaver);
      con.incDone();
    }

    if (hasHmapPaches)
    {
      for (int i = 0; i < patches_objects.size(); ++i)
      {
        ShaderMeshData &md = patches_objects[i];
        if (!tools_internal)
          patches_objects[i].optimizeForCache(false);

        for (int j = 0; j < md.elems.size(); ++j)
          matSaver.addMaterial(md.elems[j].mat, texSaver, NULL);
        md.enumVertexData(matSaver);
        con.incDone();
      }
    }

    con.endProgress();

    time0 = dagTools->getTimeMsec();


    bool retval = true;
    Tab<String> texNames(tmpmem);
    texSaver.prepareTexNames(texNames);

    matSaver.writeMatVdataHdr(cwr, texSaver);
    int tex_atype = DAEDITOR3.getAssetTypeId("tex");
    for (int i = 0; i < texSaver.texNames.size(); i++)
    {
      char *p = strchr(texSaver.texNames[i], '*');
      if (p)
        *p = '\0';
      DagorAsset *a = DAEDITOR3.getAssetByName(texSaver.texNames[i], tex_atype);
      if (!a)
      {
        DAEDITOR3.conError("Cannot resolve landmesh tex: %s", texSaver.texNames[i].str());
        texSaver.texNames[i] = "";
        retval = false;
        continue;
      }
      texSaver.texNames[i].printf(128, "%s*", a->getName());
      dd_strlwr(texSaver.texNames[i]);
    }
    texSaver.writeTexStr(cwr);
    // texSaver.writeTexIdx(cwr, texSaver.textures);
    matSaver.writeMatVdata(cwr, texSaver);

    cwr.beginBlock();
    for (int i = 0; i < land_objects.size(); ++i)
    {
      cwr.beginBlock();

      cwr.beginBlock(); // land lods

      cwr.beginBlock();
      land_objects[i].save(cwr, matSaver, true);
      cwr.endBlock();

      if (lodobjects.size())
      {
        cwr.beginBlock();
        lodobjects[i].save(cwr, matSaver, true);
        cwr.endBlock();
      }

      cwr.endBlock();

      cwr.beginBlock();
      decal_objects[i].save(cwr, matSaver, true);
      cwr.endBlock();

      cwr.beginBlock();
      combined_objects[i].save(cwr, matSaver, true);
      cwr.endBlock();

      if (hasHmapPaches)
      {
        cwr.beginBlock();
        patches_objects[i].save(cwr, matSaver, true);
        cwr.endBlock();
      }

      cwr.endBlock();
    }
    cwr.beginBlock();
    for (int i = 0; i < cells.size(); ++i)
      cwr.write32ex(&cells[i].box, sizeof(cells[i].box));
    for (int i = 0; i < cells.size(); ++i)
    {
      Point3 c = cells[i].box.center();
      float sphereRadius = cells[i].land_mesh->calc_mesh_rad(c);
      cwr.writeFloat32e(sphereRadius);
    }
    cwr.endBlock();

    cwr.endBlock();
    clear_and_shrink(land_objects);
    clear_and_shrink(decal_objects);
    clear_and_shrink(combined_objects);
    clear_and_shrink(patches_objects);
    clear_and_shrink(lodobjects);
    DAEDITOR3.conRemark("data saved in %g", (dagTools->getTimeMsec() - time0) / 1000.0);
    ShaderMeshData::buildForTargetCode = _MAKE4C('PC');
    return retval;
  }

  bool exportGeomToShaderMesh(mkbindump::BinDumpSaveCB &cwr, StaticGeometryContainer &geom, const char *tmp_lms_fn,
    const ITextureNumerator &tn) const override
  {
    CoolConsole &con = DAGORED2->getConsole();
    con.startProgress();
    String lmsDir(tmp_lms_fn);
    location_from_path(lmsDir);

    ::dd_mkpath(tmp_lms_fn);
    QuietProgressIndicator qpi;
    if (!StaticSceneBuilder::export_envi_to_LMS1(geom, tmp_lms_fn, qpi, con))
    {
      DAEDITOR3.conError("Error saving lightmapped envi");
      return false;
    }
    StaticSceneBuilder::LightmappedScene scene;

    if (scene.load(tmp_lms_fn, con))
    {
      StaticSceneBuilder::LightingProvider lighting;
      StaticSceneBuilder::StdTonemapper toneMapper;
      StaticSceneBuilder::Splitter splitter;
      splitter.combineObjects = false;
      splitter.splitObjects = false;

      if (!build_and_save_scene1(scene, lighting, toneMapper, splitter, lmsDir, StaticSceneBuilder::SCENE_FORMAT_LdrTgaDds,
            cwr.getTarget(), con, con, true))
      {
        con.endProgress();
        con.endLog();
        DAEDITOR3.conError("ScnExport: cannot build scene");
        return false;
      }

      store_built_scene_data(lmsDir, cwr, tn);
      con.endProgress();
      return true;
    }
    else
      DAEDITOR3.conError("ScnExport: cannot load %s", tmp_lms_fn);
    con.endProgress();
    return false;
  }

  bool exportLoftMasks(const char *fn_mask, const char *fn_id, const char *fn_ht, const char *fn_dir, int tex_sz, const Point3 &w0,
    const Point3 &w1, GeomObject *obj) override
  {
    Texture *texM = d3d::create_tex(NULL, tex_sz, tex_sz, TEXFMT_R8 | TEXCF_RTARGET, 1);
    Texture *texD = d3d::create_tex(NULL, tex_sz, tex_sz, TEXFMT_DEPTH16 | TEXCF_RTARGET, 1);
    Texture *texF = 0;
    Texture *texId = 0;
    if (fn_id)
      texId = d3d::create_tex(NULL, tex_sz, tex_sz, TEXFMT_L16 | TEXCF_RTARGET, 1);
    if (fn_dir)
      texF = d3d::create_tex(NULL, tex_sz, tex_sz, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1);
    G_ASSERTF(texM && texD, "tex_sz=%d", tex_sz);
    if (!texM || !texD)
      return false;

    {
      d3d::GpuAutoLock gpuLock;
      ScopeRenderTarget prevRt;
      ViewProjMatrixContainer prevViewProj;

      Matrix44 projection;
      TMatrix itm;
      itm.setcol(2, 0, -1, 0);
      itm.setcol(1, 0, 0, 1);
      itm.setcol(0, itm.getcol(1) % itm.getcol(2));
      itm.setcol(3, 0, 0, 0);

      ::grs_cur_view.itm = itm;
      ::grs_cur_view.tm = inverse(itm);
      d3d::settm(TM_VIEW, ::grs_cur_view.tm);

      projection = matrix_ortho_off_center_lh(w0.x, w1.x, w0.z, w1.z, -w0.y, -w1.y);
      d3d::settm(TM_PROJ, &projection);

      d3d::set_render_target(texM, 0);
      if (texId)
        d3d::set_render_target(1, texId, 0);
      if (texF)
        d3d::set_render_target(2, texF, 0);
      d3d::set_depth(texD, DepthAccess::RW);
      d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(0, 0, 0, 0), 0, 0);
      shaders::overrides::set(disableDepthClipStateId);
      GeomObject::setNodeNameHashToShaders = true;
      obj->render();
      obj->renderTrans();
      GeomObject::setNodeNameHashToShaders = false;
      shaders::overrides::reset();

      d3d::set_render_target();
    }

    save_tex_as_ddsx(texM, fn_mask);
    del_d3dres(texM);

    if (texId)
    {
      save_tex_as_ddsx(texId, fn_id);
      del_d3dres(texId);
    }

    if (texF)
    {
      save_tex_as_ddsx(texF, fn_dir);
      del_d3dres(texF);
    }

    Texture *texH = d3d::create_tex(NULL, tex_sz, tex_sz, TEXFMT_L16 | TEXCF_RTARGET, 1);
    if (texH)
    {
      d3d::GpuAutoLock gpuLock;
      d3d::stretch_rect(texD, texH);
      del_d3dres(texD);
      save_tex_as_ddsx(texH, fn_ht);
    }
    del_d3dres(texH);
    // save_rt_image_as_tga(texM, String(0, "%s.tga", fn_mask));
    // save_rt_image_as_tga(texH, String(0, "%s.tga", fn_ht));

    DAEDITOR3.conNote("export loft mask: %s sz=%d %@-%@", fn_mask, tex_sz, w0, w1);
    return true;
  }

  void prepare(LandMeshRenderer &r, LandMeshManager &p, const Point3 &center_pos, const BBox3 &box) override
  {
    r.prepare(p, center_pos, 2.f);
    r.setRenderInBBox(box);
  }
};


static void optimizeMesh(Mesh &m)
{
  m.optimize_tverts(1e-4 * 1e-4);
  int id = m.find_extra_channel(SCUSAGE_EXTRA, 7);
  m.optimize_extra(id, 0.0f); // optimize compressed
}


static eastl::unique_ptr<GenericHeightMapService> srv;
static bool srv_released = false;

bool GenericHeightMapService::renderHm2ToFeedback(LandMeshRenderer &r)
{
  if (srv->hm2.texMainId == BAD_TEXTUREID && srv->hm2.texDetId == BAD_TEXTUREID)
    return false;
  srv->renderHm2(r, grs_cur_view.pos, false, false);
  return true;
}

void *get_generic_hmap_service()
{
  if (!srv)
  {
    G_ASSERT(!srv_released);
    srv.reset(new GenericHeightMapService());
    srv->init();
  }
  return srv.get();
}

void release_generic_hmap_service()
{
  G_ASSERT(!srv_released);
  srv_released = true;
  srv.reset();
}
