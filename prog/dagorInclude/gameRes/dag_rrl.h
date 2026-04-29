//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>

//! opaque class that contains restricted list of resources (RRL: resource restriction list)
class GameResRestrictionList;
typedef GameResRestrictionList *gameres_rrl_ptr_t;
typedef GameResRestrictionList const *gameres_rrl_cptr_t;

//! allocates resource restriction list object using optional reserve for total count and adds first res for resname
//! resname may be null (will be ignored in such case)
//! optional 'reuse_rrl' maybe passed to avoid allocating new object and just reuse existing one
gameres_rrl_ptr_t create_res_restriction_list(const char *resname, unsigned reserve_total_cnt = 0,
  gameres_rrl_ptr_t reuse_rrl = nullptr);

//! adds res to resource restriction list for given resname and returns it res_id
int extend_res_restriction_list(gameres_rrl_ptr_t rrl, const char *resname);

//! deallocates resource restriction list object
void free_res_restriction_list(gameres_rrl_ptr_t &rrl);

//! returns true if res_id is marked in resource restriction list as required
bool is_res_required(gameres_rrl_cptr_t rrl, int res_id);

//! allocates resource restriction list object for 'resname_count' resources using 'get_resname(idx)' to get their names
template <class CB>
inline gameres_rrl_ptr_t create_res_restriction_list(unsigned resname_count, CB get_resname /*(unsigned idx)*/,
  gameres_rrl_ptr_t reuse_rrl = nullptr)
{
  if (!resname_count)
    return reuse_rrl ? create_res_restriction_list(nullptr, 0, reuse_rrl) : nullptr;
  gameres_rrl_ptr_t rrl = create_res_restriction_list(get_resname(0), resname_count, reuse_rrl);
  for (unsigned idx = 1; idx < resname_count; idx++)
    extend_res_restriction_list(rrl, get_resname(idx));
  return rrl;
}

//! allocates resource restriction list object for NAMEMAP-like list of resources
template <class NAMEMAP>
inline gameres_rrl_ptr_t create_res_restriction_list(const NAMEMAP &nm, gameres_rrl_ptr_t reuse_rrl = nullptr)
{
  return create_res_restriction_list(nm.nameCount(), [&nm](unsigned idx) { return nm.getName(idx); }, reuse_rrl);
}

//! separates (moves) resources of 'sep_types' from src_rrl to sep_rrl (if nullptr passed, RRL is created);
//! returns true if src_rrl still not empty;
bool split_res_restriction_list(gameres_rrl_ptr_t src_rrl, gameres_rrl_ptr_t &sep_rrl, dag::ConstSpan<unsigned> sep_types);

//! utility class to help hold and release RRL object for scope (RAII way)
struct ScopedGameResRestrictionListHolder
{
  gameres_rrl_ptr_t rrl;

  ScopedGameResRestrictionListHolder(gameres_rrl_ptr_t _rrl) { rrl = _rrl; }
  ~ScopedGameResRestrictionListHolder() { free_res_restriction_list(rrl); }

  template <class CB>
  inline void reset(CB make_new_rrl)
  {
    free_res_restriction_list(rrl);
    rrl = make_new_rrl();
  }
  inline void reset() { free_res_restriction_list(rrl); }
};
