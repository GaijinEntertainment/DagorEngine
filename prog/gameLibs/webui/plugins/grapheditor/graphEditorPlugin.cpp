// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ctype.h>
#include <stdlib.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <math/dag_mathBase.h>
#include <math/dag_e3dColor.h>
#include <memory/dag_framemem.h>
#include <memory/dag_mem.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <util/dag_base64.h>
#include <util/dag_watchdog.h>
#include <webui/httpserver.h>
#include <webui/helpers.h>
#include <webui/graphEditorPlugin.h>
#include <osApiWrappers/dag_unicode.h>

#if _TARGET_PC_WIN
#include <Windows.h>
#include <shellapi.h>
SHSTDAPI_(int) SHCreateDirectoryExW(_In_opt_ HWND hwnd, _In_ LPCWSTR pszPath, _In_opt_ const SECURITY_ATTRIBUTES *psa);
#endif


using namespace webui;

static const String default_nodes_settings = String(
#include "stubNodesSettings.js.inl"
);


GraphEditor::GraphEditor(const char *plugin_name) // should not contain spaces
{
  G_ASSERT(plugin_name != NULL);
  G_ASSERT(!strchr(plugin_name, ' '));

  graphJsonToSend = "not_inited";
  pluginName.setStr(plugin_name);
  receivedGraphJsonChanged = false;
  inessentionalGraphChange = false;
  needSendGraphToHtml = true;
  needSendFilenamesToHtml = true;
  needSendGraphDescToHtml = true;
  needSendAdditionalIncludes = false;
  onAttachCallback = NULL;
  onNeedReloadGraphsCallback = NULL;
  onNewGraphCallback = NULL;
}


const String &GraphEditor::getFullIncludeFileName(const char *short_name)
{
  static String emptyStr;
  if (!short_name || !short_name[0])
    return emptyStr;

  if (strstr(short_name, "INCL: ") == short_name)
    short_name += 6;

  char buf[512] = {0};

  for (int i = 0; i < includeFilenames.size(); i++)
  {
    const char *fname = dd_get_fname_without_path_and_ext(buf, sizeof(buf), includeFilenames[i]);
    if (!strcmp(fname, short_name))
      return includeFilenames[i];
  }

  return emptyStr;
}


void GraphEditor::setIncludeFilenames(const Tab<String> &file_names)
{
  clear_and_shrink(includeFilenamesStr);
  clear_and_shrink(includeFilenames);
  for (int i = 0; i < file_names.size(); i++)
  {
    includeFilenames.push_back(file_names[i]);
    includeFilenamesStr += "INCL: ";
    char buf[512];
    const char *fname = dd_get_fname_without_path_and_ext(buf, sizeof(buf), file_names[i]);
    includeFilenamesStr += fname;
    includeFilenamesStr += "%";
  }

  needSendFilenamesToHtml = true;
}


bool GraphEditor::gatherAdditionalIncludes(const Tab<String> &file_names)
{
  needSendAdditionalIncludes = false;
  additionalIncludes.setStr("");
  for (int i = 0; i < file_names.size(); i++)
  {
    if (!dd_file_exists(file_names[i]))
    {
      G_ASSERTF(0, "GraphEditor: Additional includes: file not found '%s'", file_names[i].str());
      additionalIncludes.setStr("");
      return false;
    }

    String s = GraphEditor::readFileToString(file_names[i]);
    if (!s.empty())
    {
      if (!additionalIncludes.empty())
        additionalIncludes += ",";

      additionalIncludes += s;
    }
  }

  needSendAdditionalIncludes = true;
  return true;
}


String GraphEditor::findFileInParentDir(const char *file_name)
{
  String res(file_name);
  for (int i = 0; i < 10; i++)
    if (dd_file_exists(res))
      return res;
    else
      res = String("..\\") + res;

  G_ASSERTF(0, "GraphEditor: Cannot find file '%s' in parent directories", file_name);
  return String(file_name);
}


