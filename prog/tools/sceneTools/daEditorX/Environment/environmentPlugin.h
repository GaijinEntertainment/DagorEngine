// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>

#include <EditorCore/ec_IGizmoObject.h>
#include <EditorCore/ec_wndPublic.h>

#include <libTools/staticGeom/geomObject.h>
#include <coolConsole/iConsoleCmd.h>

namespace PropPanel
{
class PanelWindowPropertyControl;
struct ColorCorrectionInfo;
} // namespace PropPanel

class CapsuleCreator;
class EnvironmentPlugin;
class ISceneLightService;
class ISkiesService;
class IWindService;

struct EnvironmentAces
{
  String name;
  String environmentFileName;
  String dagFileName;
  DataBlock shaderVarsBlk;
  DataBlock postfxBlk;
  DataBlock enviBlk;
  DataBlock hmapBlk;

  struct
  {
    struct S1
    {
      E3DCOLOR sunColor;
      E3DCOLOR skyColor;
      real sunDensity;
      real skyDensity;
    } fog;

    real z_near, z_far;
  } envParams;

  EnvironmentAces() { memset(&envParams, 0, sizeof(envParams)); }
  ~EnvironmentAces() {}
};


class EnvironmentPlugin : public IGenEditorPlugin,
                          public IGenEventHandler,
                          public IBinaryDataBuilder,
                          public IRenderingService,
                          public IRenderOnCubeTex,
                          public IPluginAutoSave,
                          public IConsoleCmd,
                          public PropPanel::ControlEventHandler,
                          public IWndManagerWindowHandler
{
public:
  EnvironmentPlugin();
  ~EnvironmentPlugin() override;

  const char *getInternalName() const override { return "environment"; }
  const char *getMenuCommandName() const override { return "Environment"; }
  const char *getHelpUrl() const override { return "/html/Plugins/Environment/index.htm"; }

  int getRenderOrder() const override { return -200; }
  int getBuildOrder() const override { return 0; }
  bool showInTabs() const override { return true; }

  void registered() override;
  void unregistered() override;
  void beforeMainLoop() override { needSkiesUpdate = true; }

  void registerMenuAccelerators() override;
  bool begin(int toolbar_id, unsigned menu_id) override;
  bool end() override;
  IGenEventHandler *getEventHandler() override { return this; }

  void setVisible(bool vis) override { isVisible = vis; }
  bool getVisible() const override { return isVisible; }

  bool getSelectionBox(BBox3 &) const override { return false; }
  bool getStatusBarPos(Point3 &pos) const override { return false; }

  void clearObjects() override;
  void onNewProject() override {}
  void autoSaveObjects(DataBlock &local_data) override;
  void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path) override;
  void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path) override;
  bool acceptSaveLoad() const override { return true; }
  void loadBlk(bool ps3, const char *selectedName);

  void selectAll() override {}
  void deselectAll() override {}

  void actObjects(float dt) override;
  void beforeRenderObjects(IGenViewportWnd *vp) override;
  void renderObjects() override {}
  void renderTransObjects() override;
  void updateImgui() override;
  void renderObjectsToViewport(IGenViewportWnd *vp);

  void *queryInterfacePtr(unsigned huid) override;

  bool catchEvent(unsigned ev_huid, void *userData) override;

  // command handlers
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override { return false; }
  bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override { return false; }
  bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override { return false; }
  bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) override { return false; }
  bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) override { return false; }
  void handleViewportPaint(IGenViewportWnd *wnd) override;
  void handleViewChange(IGenViewportWnd *wnd) override {}

  bool buildLmEnviScene(ILogWriter &rep, PropPanel::ContainerPropertyControl *panel, unsigned target);

  bool onPluginMenuClick(unsigned id) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  // IWndManagerWindowHandler
  void *onWmCreateWindow(int type) override;
  bool onWmDestroyWindow(void *window) override;

  // IBinaryDataBuilder implemenatation
  bool validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *params) override;
  bool addUsedTextures(ITextureNumerator &tn) override;
  bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *pp) override;
  bool useExportParameters() const override { return false; }
  void fillExportPanel(PropPanel::ContainerPropertyControl &params) override;
  bool checkMetrics(const DataBlock &metrics_blk) override;

  // IRenderingService interface
  void renderGeometry(Stage stage) override;

  void prepareCubeTex(bool renderEnvi, bool renderLit, bool renderStreamLit) override;

  // IConsoleCmd
  bool onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params) override;
  const char *onConsoleCommandHelp(const char *cmd) override;

