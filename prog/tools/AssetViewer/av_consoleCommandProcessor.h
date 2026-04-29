// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_console.h>

class AssetViewerConsoleCommandProcessor : public console::ICommandProcessor
{
public:
  using ICommandProcessor::ICommandProcessor;

  void destroy() override { delete this; }

  bool processCommand(const char *argv[], int argc) override;

  static void runShadersReload(dag::ConstSpan<const char *> params);

private:
  static void runCameraAt(dag::ConstSpan<const char *> params);
  static void runCameraDir(dag::ConstSpan<const char *> params);
  static void runCameraPos(dag::ConstSpan<const char *> params);
  static void runPerfDump();
  static void runShadersListVars(dag::ConstSpan<const char *> params);
  static void runShadersSetVar(dag::ConstSpan<const char *> params);
  static void runTexInfo(dag::ConstSpan<const char *> params);
  static void runTexRefs();
  static void runTexShow(const char *argv[], int argc);
  static void runTexHide();
  static void runSelectAsset(dag::ConstSpan<const char *> params);
};
