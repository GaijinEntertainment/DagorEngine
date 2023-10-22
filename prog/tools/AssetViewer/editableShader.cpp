#include "av_appwnd.h"
#include <debug/dag_debug.h>
#include <webui/httpserver.h>
#include <webui/helpers.h>
#include <webui/graphEditorPlugin.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <sepGui/wndGlobal.h>
#include <de3_interface.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_threads.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_shaderCommon.h>
#include "shaderGraph/shaderGraph.h"
#include "../../engine/shaders/shadersBinaryData.h" // hack include to get direct access to shaders internal representation

#include <Windows.h>


#define SHADER_GRAPH_EDITOR_PLUGIN_NAME "shader_editor"

webui::GraphEditor *graph_editor = NULL;
static WinCritSec cs;

static void on_shader_graph_editor(webui::RequestInfo *params) { graph_editor->processRequest(params); }

webui::HttpPlugin shader_graph_editor_http_plugin = {
  SHADER_GRAPH_EDITOR_PLUGIN_NAME, "show shader graph editor", NULL, on_shader_graph_editor};

namespace
{
struct ShaderGraphRecompiler
{
  bool initialized;
  bool compilerExecuted;
  bool reloadShadersAfterCompile;
  String currentGraphJsonFileName;
  String shaderCodeFileName;
  String shaderVarsFileName;
  String compilerCommandLine;
  String compilerCurrentDir;
  String compilerOutFileName;
  String compilerErrorFileName;
  HANDLE compilerOutFileHandle;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ShaderGraphRecompiler()
  {
    initialized = false;
    currentGraphJsonFileName = "temp_shader_graph.json";
  }

  ~ShaderGraphRecompiler()
  {
    if (initialized && compilerExecuted)
    {
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
      removeCompilerOutFile();
    }
  }

  void createCompilerOutFile()
  {
    if (compilerOutFileName.empty())
    {
      char tmp[MAX_PATH + 1] = {0};
      GetTempPath(sizeof(tmp), tmp);

      compilerOutFileName.printf(128, "%s\\%d-%d.tmp", tmp, get_time_msec(), get_process_uid());
      compilerErrorFileName.printf(128, "%s\\%d-%de.tmp", tmp, get_time_msec(), get_process_uid());
    }

    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    compilerOutFileHandle =
      CreateFileA(compilerOutFileName.str(), GENERIC_READ | GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    G_ASSERT(compilerOutFileHandle != INVALID_HANDLE_VALUE);
  }

  void removeCompilerOutFile()
  {
    CloseHandle(compilerOutFileHandle);
    ::unlink(compilerOutFileName.str());
  }


  bool writeStringToFile(const char *file_name, const char *str)
  {
    G_ASSERT(file_name && *file_name);
    G_ASSERT(str);

    file_ptr_t h = df_open(file_name, DF_WRITE | DF_CREATE);
    if (!h)
      return false;

    df_write(h, str, int(strlen(str)));
    df_close(h);
    return true;
  }

  String readFileToString(const char *file_name)
  {
    file_ptr_t h = df_open(file_name, DF_READ | DF_IGNORE_MISSING);
    if (!h)
      return String("");

    int len = df_length(h);

    String res;
    res.resize(len + 1);
    df_read(h, &res[0], len);
    res.back() = '\0';

    df_close(h);
    return res;
  }

  void createCompilerProcess()
  {
    createCompilerOutFile();

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdError = compilerOutFileHandle;
    si.hStdOutput = compilerOutFileHandle;

    if (!CreateProcessA(NULL, compilerCommandLine.str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, compilerCurrentDir.str(), &si, &pi))
    {
      char buf[256] = {0};
      char msg[256] = {0};
      FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), msg, 255, NULL);

      SNPRINTF(buf, sizeof(buf), "error: CreateProcess failed (%d) %s", int(GetLastError()), msg);
      graph_editor->sendCommand(buf);
      return;
    }

    compilerExecuted = true;
  }

