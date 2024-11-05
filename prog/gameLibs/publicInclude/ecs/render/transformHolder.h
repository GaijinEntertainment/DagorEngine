//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

struct TransformHolder
{
  TMatrix viewTm;
  TMatrix4 projTm;
  TMatrix4 globTm;

  TransformHolder(const TMatrix &view_tm, const TMatrix4 &proj_tm) : viewTm(view_tm), projTm(proj_tm)
  {
    globTm = TMatrix4(viewTm) * projTm;
  }

  mat44f loadViewTm() const
  {
    mat44f ssViewTm;
    v_mat44_make_from_43cu(ssViewTm, viewTm.array);
    return ssViewTm;
  }

  mat44f loadProjTm() const
  {
    mat44f ssProjTm;
    v_mat44_make_from_44cu(ssProjTm, &projTm._11);
    return ssProjTm;
  }

  mat44f loadGlobTm() const
  {
    mat44f ssGlobTm;
    v_mat44_make_from_44cu(ssGlobTm, &globTm._11);
    return ssGlobTm;
  }
};