//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <assets/assetPlugin.h>

#if _TARGET_STATIC_LIB
struct DagorAssetExporterPlugin
{
  DagorAssetExporterPlugin *next = nullptr; // slist
  get_dabuild_plugin_t get_plugin = nullptr;
  dabuild_plugin_install_dds_helper_t install_hlp = nullptr;
  const char *name = nullptr;
  static DagorAssetExporterPlugin *tail;
  DagorAssetExporterPlugin(const char *n, get_dabuild_plugin_t p, dabuild_plugin_install_dds_helper_t h)
  {
    next = tail;
    tail = this;
    name = n;
    get_plugin = p;
    install_hlp = h;
  }
};

#define BEGIN_DABUILD_PLUGIN_NAMESPACE(X) \
  namespace dabuildExp_##X                \
  {
#define END_DABUILD_PLUGIN_NAMESPACE(X)   }
#define USING_DABUILD_PLUGIN_NAMESPACE(X) using namespace dabuildExp_##X;

#define REGISTER_DABUILD_PLUGIN(X, H)                                                \
  static DagorAssetExporterPlugin p_##X(#X, &dabuildExp_##X::get_dabuild_plugin, H); \
  int pull_##X = 1;
#define DABUILD_PLUGIN_API static

extern int pull_all_dabuild_plugins;

#else
#define BEGIN_DABUILD_PLUGIN_NAMESPACE(X)
#define END_DABUILD_PLUGIN_NAMESPACE(X)
#define USING_DABUILD_PLUGIN_NAMESPACE(X)

#define REGISTER_DABUILD_PLUGIN(X, HLP) \
  extern int pull_rtlOverride_stubRtl;  \
  int pull_rtl_mem_override_to_dll = pull_rtlOverride_stubRtl;

#if _TARGET_PC_WIN
#define DABUILD_PLUGIN_API extern "C" __declspec(dllexport)
#else
#define DABUILD_PLUGIN_API extern "C" __attribute__((visibility("default")))
#endif

#endif
