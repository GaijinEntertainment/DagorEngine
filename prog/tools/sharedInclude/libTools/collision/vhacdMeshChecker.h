// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/dagFileRW/geomMeshHelper.h>
#include <math/dag_plane3.h>

bool check_face(const Tab<Point3> &verts, const Plane3 &plane, const BSphere3 &boundingSphere);

void fix_vhacd_faces(GeomMeshHelperDagObject &collision_object);

bool is_vhacd_has_not_valid_faces(const Tab<Point3> &verts, const Tab<int> &indices);
