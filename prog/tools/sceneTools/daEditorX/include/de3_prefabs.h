//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class BBox3;
class DagorAsset;
class TextureIdSet;
class TMatrix;

namespace prefabs
{
enum PrefabStage
{
  DEFAULT = 0,
  LOFT_MASK_EXPORT = 2
};
}

// Each method creates the PrefabEntityPool for the asset when the pool does not exist, which loads the prefab .dag
// file. The service keeps the pool.
class IPrefabIsolatedRender
{
public:
  // Returns false when the asset is not a prefab or has no renderable geometry. Otherwise gives its local box and adds
  // its textures to tex_ids. A loft mask prefab counts as renderable, as in Asset Viewer's viewport.
  virtual bool getIsolatedRenderData(const DagorAsset &asset, BBox3 &local_box, TextureIdSet &tex_ids) = 0;

  // Returns true for a prefab that the level export draws into the loft masks (prefabs::PrefabStage::LOFT_MASK_EXPORT),
  // not into the level geometry.
  virtual bool isLoftMaskPrefab(const DagorAsset &asset) = 0;

  // Draws the opaque geometry of the prefab at tm into the bound gbuffer, as the viewport does. Call it only from the
  // render_opaque callback of IDynRenderService::renderIsolated(), with the world_offset of that callback added to tm.
  virtual void renderIsolatedOpaque(const DagorAsset &asset, const TMatrix &tm) = 0;

  static constexpr unsigned HUID = 0x14D2FBEDu; // IPrefabIsolatedRender
};