String GraphEditor::readFileToString(const char *file_name)
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

bool GraphEditor::writeStringToFile(const char *file_name, const char *str)
{
  G_ASSERT(file_name && *file_name);
  G_ASSERT(str);

  file_ptr_t h = df_open(file_name, DF_WRITE | DF_CREATE);
  if (!h)
    return false;

  df_write(h, str, strlen(str));
  df_close(h);
  return true;
}

String GraphEditor::getSubstring(const char *str, const char *beginMarker, const char *endMarker)
{
  const char *begin = strstr(str, beginMarker);
  if (begin)
  {
    const char *end = strstr(begin, endMarker);
    if (end)
    {
      String res(tmpmem);
      res.setSubStr(begin + strlen(beginMarker), end);
      res.replaceAll("\\\\", "\x01");
      res.replaceAll("\\n", "\n");
      res.replaceAll("\\\"", "\"");
      res.replaceAll("\x01", "\\");
      return res;
    }
  }

  return String("");
}

void GraphEditor::setSaveDir(const char *save_dir)
{
  G_ASSERT(save_dir != NULL && save_dir[0]);
  saveDir.setStr(save_dir);
}

void GraphEditor::setRootGraphFileName(const char *root_graph_fname)
{
  G_ASSERT(root_graph_fname != NULL && root_graph_fname[0]);
  rootGraphFileName.setStr(root_graph_fname);

  if (!dd_file_exists(rootGraphFileName))
  {
#if _TARGET_PC_WIN
    char path[DAGOR_MAX_PATH];
    dd_get_fname_location(path, root_graph_fname);
    for (int i = 0; i < sizeof(path) - 1; i++)
      if (path[i] == '/')
        path[i] = path[i + 1] ? '\\' : 0;

    wchar_t wPath[DAGOR_MAX_PATH];
    wchar_t wFullPath[DAGOR_MAX_PATH + 4];
    utf8_to_wcs("\\\\?\\", wFullPath, countof(wFullPath));
    int len = GetFullPathNameW(utf8_to_wcs(path, wPath, DAGOR_MAX_PATH), DAGOR_MAX_PATH, wFullPath + 4, NULL);
    G_ASSERTF(len, "GetFullPathNameW returned %d", int(GetLastError()));
    int result = SHCreateDirectoryExW(NULL, wFullPath, NULL);
    G_ASSERTF(result == ERROR_SUCCESS || result == ERROR_ALREADY_EXISTS, "SHCreateDirectoryExW for '%s' returns %d, (0x%X)", path,
      result, result);

    G_UNUSED(len);
    G_UNUSED(result);
#else
    char path[DAGOR_MAX_PATH];
    dd_get_fname_location(path, root_graph_fname);
    dd_mkpath(path);
#endif

    writeStringToFile(rootGraphFileName, "");
  }
}


bool GraphEditor::collectGraphs(const char *search_dir, const char *mask, const char *expected_plugin_name, const char *set_category,
  Tab<String> &out_filenames, String &out_descriptions, bool allowEmptyFiles, bool add_root)
{
  G_ASSERT(search_dir != NULL);
  G_ASSERT(mask != NULL);
  G_ASSERT(set_category != NULL);
  G_ASSERT(expected_plugin_name != NULL);

  char fname[256] = {0};

  out_descriptions.clear();
  out_filenames.clear();

  String pluginId(128, "[[plugin:%s]]", expected_plugin_name);
  bool first = true;

  String pathMask(search_dir);
  pathMask += "/";
  pathMask += mask;
  alefind_t fs;
  if (::dd_find_first(pathMask, 0, &fs))
  {
    do
    {
      String filePath(128, "%s/%s", search_dir, fs.name);
      dd_get_fname_without_path_and_ext(fname, sizeof(fname), fs.name);

      if (find_value_idx(out_filenames, String(fname)) >= 0)
        continue;

      String s = readFileToString(filePath);
      String description = getSubstring(s, "/*DESCRIPTION_TEXT_BEGIN*/", "/*DESCRIPTION_TEXT_END*/");
      if ((description.length() > 0 && strstr(s.str(), pluginId.str()) != NULL) || (s.length() < 4 && allowEmptyFiles))
      {
        out_filenames.push_back(String(fname));

        if (description.length() > 0)
        {
          description.replaceAll("[[description-category]]", set_category);
          description.replaceAll("[[description-name]]", fname);

          if (!first)
            out_descriptions += ",";
          out_descriptions += description;

          first = false;
        }
      }
      // watchdog_kick();
    } while (::dd_find_next(&fs));
    dd_find_close(&fs);
  }

  if (!rootGraphFileName.empty() && dd_file_exists(rootGraphFileName) && add_root)
  {
    char buf[300] = {0};
    const char *fname = dd_get_fname_without_path_and_ext(buf, sizeof(buf), rootGraphFileName);
    out_filenames.push_back(String(0, "ROOT: %s", fname));
  }

  return true;
}


