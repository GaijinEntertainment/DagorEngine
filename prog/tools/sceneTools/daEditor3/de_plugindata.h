// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __GAIJIN_DAGORED_DE_PLUGINDATA_H__
#define __GAIJIN_DAGORED_DE_PLUGINDATA_H__

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

  String externalPath;

  // plugin's hotkey (or -1 if not presented)
  int hotkey;

  DagorEdPluginData() :
    oldVisible(false), doExport(true), doExportFilter(true), externalExport(false), hotkey(-1), dll(NULL), externalSource(false)
  {}
};

#endif //__GAIJIN_DAGORED_DE_PLUGINDATA_H__
