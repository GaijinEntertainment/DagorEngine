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
#include <heightmap/heightmapRenderer.h>
#include <heightmap/lodGrid.h>
#include <render/grassTranslucency.h>
#include <scene/dag_physMat.h>

#include <3d/dag_drv3d_pc.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <util/dag_bitArray.h>
#include <libTools/util/progressInd.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_ioUtils.h>
#include <util/dag_simpleString.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_fastPtrList.h>
#include <debug/dag_debug.h>
#include <assets/asset.h>
#include <math/dag_mesh.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
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
#include <dllPluginCore/core.h>
#include <ctype.h>
#include <debug/dag_debug3d.h>
#include <render/toroidalHeightmap.h>
#include <3d/dag_resPtr.h>
#include <de3_editorEvents.h>
#include <de3_splineGenSrv.h>

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


extern const char *filter_class_name;
static void optimizeMesh(Mesh &m);
extern TEXTUREID load_land_micro_details(const DataBlock &blk);

#define CALL_LMESH_TYPED_RENDER(X, p) X.render(p, X.RENDER_WITH_CLIPMAP, false)


static int hmap_stor_mismatch_cnt = 0;

enum
{
  RENDERING_LANDMESH = 0,
  RENDERING_CLIPMAP,
  RENDERING_SPOT, // obsolete
  GRASS_COLOR,    // obsolete
  GRASS_MASK,
  SPOT_TO_GRASS_MASK, // obsolete
  RENDERING_HEIGHTMAP,
  RENDER_DEPTH,
  RENDERING_VSM,
  RENDERING_REFLECTION,
  RENDERING_FEEDBACK,
  LMESH_MAX
};
static int render_with_normalmapVarId = -1;
static int should_use_clipmapVarId = -1, bake_landmesh_combineVarId = -1;
static int lmesh_rendering_mode_glob_varId = -1;
static int global_frame_blockid = -1;
static int lmesh_rendering_mode = RENDERING_LANDMESH;
static int set_lmesh_rendering_mode(int mode)
{
  G_ASSERT(mode < LMESH_MAX);
  int old = lmesh_rendering_mode;
  lmesh_rendering_mode = mode;
  ShaderGlobal::set_int_fast(lmesh_rendering_mode_glob_varId, mode);
  return old;
}

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
  template <class T>
  class SubMapStorageImpl : public SubMapStorage<T>
  {
  public:
    SubMapStorageImpl(int sx, int sy, int esz, const T &defval)
    {
      modifications = 0;
      mapSizeX = sx;
      mapSizeY = sy;
      elemSize = esz;
      fileHandle = NULL;
      defaultValue = defval;
      reset(sx, sy, esz, defaultValue);
    }
    virtual ~SubMapStorageImpl()
    {
      closeFile();
      dd_erase(workFn);
      workFn = "";
      mainFn = "";
    }

    bool createFile(const char *basepath, const char *file_name)
    {
      if (fileHandle)
        ::df_close(fileHandle);

      fileHandle = NULL;
      modifications = 0;
      if (basepath && file_name)
        if (!buildPathes(basepath, file_name))
        {
          // debug("%p.createFile(%s,%s) failed; mapSize=%dx%d", this, basepath, file_name, mapSizeX, mapSizeY);
          return true;
        }

      dd_mkpath(workFn);
      fileHandle = ::df_open(workFn, DF_RDWR | DF_CREATE);

      if (!fileHandle)
        return false;

      LFileGeneralSaveCB cwr(fileHandle);

      DAGOR_TRY
      {
        cwr.writeInt(1); // version

        cwr.writeInt(mapSizeX);
        cwr.writeInt(mapSizeY);
        cwr.writeInt(elemSize);

        T *data = new (tmpmem) T[elemSize * elemSize];
        for (int i = 0; i < elemSize * elemSize; ++i)
          data[i] = defaultValue;

        for (int i = 0; i < elems.size(); ++i)
          cwr.write(elems[i].data ? elems[i].data : data, sizeof(T) * elemSize * elemSize);

        delete[] data;
        modifications++;
      }
      DAGOR_CATCH(IGenSave::SaveException)
      {
        DAEDITOR3.conError("error while saving %s", workFn);
        ::df_close(fileHandle);
        fileHandle = NULL;

        return false;
      }

      for (int i = 0; i < elems.size(); ++i)
        elems[i].isModified = false;

      return true;
    }

    bool openFile(const char *basepath, const char *file_name)
    {
      if (fileHandle)
        ::df_close(fileHandle);

      modifications = 0;
      fileHandle = NULL;
      if (!buildPathes(basepath, file_name))
        return false;

      if (!decompressToWorkfile())
        return false;

      fileHandle = ::df_open(workFn, DF_RDWR);
      if (!fileHandle)
      {
        resetElems();
        return true;
      }

      LFileGeneralLoadCB crd(fileHandle);
      DAGOR_TRY
      {
        int version = crd.readInt();

        if (version != 1)
        {
          ::df_close(fileHandle);
          fileHandle = NULL;
          return false;
        }

        crd.readInt(mapSizeX);
        crd.readInt(mapSizeY);
        crd.readInt(elemSize);
      }
      DAGOR_CATCH(IGenLoad::LoadException)
      {
        DAEDITOR3.conError("error while loading %s", workFn);
        ::df_close(fileHandle);
        fileHandle = NULL;
        return false;
      }

      if (16 + mapSizeX * mapSizeY * sizeof(T) != df_length(fileHandle))
      {
        debug("%s: %dx%dx%d sz_req=%d sz_real=%d", file_name, mapSizeX, mapSizeY, elemSize, 16 + mapSizeX * mapSizeY * sizeof(T),
          df_length(fileHandle));
        ::df_close(fileHandle);
        fileHandle = NULL;
        return false;
      }

      resetElems();

      return true;
    }

    virtual void closeFile(bool finalize = false)
    {
      if (fileHandle)
        ::df_close(fileHandle);
      fileHandle = NULL;

      if (finalize && modifications)
        if (!compressToMainfile())
          return;

      if (finalize)
      {
        dd_erase(workFn);
        workFn = "";
        mainFn = "";
      }
    }

    void eraseFile()
    {
      flushData();
      if (fileHandle)
      {
        ::df_close(fileHandle);
        fileHandle = NULL;
      }

      if (!workFn.empty())
        ::dd_erase(workFn);
      if (!mainFn.empty())
        compressToMainfile();

      clear_and_shrink(elems);
    }

    virtual const char *getFileName() const { return mainFn; }
    virtual bool isFileOpened() const { return fileHandle != NULL; }

    virtual bool flushData()
    {
      if (!fileHandle)
      {
        bool mod = false;
        for (int i = 0; i < elems.size(); ++i)
          if (elems[i].isModified && elems[i].data)
            mod = true;
        if (mod && !mainFn.empty())
          return createFile(NULL, NULL);
        return !mainFn.empty();
      }
      LFileGeneralSaveCB cwr(fileHandle);
      bool ret = true;

      DAGOR_TRY
      {
        int esize = sizeof(T) * elemSize * elemSize;

        for (int i = 0; i < elems.size(); ++i)
        {
          Element &e = elems[i];
          if (!e.isModified)
            continue;

          cwr.seekto(FILE_DATA_OFFSET + i * esize);
          cwr.write(e.data, esize);

          e.isModified = false;
          modifications++;
        }
      }
      DAGOR_CATCH(IGenSave::SaveException)
      {
        DAEDITOR3.conError("error while flushing %s", mainFn);
        ret = false;
      }

      df_flush(fileHandle);
      return ret;
    }

    virtual Element *loadElement(int index)
    {
      if (!fileHandle)
        return NULL;

      LFileGeneralLoadCB crd(fileHandle);

      Element *e = &elems[index];

      DAGOR_TRY
      {
        int esize = sizeof(T) * elemSize * elemSize;

        crd.seekto(FILE_DATA_OFFSET + index * esize);

        e->allocate(elemSize, dag::get_allocator(elems));
        crd.read(e->data, esize);
      }
      DAGOR_CATCH(IGenLoad::LoadException)
      {
        DAEDITOR3.conError("error while loading element of %s", mainFn);
        e = NULL;
      }

      return e;
    }

    int getModifications() const
    {
      if (modifications)
        return modifications;
      for (int i = 0; i < elems.size(); i++)
        if (elems[i].isModified)
          return 1;
      return 0;
    }
    bool buildPathes(const char *basepath, const char *file_name)
    {
      if (!basepath || !file_name || !*basepath || !*file_name)
      {
        mainFn = "";
        workFn = "";
        return false;
      }
      mainFn.printf(260, "%s%s", basepath, file_name);
      workFn.printf(260, "%s../.work/%s", basepath, file_name);
      dd_simplify_fname_c(mainFn);
      mainFn.resize(strlen(mainFn) + 1);
      dd_simplify_fname_c(workFn);
      mainFn.resize(strlen(workFn) + 1);
      return true;
    }

    bool compressToMainfile()
    {
      FullFileLoadCB crd(workFn);
      if (!crd.fileHandle && !dd_file_exist(mainFn))
        return true;
      FullFileSaveCB cwr(mainFn);
      if (!cwr.fileHandle)
        return false;

      int ver = 1 | 0x80000000; // version + compressed
      if (!crd.fileHandle)
        ver |= 0x40000000;
      cwr.writeInt(ver);
      cwr.writeInt(mapSizeX);
      cwr.writeInt(mapSizeY);
      cwr.writeInt(elemSize);

      if (crd.fileHandle)
      {
        crd.seekto(FILE_DATA_OFFSET);
        lzma_compress_data(cwr, 9, crd, mapSizeX * mapSizeY * sizeof(T));
      }
      crd.close();
      cwr.close();

      modifications = 0;
      return true;
    }
    bool decompressToWorkfile()
    {
      dd_mkpath(workFn);
      dd_erase(workFn);

      FullFileLoadCB crd(mainFn);
      if (!crd.fileHandle)
        return true;

      if (df_length(crd.fileHandle) < 16)
      {
        DAEDITOR3.conError("%s: too small (%d bytes) during decompression", mainFn, df_length(crd.fileHandle));
        return false;
      }
      int ver = crd.readInt();
      if ((ver & 0x8000FFFF) != (1 | 0x80000000))
      {
        logerr("%s: bad version %08X", ver);
        return false;
      }
      if (ver & 0x40000000)
        return true;
      int sx = crd.readInt();
      int sy = crd.readInt();
      int esz = crd.readInt();
      if (sx != mapSizeX || sy != mapSizeY || esz != elemSize)
      {
        logerr("%s: size mismatch (%dx%d,%d) != (%dx%d,%d)", mainFn.str(), sx, sy, esz, mapSizeX, mapSizeY, elemSize);
        hmap_stor_mismatch_cnt++;
        if (hmap_stor_mismatch_cnt == 1000)
          DAG_FATAL("%s: size mismatch (%dx%d,%d) != (%dx%d,%d)\n(too many errors)", mainFn.str(), sx, sy, esz, mapSizeX, mapSizeY,
            elemSize);
        return false;
      }

      FullFileSaveCB cwr(workFn);
      if (!cwr.fileHandle)
        return false;

      cwr.writeInt(ver & ~0x80000000);
      cwr.writeInt(sx);
      cwr.writeInt(sy);
      cwr.writeInt(esz);

      DAGOR_TRY
      {
        LzmaLoadCB z_crd(crd, df_length(crd.fileHandle) - 16);
        copy_stream_to_stream(z_crd, cwr, sx * sy * sizeof(T));
        z_crd.close();
        crd.close();
        cwr.close();
      }
      DAGOR_CATCH(DagorException e)
      {
        DAEDITOR3.conError("%s: exception <%s> during decompression", mainFn, e.excDesc);

        cwr.seekto(0);
        cwr.writeInt(ver & ~0x80000000);
        cwr.writeInt(sx);
        cwr.writeInt(sy);
        cwr.writeInt(esz);
        for (int i = 0; i < sx * sy; ++i)
          cwr.write(&defaultValue, sizeof(T));
        modifications++;

        DAEDITOR3.conWarning("%s: inited with default value", mainFn);
      }
      return true;
    }

    bool openOldFile(const char *file_name)
    {
      if (fileHandle)
        ::df_close(fileHandle);

      mainFn = file_name;
      workFn.printf(260, "%s.unz", file_name);
      modifications = 0;

      fileHandle = ::df_open(mainFn, DF_RDWR);
      if (!fileHandle)
        return false;

      LFileGeneralLoadCB crd(fileHandle);
      if (crd.readInt() != _MAKE4C('ZLIB'))
        crd.seekto(0);
      else
      {
        int fsize = crd.readInt();

        FullFileSaveCB cwr(workFn);
        G_ASSERT(cwr.fileHandle);

        ZlibLoadCB z_crd(crd, fsize);
        copy_stream_to_stream(z_crd, cwr, fsize);
        z_crd.close();

        df_close(fileHandle);
        fileHandle = ::df_open(workFn, DF_RDWR);
        crd.fileHandle = fileHandle;
        crd.seekto(0);
      }

      DAGOR_TRY
      {
        int version = crd.readInt();

        if (version != 1)
        {
          ::df_close(fileHandle);
          fileHandle = NULL;
          return false;
        }

        crd.readInt(mapSizeX);
        crd.readInt(mapSizeY);
        crd.readInt(elemSize);
      }
      DAGOR_CATCH(IGenLoad::LoadException)
      {
        DAEDITOR3.conError("%s: exception <%s> during decompression (old file)", mainFn);
        ::df_close(fileHandle);
        fileHandle = NULL;
        return false;
      }

      if (FILE_DATA_OFFSET + mapSizeX * mapSizeY * sizeof(T) != df_length(fileHandle))
      {
        debug("%s: %dx%dx%d sz_req=%d sz_real=%d", file_name, mapSizeX, mapSizeY, elemSize, 16 + mapSizeX * mapSizeY * sizeof(T),
          df_length(fileHandle));
        ::df_close(fileHandle);
        fileHandle = NULL;
        return false;
      }

      resetElems();

      return true;
    }

  protected:
    static const int FILE_DATA_OFFSET = 16;

    file_ptr_t fileHandle;
    String mainFn, workFn;
    int modifications;
  };

  template <class T>
  class MapStorageImpl : public MapStorage<T>
  {
  public:
    typedef SubMapStorageImpl<T> SubMapImpl;

    MapStorageImpl(int sx, int sy, const T &defval)
    {
      mapSizeX = (sx + ELEM_SZ - 1) / ELEM_SZ * ELEM_SZ;
      mapSizeY = (sy + ELEM_SZ - 1) / ELEM_SZ * ELEM_SZ;
      defaultValue = defval;
      reset(sx, sy, defaultValue);
    }
    virtual ~MapStorageImpl() { closeFile(); }

    virtual bool createFile(const char *file_name)
    {
      if (!buildPathes(file_name))
        return false;

      dd_mkdir(path);
      DataBlock blk;
      blk.setInt("w", mapSizeX);
      blk.setInt("h", mapSizeY);
      int64_t dv = 0;
      *(T *)&dv = defaultValue;
      blk.setInt64("defVal", dv);
      for (int i = 0; i < subMaps.size(); i++)
        if (subMaps[i])
          subMap(i)->createFile(path, getSubMapPath(i));

      blk.saveToTextFile(String(260, "%s%s.blk", path.str(), fname.str()));
      return true;
    }

    virtual bool openFile(const char *file_name)
    {
      if (!buildPathes(file_name))
        return false;

      String fn_blk(260, "%s%s.blk", path.str(), fname.str());
      if (!dd_file_exist(fn_blk) && dd_file_exist(file_name))
      {
        // do conversion
        logwarn("convert old %s to new %s", file_name, path.str());
        SubMapImpl *e = new SubMapImpl(128, 128, 128, 0);
        if (!e->openOldFile(file_name))
        {
          logerr("failed to open old %s", file_name);
          return false;
        }

        dd_mkdir(path + "../.work");
        reset(e->getMapSizeX(), e->getMapSizeY(), e->defaultValue);
        for (int y = 0, ye = e->getMapSizeY(); y < ye; y++)
          for (int x = 0, xe = e->getMapSizeX(); x < xe; x++)
            setData(x, y, e->getData(x, y));
        e->closeFile(true);
        delete e;

        if (!createFile(file_name))
          return false;
        flushData();
        dd_erase(file_name);

        for (int i = 0; i < subMaps.size(); i++)
          if (subMaps[i])
            subMap(i)->compressToMainfile();
        return true;
      }

      DataBlock blk;
      if (!blk.load(fn_blk))
        return false;
      if (!blk.paramCount())
        return false;

      int64_t dv = blk.getInt64("defVal");
      reset(blk.getInt("w"), blk.getInt("h"), *(T *)&dv);
      return true;
    }

    virtual void closeFile(bool finalize = false)
    {
      for (int i = 0; i < subMaps.size(); i++)
        if (subMaps[i])
          subMaps[i]->closeFile(finalize);

      if (finalize)
      {
        fname = "";
        path = "";
      }
    }

    virtual void eraseFile()
    {
      for (int i = 0; i < subMaps.size(); i++)
        if (subMaps[i])
          subMap(i)->eraseFile();
      clear_and_shrink(subMaps);
      if (!fname.empty())
      {
        String blk_fn(260, "%s%s.blk", path.str(), fname.str());
        if (dd_file_exist(blk_fn))
          DataBlock().saveToTextFile(blk_fn);
      }
      fname = "";
    }

    virtual const char *getFileName() const { return path; }
    virtual bool isFileOpened() const { return !fname.empty(); }
    virtual bool canBeReverted() const
    {
      int changes = 0;
      for (int i = 0; i < subMaps.size(); i++)
        if (subMaps[i])
          changes += subMap(i)->getModifications();
      return changes > 0;
    }
    virtual bool revertChanges()
    {
      for (int i = 0; i < subMaps.size(); i++)
        if (subMaps[i] && subMap(i)->getModifications())
          del_it(subMaps[i]);
      return true;
    }

    virtual typename MapStorage<T>::SubMap *createSubMap(int ex, int ey)
    {
      SubMapImpl *e = new SubMapImpl(calcSubMapSizeX(ex), calcSubMapSizeY(ey), ELEM_SZ, defaultValue);
      if (!e->createFile(path, getSubMapPath(ex, ey)))
      {
        delete e;
        return NULL;
      }
      return subMaps[ey * numSubMapsX + ex] = e;
    }
    virtual typename MapStorage<T>::SubMap *loadSubMap(int index)
    {
      if (fname.empty())
        return NULL;
      int ex = index % numSubMapsX;
      int ey = index / numSubMapsY;
      SubMapImpl *e = new SubMapImpl(calcSubMapSizeX(ex), calcSubMapSizeY(ey), ELEM_SZ, defaultValue);
      if (!e->openFile(path, getSubMapPath(index)))
      {
        DAEDITOR3.conError("failed to load subelem %s : %d -> %s", path, index, getSubMapPath(index));
        delete e;
        e = new SubMapImpl(calcSubMapSizeX(ex), calcSubMapSizeY(ey), ELEM_SZ, defaultValue);
      }
      return subMaps[index] = e;
    }

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
    String getSubMapPath(int ex, int ey) const { return String(260, "%04d-%04d.%s.dat", ex, ey, fname.str()); }
    String getSubMapPath(int index) const { return getSubMapPath(index % numSubMapsX, index / numSubMapsX); }
    int calcSubMapSizeX(int ex) const { return ex == numSubMapsX - 1 ? mapSizeX - (numSubMapsX - 1) * SUBMAP_SZ : SUBMAP_SZ; }
    int calcSubMapSizeY(int ey) const { return ey == numSubMapsY - 1 ? mapSizeY - (numSubMapsY - 1) * SUBMAP_SZ : SUBMAP_SZ; }

    SubMapImpl *subMap(int idx) { return static_cast<SubMapImpl *>(subMaps[idx]); }
    const SubMapImpl *subMap(int idx) const { return static_cast<const SubMapImpl *>(subMaps[idx]); }
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
      hmInitial = new MapStorageImpl<float>(sx, sy, 0);
      if (sep_final)
        hmFinal = new MapStorageImpl<float>(sx, sy, 0);
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
            debug_ctx("unnamed layer, skipped");
            continue;
          }

          int id = layerNames.getNameId(name);
          if (id != -1)
          {
            debug_ctx("duplicate layer: %s, skipped", name);
            continue;
          }
          if (bits <= 0 || bits > 32)
          {
            debug_ctx("invalid layer bit width: %s - %d bits, skipped", name, bits);
            continue;
          }
          if (total_bits + bits > 32)
          {
            debug_ctx("too big layer: %s - %d bits, previous layers already use %d bits, skipped", name, bits, total_bits);
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
                debug_ctx("too many (max=32) different attributes; %s skipped", attr_name);
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
  bool rebuilLastClip;
  bool preparingClipmap;

  struct HeightMap2
  {
    LodGrid lodGrid;
    HeightmapRenderer rend;
    float sx, sy, ax, ay;
    TEXTUREID texMainId, texDetId;
    BBox2 bboxMain, bboxDet;

    HeightMap2() : sx(1), sy(1), ax(1), ay(1), texMainId(BAD_TEXTUREID), texDetId(BAD_TEXTUREID)
    {
      bboxMain.setempty(), bboxDet.setempty();
    }
  } hm2;

  GrassTranslucency *grassTranslucency;
  float grassTranslucencyHalfSize;

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
    rebuilLastClip = true;
    lastClipTexSz = 4096;
  }
  void init()
  {
    DataBlock appblk(String(260, "%s/application.blk", DAGORED2->getWorkspace().getAppDir()));

    const Driver3dDesc &desc = d3d::get_driver_desc();
    lastClipTexSz = min(desc.maxtexw, desc.maxtexh);

    shouldUseClipmap = appblk.getBlockByName("clipmap") != NULL;
    clipmap = NULL;

    lmesh_rendering_mode = RENDERING_LANDMESH;
    lmesh_rendering_mode_glob_varId = ::get_shader_glob_var_id("lmesh_rendering_mode", true);
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

    Ptr<ShaderMaterial> hm2_mat = new_shader_material_by_name("heightmap");
    if (hm2_mat)
    {
      hm2.lodGrid.init(8, 1, 0, 1);
      hm2.rend.init("heightmap", "", true);
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
    del_it(toroidalHeightmap);
    closeFixedClip();
    del_it(grassTranslucency);
  }

  DataBlock microDetails;
  TEXTUREID landMicrodetailsId = BAD_TEXTUREID;

  DataBlock grassBlk;

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
  }

  void setGrassBlk(const DataBlock *blk) override { grassBlk = *blk; }

  void initClipmap()
  {
    DataBlock appblk(String(260, "%s/application.blk", DAGORED2->getWorkspace().getAppDir()));
    const DataBlock *clipBlk = appblk.getBlockByName("clipmap");
    G_ASSERT(shouldUseClipmap);
    G_ASSERT(clipBlk != NULL);

    float clipmapScale = safediv(1.0f, sqrtf(clamp(clipBlk->getReal("clipmapScale", 1.f), 0.5f, 4.0f)));
    float texel_sz = clamp(clipBlk->getReal("texelSize", 0.015) * clipmapScale, 0.0025f, 1.0f);
    int cache_cnt = clipBlk->getInt("cacheCnt", 0);
    int buf_cnt = clipBlk->getInt("bufferCnt", 0);

    bool useToroidalHeightmap = clipBlk->getBool("useToroidalHeightmap", false);

    clipmap = new Clipmap(NULL, clipBlk->getBool("useUAVFeedback", false));
    clipmap->init(texel_sz, Clipmap::CPU_HW_FEEDBACK, clipBlk->getInt("texMips", 6));
    clipmap->initVirtualTexture(clipBlk->getInt("cacheWidth", 4096), clipBlk->getInt("cacheHeight", 8192), float(2 * 1920 * 1080));
    if (useToroidalHeightmap)
    {
      toroidalHeightmap = new ToroidalHeightmap;
      toroidalHeightmap->init(2048, 32.0f, 96.0f, TEXFMT_L8, 128);
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

    lastClipTexSz = min(clipBlk->getInt("lastClipTexSz", 4096), lastClipTexSz);
    G_ASSERT(clipmap != NULL);
    DAEDITOR3.conNote("hmapSrv: clipmap %s inited on-demand (texelSz=%.4f cacheCnt=%d lastClipSz=%d)",
      clipBlk->getBool("argb32bit", false) ? "ARGB8" : "RGB565", texel_sz, cache_cnt, lastClipTexSz);
    microDetails.setBool("__not_inited", true);
  }

  virtual bool mapStorageFileExists(const char *fname) const
  {
    const char *fn = dd_get_fname(fname);
    const char *ext = dd_get_fname_ext(fname);
    String blk_fn(260, "%.*s/%.*s.blk", ext - fname, fname, ext - fn, fn);
    if (dd_file_exist(blk_fn))
    {
      DataBlock blk;
      return blk.load(blk_fn) && blk.paramCount();
    }
    return dd_file_exist(fname);
  }

  virtual bool createStorage(HeightMapStorage &_hm, int sx, int sy, bool sep_final)
  {
    HmapStorage &hm = (HmapStorage &)_hm;
    hm.alloc(sx, sy, sep_final);
    return true;
  }
  virtual void destroyStorage(HeightMapStorage &_hm)
  {
    HmapStorage &hm = (HmapStorage &)_hm;
    hm.destroy();
  }

  virtual FloatMapStorage *createFloatMapStorage(int sx, int sy, float defval) { return new MapStorageImpl<float>(sx, sy, defval); }
  virtual Uint64MapStorage *createUint64MapStorage(int sx, int sy, uint64_t defval)
  {
    return new MapStorageImpl<uint64_t>(sx, sy, defval);
  }
  virtual Uint32MapStorage *createUint32MapStorage(int sx, int sy, unsigned defval)
  {
    return new MapStorageImpl<uint32_t>(sx, sy, defval);
  }
  virtual Uint16MapStorage *createUint16MapStorage(int sx, int sy, unsigned defval)
  {
    return new MapStorageImpl<uint16_t>(sx, sy, defval);
  }
  virtual ColorMapStorage *createColorMapStorage(int sx, int sy, E3DCOLOR defval)
  {
    return new MapStorageImpl<E3DCOLOR>(sx, sy, defval);
  }


  virtual void *createBitLayersList(const DataBlock &desc) { return new (midmem) BitLayersList(desc); }
  virtual void destroyBitLayersList(void *handle)
  {
    BitLayersList *bll = (BitLayersList *)handle;
    del_it(bll);
  }
  virtual dag::Span<HmapBitLayerDesc> getBitLayersList(void *handle)
  {
    BitLayersList *bll = (BitLayersList *)handle;
    return bll ? make_span(bll->layers) : dag::Span<HmapBitLayerDesc>();
  }
  virtual int getBitLayerIndexByName(void *handle, const char *layer_name)
  {
    const BitLayersList *bll = (BitLayersList *)handle;
    return bll ? bll->layerNames.getNameId(layer_name) : -1;
  }
  virtual const char *getBitLayerName(void *handle, int layer_idx)
  {
    const BitLayersList *bll = (BitLayersList *)handle;
    return bll ? bll->layerNames.getName(layer_idx) : NULL;
  }
  virtual int getBitLayerAttrId(void *handle, const char *attr_name)
  {
    const BitLayersList *bll = (BitLayersList *)handle;
    return bll ? bll->attrNames.getNameId(attr_name) : -1;
  }
  virtual bool testBitLayerAttr(void *handle, int layer_idx, int attr_id)
  {
    if (!handle || layer_idx < 0 || attr_id < 0 || attr_id >= 32)
      return false;

    const BitLayersList *bll = (BitLayersList *)handle;
    return (bll->layerAttr[layer_idx] & (1U << attr_id)) ? true : false;
  }
  virtual int findBitLayerByAttr(void *handle, int attr_id, int start_with_layer)
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

  /*virtual void createDetLayerClassList(const DataBlock &blk)
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
  virtual dag::Span<HmapDetLayerProps> getDetLayerClassList()
  {
    return make_span(detLayers);
  }
  virtual int resolveDetLayerClassName(const char *nm)
  {
    return detLayersMap.getNameId(nm);
  }

  virtual void *createDetailRenderData(const DataBlock &layers, LandClassDetailInfo *detTex)
  {
    void *h = new DetailRenderData(layers, detTex, detLayers);
    updateDetailRenderData(h, layers);
    return h;
  }
  virtual void destroyDetailRenderData(void *handle)
  {
    if (handle)
    {
      handlesToUpdate.delPtr(handle);
      delete (DetailRenderData*)handle;
    }
  }
  virtual void updateDetailRenderData(void *handle, const DataBlock &layers)
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

  virtual Texture *getDetailRenderDataMaskTex(void *handle, const char *name)
  {
    return handle ? ((DetailRenderData*)handle)->getMaskTex(name) : NULL;
  }
  virtual TEXTUREID getDetailRenderDataTex(void *handle, const char *name)
  {
    return handle ? ((DetailRenderData*)handle)->getAuxTex(name) : BAD_TEXTUREID;
  }
  virtual void storeDetailRenderData(void *handle, const char *prefix, bool store_cmap, bool store_smap)
  {
    if (handle)
      ((DetailRenderData*)handle)->storeData(prefix, store_cmap, store_smap);
  }
  */

  virtual const char *getLandPhysMatName(int pmatId) { return pmatId >= 0 ? PhysMat::getMaterial(pmatId).name.str() : NULL; }

  virtual bool getIsSolidByPhysMatName(const char *name)
  {
    int pmatId = PhysMat::getMaterialId(name);
    return pmatId >= 0 ? PhysMat::getMaterial(pmatId).isSolid : false;
  }

  virtual GenHmapData *registerGenHmap(const char *name, HeightMapStorage *hm, Uint32MapStorage *landclsmap,
    dag::ConstSpan<HmapBitLayerDesc> lndclass_layers, ColorMapStorage *colormap, Uint32MapStorage *ltmap, const Point2 &hmap_ofs,
    float cell_size)
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
  virtual bool unregisterGenHmap(const char *name)
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
  virtual GenHmapData *findGenHmap(const char *name)
  {
    int id = genHmapNames.getNameId(name);
    return (id != -1 && (genHmapReg & (1 << id))) ? &genHmap[id] : NULL;
  }

  virtual void initLodGridHm2(const DataBlock &blk)
  {
    static const int MAX_LOD_CNT = 16;
    int nid = blk.getNameId("lodRad");
    int lod_cnt = clamp(blk.getInt("lodCount", 8), 3, (int)MAX_LOD_CNT);
    int lod0Rad = blk.getInt("lod0Rad", 1);
    int tesselation = blk.getInt("tesselation", 0);
    int lodRadMul = blk.getInt("lodRadMul", 1);
    hm2.lodGrid.init(lod_cnt, lod0Rad, tesselation, lodRadMul);
    DAEDITOR3.conNote("initLodGridHm2(%d, %d, %d, %d)", lod_cnt, lod0Rad, tesselation, lodRadMul);
  }
  virtual void setupRenderHm2(float sx, float sy, float ax, float ay, Texture *htTexMain, TEXTUREID htTexIdMain, float mx0, float my0,
    float mw, float mh, Texture *htTexDet, TEXTUREID htTexIdDet, float dx0, float dy0, float dw, float dh)
  {
    static int w2hm_main_gvid = ::get_shader_glob_var_id("world_to_hmap_low", true);
    static int w2hm_det_gvid = ::get_shader_glob_var_id("world_to_hmap_high", true);
    static int tex_main_gvid = ::get_shader_glob_var_id("tex_hmap_low", true);
    static int tex_det_gvid = ::get_shader_glob_var_id("tex_hmap_high", true);
    static int tex_hmap_inv_sizes_gvid = ::get_shader_glob_var_id("tex_hmap_inv_sizes", true);

    hm2.sx = sx;
    hm2.sy = sy;
    hm2.ax = ax;
    hm2.ay = ay;
    hm2.texMainId = htTexMain ? htTexIdMain : BAD_TEXTUREID;
    hm2.texDetId = htTexDet ? htTexIdDet : BAD_TEXTUREID;

    TextureInfo ti;
    Color4 tisz(1.0f / 4096, 1.0f / 4096, 1.0f / 4096, 1.0f / 4096);
    if (htTexMain)
    {
      if (!htTexMain->getinfo(ti))
        ti.w = ti.h = 4096;
      tisz.r = 1.0f / ti.w;
      tisz.g = 1.0f / ti.h;
      ShaderGlobal::set_color4_fast(w2hm_main_gvid, Color4(1.f / mw, 1.f / mh, -mx0 / mw, -my0 / mh));
      ShaderGlobal::set_texture_fast(tex_main_gvid, htTexIdMain);
      hm2.bboxMain[0].x = mx0;
      hm2.bboxMain[0].y = my0;
      hm2.bboxMain[1].x = mx0 + mw;
      hm2.bboxMain[1].y = my0 + mh;
    }

    if (htTexDet)
    {
      if (!htTexDet->getinfo(ti))
        ti.w = ti.h = 4096;
      tisz.b = 1.0f / ti.w;
      tisz.a = 1.0f / ti.h;
      ShaderGlobal::set_color4_fast(w2hm_det_gvid, Color4(1.f / dw, 1.f / dh, -dx0 / dw, -dy0 / dh));
      ShaderGlobal::set_texture_fast(tex_det_gvid, htTexIdDet);
      if (!htTexMain)
      {
        ShaderGlobal::set_color4_fast(w2hm_main_gvid, Color4(1.f / dw, 1.f / dh, -dx0 / dw, -dy0 / dh));
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
      ShaderGlobal::set_color4_fast(w2hm_det_gvid, Color4(0, 0, -1, -1));

    if (!htTexMain && !htTexDet)
    {
      ShaderGlobal::set_texture_fast(tex_main_gvid, BAD_TEXTUREID);
      ShaderGlobal::set_texture_fast(tex_det_gvid, BAD_TEXTUREID);
    }
    ShaderGlobal::set_color4_fast(tex_hmap_inv_sizes_gvid, tisz);
  }

  virtual void startUAVFeedback() const
  {
    if (clipmap && lmesh_rendering_mode == RENDERING_LANDMESH)
      clipmap->startUAVFeedback();
  }

  virtual void endUAVFeedback() const
  {
    if (clipmap && lmesh_rendering_mode == RENDERING_LANDMESH)
    {
      clipmap->endUAVFeedback();
      clipmap->copyUAVFeedback();
    }
  }

  virtual int setLod0SubDiv(int lod)
  {
    int old = hm2.lodGrid.lod0SubDiv;
    hm2.lodGrid.lod0SubDiv = lod;
    return old;
  }

  virtual BBox3 getLMeshBBoxWithHMapWBBox(LandMeshManager &p) const override { return p.getBBoxWithHMapWBBox(); }

  virtual void renderHm2(const Point3 &vpos, bool infinite, bool render_hm) const
  {
    if (hm2.texMainId == BAD_TEXTUREID && hm2.texDetId == BAD_TEXTUREID)
      return;

    int oldMode;
    if (render_hm)
      oldMode = set_lmesh_rendering_mode(RENDERING_HEIGHTMAP);

    d3d::settm(TM_WORLD, TMatrix::IDENT);
    static int heightmap_vs_texture_noVarId = get_shader_variable_id("heightmap_vs_texture_no", true);
    static int heightmap_vs_high_texture_noVarId = get_shader_variable_id("heightmap_vs_high_texture_no", true);

    static int hmapLdetailVarId = get_shader_variable_id("hmap_ldetail", true);
    static int hmapHdetailVarId = get_shader_variable_id("hmap_hdetail", true);

    ShaderGlobal::set_texture(hmapLdetailVarId, hm2.texMainId != BAD_TEXTUREID ? hm2.texMainId : hm2.texDetId);
    ShaderGlobal::set_texture(hmapHdetailVarId, hm2.texDetId != BAD_TEXTUREID ? hm2.texDetId : hm2.texMainId);
    const BBox2 *clip = hm2.texMainId != BAD_TEXTUREID ? (infinite ? NULL : &hm2.bboxMain) : &hm2.bboxDet;
    hm2.rend.setRenderClip(clip);
    static LodGridCullData cullData;
    mat44f globtm;
    d3d::getglobtm(globtm);
    Frustum frustum;
    frustum.construct(globtm);
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
      hm2.ay * lodScale, -10000, 10000, &frustum, clip, cullData, NULL, lod0AreaSize);
    hm2.rend.render(hm2.lodGrid, cullData);
    ShaderGlobal::set_texture(hmapLdetailVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(hmapHdetailVarId, BAD_TEXTUREID);

    if (render_hm)
      set_lmesh_rendering_mode(oldMode);
    cullData.frustum.reset();
  }
  virtual void renderHm2VSM(const Point3 &vpos) const
  {
    if (hm2.texMainId == BAD_TEXTUREID && hm2.texDetId == BAD_TEXTUREID)
      return;
    int oldMode = set_lmesh_rendering_mode(RENDERING_VSM);
    renderHm2(vpos, true, false);
    set_lmesh_rendering_mode(oldMode);
  }

  static bool renderHm2ToFeedback();

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
      ViewProjMatrixContainer oviewproj;
      float texelSize;
      int texSize;
      IPoint2 newOrigin;
      float minZ, maxZ;
      int prevLandmeshRenderingMode;
      Tab<IRenderingService *> rendSrv;

      HmapPatchesCallback(float texel, const IPoint2 &new_origin, int tex_size, float minz, float maxz) :
        oviewproj(), texelSize(texel), texSize(tex_size), newOrigin(new_origin), minZ(minz), maxZ(maxz), prevLandmeshRenderingMode(0)
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
        ShaderGlobal::set_color4(get_shader_variable_id("hmap_patches_min_max_z", true), minZ, maxZ, 0.0, 1.0);
        filter_class_name = "land_mesh";
        prevLandmeshRenderingMode = ::set_lmesh_rendering_mode(RENDERING_CLIPMAP);
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
        ISplineGenService *splSrv = EDITORCORE->queryEditorInterface<ISplineGenService>();
        FastIntList loft_layers;
        loft_layers.addInt(0);
        if (splSrv)
          splSrv->gatherLoftLayers(loft_layers, true);
        d3d::settm(TM_WORLD, TMatrix::IDENT);
        TMatrix4 globtm;
        d3d::getglobtm(globtm);
        Frustum frustum;
        frustum.construct(globtm);
        for (int lli = 0; lli < loft_layers.size(); lli++)
        {
          int ll = loft_layers.getList()[lli];
          if (splSrv)
            splSrv->renderLoftGeom(ll, true, frustum, -1, true);
        }
      }
      void end()
      {
        d3d_set_view_proj(oviewproj);
        ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
        ::set_lmesh_rendering_mode(prevLandmeshRenderingMode);
        filter_class_name = NULL;
        shaders::overrides::reset();
      }
    };

  public:
    explicit HeightmapPatchesRenderer(int resolution) : texSize(resolution)
    {
      hmapPatchesDepthTex = dag::create_tex(NULL, texSize, texSize, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "hmap_patches_depth_tex");
      hmapPatchesDepthTex->texaddr(TEXADDR_WRAP);
      hmapPatchesDepthTex->texfilter(TEXFILTER_POINT);
      hmapPatchesTex = dag::create_tex(NULL, texSize, texSize, TEXCF_RTARGET | TEXFMT_L16, 1, "hmap_patches_tex");
      hmapPatchesTex->texaddr(TEXADDR_WRAP);
      hmapPatchesTex->texfilter(TEXFILTER_SMOOTH);
      processHmapPatchesDepth.init("process_hmap_patches_depth");
      hmapPatchesData.curOrigin = IPoint2(-1000000, 1000000);
      hmapPatchesData.texSize = texSize;
      world_to_hmap_patches_tex_ofsVarId = ::get_shader_glob_var_id("world_to_hmap_patches_tex_ofs", true);
      world_to_hmap_patches_ofsVarId = ::get_shader_glob_var_id("world_to_hmap_patches_ofs", true);
    }
    void invalidate() { hmapPatchesData.curOrigin = IPoint2(-1000000, 1000000); }
    void render(const LandMeshManager &provider, const Point3 &origin, float distance)
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
        HmapPatchesCallback patchCb(texelSize, newTexelsOrigin, hmapPatchesData.texSize, minZ, maxZ);
        toroidal_update(newTexelsOrigin, hmapPatchesData, fullUpdateThresholdTexels, patchCb);

        d3d::resource_barrier({hmapPatchesDepthTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
        d3d::set_render_target(0, hmapPatchesTex.getTex2D(), 0);
        d3d::set_depth((Texture *)nullptr, DepthAccess::RW);
        processHmapPatchesDepth.render();
        d3d::resource_barrier({hmapPatchesTex.getTex2D(), RB_RO_SRV | RB_STAGE_VERTEX | RB_STAGE_COMPUTE, 0, 0});
        Point2 ofs =
          point2((hmapPatchesData.mainOrigin - hmapPatchesData.curOrigin) % hmapPatchesData.texSize) / hmapPatchesData.texSize;

        ShaderGlobal::set_color4(world_to_hmap_patches_tex_ofsVarId, ofs.x, ofs.y, 0, 0);
        alignedOrigin = point2(hmapPatchesData.curOrigin) * texelSize;
        ShaderGlobal::set_color4(world_to_hmap_patches_ofsVarId, 1.0f / fullDistance, 1.0f / fullDistance,
          -alignedOrigin.x / fullDistance + 0.5, -alignedOrigin.y / fullDistance + 0.5);
      }
    }
  };
  class LandmeshCMRenderer : public ClipmapRenderer, public ToroidalHeightmapRenderer
  {
  public:
    LandMeshRenderer &renderer;
    LandMeshManager &provider;
    Tab<IRenderingService *> rendSrv;
    IRenderingService *hmap;
    float minZ, maxZ;
    float hRel;
    int omode;
    TMatrix4 ovtm, oproj;
    bool perspvalid;
    Driver3dPerspective persp;
    unsigned old_st_mask;
    unsigned new_st_mask;
    shaders::OverrideStateId flipCullStateId;

    LandmeshCMRenderer(LandMeshRenderer &r, LandMeshManager &p) :
      rendSrv(tmpmem), renderer(r), provider(p), hmap(0), omode(0), old_st_mask(0), new_st_mask(0)
    {
      shaders::OverrideState flipCullState;
      flipCullState.set(shaders::OverrideState::FLIP_CULL);
      flipCullStateId = shaders::overrides::create(flipCullState);
    }
    void startRender() { get_clipmap_rendering_services(hmap, rendSrv); }

    virtual void startRenderTiles(const Point2 &center)
    {
      Point3 pos(center.x, hRel, center.y);
      BBox3 landBox = provider.getBBox();
      minZ = landBox[0].y - 100;
      maxZ = landBox[1].y + 500;

      omode = ::set_lmesh_rendering_mode(RENDERING_CLIPMAP);
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

      renderer.prepare(provider, pos, pos.y);

      get_subtype_mask_for_clipmap_rendering(old_st_mask, new_st_mask);
    }
    virtual void endRenderTiles()
    {
      renderer.setRenderInBBox(BBox3());
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
      ::set_lmesh_rendering_mode(omode);
      d3d::settm(TM_VIEW, &ovtm);
      d3d::settm(TM_PROJ, &oproj);
      if (perspvalid)
        d3d::setpersp(persp);
      shaders::overrides::reset();
    }

    virtual void renderTile(const BBox2 &region)
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
    }
    virtual void renderFeedback(const TMatrix4 &globtm)
    {
      d3d::settm(TM_WORLD, TMatrix::IDENT);

      int oldMode = set_lmesh_rendering_mode(RENDERING_FEEDBACK);
      if (!renderHm2ToFeedback())
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
      set_lmesh_rendering_mode(oldMode);
    }
  };

  Clipmap *clipmap;
  ToroidalHeightmap *toroidalHeightmap;
  eastl::unique_ptr<HeightmapPatchesRenderer> heightmapPatchesRenderer;
  bool shouldUseClipmap;
  shaders::UniqueOverrideStateId flipCullStateId;
  shaders::UniqueOverrideStateId blendOpMaxStateId;
  shaders::UniqueOverrideStateId disableDepthClipStateId;

  virtual LandMeshManager *createLandMeshManager(IGenLoad &crd)
  {
    LandMeshManager *p = new LandMeshManager(true);
    p->setGrassMaskBlk(grassBlk);
    if (p->loadDump(crd))
      return p;
    delete p;
    return NULL;
  }
  virtual void destroyLandMeshManager(LandMeshManager *&p) const
  {
    if (!p)
      return;
    del_it(p);
  }

  virtual LandMeshRenderer *createLandMeshRenderer(LandMeshManager &p)
  {
    Ptr<ShaderMaterial> mat = new_shader_material_by_name("land_mesh", NULL);
    if (!mat.get())
      return NULL;
    DAEDITOR3.setFatalHandler();
    LandMeshRenderer *r = p.createRenderer();

    DAEDITOR3.popFatalHandler();
    return (LandMeshRenderer *)r;
  }

  virtual void destroyLandMeshRenderer(LandMeshRenderer *&r) const
  {
    if (!r)
      return;
    del_it(r);
  }

  virtual void prepareLandMesh(LandMeshRenderer &r, LandMeshManager &p, const Point3 &pos) const { r.prepare(p, pos, pos.y); }

  virtual void invalidateClipmap(bool force_redraw, bool rebuild_last_clip)
  {
    if (!clipmap)
      return;
    clipmap->invalidate(force_redraw);

    if (toroidalHeightmap)
      toroidalHeightmap->invalidate();
    if (rebuild_last_clip)
      rebuilLastClip = true;
    if (heightmapPatchesRenderer)
      heightmapPatchesRenderer->invalidate();
  }
  virtual void prepareClipmap(LandMeshRenderer &r, LandMeshManager &p, float ht_rel)
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

    int64_t reft = ref_time_ticks();
    DagorCurView savedView = ::grs_cur_view;
    LandmeshCMRenderer landmeshCMRenderer(r, p);
    landmeshCMRenderer.hRel = ht_rel;

    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    d3d::settm(TM_WORLD, TMatrix::IDENT);
    TMatrix4 gtm;
    d3d::getglobtm(gtm);
    if (::grs_draw_wire)
      d3d::setwire(0);
    ShaderGlobal::set_int_fast(render_with_normalmapVarId, 1);
    landmeshCMRenderer.startRender();
    // clipmap->prepareFeedback(landmeshCMRenderer, ::grs_cur_view.pos, gtm, ht_rel+ht_rel, 0.f);
    clipmap->finalizeFeedback();

    int w, h;
    d3d::get_target_size(w, h);
    clipmap->setTargetSize(w, h, 0.f);
    clipmap->prepareRender(landmeshCMRenderer);
    clipmap->prepareFeedback(landmeshCMRenderer, ::grs_cur_view.pos, ::grs_cur_view.itm, gtm, ht_rel + ht_rel, 0.f);

    ShaderGlobal::set_int_fast(render_with_normalmapVarId, 2);
    if (toroidalHeightmap != NULL)
    {
      // render with parallax
      toroidalHeightmap->updateHeightmap(landmeshCMRenderer, ::grs_cur_view.pos, 0.0f, 0.5f);
    }
    heightmapPatchesRenderer->render(p, ::grs_cur_view.pos, 16.0f);

    preparingClipmap = false;

    ::grs_cur_view = savedView;


    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    setFixedClipToShader(&p);
    if (::grs_draw_wire)
      d3d::setwire(1);
    if (get_time_usec(reft) > 50000)
      debug("prepareClipmap in %dus", get_time_usec(reft));
  }

  static void start_rendering_clipmap_cb()
  {
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    ::set_lmesh_rendering_mode(RENDERING_CLIPMAP);
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
    data.start_render = start_rendering_clipmap_cb;
    data.decals_cb = render_decals_cb;
    data.global_frame_id = ShaderGlobal::getBlockId("global_frame");
    data.flipCullStateId = flipCullStateId.get();
    int oldm = ShaderGlobal::get_int_fast(lmesh_rendering_mode_glob_varId);
    ShaderGlobal::enableAutoBlockChange(false);

    prepare_fixed_clip(last_clip, data, false, ::grs_cur_view.pos);
    ::set_lmesh_rendering_mode(oldm);
    ShaderGlobal::enableAutoBlockChange(true);

    if (get_time_usec(reft) > 20000)
      debug("create last clip of size %dx%d in %dus", texture_size, texture_size, get_time_usec(reft));
  }

  void setFixedClipToShader(const LandMeshManager *lmMgr) { last_clip.setVar(); }

  void closeFixedClip() { last_clip.close(); }

  virtual void renderLandMesh(LandMeshRenderer &r, LandMeshManager &p) const
  {
    if (preparingClipmap) // avoid invalid render landmesh
      return;
    DagorCurView savedView = ::grs_cur_view;
    d3d::settm(TM_WORLD, TMatrix::IDENT);
    r.render(p, r.RENDER_WITH_CLIPMAP, ::grs_cur_view.pos);
    ::grs_cur_view = savedView;
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
  }

  virtual void renderLandMeshClipmap(LandMeshRenderer &r, LandMeshManager &p) const
  {
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    DagorCurView savedView = ::grs_cur_view;
    d3d::settm(TM_WORLD, TMatrix::IDENT);
    int oldMode = set_lmesh_rendering_mode(RENDERING_CLIPMAP);
    ShaderGlobal::set_int_fast(render_with_normalmapVarId, 1);
    r.render(p, r.RENDER_CLIPMAP, ::grs_cur_view.pos);
    render_decals_cb(BBox3());
    ShaderGlobal::set_int_fast(render_with_normalmapVarId, 2);
    set_lmesh_rendering_mode(oldMode);
    ::grs_cur_view = savedView;
  }

  virtual void renderLandMeshVSM(LandMeshRenderer &r, LandMeshManager &p) const
  {
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    DagorCurView savedView = ::grs_cur_view;
    r.forceLowQuality(true);
    int oldMode = set_lmesh_rendering_mode(RENDERING_VSM);
    r.render(p, r.RENDER_ONE_SHADER, ::grs_cur_view.pos);
    r.forceLowQuality(false);
    set_lmesh_rendering_mode(oldMode);
    ::grs_cur_view = savedView;
  }
  virtual void renderLandMeshDepth(LandMeshRenderer &r, LandMeshManager &p) const
  {
    DagorCurView savedView = ::grs_cur_view;
    int oldMode = set_lmesh_rendering_mode(RENDER_DEPTH);
    r.render(p, r.RENDER_DEPTH, ::grs_cur_view.pos);
    set_lmesh_rendering_mode(oldMode);
    ::grs_cur_view = savedView;
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
  }


  virtual void renderLandMeshHeight(LandMeshRenderer &r, LandMeshManager &p) const
  {
    DagorCurView savedView = ::grs_cur_view;
    int oldMode = set_lmesh_rendering_mode(RENDERING_HEIGHTMAP);
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
    set_lmesh_rendering_mode(oldMode);
    ::grs_cur_view = savedView;
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    shaders::overrides::reset();
    r.setRenderInBBox(BBox3());
    r.setInvGeomLodDist(oldInvGeomDist);
  }

  virtual void renderDecals(LandMeshRenderer &r, LandMeshManager &p) const
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
    int omode = ::set_lmesh_rendering_mode(RENDERING_LANDMESH);
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
    r.renderDecals(p, LandMeshRenderer::RENDER_WITH_CLIPMAP, globtm, false);
    ::set_lmesh_rendering_mode(omode);

    r.setRenderInBBox(BBox3());
  }

  virtual int setGrassMaskRenderingMode()
  {
    int oldMode = set_lmesh_rendering_mode(GRASS_MASK);
    return oldMode;
  }

  virtual void restoreRenderingMode(int oldMode)
  {
    set_lmesh_rendering_mode(oldMode);
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
  }

  virtual void renderLandMeshGrassMask(LandMeshRenderer &r, LandMeshManager &p) const
  {
    DagorCurView savedView = ::grs_cur_view;
    int oldMode = set_lmesh_rendering_mode(GRASS_MASK);
    r.render(p, r.RENDER_GRASS_MASK, ::grs_cur_view.pos, LandMeshRenderer::RENDER_FOR_GRASS);
    set_lmesh_rendering_mode(oldMode);
    ::grs_cur_view = savedView;
    ShaderGlobal::setBlock(global_frame_blockid, ShaderGlobal::LAYER_FRAME);
  }

  virtual void updateGrassTranslucency(LandMeshRenderer &r, LandMeshManager &p) const
  {
    if (!grassTranslucency)
      return;

    class MyGrassTranlucencyCB : public GrassTranlucencyCB
    {
    public:
      LandMeshRenderer &r;
      LandMeshManager &p;
      int omode;
      uint32_t oldflags;

      MyGrassTranlucencyCB(LandMeshRenderer &lmr, LandMeshManager &lmp) : r(lmr), p(lmp) {}
      void start(const BBox3 &box)
      {
        if (::grs_draw_wire)
          d3d::setwire(0);
        oldflags = LandMeshRenderer::lmesh_render_flags;
        omode = set_lmesh_rendering_mode(RENDERING_CLIPMAP);
        r.setRenderInBBox(box);
      }
      void renderTranslucencyColor()
      {
        LandMeshRenderer::lmesh_render_flags &= ~(LandMeshRenderer::RENDER_DECALS | LandMeshRenderer::RENDER_COMBINED);
        set_lmesh_rendering_mode(RENDERING_CLIPMAP);
        r.render(p, LandMeshRenderer::RENDER_CLIPMAP, ::grs_cur_view.pos);
      }
      void renderTranslucencyMask()
      {
        LandMeshRenderer::lmesh_render_flags = oldflags;
        set_lmesh_rendering_mode(GRASS_MASK);
        r.render(p, LandMeshRenderer::RENDER_GRASS_MASK, ::grs_cur_view.pos);
      }
      void finish()
      {
        LandMeshRenderer::lmesh_render_flags = oldflags;
        set_lmesh_rendering_mode(omode);
        r.setRenderInBBox(BBox3());
        d3d::setwire(::grs_draw_wire);
      }
    };
    ddsx::tex_pack2_perform_delayed_data_loading();

    MyGrassTranlucencyCB cb(r, p);
    r.prepare(p, ::grs_cur_view.pos, ::grs_cur_view.pos.y);
    grassTranslucency->update(::grs_cur_view.pos, grassTranslucencyHalfSize, cb);
  }

  virtual void updatePropertiesFromLevelBlk(const DataBlock &level_blk)
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
  }

  virtual void resetTexturesLandMesh(LandMeshRenderer &r) const { r.resetTextures(); }

  virtual bool exportToGameLandMesh(mkbindump::BinDumpSaveCB &cwr, dag::ConstSpan<landmesh::Cell> cells,
    dag::ConstSpan<MaterialData> materials, int lod1TrisDensity, bool tools_internal) const
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
    bool old_no_pack = ShaderMeshData::fastNoPacking;
    if (tools_internal)
      ShaderMeshData::fastNoPacking = true;

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
        ShaderMeshData::fastNoPacking = old_no_pack;
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
        ShaderMeshData::fastNoPacking = old_no_pack;
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
    ShaderMeshData::fastNoPacking = old_no_pack;
    return retval;
  }

  virtual bool exportGeomToShaderMesh(mkbindump::BinDumpSaveCB &cwr, StaticGeometryContainer &geom, const char *tmp_lms_fn,
    const ITextureNumerator &tn) const
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

  virtual bool exportLoftMasks(const char *fn_mask, const char *fn_id, const char *fn_ht, const char *fn_dir, int tex_sz,
    const Point3 &w0, const Point3 &w1, GeomObject *obj)
  {
    Texture *texM = d3d::create_tex(NULL, tex_sz, tex_sz, TEXFMT_L8 | TEXCF_RTARGET, 1);
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
    texD->texfilter(TEXFILTER_POINT);

    {
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

  virtual void prepare(LandMeshRenderer &r, LandMeshManager &p, const Point3 &center_pos, const BBox3 &box) override
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


static GenericHeightMapService srv;

bool GenericHeightMapService::renderHm2ToFeedback()
{
  if (srv.hm2.texMainId == BAD_TEXTUREID && srv.hm2.texDetId == BAD_TEXTUREID)
    return false;
  srv.renderHm2(grs_cur_view.pos, false, false);
  return true;
}

void *get_generic_hmap_service()
{
  static bool inited = false;
  if (!inited)
  {
    srv.init();
    inited = true;
  }
  return &srv;
}
