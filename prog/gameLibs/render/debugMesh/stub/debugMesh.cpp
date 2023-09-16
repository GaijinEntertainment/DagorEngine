#include <render/debugMesh.h>


namespace debug_mesh
{
bool enable_debug(Type) { return false; }
bool is_enabled(Type) { return false; }

bool set_debug_value(int) { return false; }
void reset_debug_value() {}

void activate_mesh_coloring_master_override() {}
void deactivate_mesh_coloring_master_override() {}
} // namespace debug_mesh
