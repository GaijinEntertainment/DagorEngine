//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>

inline TMatrix cube_matrix(const TMatrix &heroTm, int face_no)
{
  G_ASSERT(face_no >= 0 && face_no < 6);

  TMatrix cameraMatrix;
  switch (face_no)
  {
    case 0:
      cameraMatrix.setcol(0, -heroTm.getcol(2));
      cameraMatrix.setcol(1, heroTm.getcol(1));
      cameraMatrix.setcol(2, heroTm.getcol(0));
      break;
    case 1:
      cameraMatrix.setcol(0, heroTm.getcol(2));
      cameraMatrix.setcol(1, heroTm.getcol(1));
      cameraMatrix.setcol(2, -heroTm.getcol(0));
      break;
    case 2:
      cameraMatrix.setcol(0, heroTm.getcol(0));
      cameraMatrix.setcol(1, -heroTm.getcol(2));
      cameraMatrix.setcol(2, heroTm.getcol(1));
      break;
    case 3:
      cameraMatrix.setcol(0, heroTm.getcol(0));
      cameraMatrix.setcol(1, heroTm.getcol(2));
      cameraMatrix.setcol(2, -heroTm.getcol(1));
      break;
    case 4:
      cameraMatrix.setcol(0, heroTm.getcol(0));
      cameraMatrix.setcol(1, heroTm.getcol(1));
      cameraMatrix.setcol(2, heroTm.getcol(2));
      break;
    case 5:
      cameraMatrix.setcol(0, -heroTm.getcol(0));
      cameraMatrix.setcol(1, heroTm.getcol(1));
      cameraMatrix.setcol(2, -heroTm.getcol(2));
      break;
  }
  cameraMatrix.setcol(3, heroTm.getcol(3));
  return cameraMatrix;
}
