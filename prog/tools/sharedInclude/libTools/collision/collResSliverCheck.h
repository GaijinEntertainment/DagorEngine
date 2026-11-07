// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameRes/dag_collisionResource.h>
#include <ioSys/dag_genIo.h>
#include <debug/dag_logSys.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <util/dag_globDef.h>
#include <EASTL/unique_ptr.h>
#include <stdio.h>

// Returns null and prints the error when the stream does not load. The loader refuses a stream with a logged error and an empty
// resource; a landed one can log errors too.
inline eastl::unique_ptr<CollisionResource> load_collres_checked(IGenLoad &crd, const char *name)
{
  static int errors;
  errors = 0;
  const LogHandlerId countErrors = debug_add_log_handler([](void *, LogMessage &) { errors++; }, nullptr, {.levelMask = LOGMASK_ERR});
  eastl::unique_ptr<CollisionResource> coll(new CollisionResource(crd, -1, name));
  debug_remove_log_handler(countErrors);
  if (errors && coll->getAllNodes().empty())
    coll.reset();
  if (!coll)
    printf("ERROR: %s does not load\n", name);
  return coll;
}

// A sliver, on the decoded verts Jolt gets: (2*area)^2 <= 1e-12 (Jolt IsNearZero), or its apex is under 0.1 mm from the
// longest edge. The Jolt collide path asserts on its normal, which is float32 noise.
struct CollResSliverStats
{
  static constexpr double JOLT_CROSS_SQ = 1e-12;
  static constexpr double H_MIN = 1e-4;

  int faces = 0, bad = 0, jolt = 0, convexBad = 0;

  void print() const { printf("  %d faces, %d slivers (%d Jolt-degenerate), %d more in convex nodes\n", faces, bad, jolt, convexBad); }
};

// Prints each sliver with its node, face index and corners in the resource frame, prefixed with res_name when set
inline void check_collres_slivers(const CollisionResource &coll, CollResSliverStats &st, const char *res_name = nullptr,
  bool print_faces = true)
{
  for (uint16_t ni : coll.meshNodes())
  {
    const CollisionNode &node = coll.getAllNodes()[ni];
    const TMatrix tm = coll.getNodeTm(ni);
    const char *name = coll.getNodeNameStr(node);
    coll.iterateNodeFacesVerts(ni, [&](int fi, vec4f v0, vec4f v1, vec4f v2) {
      Point3 p[3];
      v_stu_p3(&p[0].x, v0);
      v_stu_p3(&p[1].x, v1);
      v_stu_p3(&p[2].x, v2);
      st.faces++;
      const DPoint3 d0(p[0]), d1(p[1]), d2(p[2]);
      const DPoint3 e10 = d1 - d0, e20 = d2 - d0, e21 = d2 - d1;
      const double crossSq = lengthSq(e10 % e20), fa2 = sqrt(crossSq);
      const double apexHeight = fa2 / sqrt(max(max(lengthSq(e10), lengthSq(e20)), max(lengthSq(e21), 1e-60)));
      const bool jolt = crossSq <= st.JOLT_CROSS_SQ;
      if (!jolt && apexHeight >= st.H_MIN)
        return;
      // a pack's convex node becomes a hull of its points (PhysConvexHullCollision), a level's goes as triangles: the caller counts
      const bool convex = node.type == COLLISION_NODE_TYPE_CONVEX;
      (convex ? st.convexBad : st.bad)++;
      st.jolt += jolt && !convex;
      if (!print_faces)
        return;
      Point3 w[3] = {tm * p[0], tm * p[1], tm * p[2]};
      printf("  %s%snode '%s' face %d: (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) 2*area %.3g h %.3g%s%s\n",
        res_name ? res_name : "", res_name ? " " : "", name, fi, P3D(w[0]), P3D(w[1]), P3D(w[2]), fa2, apexHeight,
        jolt ? " JOLT-DEGENERATE" : "", convex ? " CONVEX" : "");
    });
  }
}
