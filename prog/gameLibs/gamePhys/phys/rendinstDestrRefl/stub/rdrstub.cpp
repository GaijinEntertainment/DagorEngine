#include <gamePhys/phys/rendinstDestrRefl.h>

namespace rendinstdestr
{

void init_refl_object(mpi::ObjectID) {}
void destroy_refl_object() {}
void enable_reflection_refl_object(bool) {}
mpi::IObject *get_refl_object() { return nullptr; }

} // namespace rendinstdestr
