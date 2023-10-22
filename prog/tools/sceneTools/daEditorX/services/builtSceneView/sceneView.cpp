#include <de3_interface.h>
#include <de3_entityFilter.h>
#include <de3_objEntity.h>
#include <de3_editorEvents.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <oldEditor/de_util.h>

#include <libTools/util/de_TextureName.h>
#include <libTools/util/makeBindump.h>
#include <libTools/dagFileRW/textureNameResolver.h>
#include <util/dag_loadingProgress.h>

#include <render/waterObjects.h>

#include <fx/dag_hdrRender.h>
#include <shaders/dag_renderScene.h>
#include <3d/dag_drv3d.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>
#include <osApiWrappers/dag_direct.h>


class BuiltSceneViewService : public IEditorService, public IRenderingService, public IRenderOnCubeTex, public IBinaryDataBuilder
{
public:
  BuiltSceneViewService() : visible(false), wasLoaded(false), scene(NULL), oldViewPos(0, 1000, 0)
  {
    fakeLtGvId = get_shader_glob_var_id("fake_lighting_computations", true);
  }
  ~BuiltSceneViewService()
  {
    waterobjects::del(scene);
    del_it(scene);
    fakeLtGvId = -1;
  }

  // IEditorService interface
  virtual const char *getServiceName() const { return "_builtSceneView"; }
  virtual const char *getServiceFriendlyName() const { return "(srv) Built scene view"; }

  virtual void setServiceVisible(bool vis)
  {
    visible = vis;
    if (visible)
      loadScene();
  }
  virtual bool getServiceVisible() const { return visible; }

  virtual void actService(float dt) {}
  virtual void beforeRenderService()
  {
    if (scene && lengthSq(oldViewPos - ::grs_cur_view.pos) > 200)
    {
      scene->sort_objects(::grs_cur_view.pos);
    }
  }
  virtual void renderService() {}
  virtual void renderTransService() {}
  virtual void onBeforeReset3dDevice() {}
  virtual void clearServiceData()
  {
    waterobjects::del(scene);
    del_it(scene);
    wasLoaded = false;
  }

  virtual bool catchEvent(unsigned ev_huid, void *userData)
  {
    switch (ev_huid)
    {
      case HUID_ReloadBuiltScene:
        waterobjects::del(scene);
        del_it(scene);
        wasLoaded = false;

        if (visible)
          loadScene();
        break;
    }
    return false;
  }

  virtual void *queryInterfacePtr(unsigned huid)
  {
    RETURN_INTERFACE(huid, IRenderingService);
    RETURN_INTERFACE(huid, IRenderOnCubeTex);
    RETURN_INTERFACE(huid, IBinaryDataBuilder);
    return NULL;
  }

  // IRenderingService interface
  virtual void renderGeometry(Stage stage)
  {
    static int inEditorGvId = ::get_shader_glob_var_id("in_editor");
    int prev = inEditorGvId >= 0 ? ShaderGlobal::get_int_fast(inEditorGvId) : 0;
    if (inEditorGvId >= 0)
      ShaderGlobal::set_int_fast(inEditorGvId, 0);

    switch (stage)
    {
      case STG_RENDER_STATIC_OPAQUE: renderScene(0, 0xFFFFFFFF); break;

      case STG_RENDER_STATIC_TRANS: renderSceneTrans(); break;

      case STG_RENDER_SHADOWS: renderScene(1, RenderScene::RenderObject::ROF_CastShadows); break;
    }

    if (inEditorGvId >= 0)
      ShaderGlobal::set_int_fast(inEditorGvId, prev);
  }

  // IRenderOnCubeTex interface
  void prepareCubeTex(bool renderEnvi, bool renderLit, bool renderStreamLit)
  {
    if (!renderLit)
      return;
    renderScene(0, 0xFFFFFFFF);
    renderSceneTrans();
  }