public:
  static ISceneLightService *ltService;
  static ISkiesService *skiesSrv;
  static IWindService *windSrv;
  static bool needSkiesUpdate;

  static bool prepareRequiredServices();

  bool isAcesPlugin;

private:
  class PostfxPanel;
  class ShaderGlobVarsPanel;

  GeomObject geom;
  bool isHdr, isLdr;
  String dagFileName;

  DataBlock postfxBlk, shgvBlk, mainPanelState;

  struct
  {
    struct S1
    {
      bool used, exp2;
      E3DCOLOR sunColor;
      E3DCOLOR skyColor;
      real sunDensity;
      real skyDensity;
      bool useSunFog;

      E3DCOLOR lfogSunColor;
      E3DCOLOR lfogSkyColor;
      real lfogDensity;
      real lfogMinHeight, lfogMaxHeight;
    } fog;

    real z_near, z_far;
    real znzfScale;
    real skyHdrMultiplier;
    real rotY;
    bool exportEnviScene;
  } envParams;

  int toolBarId;
  PropPanel::PanelWindowPropertyControl *propPanel;
  PostfxPanel *postfxPanel;
  ShaderGlobVarsPanel *shgvPanel;

  bool isVisible;
  bool doResetEnvi;
  bool isPPVisible;

  bool previewFog;

  String lastImportedEnvi;
  int skyScaleVarId;
  int skyWorldXVarId;
  int skyWorldYVarId;
  int skyWorldZVarId;
  int skyFogColorAndDensityVarId;
  int sunFogColorAndDensityVarId;
  int sunFogAzimuthSincosVarId;

  int selectedWeatherPreset, selectedWeatherType;
  Tab<String> weatherPresetsList, weatherTypesList;
  int selectedEnvironmentNo;
  Tab<String> environmentNamesList;
  EnvironmentAces currentEnvironmentAces;
  struct EnviTacticalSettings
  {
    bool forceWeather, forceGeoDate, forceTime, forceSeed;
    float latitude;
    int yyyy, mm, dd;
    float timeOfDay;
    int rndSeed;

    EnviTacticalSettings() { setDef(); }
    void setDef()
    {
      forceWeather = forceGeoDate = forceTime = forceSeed = false;
      latitude = 55;
      yyyy = 1944;
      mm = 6;
      dd = 6;
      timeOfDay = 12;
      rndSeed = 1234;
    }
  };
  EnviTacticalSettings ets;

  String commonFileNamePcXbox;
  String commonFileNamePs3;
  String commonFileName;
  DataBlock commonBlk;

  unsigned long gameHttpServerAddr; // in_addr_t

  String levelFileNamePcXbox;
  String levelFileNamePs3;
  String levelFileName;
  DataBlock levelBlk;

  real sunAzimuth;

  Texture *colorTex;
  PropPanel::ColorCorrectionInfo *ccInfo;

  void importEnvi();
  void resetEnvi();

  void showPanel();
  void fillPanel();
  void updateLight();

  void setFogDefaults();

  void recreatePanel(PropPanel::PanelWindowPropertyControl *panel, int wtype_id);
  void recreatePostfxPanel();
  void recreateShGVPanel();
  void hdrViewSettings();
  void connectToGameHttpServer();
  void flushDataBlocksToGame();

  void onDirLightChanged();
  void onLightingSettingsChanged(bool rest_postfx = true);
  bool exportLms(const char *dag_file, const char *lms_file, CoolConsole &con);

  void getSettingsFromPlugins();
  void setSettingsToPlugins();

  void prepareColorStats(int r_channel[256], int g_channel[256], int b_channel[256]);
  void prepareColorTex(unsigned char r_channel[256], unsigned char g_channel[256], unsigned char b_channel[256]);
  void renderColorTex();
  void updateSunSkyEnvi();

  bool onPluginMenuClickInternal(unsigned id, PropPanel::ContainerPropertyControl *panel);

  static bool recreatePlugin;
};
