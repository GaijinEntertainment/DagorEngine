//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_e3dColor.h>
#include <math/dag_Point3.h>
#include <sceneRay/dag_sceneRayDecl.h>


#define MAX_VISCLIPMESH_FACETS 1024


//************************************************************************
//* forwards
//************************************************************************
class CfgReader;
class FastRtDump;
class FastRtDumpManager;

struct VisClipMeshVertex
{
  Point3 pos;
  E3DCOLOR color;
};


bool create_visclipmesh(CfgReader &cfg, bool for_game = false);
void delete_visclipmesh(void);

void render_visclipmesh(const FastRtDump &cliprtr, const Point3 &pos);
void render_visclipmesh(const FastRtDumpManager &cliprtr, const Point3 &pos);
void render_visclipmesh(StaticSceneRayTracer &rt, const Point3 &pos);

float get_vcm_rad();
void set_vcm_rad(float rad);

bool is_vcm_visible();
void set_vcm_visible(bool visible);
int set_vcm_draw_type(int type);

// call it after all objects are rendered (after all)
void render_visclipmesh_info(bool need_start_render);
