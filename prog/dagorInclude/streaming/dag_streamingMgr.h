//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_span.h>
#include <scene/dag_loadLevel.h>

// forward declarations of external classes
class RenderScene;


class IStreamingSceneStorage : public IBinaryDumpLoaderClient
{
public:
  // called when binary is unloaded
  virtual void delBinDump(unsigned bindump_id) = 0;

  // called each time before starting async binary loading
  // finds MINIMAL optima value among all scheduled binaries to load
  virtual float getBinDumpOptima(unsigned bindump_id) = 0;
};


class IStreamingSceneManager
{
public:
  virtual void destroy() = 0;

  virtual void setClient(IStreamingSceneStorage *cli) = 0;

  virtual void act() = 0;

  virtual int loadBinDump(const char *bindump) = 0;
  virtual int loadBinDumpAsync(const char *bindump) = 0;

  virtual void unloadBinDump(const char *bindump, bool sync) = 0;
  virtual void unloadAllBinDumps() = 0;

  virtual bool isBinDumpValid(int id) = 0;
  virtual bool isBinDumpLoaded(int id) = 0;

  virtual bool isLoading() = 0;
  virtual bool isLoadingNow() = 0;
  virtual bool isLoadingScheduled() = 0;
  virtual void clearLoadingSchedule() = 0;
  virtual void setAllowedTimeFactor(float factor) = 0;

  virtual int findSceneRec(const char *name) = 0;

  // creator funtion
  static IStreamingSceneManager *create();
};
