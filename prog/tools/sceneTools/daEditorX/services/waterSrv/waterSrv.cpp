// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_waterSrv.h>
#include <de3_interface.h>
#include "../../de_appwnd.h"

#include <math/dag_mathUtils.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <debug/dag_debug.h>
#include <render/dag_cur_view.h>
#include <3d/dag_render.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderBlock.h>
#include <oldEditor/de_util.h>
#include <EditorCore/ec_IEditorCore.h>
#include <EditorCore/ec_wndGlobal.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <de3_hmapStorage.h>

#include <fftWater/fftWater.h>
#include <3d/dag_textureIDHolder.h>
#include <debug/dag_debug3d.h>
#include <perfMon/dag_cpuFreq.h>

#include <libTools/util/makeBindump.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>

using editorcore_extapi::dagGeom;

static int water_refraction_texVarId = -1, prev_frame_texVarId = -1;
const int heightmap_w = 2048, shore_w = 1024;
class WaterService : public IWaterService
{
public:
  FFTWater *water = nullptr;
  UniqueTexHolder distTex;
  TextureIDHolder heightmap;
  TextureIDHolder wfx_normals, wfx_details;
  float waterLevel = 0.f;
  double totalTime = 0.0;
  bool srvDisabled = false;
  bool noDistanceField = false;

  bool initSrv()
  {
    if (!VariableMap::isVariablePresent(get_shader_variable_id("fft_source_texture_no", true)))
    {
      DAEDITOR3.conError("WaterService disabled: var '%s' not found", "fft_source_texture_no");
      return false;
    }

    Ptr<ShaderMaterial> mat1 = new_shader_material_by_name_optional("water_nv2");
    Ptr<ShaderMaterial> mat2 = new_shader_material_by_name_optional("water3d_compatibility");
    if (!mat1.get() && !mat2.get())
    {
      DAEDITOR3.conError("WaterService disabled: shaders '%s' and '%s' not found", "water_nv2", "water3d_compatibility");
      return false;
    }
    else if (!mat1.get())
      DAEDITOR3.conWarning("WaterService: shader '%s' not found", "water_nv2");
    else if (!mat2.get())
      DAEDITOR3.conWarning("WaterService: shader '%s' not found", "water3d_compatibility");

    mat1 = new_shader_material_by_name("water_distance_field");
    if (!mat1.get())
    {
      noDistanceField = true;
      DAEDITOR3.conWarning("WaterService: shader '%s' not found", "water_distance_field");
    }

    init();
    return true;
  }

  void loadSettings(const DataBlock &w3dSettings) override
  {
    const char *foamTexName = w3dSettings.getStr("water_foam", "water_surface_foam_tex");
    TEXTUREID waterFoamTexId = ::get_tex_gameres(foamTexName);
    // G_ASSERTF(waterFoamTexId != BAD_TEXTUREID, "water foam texture '%s' not found.", foamTexName);
    ShaderGlobal::set_texture(get_shader_variable_id("foam_tex", true), waterFoamTexId);
    ShaderGlobal::set_sampler(get_shader_variable_id("foam_tex_samplerstate", true), get_texture_separate_sampler(waterFoamTexId));

    const char *perlinName = w3dSettings.getStr("perlin_noise", "water_perlin");
    TEXTUREID perlinTexId = ::get_tex_gameres(perlinName);
    // G_ASSERTF(perlinTexId != BAD_TEXTUREID, "water perlin noise texture '%s' not found.", perlinName);
    ShaderGlobal::set_texture(get_shader_variable_id("perlin_noise"), perlinTexId);
    ShaderGlobal::set_sampler(get_shader_variable_id("perlin_noise_samplerstate"), get_texture_separate_sampler(perlinTexId));

    release_managed_tex(waterFoamTexId);
    release_managed_tex(perlinTexId);
  }
  ~WaterService() { termSrv(); }