void GraphEditor::setSplashScreenText(const char *text) { splashScreenText = text; }

void GraphEditor::setNodesSettingsFileName(const char *nodes_settings_file_name) { nodes_settings_fn = nodes_settings_file_name; }

void GraphEditor::setNodesSettingsText(const char *nodes_settings_text_) { nodes_settings_text = nodes_settings_text_; }

void GraphEditor::setUserScript(const char *user_script_js) { user_script = user_script_js; }

const String &GraphEditor::getUserScript() { return user_script; }

bool GraphEditor::fetchCommands(Tab<String> &out_commands)
{
  out_commands = receivedCommands;
  receivedCommands.clear();
  return !out_commands.empty();
}

bool GraphEditor::sendCommand(const char *command)
{
  commandsToSend.push_back(String(command));
  return true;
}

bool GraphEditor::getCurrentGraphJson(String &out_graph_json, bool only_if_changed, bool *inessentional_change)
{
  if (inessentional_change)
    *inessentional_change = inessentionalGraphChange;

  if (only_if_changed && !receivedGraphJsonChanged)
    return false;

  out_graph_json = receivedGraphJson;
  receivedGraphJsonChanged = false;
  return !out_graph_json.empty();
}

void GraphEditor::setGraphDescriptions(const char *category, const char *descriptions) // call this before setGraphJson()
{
  G_ASSERT(category != NULL && descriptions != NULL);

  graphCategory.push_back(String(category));
  graphDescriptions.push_back(String(descriptions));
  needSendGraphDescToHtml = true;
}

void GraphEditor::setGraphJson(const char *graph_json)
{
  extractGraphId(graph_json, currentGraphId);
  graphJsonToSend = graph_json;
  needSendGraphToHtml = true;
}

void GraphEditor::setCurrentFileName(const char *text)
{
  fileNameCmd = String(128, "filename:%s", text);
  sendCommand(fileNameCmd.str());
}

void GraphEditor::setFilenames(const Tab<String> &filenames)
{
  clear_and_shrink(filenamesToSend);
  filenamesToSend.reserve(8000);
  filenamesToSend.aprintf(128, "%s", "%file_names:");

  for (int i = 0; i < filenames.size(); i++)
    filenamesToSend.aprintf(128, "%s%%", filenames[i].str());

  needSendFilenamesToHtml = true;
}

void GraphEditor::extractGraphId(const char *graph_json, String &out_id)
{
  clear_and_shrink(out_id);
  if (!graph_json)
    return;

  const char *header = "[[GID:0A-";

  const char *begin = strstr(graph_json, header);
  if (!begin)
    return;

  const char *end = strstr(begin, "]]");
  if (!end)
    return;

  out_id.setSubStr(begin + strlen(header), end);
}


