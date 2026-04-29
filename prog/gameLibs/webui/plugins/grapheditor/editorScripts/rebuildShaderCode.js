"use strict";

var offlineSettings = {};

function normalizeSlashes(s)
{
  return s.split("/").join("\\");
}

function runBatchCommandGetOutput(cmd, tag)
{
  var tmpFileName = "__temp__" + tag;
  systemExec("(" + cmd + ")" + " > " + tmpFileName);
  var output = offlineReadFileToString(tmpFileName);
  systemExec("del " + tmpFileName);
  return output;
}

function getDirectoryFilesList(dir, tag)
{
  var exists = runBatchCommandGetOutput("if exist " + dir + " (echo yes) else (echo no)", tag);
  if (exists.includes("no"))
    return [];
  var files = runBatchCommandGetOutput("call dir /b " + dir, tag);
  files = files.split("\r").join("");
  return files.split("\n");
}


function renameFile(source, dest)
{
  source = normalizeSlashes(source);
  dest = normalizeSlashes(dest);
  systemExec('copy /b/v/y "' + source + '" "' + dest + '">nul && del "' + source + '">nul');
}


function collectSubgraphs(dir, tag)
{
  var getSubstring = function(s, begin, end)
  {
    return s.split(begin)[1].split(end)[0];
  }

  var out_file_names = [];
  var out_descriptions = "";
  var files = getDirectoryFilesList(dir, tag);
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

function getFilename(path)
{
  if (path.length == 0)
    return path;
  var i = path.length - 1;
  while (i >= 0)
  {
    if (path[i] == "/" || path[i] == "\\")
      break;
    --i;
  }
  return path.slice(i + 1, path.length);
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
  var additionalIncludesPermTable = [];

  var additionalIncludesFnFlat = [];

  // @TODO: would be good to have a single source of truth for
  // group x perm limits. Not worth the hassle probably, as it's in dshl
  // and js. These limits mirror the intervals nbsPermIdG-X defined in
  // node_based_common.dshl
  var maxGroupCount = 2;
  var maxPermsPerGroup = 4;

  var group = 0;
  for (var groupFnamesId in additionalIncludesFn)
  {
    var groupFnames = additionalIncludesFn[groupFnamesId];
    if (groupFnames[0] !== '(' || groupFnames[groupFnames.length - 1] !== ')')
    {
      alert("dshl opt graph groups must be enclosed in parens");
      return;
    }
    var permFiles = groupFnames.slice(1, -1).split(':');

    // @HACK: coalesce disc identifiers back with their pathes
    // @TODO: change identifiers to not have this issue (requires altering locShadersExp and a tools build)

    {
      var i = 0;
      for (; i < permFiles.length; ++i)
      {
        if (permFiles[i].length <= 1 && i < permFiles.length - 1)
        {
          if (permFiles[i].length == 1)
            permFiles[i + 1] = permFiles[i] + ":" + permFiles[i + 1];
          permFiles.splice(i, 1);
          --i;
        }
      }
    }

    if (permFiles.length < 1)
    {
      alert("Empty opt graph groups () are not allowed.");
      return;
    }
    if (group >= maxGroupCount)
    {
      alert(
        "NBS shader can't use more than " +
        maxGroupCount + " permutation groups");
      return;
    }

    var perm = 0;
    for (var permFnameId in permFiles)
    {
      var permFname = permFiles[permFnameId];
      if (perm >= maxPermsPerGroup)
      {
        alert(
          "NBS shader can't use more than " +
          maxPermsPerGroup + " optional permutation graphs in one group");
        return;
      }

      var permFname = permFiles[permFnameId];
      additionalIncludesFnFlat.push(permFname);
      additionalIncludesPermTable.push({g: group, p: perm});

      ++perm;
    }

    ++group;
  }

  additionalIncludesFn = additionalIncludesFnFlat;

  pluginName = offlineSettings.pluginName;

  var subgraphs = collectSubgraphs(offlineSettings.globalSubgraphsDir, getFilename(offlineSettings.outputFileName));

  offlineBuild(
    offlineSettings.rootFileName, offlineSettings.outputFileName + ".x",
    subgraphs, additionalIncludesFn, additionalIncludesPermTable);
  renameFile(offlineSettings.outputFileName + ".x", offlineSettings.outputFileName);
}

offlineRun();
