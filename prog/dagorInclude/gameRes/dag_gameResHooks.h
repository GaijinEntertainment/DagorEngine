//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>

class GameResource;
class GameResourceFactory;
class String;
typedef const class GameResourceDummyHandleType *GameResHandle;


//! gameres hooks are used to customize resource creation/destruction and for logging;
//! if hook returns false, default implementation is used
namespace gamereshooks
{
//! called to resolve GameResHandle:class_id to unique res_id;
extern bool (*resolve_res_handle)(GameResHandle rh, unsigned class_id, int &out_res_id);

//! called to resolve GameResHandle:class_id to unique res_id;
extern bool (*get_res_refs)(int res_id, Tab<int> &out_refs);


//! called when res_id validation is needed;
extern bool (*on_validate_game_res_id)(int res_id, int &out_res_id);

//! called when resource class id is needed;
extern bool (*on_get_game_res_class_id)(int res_id, unsigned &out_class_id);

//! called from get_game_resource() and get_game_resource_ex();
extern bool (*on_get_game_resource)(int res_id, dag::Span<GameResourceFactory *> f, GameResource *&out_res);

//! called from release_game_resource();
extern bool (*on_release_game_resource)(int res_id, dag::Span<GameResourceFactory *> f);

//! called from unreliable version of release_game_resource() by GameResHandle;
extern bool (*on_release_game_res2)(GameResHandle rh, dag::Span<GameResourceFactory *> f);

//! called from when resource class id is needed;
extern bool (*on_load_game_resource_pack)(int res_id, dag::Span<GameResourceFactory *> f);

//! called to resolve GameResHandle:class_id to unique res_id;
extern bool (*on_get_res_name)(int res_id, String &out_res_name);

//! not really hook, but helper function to be used in tools
int aux_game_res_handle_to_id(GameResHandle handle, unsigned class_id);
} // namespace gamereshooks
