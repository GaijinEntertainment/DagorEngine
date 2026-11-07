//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class TMatrix;
class Point3;

// angle: full cone angle in radians
// radius: light range, the radius of the sphere around apex that bounds the cone
// innerAttenuation: inner half angle / outer half angle.
// apex and dir are in the space of tm, which must be rigid: the silhouette lines take the camera from an orthonormalized tm.
void draw_spot_light_visualization(const TMatrix &tm, const Point3 &apex, const Point3 &dir, const float angle, float radius,
  const float innerAttenuation);

// Radius sphere and axes around the origin of light_tm, which must be rigid: the axis letters ignore its scale.
void draw_omni_light_visualization(const TMatrix &light_tm, float radius);
