#include "../av_plugin.h"
#include <assets/asset.h>
#include <3d/dag_drv3d.h>
#include <image/dag_texPixel.h>
#include <de3_bitMaskMgr.h>
#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>
#include <workCycle/dag_gameSettings.h>

#include <propPanel2/c_panel_base.h>
#include <propPanel2/c_control_event_handler.h>

#include <de3_interface.h>

class ShaderGraphViewPlugin : public IGenEditorPlugin, public ControlEventHandler
{
  String currentName;
  static String graphJsonFileName;
  static bool nameChanged;

  static const int PID_EDIT_SHADER = 1;

public:
  ShaderGraphViewPlugin()
  {
    clear_and_shrink(graphJsonFileName);
    nameChanged = true;
  }

  ~ShaderGraphViewPlugin() {}

  virtual const char *getInternalName() const { return "shaderGraph"; }

  virtual void registered() { debug("ShaderGraphViewPlugin registred"); }
  virtual void unregistered() {}

  virtual bool begin(DagorAsset *asset)
  {
    currentName = asset->getTargetFilePath();
    DAEDITOR3.conNote("ShaderGraphViewPlugin: selected shader graph %s", currentName.str());
    return true;
  }

  virtual bool end() { return true; }
  virtual IGenEventHandler *getEventHandler() { return NULL; }

  virtual void clearObjects() {}
  virtual void onSaveLibrary() {}
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const
  {
    box.lim[0] = Point3(0, 0, 0);
    box.lim[1] = Point3(0, 4, 0);
    return true;
  }

  virtual void actObjects(float dt) {}
  virtual void beforeRenderObjects() {}
  virtual void renderObjects() {}
  virtual void renderTransObjects() {}

  virtual void renderGeometry(Stage stage) {}

  virtual bool supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "shaderGraph") == 0; }

  virtual void fillPropPanel(PropertyContainerControlBase &panel)
  {
    panel.setEventHandler(this);
    panel.createButton(PID_EDIT_SHADER, "Switch Editor to This");
  }

  virtual void postFillPropPanel() {}

  static bool fetchJsonFileName(String &out_name)
  {
    if (nameChanged)
    {
      out_name = graphJsonFileName;
      nameChanged = false;
      return true;
    }
    else
      return false;
  }

  virtual void onClick(int pid, PropertyContainerControlBase *panel)
  {
    if (pid == PID_EDIT_SHADER)
    {
      if (strcmp(graphJsonFileName.str(), currentName.str()) != 0)
      {
        graphJsonFileName = currentName;
        nameChanged = true;
        //::dgs_dont_use_cpu_in_background = false; // need for better response of webui
      }
    }
  }
};

bool fetch_graph_json_file_name(String &out_name) { return ShaderGraphViewPlugin::fetchJsonFileName(out_name); }

static InitOnDemand<ShaderGraphViewPlugin> plugin;

String ShaderGraphViewPlugin::graphJsonFileName;
bool ShaderGraphViewPlugin::nameChanged = false;


void init_plugin_shader_graph()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
