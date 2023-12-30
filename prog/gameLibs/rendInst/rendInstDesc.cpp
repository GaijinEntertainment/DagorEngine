#include <rendInst/rendInstDesc.h>
#include "riGen/riUtil.h"
#include "riGen/riGenExtra.h"

bool rendinst::isRiGenDescValid(const rendinst::RendInstDesc &desc)
{
  if (EASTL_UNLIKELY(!desc.isValid()))
    return false;

  if (desc.isRiExtra())
    return desc.pool < rendinst::riExtra.size() && rendinst::riExtra[desc.pool].isInGrid(desc.idx);
  else
    return !riutil::is_rendinst_data_destroyed(desc);
}

float rendinst::getTtl(const rendinst::RendInstDesc &desc)
{
  if (!desc.isValid() || !desc.isRiExtra())
    return -1.f;

  return desc.pool < rendinst::riExtra.size() ? rendinst::riExtra[desc.pool].ttl : -1.f;
}

bool rendinst::isRgLayerPrimary(const RendInstDesc &desc) { return rendinst::isRgLayerPrimary(desc.layer); }
