#include <gameRes/dag_gameResHooks.h>

bool (*gamereshooks::resolve_res_handle)(GameResHandle rh, unsigned class_id, int &out_res_id) = 0;
bool (*gamereshooks::get_res_refs)(int res_id, Tab<int> &out_refs) = 0;
bool (*gamereshooks::on_get_game_res_class_id)(int res_id, unsigned &out_class_id) = 0;
bool (*gamereshooks::on_validate_game_res_id)(int res_id, int &out_res_id) = 0;
bool (*gamereshooks::on_get_game_resource)(int res_id, dag::Span<GameResourceFactory *> f, GameResource *&out_res) = 0;
bool (*gamereshooks::on_release_game_resource)(int res_id, dag::Span<GameResourceFactory *> f) = 0;
bool (*gamereshooks::on_release_game_res2)(GameResHandle rh, dag::Span<GameResourceFactory *> f) = 0;
bool (*gamereshooks::on_load_game_resource_pack)(int res_id, dag::Span<GameResourceFactory *> f) = 0;
bool (*gamereshooks::on_get_res_name)(int res_id, String &out_res_name) = 0;

// private (hidden) hooks
namespace gamereshooks
{
void (*on_load_res_packs_from_list_complete)(const char *pack_list_blk_fname, bool load_grp, bool load_tex) = 0;
}
