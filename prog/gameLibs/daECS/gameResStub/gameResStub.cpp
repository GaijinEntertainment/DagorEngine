#include <daECS/core/ecsGameRes.h>

namespace ecs
{

bool load_gameres_list(const gameres_list_t &) { return true; }
bool filter_out_loaded_gameres(gameres_list_t &) { return false; }
void place_gameres_request(eastl::vector<ecs::EntityId> &&, gameres_list_t &&) {}

} // namespace ecs
