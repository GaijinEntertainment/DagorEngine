// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/shaderResBuilder/shSkinMeshData.h>
#include <generic/dag_span.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>

struct Face;

struct CorSkinWeights
{
  int16_t bone[MAX_SKINBONES];
  float weight[MAX_SKINBONES];
  int count;
};

// Optimized centers of rotation (Le and Hodgins, SIGGRAPH 2016) for the rest pose in vert; skin and
// out_cor_offset hold one entry per vert. skin[i] lists only the bones with a weight above zero (a zero
// weight costs time, not accuracy), with the weights the shader blends: on a four-bone skin the quantized
// E3DCOLOR weights with the fourth one as 1-x-y-z, or the centers disagree with the skinned vertex.
// face must list each surface triangle once: a repeated face counts its area twice.
// out_cor_offset[i] is the center minus vert[i], with w zero; it is zero when fewer than
// two bones of the vertex have a weight above zero, or when no triangle shares two of them.
void compute_centers_of_rotation(dag::ConstSpan<Point3> vert, dag::ConstSpan<Face> face, dag::ConstSpan<CorSkinWeights> skin,
  dag::Span<Point4> out_cor_offset);
