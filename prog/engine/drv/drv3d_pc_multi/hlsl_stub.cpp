#include <3d/dag_drv3d_pc.h>
#include <3d/dag_drv3d.h>

VPROG d3d::create_vertex_shader_hlsl(const char *, unsigned, const char *, const char *, String *) { return 1; }
FSHADER d3d::create_pixel_shader_hlsl(const char *, unsigned, const char *, const char *, String *) { return 1; }

bool d3d::compile_compute_shader_hlsl(const char *, unsigned, const char *entry, const char *profile, Tab<uint32_t> &, String &)
{
  logerr("CS compile failed: %s, %s", entry, profile);
  return false;
}
