// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace darg
{
struct PanelSpatialInfo;
}
struct Frustum;
class TMatrix;

namespace darg_panel_renderer
{

using EntityResolver = TMatrix (*)(uint32_t, const char *);
void panel_spatial_resolver(const TMatrix &vr_space, const TMatrix &camera, const TMatrix &left_hand_transform,
  const TMatrix &right_hand_transform, const Frustum &camera_frustum, darg::PanelSpatialInfo &info, EntityResolver entity_resolver,
  TMatrix &out_transform);

void clean_up();

} // namespace darg_panel_renderer
