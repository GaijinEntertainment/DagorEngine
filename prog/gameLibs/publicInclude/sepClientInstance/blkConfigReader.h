//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sepClientInstance/sepClientInstanceTypes.h>


class DataBlock;

namespace sepclientinstance::configreader
{

typedef const DataBlock *(*get_config_cb_t)();

SepClientInstanceConfig from_circuit_blk_config(get_config_cb_t get_circuit_config_cb);

} // namespace sepclientinstance::configreader
