//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <EASTL/functional.h>

namespace webui
{

struct RequestInfo;

class GraphEditor
{
  String nodes_settings_fn;
  String nodes_settings_text;
  String user_script;

  Tab<String> receivedCommands;
  Tab<String> commandsToSend;
  bool receivedGraphJsonChanged;
  bool inessentionalGraphChange;
  String receivedGraphJson;
  bool needSendGraphToHtml;
  String graphJsonToSend;
  bool needSendFilenamesToHtml;
  String filenamesToSend;
  bool needSendGraphDescToHtml;
  String additionalIncludes;
  String includeFilenamesStr;
  bool needSendAdditionalIncludes;
  Tab<String> graphCategory;
  Tab<String> graphDescriptions;
  String clientId;
  String pluginName;
  String currentGraphId;
  String splashScreenText;
  String fileNameCmd;
  String saveDir;
  String rootGraphFileName;

  Tab<String> includeFilenames;


public:
  GraphEditor(const char *plugin_name);

  eastl::function<void()> onAttachCallback;

  void (*onNeedReloadGraphsCallback)(void);
  void (*onNewGraphCallback)(const char *descName, const char *typeName, const char *creationInfo, const char *emptyGraph);

  void setNodesSettingsFileName(const char *nodes_settings_filename);
  void setNodesSettingsText(const char *nodes_settings_text_);
  void setUserScript(const char *user_script_js);
  bool fetchCommands(Tab<String> &out_commands);
  bool sendCommand(const char *command);
  bool getCurrentGraphJson(String &out_graph_json, bool only_if_changed, bool *inessentional_change = NULL);
  void setGraphDescriptions(const char *category, const char *descriptions); // call this before setGraphJson()
  void setGraphJson(const char *graph_json);
  void setFilenames(const Tab<String> &filenames);
  void setCurrentFileName(const char *text);
  void processRequest(RequestInfo *params);
  void setSplashScreenText(const char *text);
  void setSaveDir(const char *save_dir);
  void setRootGraphFileName(const char *root_graph_fname);
  void setIncludeFilenames(const Tab<String> &file_names);
  bool gatherAdditionalIncludes(const Tab<String> &file_names);
  static void extractGraphId(const char *graph_json, String &out_id);
  static String findFileInParentDir(const char *file_name);
  static String readFileToString(const char *file_name);
  static bool writeStringToFile(const char *file_name, const char *str);
  static String getSubstring(const char *str, const char *beginMarker, const char *endMarker);
  bool collectGraphs(const char *search_dir, const char *mask, const char *expected_plugin_name, const char *set_category,
    Tab<String> &out_filenames, String &out_descriptions, bool allowEmptyFiles, bool add_root = true);
  const String &getFullIncludeFileName(const char *short_name);
  const String &getUserScript();
};

} // namespace webui
