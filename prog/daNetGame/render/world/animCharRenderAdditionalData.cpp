// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animCharRenderAdditionalData.h"


namespace animchar_additional_data
{

const Point4 AnimcharAdditionalDataView::NULL_METADATA_RAW[] = {Point4::ZERO, Point4::ZERO};
const AnimcharAdditionalDataView AnimcharAdditionalDataView::NULL_METADATA =
  AnimcharAdditionalDataView(make_span_const(NULL_METADATA_RAW));

}; // namespace animchar_additional_data