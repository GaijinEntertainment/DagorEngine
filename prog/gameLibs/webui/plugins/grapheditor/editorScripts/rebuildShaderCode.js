"use strict";

var offlineSettings = {};

function normalizeSlashes(s)
{
  return s.split("/").join("\\");
}


function getDirectoryFilesList(dir)
{
  var tmpFileName = normalizeSlashes(dir) + "\\__temp__";
  systemExec("call dir /b " + normalizeSlashes(offlineSettings.globalSubgraphsDir) + " > " + tmpFileName);
  var files = offlineReadFileToString(tmpFileName);
  systemExec("del " + tmpFileName);
  files = files.split("\r").join("");
  return files.split("\n");
}


function renameFile(source, dest)
{
  source = normalizeSlashes(source);
  dest = normalizeSlashes(dest);
  systemExec('copy /b/v/y "' + source + '" "' + dest + '">nul && del "' + source + '">nul');
}


function collectSubgraphs(dir)
{
  var getSubstring = function(s, begin, end)
  {
    return s.split(begin)[1].split(end)[0];
  }

  var out_file_names = [];
  var out_descriptions = "";
  var files = getDirectoryFilesList(dir);
  for (var i = 0; i < files.length; i++)
    if (files[i].indexOf(".json") >= 0)
    {
      var fname = files[i].split(".json")[0];
      out_file_names.push(fname);
      var txt = offlineReadFileToString(offlineSettings.globalSubgraphsDir + "\\" + files[i]);
       
      if (txt.indexOf("[[plugin:" + pluginName + "]]") < 0)
        continue;

      txt = txt.split("\\\"").join("\"").split("\\n").join("\n");

      if (out_descriptions.length > 0)
        out_descriptions += ",";

      var desc = getSubstring(txt, "/*DESCRIPTION_TEXT_BEGIN*/", "/*DESCRIPTION_TEXT_END*/");
      desc = desc.split("[[description-name]]").join(fname);
      desc = desc.split("[[description-category]]").join("ShadersTemp");
      out_descriptions += desc;
    }

  return {fileNames: out_file_names, descriptions: out_descriptions};
}


function offlineRun()
{
  var commandLineArgs = getCommandLineArgs();
  for (var i = 0; i < commandLineArgs.length; i++)
  {
    if (commandLineArgs[i].indexOf('=') >= 0)
    {
      var a = commandLineArgs[i].split('=');
      offlineSettings[a[0]] = a.slice(1).join('=');
    }
  }

  var additionalIncludesFn = offlineSettings["includes"] ? offlineSettings["includes"].split(';') : [];

  pluginName = offlineSettings.pluginName;

  var subgraphs = collectSubgraphs(offlineSettings.globalSubgraphsDir);

  offlineBuild(offlineSettings.rootFileName, offlineSettings.outputFileName + ".x", subgraphs, additionalIncludesFn);
  renameFile(offlineSettings.outputFileName + ".x", offlineSettings.outputFileName);
}

offlineRun();
