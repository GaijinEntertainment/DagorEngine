//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/genericLUT/dynamicLutParams.h>
#include <ioSys/dag_dataBlock.h>

struct DynamicLutManager
{
  Tab<DynamicLutParams> dynamicLutParamsList;
  DataBlock dynamicLutBlk;
  float tonemappingEaseInEaseOutPower;

  void setColorGradingToShader(float baseWhiteTemp, float timeOfDay, int whiteTempVarId);
  void setTonemapToShader(float timeOfDay);
  void loadBlk(const DataBlock &weatherBlk);
  DynamicLutParams getLerpedLutParams(const Tab<DynamicLutParams> &dynamicLut_array, float time, float lerping_power) const;

#if DAGOR_DBGLEVEL > 0
  char imGuiWeatherPath[255];
  void setImGuiWeatherPath(const char *path);

  void setDynamicLutParamsAt(DynamicLutParams &param, int at);
  DynamicLutParams *getDynamicLutParamsAt(int at);
  bool addNewCopyLut(float at, float keyToCopy);
  void save();
  void addNewLut(float at);
  void removeDynamicLutParamAt(int at);
  void removeAt(float at);

  void showLutWindow(double star_julian_day, double &minute_change);
#endif
};
