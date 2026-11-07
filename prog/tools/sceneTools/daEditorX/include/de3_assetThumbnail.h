//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class IAssetThumbnailRenderer;

// Renders thumbnails in isolation, so nothing of the scene or the level shows in them.
// renderThumbnail() builds the asset data when it is out of date, loads it, and waits until it is ready to render.
IAssetThumbnailRenderer &get_asset_thumbnail_renderer();

// Frees the thumbnail rendering resources. Call it before reset_game_resources(), before daBuild is unloaded, and while
// the IDaEditor3Engine object exists.
void term_asset_thumbnail_rendering();
