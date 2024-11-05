// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <supp/dag_define_KRNLIMP.h>

//! clears global map (all pointers/IDs returned earlier become invalid after clear)
KRNLIMP void dagor_fname_map_clear();

//! adds (or finds existing) filename to global map and returns persistent pointer to filename string
KRNLIMP const char *dagor_fname_map_add_fn(const char *fn);

//! finds (and optionally adds, when missing) filename in global map and returns its ID
KRNLIMP int dagor_fname_map_get_fn_id(const char *fn, bool allow_add = false);
//! resolved filename ID in global map and returns persistent pointer to filename string
KRNLIMP const char *dagor_fname_map_resolve_id(int id);

//! returns number of registered filenames (and, optionally, memory consumption)
KRNLIMP int dagor_fname_map_count(int *out_mem_allocated = 0, int *out_mem_used = 0);

#include <supp/dag_undef_KRNLIMP.h>
