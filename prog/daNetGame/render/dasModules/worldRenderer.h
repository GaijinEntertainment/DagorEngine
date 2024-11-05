// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <ecs/render/updateStageRender.h>
#include <render/rendererFeatures.h>
#include <dasModules/dasModulesCommon.h>
#include <drv/3d/dag_tex3d.h>
#include <render/resolution.h>
#include <render/world/cameraParams.h>


class Point3;
class BBox3;

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(UpdateStageInfoRender::RenderPass, RenderPass);
DAS_BIND_ENUM_CAST_98(FeatureRenderFlags)

MAKE_TYPE_FACTORY(CameraParams, CameraParams)

namespace bind_dascript
{
void toggleFeature(FeatureRenderFlags flag, bool enable);
bool worldRenderer_getWorldBBox3(BBox3 &bbox);
void worldRenderer_shadowsInvalidate(const BBox3 &bbox);
void worldRenderer_invalidateAllShadows();
void worldRenderer_renderDebug();
int worldRenderer_getDynamicResolutionTargetFps();
bool does_world_renderer_exist();
} // namespace bind_dascript

void erase_grass(const Point3 &world_pos, float radius);
void invalidate_after_heightmap_change(const BBox3 &box);
