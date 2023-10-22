#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>


const CollisionResource *lru_collision_get_collres(uint32_t i) { return rendinst::getRIGenExtraCollRes(i); }
mat43f lru_collision_get_transform(rendinst::riex_handle_t h) { return rendinst::getRIGenExtra43(h); }
uint32_t lru_collision_get_type(rendinst::riex_handle_t h) { return rendinst::handle_to_ri_type(h); }
