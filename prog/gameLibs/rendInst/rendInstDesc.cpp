// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstDesc.h>
#include "riGen/riUtil.h"
#include "riGen/riGenExtra.h"

bool rendinst::isRiGenInWorld(const rendinst::RendInstDesc &desc)
{
  if (DAGOR_UNLIKELY(!desc.isValid()))
    return false;

  if (desc.isRiExtra())
    return desc.pool < rendinst::riExtra.size() && rendinst::riExtra[desc.pool].isInGrid(desc.idx);
  else
    return !riutil::is_rendinst_data_destroyed(desc);
}

bool rendinst::isRiGenDescValid(const rendinst::RendInstDesc &desc)
{
  if (DAGOR_UNLIKELY(!desc.isValid()))
    return false;

  if (desc.isRiExtra())
    return desc.pool < rendinst::riExtra.size() && rendinst::riExtra[desc.pool].isValid(desc.idx);
  else
    return !riutil::is_rendinst_data_destroyed(desc);
}

bool rendinst::isRgLayerPrimary(const RendInstDesc &desc) { return rendinst::isRgLayerPrimary(desc.layer); }
