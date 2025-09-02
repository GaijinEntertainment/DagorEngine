"use strict";

var clientId = "offline";
var errorState = false;
var attached = false;
var lastErrorString = null;
var prevErrorString = null;
var offlineQueryQueue = [];
var queryFileCache = {};
var offlineSubgraphs = "";
var offlineOutputFile = "";

var document =
{
  getElementById : function(s) { return null; }
}

var console =
{
  log : function(s) { print(s); }
}

function query(q, callback, body)
{
  if (!body)
    body = null;

  //print("QUERY: " + q + " BODY:" + body + "\n");

  offlineQueryQueue.push({ query: q, callback: callback, body: body });
}


function alert(msg)
{
  print("ALERT: " + msg);
}


function setTimeout(callback, msec)
{
  offlineQueryQueue.push({ query: null, callback: callback, body: null });
}


function offlineReadFileToString(filename)
{
  var utf8Decoder = new TextDecoder('utf-8');
  var buf = readFile(filename);
  var txt = utf8Decoder.decode(buf);
  return txt;
}


function offlineWriteStringToFile(filename, str)
{
  writeFile(filename, str);
}


function offlineUpdate()
{
  if (offlineQueryQueue.length == 0)
    return false;

  var q = offlineQueryQueue.shift();

  if (!q.query)
  {
    if (q.callback)
      q.callback();
    return true;
  }
  else if (q.query.indexOf("get_json_files&offline") == 0)
  {
    var fileName = q.body;
    fileName = fileName.split("SAVE-DIR").join(offlineSettings.globalSubgraphsDir);
    var txt = offlineReadFileToString(fileName);
    if (q.callback)
      q.callback(txt);

    return true;
  }
  else if (q.query.indexOf("graph&offline") == 0)
  {
    offlineWriteStringToFile(offlineOutputFile, q.body);
    return true;
  }
  else
  {
    alert(q.query);
  }

  return false;
}

function readAdditionalIncludesToString(additionalIncludesFn)
{
  var res = "";
  for (var i = 0; i < additionalIncludesFn.length; i++)
    if (additionalIncludesFn[i] !== "")
    {
      var txt = offlineReadFileToString(additionalIncludesFn[i]);
      if (i > 0)
        res += ",";
      res += txt;
    }

  return res;
}


function offlineBuild(fileName, outputFile, subgraphs, additionalIncludesFn)
{
  offlineOutputFile = outputFile;
  var graphText = offlineReadFileToString(fileName);
  var additionalIncludesStr = readAdditionalIncludesToString(additionalIncludesFn);

  typeConv = new TypeConv();
  nodeDescriptions = new NodeDescriptions();
  editor = new GraphEditor();
  editor.init();

  editor.saveDescriptionNames();
  nodeDescriptions.replaceSubgraphDescriptions("ShadersTemp", '{"descriptions":[' + subgraphs.descriptions + "]}");
  editor.restoreDescriptions();

  editor.parseAdditionalIncludes(additionalIncludesStr);
  editor.parseGraph(graphText.length > 2 ? graphText : editor.emptyGraphStr, true);
  editor.resetHistory();
  editor.onChanged(false, true);

  while (offlineUpdate()) {};
}



