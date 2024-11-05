//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
class IGenSave;

enum BsdiffStatus
{
  BSDIFF_OK = 0,
  BSDIFF_INVALID_COMPRESSION,
  BSDIFF_INTERNAL_ERROR,
  BSDIFF_COMPRESSION_ERROR,
  BSDIFF_DECOMPRESSION_ERROR,
  BSDIFF_CORRUPT_PATCH,
  BSDIFF_NOMEM,
  BSDIFF_INVALID_VROMFS,
};

BsdiffStatus create_vromfs_bsdiff(dag::ConstSpan<char> old_dump, dag::ConstSpan<char> new_dump, IGenSave &cwr_diff);
BsdiffStatus apply_vromfs_bsdiff(Tab<char> &out_new_dump, dag::ConstSpan<char> old_dump, dag::ConstSpan<char> diff);

BsdiffStatus create_bsdiff(const char *oldData, int oldSize, const char *newData, int newSize, Tab<char> &diff, const char *comprName);
unsigned check_bsdiff_header(const char *diff, int diffSize); //< returns size of header or 0 if check failed
BsdiffStatus apply_bsdiff(const char *oldData, int oldSize, const char *diff, int diffSize, Tab<char> &newData);
const char *bsdiff_status_str(BsdiffStatus status);
