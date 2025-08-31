// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_guiScene.h>

struct Frustum;
class TMatrix;

namespace darg_panel_renderer
{

using EntityResolver = TMatrix (*)(uint32_t, const char *);
void panel_spatial_resolver(const darg::IGuiScene::SpatialSceneData &scene_data, darg::PanelSpatialInfo &info, TMatrix &out_transform);

void clean_up();

} // namespace darg_panel_renderer
