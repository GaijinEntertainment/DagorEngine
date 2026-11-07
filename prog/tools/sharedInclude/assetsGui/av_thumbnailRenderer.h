// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_resId.h>

#include <EASTL/optional.h>

class DagorAsset;

// Renders the Asset Browser thumbnails of the asset types that are not textures.
// The interface makes it possible to avoid including renderer code in assetsGui.
class IAssetThumbnailRenderer
{
public:
  // Returns true for supported asset types.
  virtual bool canRenderThumbnail(const DagorAsset &asset) const = 0;

  // Returns a registered managed texture with one reference that the caller owns, or BAD_TEXTUREID when the asset gets no thumbnail.
  // BAD_TEXTUREID is final, so a call in the next frame only repeats the blocking render.
  // Returns an empty optional for a limited time while the asset data is not ready to be rendered. Call it again in the next
  // ImGui frame, or the limited time starts again.
  // After the limited time it renders with what is loaded, so the thumbnail can show a lower LOD, missing textures or ambient light.
  // The render blocks the main thread.
  // A device reset loses the texture content, so release the texture and render it again.
  // size: the maximum width and height of the thumbnail. The texture can be smaller on one or both axes.
  virtual eastl::optional<TEXTUREID> renderThumbnail(DagorAsset &asset, int size) = 0;
};

// Call it with nullptr before the thumbnail renderer or the render code that it uses shuts down.
void assets_gui_set_thumbnail_renderer(IAssetThumbnailRenderer *renderer);

IAssetThumbnailRenderer *assets_gui_get_thumbnail_renderer();
