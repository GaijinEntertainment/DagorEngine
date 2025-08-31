// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class DagorEdPluginData
{
public:
  // needed for:
  //   DagorEdAppWindow::disablePluginsRender and
  //   DagorEdAppWindow::enablePluginsRender
  bool oldVisible;
  bool doExport;
  bool doExportFilter;
  bool externalExport;
  bool externalSource;

  void *dll;
  String dllPath;

  String externalPath;

  // plugin's hotkey (or -1 if not presented)
  int hotkey;

  DagorEdPluginData() :
    oldVisible(false), doExport(true), doExportFilter(true), externalExport(false), hotkey(-1), dll(NULL), externalSource(false)
  {}
};
