//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_simpleString.h>

class DataBlock;

namespace blk_util
{
bool copyBlkParam(const DataBlock &source, const int s_ind, DataBlock &dest, const char *new_name = NULL);
bool cmpBlkParam(const DataBlock &source, const int s_ind, DataBlock &dest, const int d_ind);
SimpleString paramStrValue(const DataBlock &source, const int s_ind, const bool with_param_name = true);
SimpleString blockStrValue(const DataBlock &source); // show user this
SimpleString blkTextData(const DataBlock &source);   // save or transmit this
};                                                   // namespace blk_util