  void init()
  {
    DataBlock appBlk(::get_app().getWorkspace().getAppPath());
    const DataBlock *blk = appBlk.getBlockByName("shaderGraphEdit");
    if (!blk)
    {
      DAEDITOR3.conNote("shaderGraphEdit not found in application.blk, ShaderGraphRecompiler is not initialized");
      return;
    }

    compilerCommandLine = blk->getStr("compilerCommandLine"); //"cmd.exe /C echo y | call compile_shaders_tools.bat";
    compilerCurrentDir = blk->getStr("compilerCurrentDir");   // "F:\\dagor4\\prog\\samples\\testGraphEdit\\shaders";

    shaderCodeFileName = ::make_full_path(sgg::get_exe_path(), "../commonData/graphEditor/generated_node_ps.hlsl.inl");
    shaderVarsFileName = ::make_full_path(sgg::get_exe_path(), "../commonData/graphEditor/generated_node_samplers.inl");
    clear_and_shrink(compilerOutFileName);
    clear_and_shrink(compilerErrorFileName);

    compilerExecuted = false;
    reloadShadersAfterCompile = false;
    initialized = true;
  }

  String getSubstring(const char *str, const char *beginMarker, const char *endMarker)
  {
    const char *begin = strstr(str, beginMarker);
    if (begin)
    {
      const char *end = strstr(begin, endMarker);
      if (end)
      {
        String res(tmpmem);
        res.setSubStr(begin + strlen(beginMarker), end);
        res.replaceAll("\\n", "\n");
        res.replaceAll("\\\"", "\"");
        res.replaceAll("\\\\", "\\");
        return res;
      }
    }

    return String("");
  }


  void update()
  {
    if (!initialized)
      return;

    if (compilerExecuted)
    {
      sleep_msec(10);

      DWORD exitCode = 0xffff;
      if (GetExitCodeProcess(pi.hProcess, &exitCode) == 0)
      {
        char buf[256];
        SNPRINTF(buf, sizeof(buf), "error: %d", int(GetLastError()));
        graph_editor->sendCommand(buf);
      }

      if (exitCode == STILL_ACTIVE)
        return;

      compilerExecuted = false;

      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);

      if (exitCode <= 1)
      {
        reloadShadersAfterCompile = true;
        graph_editor->sendCommand("compiled"); // << send "compiled" to clear error state
        removeCompilerOutFile();
      }
      else
      {
        graph_editor->sendCommand("error: not compiled"); // << send this to indicate compiler error
        CloseHandle(compilerOutFileHandle);
        CopyFile(compilerOutFileName.str(), compilerErrorFileName.str(), FALSE);
        ::unlink(compilerOutFileName.str());
      }
    }

    if (reloadShadersAfterCompile)
    {
      reloadShadersAfterCompile = false;

      Tab<const char *> params;
      get_app().onConsoleCommand("shaders.reload", params);
      //      console::command("render.reload_shaders");
    }

    String s(tmpmem);
    bool newGraph = graph_editor->getCurrentGraphJson(s, true); // << get actual graph after each change
    if (newGraph)
    {
      if (!writeStringToFile(currentGraphJsonFileName.str(), s.str()))
        G_ASSERTF(0, "cannon write shader graph to file '%s'", currentGraphJsonFileName.str());

      // textures
      {
        String texBlk = getSubstring(s.str(), "/*TEX_BLK_START_VP*/", "/*TEX_BLK_END*/");
        DataBlock blk(tmpmem);
        blk.loadText(texBlk.str(), texBlk.length());

        ShaderGlobal::set_vars_from_blk(blk, true, true);

        String texInl = getSubstring(s.str(), "/*TEX_INL_START_VP*/", "/*TEX_INL_END*/");
        if (!writeStringToFile(shaderVarsFileName.str(), texInl.str()))
          G_ASSERTF(0, "cannon write shader to file '%s'", shaderVarsFileName.str());
      }

      // generated code
      {
        String shaderCode = getSubstring(s.str(), "/*HLSL_GRAPH_START_VP*/", "/*HLSL_GRAPH_END*/");
        if (!writeStringToFile(shaderCodeFileName.str(), shaderCode.str()))
          G_ASSERTF(0, "cannon write shader to file '%s'", shaderCodeFileName.str());
      }

      createCompilerProcess();
    }


