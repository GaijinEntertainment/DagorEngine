// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/dynmodelRenderer/animCharRenderAdditionalData.h>
#include <render/dynmodelRenderer.h>


namespace animchar_additional_data
{

const Point4 AnimcharAdditionalDataView::NULL_METADATA_RAW[] = {Point4::ZERO, Point4::ZERO};
const AnimcharAdditionalDataView AnimcharAdditionalDataView::NULL_METADATA =
  AnimcharAdditionalDataView(make_span_const(NULL_METADATA_RAW));

}; // namespace animchar_additional_data


namespace dynrend
{

void append_animchar_additional_data(PerInstanceRenderData &render_data,
  const animchar_additional_data::AnimcharAdditionalDataView &additional_data)
{
  G_ASSERTF_RETURN(!render_data.additionalDataAppended, , "additional data block already appended to these dynmodel params");
  render_data.additionalDataAppended = true;
  auto &params = render_data.params;
  if (params.size() < NUM_GENERIC_PER_INSTANCE_PARAMS)
    params.insert(params.end(), NUM_GENERIC_PER_INSTANCE_PARAMS - params.size(), Point4::ZERO);
  params.insert(params.end(), additional_data.data(), additional_data.data() + additional_data.size());
}

} // namespace dynrend