void GraphEditor::processRequest(RequestInfo *params)
{
  if (!params->params)
  {
    static const char inlinedHtml[] = {
#include "graphEditor.html.inl"
    };
    String s(tmpmem);
    s.setStr(inlinedHtml);
    s.replaceAll("__PLUGIN_NAME__", pluginName.str());

    if (!splashScreenText.empty())
      s.replaceAll("__SPLASH_SCREEN_TEXT__", splashScreenText);

    html_response_raw(params->conn, s.str());
  }
  else if (!strcmp(params->params[0], "nodesSettings.js"))
  {
    if (!nodes_settings_text.empty())
      html_response_raw(params->conn, nodes_settings_text.str());
    else if (!nodes_settings_fn.empty())
      html_response_file(params->conn, nodes_settings_fn.str());
    else
      html_response_raw(params->conn, default_nodes_settings.str());
  }
  else if (!strcmp(params->params[0], "nodeUtils.js"))
  {
    String fname = findFileInParentDir("tools/dagor_cdk/commonData/graphEditor/builder/nodeUtils.js");
    String content = readFileToString(fname);
    html_response_raw(params->conn, content);
  }
  else if (!strcmp(params->params[0], "graphEditor.js"))
  {
    String fname = findFileInParentDir("tools/dagor_cdk/commonData/graphEditor/builder/graphEditor.js");
    String content = readFileToString(fname);
    html_response_raw(params->conn, content);
  }
  else if (!strcmp(params->params[0], "userScript.js"))
  {
    html_response_raw(params->conn, user_script.str());
  }
  else if (!strcmp(params->params[0], "attach"))
  {
    if (params->params[2])
      clientId.setStr(params->params[2]);

    if (onAttachCallback)
      onAttachCallback();

    if (!additionalIncludes.empty())
      needSendAdditionalIncludes = true;

    needSendGraphDescToHtml = true;
    needSendGraphToHtml = true;
    needSendFilenamesToHtml = true;
    sendCommand(fileNameCmd.str());
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "state"))
  {
    String s(tmpmem);

    if (params->params[2] && !strcmp(clientId.str(), params->params[2]))
    {
      if (needSendGraphDescToHtml && graphCategory.size() > 0)
      {
        s.reserve(8000);
        s.printf(128, "%%subgraph_desc:%s,%s", graphCategory[0].str(), graphDescriptions[0].str());
        erase_items(graphCategory, 0, 1);
        erase_items(graphDescriptions, 0, 1);
        needSendGraphDescToHtml = graphCategory.size() > 0;
      }
      else if (needSendAdditionalIncludes)
      {
        s.reserve(8000);
        s.printf(128, "%%additional_includes:%s", additionalIncludes.str());
        needSendAdditionalIncludes = false;
      }
      else if (needSendGraphToHtml)
      {
        s.reserve(8000);
        s.printf(128, "%%graph:%s", graphJsonToSend.str());
        needSendGraphToHtml = false;
      }
      else if (needSendFilenamesToHtml && !filenamesToSend.empty())
      {
        s = filenamesToSend + includeFilenamesStr;
        needSendFilenamesToHtml = false;
      }
      else
      {
        if (!commandsToSend.empty())
        {
          s = commandsToSend[0];
          erase_items(commandsToSend, 0, 1);
        }
      }
    }

    if (s.empty())
      html_response_raw(params->conn, clientId.str());
    else
      html_response_raw(params->conn, s.str());
  }
  else if (!strcmp(params->params[0], "command"))
  {
    if (params->params[2] && !strcmp(clientId.str(), params->params[2]))
    {
      receivedCommands.push_back(String(params->params[4]));
    }
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "onfocus"))
  {
    if (params->params[2] && !strcmp(clientId.str(), params->params[2]))
    {
      if (onNeedReloadGraphsCallback)
        onNeedReloadGraphsCallback();
    }
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "graph"))
  {
    if (params->params[2] && !strcmp(clientId.str(), params->params[2]))
    {
      const char *postContent = (const char *)params->read_POST_content();
      G_ASSERT(postContent != NULL);
      if (postContent)
      {
        String gid(tmpmem);
        extractGraphId(postContent, gid);
        if (gid.length() == 0 || currentGraphId.length() == 0 || !strcmp(gid.str(), currentGraphId.str()))
        {
          if (currentGraphId.length() == 0)
            currentGraphId = gid;
          receivedGraphJson.setStr(postContent);
          if (char *pos = strstr(receivedGraphJson, "/*__INESSENTIAL_CHANGE*/"))
          {
            inessentionalGraphChange = true;
            pos += 2;
            while (*pos != '*')
            {
              *pos = '-';
              pos++;
            }
          }
          else
            inessentionalGraphChange = false;

          graphJsonToSend = receivedGraphJson;
          receivedGraphJsonChanged = true;
        }
        tmpmem->free((void *)postContent); // read_POST_content() uses tmpmem to allocate this
      }
    }
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "new_graph"))
  {
    if (params->params[2] && !strcmp(clientId.str(), params->params[2]))
    {
      const char *descName = params->params[4];
      const char *typeName = params->params[6];
      const char *creationInfo = params->params[8];
      const char *emptyGraph = (const char *)params->read_POST_content();
      G_ASSERT(emptyGraph != NULL);
      if (emptyGraph)
      {
        if (onNewGraphCallback)
          onNewGraphCallback(descName, typeName, creationInfo, emptyGraph);
      }

      tmpmem->free((void *)emptyGraph); // read_POST_content() uses tmpmem to allocate this
    }
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "new_subgraph"))
  {
    if (params->params[2] && !strcmp(clientId.str(), params->params[2]) && params->params[4])
    {
      const char *graphName = params->params[4];
      const char *postContent = (const char *)params->read_POST_content();
      G_ASSERT(postContent != NULL);
      if (postContent)
      {
        // String receivedSubgraph(tmpmem);
        // receivedSubgraph.setStr(postContent);

        if (saveDir.empty())
          sendCommand("error: saveDir is not inited");
        else
        {
          String path(128, "%s/%s.json", saveDir.str(), graphName);
          if (dd_file_exists(path.str()))
          {
            String err(128, "error: graph '%s' already exists", graphName);
            sendCommand(err.str());
          }
          else
          {
            if (writeStringToFile(path.str(), postContent))
            {
              debug("created graph %s", path.str());
              if (onNeedReloadGraphsCallback)
                onNeedReloadGraphsCallback();

              sendCommand("replace_with_subgraph");
            }
            else
            {
              String err(128, "error: cannot write file '%s'", path.str());
              sendCommand(err.str());
            }
          }
        }

        tmpmem->free((void *)postContent); // read_POST_content() uses tmpmem to allocate this
      }
    }
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "get_file"))
  {
    if (params->params[2] && !strcmp(clientId.str(), params->params[2]) && params->params[4])
    {
      const char *fileName = params->params[4];
      if (!fileName[0] || strstr(fileName, "..") != NULL || strstr(fileName, ":") == fileName + 1)
        html_response_raw(params->conn, "error: invalid file name");
      else
      {
        if (strstr(fileName, "ROOT: ") == fileName)
          fileName = rootGraphFileName.str();

        if (strstr(fileName, "INCL: ") == fileName)
          fileName = getFullIncludeFileName(fileName);

        if (!dd_file_exists(fileName))
          html_response_raw(params->conn, "error: file not exists");
        else
        {
          String s(tmpmem);
          String fName(fileName);
          fName.replaceAll("SAVE-DIR", saveDir.str());
          s.printf(1024, "file:%s", readFileToString(fName.str()).str());
          html_response_raw(params->conn, s.str());
        }
      }
    }
    else
      html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "save_file"))
  {
    if (params->params[2] && !strcmp(clientId.str(), params->params[2]) && params->params[4])
    {
      const char *fileName = params->params[4];
      if (!fileName[0] || strstr(fileName, "..") != NULL || strstr(fileName, ":") == fileName + 1)
        html_response_raw(params->conn, "error: invalid file name");
      else
      {
        const char *postContent = (const char *)params->read_POST_content();
        G_ASSERT(postContent != NULL);
        if (postContent)
        {
          String tmpName(128, "%s.tmp", fileName);
          tmpName.replaceAll("SAVE-DIR", saveDir.str());
          if (strstr(tmpName.str(), "ROOT: ") == tmpName.str())
            tmpName = rootGraphFileName;

          if (strstr(tmpName.str(), "INCL: ") == tmpName.str())
            tmpName = getFullIncludeFileName(tmpName);

          bool ok = writeStringToFile(tmpName.str(), postContent);
          if (!ok)
            html_response_raw(params->conn, "error: cannot write file");
          else if (rename(tmpName.str(), fileName) != 0)
            html_response_raw(params->conn, "error: cannot rename file");
          else
            html_response_raw(params->conn, "");

          tmpmem->free((void *)postContent); // read_POST_content() uses tmpmem to allocate this
        }
      }
    }
    else
      html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "collect_graphs"))
  {
    if (params->params[2] && !strcmp(clientId.str(), params->params[2]))
    {
      const char *searchDir = params->params[4];
      G_ASSERT(searchDir);
      const char *mask = params->params[6];
      G_ASSERT(mask);
      const char *expectedPluginName = params->params[8];
      G_ASSERT(expectedPluginName);
      const char *setCategory = params->params[10];
      G_ASSERT(setCategory);

      if (!searchDir[0] || strstr(searchDir, "..") != NULL || strstr(searchDir, ":") == searchDir + 1)
        html_response_raw(params->conn, "error: invalid search dir");
      else
      {
        Tab<String> fileNames;
        String descriptions;
        String sDir(searchDir);
        sDir.replaceAll("SAVE-DIR", saveDir.str());
        collectGraphs(sDir, mask, expectedPluginName, setCategory, fileNames, descriptions, true);

        String s(tmpmem);
        s.printf(1024, "files:");
        for (int i = 0; i < fileNames.size(); i++)
          s.aprintf(128, "%s%%", fileNames[i].str());
        s.aprintf(128, ":descriptions:%s", descriptions.str());

        html_response_raw(params->conn, s.str());
      }
    }
    else
      html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "get_json_files"))
  {
    if (params->params[2] && !strcmp(clientId.str(), params->params[2]))
    {
      const char *postContent = (const char *)params->read_POST_content();
      G_ASSERT(postContent != NULL);
      if (postContent)
      {
        String s(tmpmem);
        s.setStr(postContent);
        Tab<String> fileNames;
        for (int i = 0; i < s.length(); i++)
        {
          char *delim = strchr(&s[i], '%');
          int nextPos = delim ? int(delim - &s[i]) : s.length();
          s[nextPos] = 0;
          fileNames.push_back(String(&s[i]));
          i = nextPos;
        }

        clear_and_shrink(s);

        for (int i = 0; i < fileNames.size(); i++)
        {
          fileNames[i].replaceAll("SAVE-DIR", saveDir.str());
          const char *fileName = fileNames[i].str();
          if (fileName[0] != 0 || strstr(fileName, "..") != NULL || strstr(fileName, ":") == fileName + 1)
          {
            if (strstr(fileName, "ROOT: ") == fileName)
              fileName = rootGraphFileName.str();

            if (strstr(fileName, "INCL: ") == fileName)
              fileName = getFullIncludeFileName(fileName);

            // TODO: return errors
            String file = readFileToString(fileName);
            if (file.length() > 2)
            {
              s.aprintf(128, (i == fileNames.size() - 1) ? "%s" : "%s,", file);
            }
          }
        }

        html_response_raw(params->conn, s.str());
        tmpmem->free((void *)postContent); // read_POST_content() uses tmpmem to allocate this
        return;                            // ok
      }
    }

    html_response_raw(params->conn, "");
  }
  // else if (!strcmp(params->params[0], "detach")) {}
}
