// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class GeomObject;
class ShaderMesh;

extern void (*stat_geom_on_remove)(GeomObject *p);
extern void (*stat_geom_on_water_add)(GeomObject *p, const ShaderMesh *m);
