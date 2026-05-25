// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_texMgr.h>

class BaseTexture;

// Host-side landscape preview rendering service for daEditorX plugins.
//
// Mirrors the IGraphTexGenService pattern: all render primitives
// (DeferredRenderTarget, HeightmapRenderer, CascadeShadows, DaSkies,
// SSAORenderer, ScreenSpaceReflections, PostFxRenderer) live here,
// where the daEditorX host already statically links them. Plugin DLLs
// drive the preview through this interface via
// IDaEditor3Engine::findService("landscape-preview")->queryInterface<ILandscapePreviewService>().
struct ILandscapePreviewService
{
  static constexpr unsigned HUID = 0x1A57F07Du; // "LaSTPOTD" - unique id

  // True once all required shader classes ("heightmap", "deferred_shadow_to_buffer",
  // "combine_shadows", "postfx") and shader globals are present in the host shader dump.
  virtual bool isAvailable() const = 0;

  // When isAvailable() returns false this returns a short human-readable reason
  // that the panel can surface via ImGui::TextWrapped.
  virtual const char *getUnavailableReason() const = 0;

  // Heightmap metadata from the graph. Pass 0 for any value to fall back to defaults.
  virtual void setHeightmapParams(float scale, float min_height, float cell_size) = 0;

  // Heightmap texture from the graph. Must be passed explicitly because the graph's
  // "heightmap" final name does not exist as a shader global in the daEditorX shader
  // dump (the engine shader uses "tex_hmap_low" instead).
  virtual void setHeightmapTexture(TEXTUREID tex_id) = 0;

  // Camera/view state for the next renderPreview() call.
  virtual void setCameraTransform(const TMatrix &view_tm, const Point3 &camera_pos) = 0;

  // Optional: direction to sun (normalized). Default: (-0.5, 0.8, -0.3) if never set.
  virtual void setSunDirection(const Point3 &dir_to_sun) = 0;

  // Arm a deferred render with the given frame delta. The render is allocated
  // at a fixed backing size (getOutputAllocSize()); callers display it by
  // UV-cropping a centered sub-rect to their target aspect.
  virtual void setRenderParams(float dt) = 0;

  // Execute the deferred render NOW using stored params. Call this from a
  // plugin's renderObjects() -- NOT from updateImgui. The d3d state, shader
  // blocks, and globals are only valid during the normal render phase.
  virtual void executeRender() = 0;

  // Returns the last rendered output texture for display via ImGui::Image,
  // or nullptr if nothing has been rendered yet.
  virtual BaseTexture *getOutputTexture() = 0;

  // Backing texture dimensions of the output. The preview renders at a fixed
  // alloc size independent of the panel size to avoid per-resize texture
  // reallocation; callers that display the output should UV-crop a centered
  // sub-rect of these dimensions to match their target aspect.
  virtual IPoint2 getOutputAllocSize() const = 0;
};

void init_landscape_preview_service();