  void act(float dt) override { totalTime += dt; }
  void init() override
  {
    fft_water::init();
    water_refraction_texVarId = get_shader_variable_id("water_refraction_tex", true);
    prev_frame_texVarId = get_shader_variable_id("prev_frame_tex", true);
    water = fft_water::create_water(fft_water::RENDER_GOOD);
    if (!heightmap.getTex())
    {
      heightmap.set(d3d::create_tex(NULL, heightmap_w, heightmap_w, TEXCF_RTARGET | TEXFMT_R16F, 1, "water_heightmap_tex"),
        "water_heightmap_tex");
    }
    if (!wfx_normals.getTex())
    {
      wfx_normals.set(d3d::create_tex(NULL, 1, 1, TEXCF_RTARGET | TEXFMT_A8R8G8B8, 1, "wfx_normals"), "wfx_normals");
      d3d::clear_rt({wfx_normals.getTex2D()}, make_clear_value(0.5f, 0.5f, 0.0f, 0.0f));
      ShaderGlobal::set_texture(get_shader_variable_id("wfx_normals", true), wfx_normals.getId());
    }
    if (!wfx_details.getTex())
    {
      wfx_details.set(d3d::create_tex(NULL, 1, 1, TEXCF_RTARGET | TEXFMT_A8R8G8B8, 1, "wfx_details"), "wfx_details");
      d3d::clear_rt({wfx_details.getTex2D()}, make_clear_value(0.0f, 1.0f, 1.0f, 0.0f));
      ShaderGlobal::set_texture(get_shader_variable_id("wfx_details", true), wfx_details.getId());
    }
  }
  void term() override
  {
    if (water)
    {
      fft_water::delete_water(water);
      water = NULL;
    }
  }

  void termSrv()
  {
    term();
    fft_water::close();
    distTex.close();
    heightmap.close();
    wfx_details.close();
    wfx_normals.close();
  }

  void beforeRender(Stage stage) override
  {
    if (!water)
      return;
    fft_water::set_level(water, waterLevel);
    fft_water::simulate(water, totalTime);
    fft_water::before_render(water);
    static int foam_time_id = get_shader_glob_var_id("foam_time", true);
    ShaderGlobal::set_real(foam_time_id, max(0.f, (float)get_time_msec() / 1000.0f));
  }
  void renderGeometry(Stage stage) override
  {
    if (!water)
      return;
    TEXTUREID prev = ShaderGlobal::get_tex_fast(prev_frame_texVarId);
    ShaderGlobal::set_texture(water_refraction_texVarId, prev);
    ShaderGlobal::set_sampler(get_shader_variable_id("water_refraction_tex_samplerstate", true),
      ShaderGlobal::get_sampler(get_shader_variable_id("prev_frame_tex_samplerstate", true)));
    TMatrix4 globtm;
    d3d::getglobtm(globtm);
    Driver3dPerspective persp;
    if (!d3d::getpersp(persp))
      persp.wk = persp.hk = 1;
    fft_water::render(water, ::grs_cur_view.pos, distTex.getTexId(), globtm, persp);
  }
  void set_level(float level) override
  {
    if (water)
      fft_water::set_level(water, waterLevel = level);
  }
  float get_level() const override { return waterLevel; }
  void hide_water() override
  {
    if (water)
      fft_water::set_level(water, -3000);
  }
  void set_wind(float b_scale, const Point2 &wind_dir) override
  {
    if (water)
      fft_water::set_wind(water, b_scale, wind_dir);
  }
  void set_render_quad(const BBox2 &quad) override
  {
    if (water)
      fft_water::set_render_quad(water, quad);
  }
  void buildDistanceField(const Point2 &hmap_center, float hmap_rad, float rivers_size) override
  {
    if (noDistanceField)
      return;

    static int max_river_widthVarId = get_shader_variable_id("max_river_width", true);
    if (rivers_size < 0)
      rivers_size = max_river_widthVarId >= 0 ? ShaderGlobal::get_real_fast(max_river_widthVarId) : 250;

    float dimensions = 30;

    const fft_water::WaterHeightmap *whm = fft_water::get_heightmap(water);
    Color4 shoreHeightmapVar;
    if (whm)
      shoreHeightmapVar = Color4(1.0 / whm->heightScale, -whm->heightOffset / whm->heightScale, whm->heightScale, whm->heightOffset);
    else
      shoreHeightmapVar =
        Color4(1.f / dimensions, -(waterLevel - 0.5 * dimensions) / dimensions, dimensions, waterLevel - 0.5 * dimensions);
    ShaderGlobal::set_color4(get_shader_variable_id("heightmap_min_max", true), shoreHeightmapVar);

    ShaderGlobal::set_color4(get_shader_variable_id("shore_heightmap_min_max", true), shoreHeightmapVar);

    static int shore_waves_on_gvid = get_shader_variable_id("shore_waves_on", true);
    ShaderGlobal::set_int(shore_waves_on_gvid, 0);

    DagorCurView savedView = ::grs_cur_view;
    TMatrix4 savedViewMatrix;
    d3d::gettm(TM_VIEW, &savedViewMatrix);

    Driver3dPerspective p;
    bool persp = d3d::getpersp(p);
    TMatrix4 savedProjMatrix;
    d3d::gettm(TM_PROJ, &savedProjMatrix);

    Driver3dRenderTarget prevRt;
    d3d::get_render_target(prevRt);

    d3d::set_render_target(heightmap.getTex2D(), 0);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    renderHeightmap(hmap_rad, hmap_center);
    // save_tex_as_ddsx(heightmap.getTex2D(), String(0, "d:/hmap-%04d.ddsx", get_time_msec()));

    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
    smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
    ShaderGlobal::set_sampler(get_shader_variable_id("heightmap_tex_samplerstate", true), d3d::request_sampler(smpInfo));
    ShaderGlobal::set_texture(get_shader_variable_id("heightmap_tex", true), heightmap.getId());
    fft_water::build_distance_field(distTex, shore_w, heightmap_w, rivers_size, NULL);
    // save_tex_as_ddsx(distTex.getTex2D(), String(0, "d:/distTex-%04d.ddsx", get_time_msec()));

    // restore
    d3d::set_render_target(prevRt);
    ::grs_cur_view = savedView;
    d3d::settm(TM_VIEW, ::grs_cur_view.tm);
    if (persp)
      d3d::setpersp(p);
    else
      d3d::settm(TM_PROJ, &savedProjMatrix);

    distTex.setVar();
    ShaderGlobal::set_int(shore_waves_on_gvid, 1);
  }
  void afterD3DReset(bool full_reset) override
  {
    if (!srvDisabled && water)
      fft_water::reset_render(water);
  }

