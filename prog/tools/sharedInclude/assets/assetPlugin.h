//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_dynLib.h>

class DataBlock;
class IGenSave;
class ILogWriter;
class DagorAsset;
class IDagorAssetExporter;
class IDagorAssetRefProvider;


// daBuild plugin DLL interface
class IDaBuildPlugin
{
public:
  virtual bool __stdcall init(const DataBlock &appblk) = 0;
  virtual void __stdcall destroy() = 0;

  virtual int __stdcall getExpCount() = 0;
  virtual const char *__stdcall getExpType(int idx) = 0;
  virtual IDagorAssetExporter *__stdcall getExp(int idx) = 0;

  virtual int __stdcall getRefProvCount() = 0;
  virtual const char *__stdcall getRefProvType(int idx) = 0;
  virtual IDagorAssetRefProvider *__stdcall getRefProv(int idx) = 0;
};


//! root DLL function must be declared as:
//!   extern "C" __declspec(dllexport) IDaBuildPlugin* __stdcall get_dabuild_plugin();
typedef IDaBuildPlugin *(__stdcall *get_dabuild_plugin_t)();

//! optional DLL function, to be declared as:
//!   extern "C" __declspec(dllexport) void __stdcall dabuild_plugin_install_dds_helper(int (__stdcall *hlp)(IGenSave &, DagorAsset &,
//!   unsigned, const char *, ILogWriter *));
typedef void(__stdcall *dabuild_plugin_install_dds_helper_t)(
  int(__stdcall *)(IGenSave &, DagorAsset &, unsigned, const char *, ILogWriter *));

#if _TARGET_64BIT
#define GET_DABUILD_PLUGIN_PROC             "get_dabuild_plugin"
#define DABUILD_PLUGIN_INSTALL_DDS_HLP_PROC "dabuild_plugin_install_dds_helper"
#else
#define GET_DABUILD_PLUGIN_PROC             "_get_dabuild_plugin@0"
#define DABUILD_PLUGIN_INSTALL_DDS_HLP_PROC "_dabuild_plugin_install_dds_helper@4"
#endif

#if DAGOR_DBGLEVEL > 1
#define DAGOR_DLL "-dbg" DAGOR_OS_DLL_SUFFIX
#else
#define DAGOR_DLL "-dev" DAGOR_OS_DLL_SUFFIX
#endif
