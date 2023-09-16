// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

/** \addtogroup de3Common
  @{
*/

#include <math/dag_color.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <ioSys/dag_genIo.h>

/**
\brief Scene environment settings class.
*/

class SceneEnviSettings
{
public:
  /**
  \brief Constructor
  */
  SceneEnviSettings()
  {
    zRange[0] = 1;
    zRange[1] = 1000;
    useFog = false;
    fogExp2 = false;
    sunFogColor = fogColor = E3DCOLOR(255, 255, 255, 255);
    fogDensity = 0.0;
    skyScale = 1.0;
    envyRotY = 0;
    sunFogDensity = 0;
    sunAzimuth = 0;
    lfogSkyColor = lfogSunColor = E3DCOLOR(255, 255, 255, 255);
    lfogDensity = 0;
    lfogMinHeight = 0;
    lfogMaxHeight = 1;

    fx.fogDensityFactor = 1;
    fx.fogColor = Color3(0, 0, 0);
    fx.fogColorBlend = 0;
    fx.fogSunUseFactor = 1;

    sunColor1 = sunColor0 = skyColor = Color3(0, 0, 0);
    sunDir0 = Point3(0, -1, 0);
    sunDir1 = Point3(0, 1, 0);
  }

  /**
  \brief Loads settings from binary stream.

  \param[in] crd #IGenLoad stream.

  @see IGenLoad
  */
  void loadFromBinary(IGenLoad &crd)
  {
    useFog = crd.readInt();
    fogExp2 = crd.readInt();
    crd.readReal(fogDensity);
    crd.read(&fogColor, sizeof(fogColor));
    zRange[0] = crd.readReal();
    zRange[1] = crd.readReal();

    if (crd.getBlockRest() > 0)
      skyScale = crd.readReal();

    if (crd.getBlockRest() > 0)
    {
      crd.readReal(sunFogDensity);
      crd.read(&sunFogColor, sizeof(sunFogColor));
      crd.readReal(sunAzimuth);
      // sunAzimuth += PI;
    }

    if (crd.getBlockRest() > 0)
      crd.readReal(envyRotY);

    if (crd.getBlockRest() > 0)
    {
      crd.read(&lfogSkyColor, sizeof(lfogSkyColor));
      crd.read(&lfogSunColor, sizeof(lfogSunColor));
      crd.readReal(lfogDensity);
      crd.readReal(lfogMinHeight);
      crd.readReal(lfogMaxHeight);
    }
  }

  void applyOnRender(bool apply_zrange = true);
  void setBiDirLighting();

public:
  bool useFog, fogExp2;
  E3DCOLOR fogColor, sunFogColor;
  real fogDensity;
  real skyScale;
  Point2 zRange;
  real envyRotY;
  real sunFogDensity;
  real sunAzimuth;

  E3DCOLOR lfogSkyColor;
  E3DCOLOR lfogSunColor;
  float lfogDensity;
  float lfogMinHeight;
  float lfogMaxHeight;

  struct Fx
  {
    real fogDensityFactor;
    Color3 fogColor;
    real fogColorBlend;
    real fogSunUseFactor;
  } fx;

  Color3 skyColor;
  Point3 sunDir0;
  Color3 sunColor0;
  Point3 sunDir1;
  Color3 sunColor1;

public:
  static void init();

  static int fromSunDirectionGvId;
  static int sunColor0GvId;
  static int sunLightDir1GvId;
  static int sunColor1GvId;
  static int skyColorGvId;
  static int sunFogColorGvId;
  static int skyFogColorGvId;
  static int skyScaleGvId;
};
/** @} */
