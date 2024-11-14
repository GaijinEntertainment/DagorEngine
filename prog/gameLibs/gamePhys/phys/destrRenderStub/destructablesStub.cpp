// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/destructableRendObject.h>
#include <phys/dag_physDecl.h>
#include <phys/dag_physObject.h>
#include <gamePhys/phys/destructableObject.h>

void destructables::DestrRendDataDeleter::operator()(destructables::DestrRendData *) {}
destructables::DestrRendData *destructables::init_rend_data(DynamicPhysObjectClass<PhysWorld> *, bool) { return nullptr; }
void destructables::clear_rend_data(destructables::DestrRendData *) {}
void destructables::before_render(const Point3 &, bool) {}
void destructables::render(dynrend::ContextId, const Frustum &, float) {}
