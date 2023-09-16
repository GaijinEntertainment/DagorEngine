//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tabFwd.h>
class Mesh;
class TMatrix;
class Point3;

extern bool ignore_mapping_in_prepare_billboard_mesh;
extern bool generate_extra_in_prepare_billboard_mesh;


void prepare_billboard_mesh(Mesh &mesh, const TMatrix &wtm, dag::ConstSpan<Point3> org_verts);