  void renderHeightmap(float heightmap_size, const Point2 &center_pos)
  {
    static float MIN_HEIGHT = -5000.f;
    static float MAX_HEIGHT = 5000.f;
    static int globalFrameBlockId = ShaderGlobal::getBlockId("global_frame");
    static int worldViewPosVarId = ::get_shader_variable_id("world_view_pos");

    Point3 origin = Point3(floor(center_pos.x / 16 + 0.5) * 16, 0, floor(center_pos.y / 16 + 0.5) * 16);
    BBox3 levelBox =
      BBox3(origin - Point3(heightmap_size, 3000, heightmap_size), origin + Point3(heightmap_size, 3000, heightmap_size));

    TMatrix4 viewMatrix = ::matrix_look_at_lh(Point3(levelBox.center().x, MAX_HEIGHT, levelBox.center().z),
      Point3(levelBox.center().x, 0.f, levelBox.center().z), Point3(0.f, 0.f, 1.f));


    TMatrix4 projectionMatrix = matrix_ortho_lh(levelBox.width().x, -levelBox.width().z, 0, MAX_HEIGHT - MIN_HEIGHT);

    Color4 world_to_heightmap(1.f / levelBox.width().x, 1.f / levelBox.width().z, -levelBox[0].x / levelBox.width().x,
      -levelBox[0].z / levelBox.width().z);
    ShaderGlobal::set_color4(get_shader_variable_id("world_to_heightmap", true), world_to_heightmap);

    d3d::settm(TM_VIEW, &viewMatrix);
    ::grs_cur_view.tm = tmatrix(viewMatrix);
    ::grs_cur_view.itm = inverse(::grs_cur_view.tm);
    ::grs_cur_view.pos = Point3(center_pos.x, MAX_HEIGHT, center_pos.y);
    d3d::settm(TM_PROJ, &projectionMatrix);

    ShaderGlobal::set_color4_fast(worldViewPosVarId, ::grs_cur_view.pos.x, ::grs_cur_view.pos.y, ::grs_cur_view.pos.z, 1.f);

    setZTransformPersp(0.5, MAX_HEIGHT - MIN_HEIGHT); // fixme should set ortho

    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

    for (int i = 0, ie = IEditorCoreEngine::get()->getPluginCount(); i < ie; ++i)
    {
      IGenEditorPluginBase *p = EDITORCORE->getPluginBase(i);
      if (p->getVisible())
        if (IRenderingService *iface = p->queryInterface<IRenderingService>())
          iface->renderGeometry(IRenderingService::STG_RENDER_HEIGHT_FIELD);
    }

    ShaderGlobal::set_color4_fast(worldViewPosVarId, ::grs_cur_view.pos.x, ::grs_cur_view.pos.y, ::grs_cur_view.pos.z, 1.f);

    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
  }
  void setZTransformPersp(float zn, float zf)
  {
    static int znZfarVarId = dagGeom->getShaderVariableId("zn_zfar");
    static int transformZVarId = dagGeom->getShaderVariableId("transform_z");
    float q = zf / (zf - zn);
    ShaderGlobal::set_color4_fast(transformZVarId, Color4(q, 1, -zn * q, 0));
    ShaderGlobal::set_color4_fast(znZfarVarId, Color4(zn, zf, 0, 0));
  }
  void setHeightmap(HeightMapStorage &waterHmapDet, HeightMapStorage &waterHmapMain, const Point2 &waterHeightOfsScaleDet,
    const Point2 &waterHeightOfsScaleMain, BBox2 hmapDetRect, BBox2 hmapRectMain) override
  {
    const int MAX_PAGES_TEX_WIDTH = 4096;
    const int HEIGHTMAP_PAGE_SIZE = fft_water::WaterHeightmap::HEIGHTMAP_PAGE_SIZE;
    const int PAGE_SIZE_PADDED = fft_water::WaterHeightmap::PAGE_SIZE_PADDED;
    const int PATCHES_GRID_SIZE = fft_water::WaterHeightmap::PATCHES_GRID_SIZE;

    IPoint2 detMapSize(waterHmapDet.getMapSizeX(), waterHmapDet.getMapSizeY());
    IPoint2 mainMapSize(waterHmapMain.getMapSizeX(), waterHmapMain.getMapSizeY());

    bool noDet = false;
    if (detMapSize.x <= 1)
      noDet = true;

    Point2 detRectSize = hmapDetRect.width();
    Point2 mainRectSize = hmapRectMain.width();
    Point2 mainRectOffset = hmapRectMain[0];

    Point2 mainTcScale(mainMapSize.x / mainRectSize.x, mainMapSize.y / mainRectSize.y);
    Point2 detTcScale(detMapSize.x / detRectSize.x, detMapSize.y / detRectSize.y);

    if (mainMapSize.x <= 1)
    {
      if (noDet)
      {
        fft_water::remove_heightmap(water);
        fft_water::reset_render(water);
        fft_water::reset_physics(water);
        return;
      }
      mainRectSize = detRectSize;
      mainMapSize = detMapSize;
      mainRectOffset = hmapDetRect[0];
      mainTcScale = detTcScale;
    }

    int scale = noDet ? 1 : mainRectSize.x / detRectSize.x;
    int detPageCount = noDet ? 0 : detMapSize.x / HEIGHTMAP_PAGE_SIZE;
    int mainPageCount = mainMapSize.x / HEIGHTMAP_PAGE_SIZE;
    int gridSize = mainPageCount * scale;
    int gridCellSize = mainMapSize.x / gridSize;
    int maxPageCount = max(detPageCount, mainPageCount);

    eastl::vector<uint64_t> tempGrid;
    tempGrid.resize(gridSize * gridSize);
    eastl::map<uint64_t, uint16_t> pages;

    float minHeight = FLT_MAX, maxHeight = -FLT_MAX;

    int patchSize = mainMapSize.x / PATCHES_GRID_SIZE;
    eastl::vector<Point2> patchHeights;
    patchHeights.resize(PATCHES_GRID_SIZE * PATCHES_GRID_SIZE, Point2(FLT_MAX, -FLT_MAX));

    for (int y = 0; y < mainMapSize.y; y++)
      for (int x = 0; x < mainMapSize.x; x++)
      {
        float height = 0.0f;

        Point2 worldPos = Point2(x / mainTcScale.x, y / mainTcScale.y) + mainRectOffset;
        Point2 detPos = worldPos - hmapDetRect[0];
        detPos = Point2(detPos.x * detTcScale.x, detPos.y * detTcScale.y);

        IPoint2 offset(x / HEIGHTMAP_PAGE_SIZE, y / HEIGHTMAP_PAGE_SIZE);
        int det = 0;

        if (!noDet && detPos.x >= 0 && detPos.y >= 0 && detPos.x < detMapSize.x && detPos.y < detMapSize.y)
        {
          int detX = int(detPos.x);
          int detY = int(detPos.y);
          height = waterHmapDet.getInitialMap().getData(detX, detY) * waterHeightOfsScaleDet.y + waterHeightOfsScaleDet.x;
          offset = IPoint2(detX / HEIGHTMAP_PAGE_SIZE, detY / HEIGHTMAP_PAGE_SIZE);
          det = 1;
        }
        else
        {
          height = waterHmapMain.getInitialMap().getData(x, y) * waterHeightOfsScaleMain.y + waterHeightOfsScaleMain.x;
        }

        minHeight = min(height, minHeight);
        maxHeight = max(height, maxHeight);
        if (height <= waterLevel)
          continue;
        Point2 &patchHeight = patchHeights[(y / patchSize) * PATCHES_GRID_SIZE + x / patchSize];
        patchHeight.x = min(patchHeight.x, height);
        patchHeight.y = max(patchHeight.y, height);
        IPoint2 cellPos(x / gridCellSize, y / gridCellSize);
        uint64_t &gridData = tempGrid[cellPos.y * gridSize + cellPos.x];
        uint64_t value = ((maxPageCount * offset.y + offset.x) << 2) | (1 << det);
        if ((gridData & 3u) == 0)
        {
          gridData = value;
          pages.insert(value);
        }
        else
        {
          G_ASSERT(gridData == value);
        }
      }

    eastl::unique_ptr<fft_water::WaterHeightmap> waterHeightmap(new fft_water::WaterHeightmap());
    float range = maxHeight - minHeight;
    waterHeightmap->heightOffset = minHeight;
    waterHeightmap->heightScale = range;

    if (!pages.size())
    {
      DAGORED2->getConsole().addMessage(ILogWriter::WARNING, "skip creating empty waterHeightmap: pages.size()=%d", pages.size());
      return;
    }
    int pagesX = min(max(1, (int)pages.size()), MAX_PAGES_TEX_WIDTH / PAGE_SIZE_PADDED);
    int texWidth = pagesX * PAGE_SIZE_PADDED;
    int pagesY = max<int>((pages.size() + pagesX - 1) / pagesX, 1);
    if (!pagesX || !pagesY)
    {
      DAGORED2->getConsole().addMessage(ILogWriter::WARNING, "skip creating null-sized waterHeightmap: %dx%d", pagesX, pagesY);
      return;
    }
    waterHeightmap->pagesX = pagesX;
    waterHeightmap->pagesY = pagesY;
    waterHeightmap->scale = scale;
    waterHeightmap->pages.resize(pagesX * pagesY * PAGE_SIZE_PADDED * PAGE_SIZE_PADDED);
    waterHeightmap->patchHeights = patchHeights;
    waterHeightmap->tcOffsetScale =
      Point4(-mainRectOffset.x / mainRectSize.x, -mainRectOffset.y / mainRectSize.y, 1.0 / mainRectSize.x, 1.0 / mainRectSize.y);
    int pageCount = 0;
    int oneLevelPageCount = 0;
    for (auto &p : pages)
    {
      int value = p.first;
      int det = (value & 3) >> 1;
      value >>= 2;
      int offsetX = value % maxPageCount;
      int offsetY = value / maxPageCount;
      int pageX = pageCount % pagesX;
      int pageY = pageCount / pagesX;
      pageCount++;
      p.second = ((pageY & 0xFF) << 8) | ((pageX & 0x7F) << 1) | det;
      float pageMin = FLT_MAX;
      float pageMax = -FLT_MAX;
      for (int y = 0; y < PAGE_SIZE_PADDED; y++)
        for (int x = 0; x < PAGE_SIZE_PADDED; x++)
        {
          int destX = x + pageX * PAGE_SIZE_PADDED;
          int destY = y + pageY * PAGE_SIZE_PADDED;
          int srcX = max(0, x - 1 + offsetX * HEIGHTMAP_PAGE_SIZE);
          srcX = min(srcX, det ? detMapSize.x : mainMapSize.x);
          int srcY = max(0, y - 1 + offsetY * HEIGHTMAP_PAGE_SIZE);
          srcY = min(srcY, det ? detMapSize.y : mainMapSize.y);
          float ht = det ? waterHmapDet.getInitialMap().getData(srcX, srcY) * waterHeightOfsScaleDet.y + waterHeightOfsScaleDet.x
                         : waterHmapMain.getInitialMap().getData(srcX, srcY) * waterHeightOfsScaleMain.y + waterHeightOfsScaleMain.x;
          pageMin = min(pageMin, ht);
          pageMax = max(pageMax, ht);
          waterHeightmap->pages[destY * texWidth + destX] =
            min(max(0u, unsigned((ht - minHeight) / range * UINT16_MAX)), (unsigned)UINT16_MAX);
        }
      if (pageMin == pageMax)
        oneLevelPageCount++;
    }
    if (oneLevelPageCount > pageCount / 2)
    {
      CoolConsole &console = DAGORED2->getConsole();
      console.addMessage(ILogWriter::ERROR, "More then half of all water heightmap pages have constant value. "
                                            "Water level should be changed to reduce memory consumption.");
    }

    waterHeightmap->gridSize = gridSize;
    waterHeightmap->grid.resize(gridSize * gridSize);
    for (int y = 0; y < gridSize; y++)
      for (int x = 0; x < gridSize; x++)
      {
        uint32_t value = UINT16_MAX;
        auto res = pages.find(tempGrid[y * gridSize + x]);
        if (res != pages.end())
          value = res->second;
        waterHeightmap->grid[y * gridSize + x] = value;
      }

    fft_water::set_heightmap(water, eastl::move(waterHeightmap));
    fft_water::reset_render(water);
    fft_water::reset_physics(water);
  }
  void exportHeightmap(BinDumpSaveCB &cwr, bool preferZstdPacking, bool allowOodlePacking) override
  {
    const fft_water::WaterHeightmap *waterHeightmap = fft_water::get_heightmap(water);
    if (!waterHeightmap)
      return;
    int st_pos = cwr.tell();
    cwr.beginTaggedBlock(_MAKE4C('WHM'));

    cwr.writeInt32e(waterHeightmap->gridSize);
    cwr.writeInt32e(waterHeightmap->pagesX);
    cwr.writeInt32e(waterHeightmap->pagesY);
    cwr.writeInt32e(waterHeightmap->scale);
    cwr.writeFloat32e(waterHeightmap->heightOffset);
    cwr.writeFloat32e(waterHeightmap->heightScale);
    cwr.writeFloat32e(waterHeightmap->tcOffsetScale.x);
    cwr.writeFloat32e(waterHeightmap->tcOffsetScale.y);
    cwr.writeFloat32e(waterHeightmap->tcOffsetScale.z);
    cwr.writeFloat32e(waterHeightmap->tcOffsetScale.w);
    mkbindump::BinDumpSaveCB cwr_hm(2 << 10, cwr);
    int h = waterHeightmap->pagesY * waterHeightmap->PAGE_SIZE_PADDED;
    int w = waterHeightmap->pagesX * waterHeightmap->PAGE_SIZE_PADDED;
    for (int y = 0; y < h; ++y)
    {
      uint16_t prev_data = 0;
      for (int x = 0; x < w; ++x)
      {
        uint16_t c_data = waterHeightmap->pages[y * w + x];
        cwr_hm.writeInt16e(x == 0 ? c_data : (int16_t)(int(c_data) - int(prev_data)));
        prev_data = c_data;
      }
    }
    for (int y = 0; y < waterHeightmap->gridSize; ++y)
      for (int x = 0; x < waterHeightmap->gridSize; ++x)
        cwr_hm.writeInt16e(waterHeightmap->grid[y * waterHeightmap->gridSize + x]);
    cwr.beginBlock();
    MemoryLoadCB mcrd(cwr_hm.getMem(), false);
    if (preferZstdPacking && allowOodlePacking)
      oodle_compress_data(cwr.getRawWriter(), mcrd, cwr_hm.getSize());
    else if (preferZstdPacking)
      zstd_compress_data(cwr.getRawWriter(), mcrd, cwr_hm.getSize(), 1 << 20, 19);
    else
      lzma_compress_data(cwr.getRawWriter(), 9, mcrd, cwr_hm.getSize());
    cwr.endBlock(preferZstdPacking ? (allowOodlePacking ? btag_compr::OODLE : btag_compr::ZSTD) : btag_compr::UNSPECIFIED);

    cwr.writeRaw(waterHeightmap->patchHeights.data(), waterHeightmap->patchHeights.size() * sizeof(Point2));

    cwr.align8();
    cwr.endBlock();
    debug("Water heightmap written, %d packed, %d unpacked", cwr.tell() - st_pos, cwr_hm.getSize());
  }
};


static WaterService srv;
static bool is_inited = false;

void setup_water_service(const DataBlock &app_blk)
{
  srv.srvDisabled = app_blk.getBlockByNameEx("projectDefaults")->getBool("disableWater", false);
}
void *get_generic_water_service()
{
  if (srv.srvDisabled)
    return NULL;

  if (!is_inited)
  {
    is_inited = true;
    if (!srv.initSrv())
    {
      srv.srvDisabled = true;
      return NULL;
    }
  }
  return &srv;
}
void release_generic_water_service()
{
  if (is_inited)
  {
    is_inited = false;
    srv.termSrv();
  }
}
