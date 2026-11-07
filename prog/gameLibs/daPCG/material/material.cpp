// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "material_api.h"
#include <math/dag_check_nan.h>
#include <EASTL/algorithm.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

namespace dapcg
{
template <typename Allowed>
static bool text_of(const char *s, Allowed allowed)
{
  const size_t len = strlen(s);
  return len >= 1 && len <= 255 && eastl::all_of(s, s + len, allowed); // 255: a .dag length byte
}

int default_name_id(const char *name)
{
  const auto look = eastl::find_if(eastl::begin(LOOKS), eastl::end(LOOKS), [&](const char *l) { return strcmp(l, name) == 0; });
  if (look != eastl::end(LOOKS))
    return int(look - LOOKS);
  char *end = nullptr;
  const unsigned long id = strncmp(name, "mat", 3) == 0 && isdigit((unsigned char)name[3]) ? strtoul(name + 3, &end, 10) : 0;
  return end && !*end && id <= 0xFFFF ? int(id) : -1;
}

const char *check_new_material(dag::ConstSpan<Material> earlier, int id, const char *name, const char *shader)
{
  if (id < 0 || id > 0xFFFF)
    return "the id is not in 0..65535";
  if (!text_of(name, [](char c) { return c >= ' ' && c <= '~'; }))
    return "the name is not 1 to 255 bytes of printable ASCII";
  const int fixed = default_name_id(name);
  if (fixed >= 0 && fixed != id)
    return "the name is the .dag default name of another id";
  if (!text_of(shader, [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'; }))
    return "the shader is not 1 to 255 bytes of A-Z, a-z, 0-9 and _";
  for (const Material &m : earlier)
    if (m.id == id || m.name == name)
      return m.id == id ? "the id is used twice" : "the name is used twice";
  return nullptr;
}

const char *check_texture(int slot, const char *name)
{
  if (slot < 0 || slot >= MATERIAL_TEX_SLOTS)
    return "the slot is not in 0..15";
  if (!text_of(name, [](char c) { return c != '\r' && c != '\n'; }))
    return "the texture name is not 1 to 255 bytes without a line break";
  return nullptr;
}

const char *check_script_line(const char *line)
{
  if (strpbrk(line, "\r\n"))
    return "a script line has a line break";
  const char *key = line + strspn(line, " \t"), *end = key + strcspn(key, " \t=");
  return end > key && !strchr("#';/[", *key) && end[strspn(end, " \t")] == '=' ? nullptr : "a script line is not name=value";
}

const char *check_look(const char *look)
{
  const bool known = eastl::any_of(eastl::begin(LOOKS), eastl::end(LOOKS), [&](const char *l) { return strcmp(l, look) == 0; });
  return known ? nullptr : "the look is not paint, chrome, matte, glass or metallic";
}

const char *check_diffuse(const Point3 &c)
{
  const auto in_range = [](float v) { return check_finite(v) && v >= 0 && v <= 1; };
  return in_range(c.x) && in_range(c.y) && in_range(c.z) ? nullptr : "the diffuse is not in 0..1";
}

const char *check_material(dag::ConstSpan<Material> table, uint32_t &bad)
{
  for (bad = 0; bad < table.size(); ++bad)
  {
    const Material &m = table[bad];
    const char *err = check_new_material(table.first(bad), m.id, m.name.c_str(), m.shader.c_str());
    for (int slot = 0; !err && slot < MATERIAL_TEX_SLOTS; ++slot)
      err = m.tex[slot].empty() ? nullptr : check_texture(slot, m.tex[slot].c_str());
    for (uint32_t i = 0; !err && i < m.script.size(); ++i)
      err = check_script_line(m.script[i].c_str());
    if (!err && !m.look.empty())
      err = check_look(m.look.c_str());
    if (!err)
      err = check_diffuse(m.diffuse);
    if (err)
      return err;
  }
  return nullptr;
}

eastl::string material_script(const Material &m)
{
  eastl::string s;
  for (uint32_t i = 0; i < m.script.size(); ++i)
    s.append_sprintf("%s%s", i ? "\r\n" : "", m.script[i].c_str());
  if (!m.look.empty())
    s.append_sprintf("%slook=%s", m.script.empty() ? "" : "\r\n", m.look.c_str());
  return s;
}
} // namespace dapcg
