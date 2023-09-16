#include <render/cables.h>

Cables *get_cables_mgr() { return nullptr; }
void init_cables_mgr() {}
void close_cables_mgr() {}

void Cables::loadCables(IGenLoad &) {}
void Cables::onRIExtraDestroyed(const TMatrix &, const BBox3 &) {}