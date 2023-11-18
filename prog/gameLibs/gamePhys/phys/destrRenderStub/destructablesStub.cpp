#include <gamePhys/phys/destructableRendObject.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physObject.h>
#include <gamePhys/phys/destructableObject.h>

destructables::DestrRendData *destructables::init_rend_data(DynamicPhysObjectClass<PhysWorld> *) { return nullptr; }
void destructables::clear_rend_data(destructables::DestrRendData *) {}
void destructables::before_render(const Point3 &) {}
void destructables::render(dynrend::ContextId, const Frustum &, float) {}
