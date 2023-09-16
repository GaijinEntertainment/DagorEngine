#include <webui/httpserver.h>
#include <webui/editVarPlugin.h>
#include <webui/shaderEditors.h>

void de3_webui_init() {}
bool de3_webui_check_inited() { return false; }
void de3_webui_term() {}

void de3_webui_clear_c(const struct GuiControlDescWebUi *, int) {}
void de3_webui_build_c(const struct GuiControlDescWebUi *arr, int c)
{
  for (int i = 0; i < c; ++i)
  {
    arr[i].setFromPrev();
  }
}

void on_sqdebug_internal(int, struct webui::RequestInfo *) {}

webui::HttpPlugin shader_graph_editor_http_plugin;

namespace webui
{
HttpPlugin *plugin_lists[MAX_PLUGINS_LISTS] = {
  nullptr,
};
HttpPlugin dagor_http_plugins[] = {
  {nullptr},
};
HttpPlugin webview_http_plugins[] = {
  {nullptr},
};

HttpPlugin profiler_http_plugin;
HttpPlugin shader_http_plugin;
HttpPlugin colorpicker_http_plugin;
HttpPlugin curveeditor_http_plugin;
HttpPlugin color_pipette_http_plugin;
HttpPlugin cookie_http_plugin;
HttpPlugin editvar_http_plugin;
HttpPlugin ecsviewer_http_plugin;
HttpPlugin auto_drey_http_plugin;
HttpPlugin colorgradient_http_plugin;
HttpPlugin rendinst_colors_http_plugin;

void startup(struct Config *) {}
void shutdown() {}
void update() {}

void init_dmviewer_plugin() {}
void init_ecsviewer_plugin() {}
void set_ecsviewer_entity_manager(ecs::EntityManager *) {}
uint16_t get_binded_port() { return 0; }
} // namespace webui

EditableVariablesNotifications::~EditableVariablesNotifications() {}
void EditableVariablesNotifications::registerVariable(void *) {}
void EditableVariablesNotifications::unregisterVariable(void *) {}
void EditableVariablesNotifications::registerIntEditbox(int *, int, int, const char *, const char *) {}
void EditableVariablesNotifications::registerIntSlider(int *, int, int, const char *, const char *) {}
void EditableVariablesNotifications::registerFloatEditbox(float *, float, float, float, const char *, const char *) {}
void EditableVariablesNotifications::registerFloatSlider(float *, float, float, float, const char *, const char *) {}
void EditableVariablesNotifications::registerPoint2Editbox(Point2 *, const char *, const char *) {}
void EditableVariablesNotifications::registerIPoint2Editbox(IPoint2 *, const char *, const char *) {}
void EditableVariablesNotifications::registerPoint3Editbox(Point3 *, const char *, const char *) {}
void EditableVariablesNotifications::registerIPoint3Editbox(IPoint3 *, const char *, const char *) {}
void EditableVariablesNotifications::registerE3dcolor(E3DCOLOR *, const char *, const char *) {}
void EditableVariablesNotifications::registerCurve(EditorCurve *, const char *, const char *) {}
void EditableVariablesNotifications::removeVariable(void *) {}

void update_fog_shader_recompiler(float) {}