    Tab<String> cmd;
    if (graph_editor->fetchCommands(cmd)) // << get commands from frontend
    {
      for (int i = 0; i < cmd.size(); i++)
      {
        if (!strcmp(cmd[i].str(), "get_compiler_log"))
        {
          String error(tmpmem);
          error.aprintf(128, "%s", "%compiler_log:");
          error.aprintf(128, "%s", readFileToString(compilerErrorFileName.str()).str());
          error.aprintf(128, "\n\n%s:\n%s", shaderCodeFileName.str(), readFileToString(shaderCodeFileName.str()).str());
          error.aprintf(128, "\n\n%s:\n%s", shaderVarsFileName.str(), readFileToString(shaderVarsFileName.str()).str());
          graph_editor->sendCommand(error.str());
        }

        if (strstr(cmd[i].str(), "open:") == cmd[i].str())
        {
          String filename(tmpmem);
          filename.setStr(cmd[i].str() + strlen("open:"));
          debug("open graph: %s", filename.str());

          // TODO: Open graph here. ( Just call graph_editor->setGraphJson() )
          // + graph_editor->setCurrentFileName();
        }
      }
    }

    String openFileName(tmpmem);
    if (fetch_graph_json_file_name(openFileName) && openFileName.length() > 0)
    {
      debug("open new shader graph in browser: %s", openFileName.str());
      currentGraphJsonFileName = openFileName;
      graph_editor->setGraphJson(readFileToString(openFileName.str()).str());
      graph_editor->setCurrentFileName(openFileName.str());
    }
  }

  static String getUserScript()
  {
    const ScriptedShadersBinDump &shd = shBinDump();
    dag::ConstSpan<shaderbindump::VarList::Var> gvars = shBinDump().globVars.v;
    const char *typeName[] = {"int", "float", "float4", "texture"};
    Tab<String> varsByType;

    for (int i = 0; i < countof(typeName); i++)
    {
      String code(tmpmem);
      code.printf(128, "GE_setGlobalShaderVars(\"%s\", [\"-unknown-\"", typeName[i]);
      varsByType.push_back(code);
    }

    for (int i = 0; i < gvars.size(); i++)
    {
      const shaderbindump::VarList::Var &v = gvars[i];
      G_ASSERT((unsigned)v.type < countof(typeName));
      varsByType[v.type].aprintf(32, ",\"%s\"", (const char *)shd.varMap[v.nameId]);
    }

    String s(tmpmem);
    s.reserve(8000);

    for (int i = 0; i < countof(typeName); i++)
    {
      varsByType[i].aprintf(32, "]);\n");
      s.aprintf(1000, "%s", varsByType[i].str());
    }

    return s;
  }
};
} // namespace

static ShaderGraphRecompiler *shader_recompiler = NULL;
static DaThread *webui_updater = NULL;

class WebuiUpdater : public DaThread
{
public:
  WebuiUpdater() : DaThread("WebuiUpdater", 128 << 10) {}

  virtual void execute()
  {
    while (1)
    {
      if (terminating)
        break;

      {
        WinAutoLock lock(cs);
        webui::update();
      }

      sleep_msec(50);
    }
  }
};


void init_editable_shader()
{
  shader_recompiler = new ShaderGraphRecompiler();
  shader_recompiler->init();
  if (!shader_recompiler->initialized)
    return;

  static webui::HttpPlugin graph_http_plugins[] = {
    webui::color_pipette_http_plugin, webui::colorpicker_http_plugin, shader_graph_editor_http_plugin, NULL};
  webui::plugin_lists[0] = graph_http_plugins;
  webui::Config cfg;
  memset(&cfg, 0, sizeof(cfg));
  webui::startup(&cfg);

  graph_editor = new webui::GraphEditor(SHADER_GRAPH_EDITOR_PLUGIN_NAME);

  graph_editor->setNodesSettingsFileName(::make_full_path(sgg::get_exe_path(), "../commonData/graphEditor/hlslShaderNodes.js"));
  graph_editor->setUserScript(ShaderGraphRecompiler::getUserScript().str());

  graph_editor->setSplashScreenText("Press 'Switch Editor to This' button in Asset Viewer.");

  webui_updater = new WebuiUpdater();
  webui_updater->start();
}

void update_editable_shader()
{
  if (shader_recompiler)
  {
    WinAutoLock lock(cs);
    shader_recompiler->update();
  }
}

void shutdown_editable_shader()
{
  if (webui_updater)
  {
    webui_updater->terminate(true);
    webui::shutdown();
  }

  del_it(shader_recompiler);
  del_it(graph_editor);
}