  // IBinaryDataBuilder interface
  virtual bool validateBuild(int target, ILogWriter &rep, PropPanel2 *params)
  {
    String sceneDir;
    makeScenePath(sceneDir, "");

    const bool have = check_built_scene_data_exist(sceneDir, target);

    if (!have)
      rep.addMessage(ILogWriter::ERROR, "Scene not built yet? \"%s/scene-*.scn\" not found.", sceneDir);
    else
    {
      if (have && !check_built_scene_data(sceneDir, target))
        rep.addMessage(ILogWriter::ERROR, "Scene \"%s/scene-*.scn\" is in WRONG format. Rebuild required.", sceneDir);
    }

    return true;
  }
  virtual bool addUsedTextures(ITextureNumerator &tn)
  {
    String sceneDir;
    makeScenePath(sceneDir, "");

    ::add_built_scene_textures(sceneDir, tn);

    return true;
  }
  virtual bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel2 *pp)
  {
    String sceneDir;
    makeScenePath(sceneDir, "");

    if (check_built_scene_data_exist(sceneDir, cwr.getTarget()))
    {
      cwr.beginTaggedBlock(_MAKE4C('SCN'));
      store_built_scene_data(sceneDir, cwr, tn);
      cwr.align8();
      cwr.endBlock();

      loadScene();
    }

    return true;
  }
  virtual bool checkMetrics(const DataBlock &metrics_blk)
  {
    String scenePath;
    makeScenePath(scenePath, "/scene-PC.scn");
    String sceneDir;
    makeScenePath(sceneDir, "");

    const int maxFaces = metrics_blk.getInt("max_faces", 0);

    return ::check_scene_vertex_count(scenePath, sceneDir, maxFaces);
  }

protected:
  bool visible, wasLoaded;

  RenderScene *scene;
  Point3 oldViewPos;

  void renderScene(int render_id, unsigned mask)
  {
    if (!scene)
      return;

    ShaderGlobal::set_int_fast(fakeLtGvId, 0);
    d3d::settm(TM_WORLD, TMatrix::IDENT);
    scene->render(render_id, mask);

    ShaderGlobal::set_int_fast(fakeLtGvId, 1);
  }
  void renderSceneTrans()
  {
    if (!scene)
      return;

    ShaderGlobal::set_int_fast(fakeLtGvId, 0);
    d3d::settm(TM_WORLD, TMatrix::IDENT);
    scene->render_trans();

    ShaderGlobal::set_int_fast(fakeLtGvId, 1);
  }

  void loadScene()
  {
    if (wasLoaded)
      return;

    bool loaded = false;

    String scenePath;
    makeScenePath(scenePath, "/scene-PC.scn");

    if (dd_file_exist(scenePath))
    {
      DAEDITOR3.conNote("Loading built scene...");
      DAEDITOR3.setFatalHandler(true);
      waterobjects::del(scene);
      del_it(scene);

      RenderScene *newScene = new RenderScene;

      ::location_from_path(scenePath);

      loadRscene(*newScene, scenePath + "/scene-PC.scn");

      loaded = !DAEDITOR3.getFatalStatus();
      if (!loaded)
        del_it(newScene);
      scene = newScene;

      initWaterReflectionObjects();

      DAEDITOR3.popFatalHandler();
    }

    wasLoaded = loaded;
    oldViewPos = Point3(0, 1000, 0);
  }

  static void loadRscene(RenderScene &scn, const char *fn)
  {
    FullFileLoadCB crd(fn);
    if (!crd.fileHandle)
    {
      fatal("can't open scene file '%s'", fn);
      return;
    }

    if (crd.readInt() != _MAKE4C('scnf'))
    {
      fatal("not scene file '%s'", fn);
      return;
    }
    Tab<TEXTUREID> texMap(tmpmem);
    char namebuf[256];

    crd.beginBlock();

    texMap.resize(crd.readInt());
    for (int i = 0; i < texMap.size(); i++)
    {
      int l = 0;
      crd.read(&l, 1);
      crd.read(namebuf, l);
      namebuf[l] = 0;

      if (!l)
        texMap[i] = BAD_TEXTUREID;
      else
      {
        String name;
        if (get_global_tex_name_resolver()->resolveTextureName(namebuf, name))
          texMap[i] = add_managed_texture(name);
        else
        {
          texMap[i] = BAD_TEXTUREID;
          DAEDITOR3.conError("cannot resolve tex: %s", namebuf);
        }
      }
    }
    crd.endBlock();

    scn.loadBinary(crd, texMap, true);
  }

  void makeScenePath(String &s, const char *add_path)
  {
    DAGORED2->getProjectFolderPath(s);
    s.aprintf(64, "/builtScene%s", add_path);
  }

  void initWaterReflectionObjects() {}

  int fakeLtGvId;
};


void init_built_scene_view_service()
{
  if (!IDaEditor3Engine::get().checkVersion())
    return;

  IDaEditor3Engine::get().registerService(new (inimem) BuiltSceneViewService);
}

IEditorService *create_built_scene_view_service() { return new (inimem) BuiltSceneViewService; }
