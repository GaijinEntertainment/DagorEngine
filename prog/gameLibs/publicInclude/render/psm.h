//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_TMatrix4.h>
#include <math/dag_bounds3.h>
#include <generic/dag_tab.h>


enum
{
  SHADOWTYPE_TSM,
  SHADOWTYPE_XPSM,
  SHADOWTYPE_ORTHO,
};

struct RenderShadowObject;

// forward decl

class TSM
{
private:
  void ComputeVirtualCameraParameters(RenderShadowObject *scenes, int count, const BBox3 &ground);
  // void clipUnitCube(TMatrix4 &trapezoid_space);
  void buildTSMMatrix(const BBox3 &frustumAABB2D, const BBox3 &frustumBox, const Point3 *frustumPnts, float xi, float delta,
    const Point2 &scale, TMatrix4 &trapezoid_space);
  void BuildXPSMProjectionMatrix();
  TMatrix4 BuildClippedMatrix(const TMatrix4 &PPLSFinal, float &min_z, float &max_z, bool expand = false) const;

public:
  // data members

  Tab<BBox3> m_ShadowCasterPointsLocal;
  Tab<BBox3> m_ShadowReceiverPointsLocal;

  int m_iShadowType;
  bool m_bUnitCubeClip;
  bool m_bSlideBack;
  bool isOrthoNow;
  float m_zNear, m_zFar, m_fCosGamma;

  // various bias values
  float m_fTSM_Delta; // if m_fTSM_Delta < 0, it is % of zfar-znear (point of interest)
  float m_xi;         // tweackable param, -0.6 (80% line)
  BBox3 boxOfInterest;

  float m_XPSM_Coef;     // = 0.06f;
  float m_XPSM_Bias;     // = 0.55f;
  float m_XPSM_EpsilonW; // = 0.95f;
  float m_XPSM_ZBias;    // out


  TMatrix4 m_View;
  TMatrix4 m_Projection;
  float view_angle_wk, view_angle_hk; // for psm
  float zfar_max;                     // for psm
  float znear_min;                    // for psm
  float max_z_dispertion;

  Point3 m_lightDir;
  Tab<RenderShadowObject *> m_ShadowCasterObjects;
  Tab<RenderShadowObject *> m_ShadowReceiverObjects;

  TSM();
  void close();
  TMatrix4 buildMatrix(RenderShadowObject *scenes, int count, const BBox3 &ground);
  TMatrix4 m_lightView, m_lightProj; // define lightspace
  TMatrix4 m_LightViewProj;          // define tsm space

  void BuildTSMProjectionMatrix();
  void BuildOrthoShadowProjectionMatrix();
};
