#include <shaders/dag_renderScene.h>
#include <shaders/dag_shaderMeshTexLoadCtrl.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_drv3d.h>
#include <image/dag_tga.h>
#include <image/dag_texPixel.h>
#include <generic/dag_ptrTab.h>
#include <ioSys/dag_baseIo.h>
#include <ioSys/dag_zlibIo.h>
#include <workCycle/dag_gameSettings.h>
#include <util/dag_loadingProgress.h>
#include <osApiWrappers/dag_files.h>
#include "sceneBinaryData.h"
#include <debug/dag_log.h>
// #include <debug/dag_debug.h>


#define DUMP_GEOM_STATS 0


void RenderScene::loadBinary(IGenLoad &crd, dag::ConstSpan<TEXTUREID> texMap, bool use_vis)
{
  using namespace renderscenebindump;

  ShaderMaterial::setLoadingString("binary-dump");

// int startUpTime=get_time_msec();
// int startUpTime0=startUpTime;
#define STARTUP_POINT(name)                                                                        \
  {                                                                                                \
    /*int t=get_time_msec();*/                                                                     \
    /*debug("*** %7dms: load scene %s (line %d)", t-startUpTime, (const char*)(name), __LINE__);*/ \
    /*startUpTime=t;*/                                                                             \
    loading_progress_point();                                                                      \
  }

  int i;

  loading_progress_point();

  DAGOR_TRY
  {

    class LoadCB : public IBaseLoad
    {
    public:
      IGenLoad &crd;
      const char *fn;
      LoadCB(IGenLoad &_crd, const char *_fn) : crd(_crd), fn(_fn) {}

      void read(void *p, int l) { crd.read(p, l); }
      int tryRead(void *p, int l) { return crd.tryRead(p, l); }
      int tell() { return crd.tell(); }
      void seekto(int o) { return crd.seekto(o); }
      void seekrel(int o) { return crd.seekrel(o); }
      virtual const char *getTargetName() { return crd.getTargetName(); }
    };

    LoadCB cb(crd, "binary-dump");

    loading_progress_point();
    SceneHdr sHdr;

    cb.read(&sHdr, sizeof(sHdr));
    if (sHdr.version != _MAKE4C('scn2'))
      DAGOR_THROW(IGenLoad::LoadException("label scn2 not found", crd.tell()));
    if (sHdr.rev < 0x20130524)
      DAGOR_THROW(IGenLoad::LoadException("old scene revision, must be >=2013/05/24", crd.tell()));

    if (sHdr.ltmapFormat == _MAKE4C('SB2'))
    {
      // cb.read ( &sunColor, sizeof(sunColor));
      // cb.read ( &sunDirection, sizeof(sunDirection));
      G_ASSERT(0);
    }
    int restDataSize;
    cb.readInt(restDataSize);

    dagor_set_sm_tex_load_ctx_type('RSCN');
    dagor_set_sm_tex_load_ctx_name(crd.getTargetName());
    smvd = ShaderMatVdata::create(sHdr.texNum, sHdr.matNum, sHdr.vdataNum, sHdr.mvhdrSz);
    smvd->loadTexIdx(cb, texMap);
    STARTUP_POINT(String(100, "textures %d", sHdr.texNum));

    G_ASSERT(sHdr.ltmapNum == 0);

    // read lightmaps catalog (offsets to data)
    smvd->loadMatVdata(String(200, "%s ldBin", crd.getTargetName()).str(), cb, 0);
    dagor_reset_sm_tex_load_ctx();

    STARTUP_POINT(String(100, "mats %d, vdata %d", sHdr.matNum, sHdr.vdataNum));

    loading_progress_point();

    robjMem = memalloc(sHdr.objDataSize, midmem);
    // debug("sHdr.objDataSize=%d sHdr.objNum=%d robjMem=%p ofs=%p",
    //   sHdr.objDataSize, sHdr.objNum, robjMem, cb.tell());
    ZlibLoadCB z_crd(cb, cb.getBlockLevel() ? cb.getBlockRest() : 0x7FFFFFFF);

    z_crd.read(robjMem, sHdr.objDataSize);
    memcpy(&obj, robjMem, sizeof(obj));
    obj.patch(robjMem);

    for (i = 0; i < obj.size(); ++i)
    {
      RenderObject &o = obj[i];

      // patch RenderObject data
      o.lods.patch(robjMem);
      if (o.name.toInt() == -1)
        o.name = NULL;
      else
        o.name.patch(robjMem);

      // create shader meshes from dump
      for (int li = 0; li < o.lods.size(); ++li)
      {
        int sz = o.lods[li].mesh.toInt();
        void *smmem = memalloc(sz, midmem);
        z_crd.read(smmem, sz);
        o.lods[li].mesh = (ShaderMesh *)smmem;
        o.lods[li].mesh->patchData(smmem, *smvd);
        o.lods[li].mesh->acquireTexRefs();
      }

      if (!(i & 15))
        loading_progress_point();

      o.update();

      // when not using portals of visibility, also disable range check (useful for envi)
      if (!use_vis)
        o.visrange = -1;
    }
    SmallTab<int, MidmemAlloc> fadeObjInds;
    clear_and_resize(fadeObjInds, sHdr.fadeObjNum);
    z_crd.readTabData(fadeObjInds);

    // Load lighting objects data.
    z_crd.close();

    STARTUP_POINT(String(100, "load objs (%d)", obj.size()));

    smvd->preloadTex();

#if DUMP_GEOM_STATS
    file_ptr_t geomDumpFile = df_open("sceneGeomStats.csv", DF_RDWR);
    if (!geomDumpFile)
    {
      geomDumpFile = df_open("sceneGeomStats.csv", DF_RDWR | DF_CREATE);
      df_printf(geomDumpFile, "vdata;idata;numv;numf;stride;parts;vdesc;objs;vLost;fLost\n");
    }
    else
    {
      df_seek_end(geomDumpFile, 0);
      df_printf(geomDumpFile, "\n");
    }

    for (i = 0; i < smvd->getGlobVDataCount(); ++i)
    {
      GlobalVertexData &vd = *smvd->getGlobVData(i);
      int vb_size = vd.getVB() ? vd.getVB()->ressize() : 0;
      int ib_size = vd.getIB() ? vd.getIB()->ressize() : 0;
      df_printf(geomDumpFile, "%d;%d;%d;%d;%d;%d;", vb_size, ib_size, vb_size ? vb_size / vd.getStride() : 0,
        ib_size ? ib_size / ((vd.getIB()->getFlags() & SBCF_INDEX32) ? 4 : 2) / 3 : 0, vd.getStride(), 1);

      df_printf(geomDumpFile, ";");

      int vused = 0, fused = 0;
      for (int oi = 0; oi < obj.size(); ++oi)
      {
        RenderObject &o = obj[oi];
        bool ok = false;
        for (int li = 0; li < o.lods.size(); ++li)
        {
          ShaderMesh *m = o.lods[li].mesh;
          if (!m)
            continue;

          for (int ei = 0; ei < m->getAllElems().size(); ++ei)
          {
            ShaderMesh::RElem &e = m->getAllElems()[ei];
            if (e.vertexData != &vd)
              continue;

            vused += e.numv * vd.getStride();
            fused += e.numf * 3 * 2;
            ok = true;
          }
        }

        if (ok)
          df_printf(geomDumpFile, "%s  ", o.name);
      }

      df_printf(geomDumpFile, ";%d;%d", vb_size - vused, ib_size - fused);

      df_printf(geomDumpFile, "\n");
    }

    df_close(geomDumpFile);


    // objects
    geomDumpFile = df_open("sceneObjStats.csv", DF_RDWR);
    if (!geomDumpFile)
    {
      geomDumpFile = df_open("sceneObjStats.csv", DF_RDWR | DF_CREATE);
      df_printf(geomDumpFile, "name;vdata;idata;numv;numf\n");
    }
    else
    {
      df_seek_end(geomDumpFile, 0);
      df_printf(geomDumpFile, "\n");
    }

    for (int oi = 0; oi < obj.size(); ++oi)
    {
      RenderObject &o = obj[oi];
      int vdata = 0, idata = 0, numv = 0, numf = 0;

      for (int li = 0; li < o.lods.size(); ++li)
      {
        ShaderMesh *m = o.lods[li].mesh;
        if (!m)
          continue;

        for (int ei = 0; ei < m->getAllElems().size(); ++ei)
        {
          ShaderMesh::RElem &e = m->getAllElems()[ei];
          numv += e.numv;
          numf += e.numf;
          vdata += e.numv * e.vertexData->getStride();
          idata += e.numf * 3 * 2;
        }
      }

      df_printf(geomDumpFile, "%s;%d;%d;%d;%d\n", o.name, vdata, idata, numv, numf);
    }

    df_close(geomDumpFile);
#endif // DUMP_GEOM_STATS
  }
  DAGOR_CATCH(IGenLoad::LoadException e)
  {
#if _TARGET_PC && !_TARGET_STATIC_LIB
    logwarn("can't load binary scene: %s, %s", e.excDesc, DAGOR_EXC_STACK_STR(e));
#elif defined(DAGOR_EXCEPTIONS_ENABLED)
    fatal("can't load binary scene: %s, %s", e.excDesc, DAGOR_EXC_STACK_STR(e));
#else
    fatal("can't load binary scene");
#endif
  }


  STARTUP_POINT("closed");

  loading_progress_point();

  ShaderMaterial::setLoadingString(NULL);
  if (obj.empty())
    debug_ctx("empty RenderScene loaded");
  if (RenderScene::should_build_optscene_data && obj.size())
    buildOptSceneData(); //== temporary, until new layout will be built by daEditorX

  // debug("    %7dms total load binary scene", get_time_msec()-startUpTime0);
}
