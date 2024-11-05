// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>

#include <EditorCore/ec_IGizmoObject.h>

#include <sepGui/wndPublic.h>

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


enum EnviType
{
  ENVI_TYPE_CAPSULE_FOG,
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
  ~EnvironmentPlugin();

  virtual const char *getInternalName() const { return "environment"; }
  virtual const char *getMenuCommandName() const { return "Environment"; }
  virtual const char *getHelpUrl() const { return "/html/Plugins/Environment/index.htm"; }

  virtual int getRenderOrder() const { return -200; }
  virtual int getBuildOrder() const { return 0; }
  virtual bool showInTabs() const { return true; }

  virtual void registered();
  virtual void unregistered();
  virtual void beforeMainLoop() { needSkiesUpdate = true; }

  virtual bool begin(int toolbar_id, unsigned menu_id);
  virtual bool end();
  virtual IGenEventHandler *getEventHandler() { return this; }

  virtual void setVisible(bool vis) { isVisible = vis; }
  virtual bool getVisible() const { return isVisible; }

  virtual bool getSelectionBox(BBox3 &) const { return false; }
  virtual bool getStatusBarPos(Point3 &pos) const { return false; }

  virtual void clearObjects();
  virtual void onNewProject() {}
  virtual void autoSaveObjects(DataBlock &local_data);
  virtual void saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path);
  virtual void loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path);
  virtual bool acceptSaveLoad() const { return true; }
  void loadBlk(bool ps3, const char *selectedName);

  virtual void selectAll() {}
  virtual void deselectAll() {}

  virtual void actObjects(float dt);
  virtual void beforeRenderObjects(IGenViewportWnd *vp);
  virtual void renderObjects() {}
  virtual void renderTransObjects();
  virtual void updateImgui() override;
  void renderObjectsToViewport(IGenViewportWnd *vp);

  virtual void *queryInterfacePtr(unsigned huid);

  virtual bool catchEvent(unsigned ev_huid, void *userData);

  // command handlers
  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif);
  virtual void handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif) {}

  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }
  virtual bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }
  virtual bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }
  virtual bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) { return false; }
  virtual bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) { return false; }
  virtual void handleViewportPaint(IGenViewportWnd *wnd);
  virtual void handleViewChange(IGenViewportWnd *wnd) {}

  bool buildLmEnviScene(ILogWriter &rep, PropPanel::ContainerPropertyControl *panel, unsigned target);

  virtual bool onPluginMenuClick(unsigned id);
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);

  // IWndManagerWindowHandler
  virtual void *onWmCreateWindow(int type) override;
  virtual bool onWmDestroyWindow(void *window) override;

  // IBinaryDataBuilder implemenatation
  virtual bool validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *params);
  virtual bool addUsedTextures(ITextureNumerator &tn);
  virtual bool buildAndWrite(BinDumpSaveCB &cwr, const ITextureNumerator &tn, PropPanel::ContainerPropertyControl *pp);
  virtual bool useExportParameters() const { return false; }
  virtual void fillExportPanel(PropPanel::ContainerPropertyControl &params);
  virtual bool checkMetrics(const DataBlock &metrics_blk);

  // IRenderingService interface
  virtual void renderGeometry(Stage stage);

  virtual void prepareCubeTex(bool renderEnvi, bool renderLit, bool renderStreamLit);

  // IConsoleCmd
  virtual bool onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params);
  virtual const char *onConsoleCommandHelp(const char *cmd);

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
  bool previewZNearZFar;

  String lastImportedEnvi;
  Tab<EnviType> lastTypeList;
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
};
