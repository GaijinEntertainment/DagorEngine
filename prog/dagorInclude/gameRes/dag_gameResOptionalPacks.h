//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>
#include <generic/dag_tabFwd.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_multicastEvent.h>

// forward decl for used types
class GameResource;


namespace gameres_optional
{
enum class PackId : unsigned short
{
  Invalid = 0xFFFFu
};

extern MulticastEvent<void(const GameResource *, int res_id, const char *res_name)> on_res_updated;

//! gathers missing optional packs for specified list of gameres (optionally includes missing tex packs for models);
//! returns true when 1 or more missing packs are written to out_pack_ids
bool get_missing_packs_for_res(dag::ConstSpan<const char *> res_names, Tab<PackId> &out_pack_ids, bool incl_model_tex = true,
  bool incl_hq = true);
bool get_missing_packs_for_res(const FastNameMap &res_names, Tab<PackId> &out_pack_ids, bool incl_model_tex = true,
  bool incl_hq = true);

//! gathers missing optional tex packs for specified list of textures;
//! returns true when 1 or more missing packs are written to out_pack_ids
bool get_missing_packs_for_tex(dag::ConstSpan<TEXTUREID> tid, Tab<PackId> &out_pack_ids, bool incl_hq = true);
//! gathers missing optional tex packs for currently used textures
bool get_missing_packs_for_used_tex(Tab<PackId> &out_pack_ids, bool incl_hq = true);

//! return name of optional pack by ID
const char *get_pack_name(PackId pack_id);
//! resolve optional pack_name into ID or return PackId::Invalid if pack_name is not known
PackId get_pack_name_id(const char *pack_name);

//! tell gameres system that packs are ready to be scanned and used;
//! automatically discards earlier version of models/textures that were created using other packs
//! returns true if any resource/texture was updated and reported via callback
bool update_gameres_from_ready_packs(dag::ConstSpan<PackId> pack_ids);
} // namespace gameres_optional
