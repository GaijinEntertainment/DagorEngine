// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>

#include <util/dag_string.h>
#include <util/dag_globDef.h>

#include <math/dag_Point3.h>


struct Prefab
{
  real angle;
  Point3 pos;
  String name;

  Prefab() : angle(0.0), pos(0.0, 0.0, 0.0), name("Object") {}
};


void show_usage();
void init_dagor(const char *base_path);

bool create_prefabs_list(const char *map, Tab<Prefab> &prefabs, bool check_models);
bool generate_file(const char *path, const Tab<Prefab> &prefabs, const char *lib_path, bool show_debug);
