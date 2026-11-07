// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <math/dag_Point3.h>
#include <EASTL/string.h>

namespace dapcg
{
// MAXMATTEXNUM of a .dag material
inline constexpr int MATERIAL_TEX_SLOTS = 16;
// dagrender looks, the .dag names of 0..4
inline constexpr const char *LOOKS[] = {"paint", "chrome", "matte", "glass", "metallic"};

struct Material
{
  int id = 0;
  eastl::string name;
  eastl::string shader;                  // a shader class of the bin dump
  eastl::string tex[MATERIAL_TEX_SLOTS]; // texture asset names, empty: none
  dag::Vector<eastl::string> script;     // name=value lines
  eastl::string look;                    // dagrender's look, empty: none
  bool twoSided = false;
  Point3 diffuse = Point3(1, 1, 1);
};

// Each check returns nullptr or the broken rule
const char *check_new_material(dag::ConstSpan<Material> earlier, int id, const char *name, const char *shader);
const char *check_texture(int slot, const char *name);
const char *check_script_line(const char *line);
const char *check_look(const char *look);
const char *check_diffuse(const Point3 &c);
// a .dag default name's id, else -1
int default_name_id(const char *name);
// all rows; bad: the first failing row, else table.size()
const char *check_material(dag::ConstSpan<Material> table, uint32_t &bad);
// the lines, then look=<look>, joined by \r\n
eastl::string material_script(const Material &m);
} // namespace dapcg
