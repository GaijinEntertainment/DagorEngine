//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class BBox3;
class DagorAsset;
class TextureIdSet;
class TMatrix;

// getIsolatedRenderData() and renderIsolatedOpaque() create an entity for the asset when the service does not have one,
// which loads the asset and references its textures. The service keeps the entity until the asset changes or is
// removed, or until releaseIsolatedEntity() or releaseIsolatedRenderData().
class IDynModelIsolatedRender
{
public:
  // Gives the local box of the asset and adds the textures that renderIsolatedOpaque() draws to tex_ids. Returns false
  // when the service does not draw the asset type, or the asset cannot be loaded or has no geometry in LOD 0.
  // Keep the entity until the thumbnail has rendered, then call releaseIsolatedEntity(asset.getNameId()). Also release
  // it when getIsolatedRenderData() returned false.
  virtual bool getIsolatedRenderData(const DagorAsset &asset, BBox3 &local_box, TextureIdSet &tex_ids) = 0;

  // Draws the opaque geometry of LOD 0 of the asset at tm into the bound gbuffer, as the viewport does. Call it only
  // from the render_opaque callback of IDynRenderService::renderIsolated(), with the world_offset of that callback
  // added to tm. When dynrend draws the dynModel, the shaders get per_instance_seed as the instance seed.
  virtual void renderIsolatedOpaque(const DagorAsset &asset, const TMatrix &tm, int per_instance_seed) = 0;

  // Frees the entity of the asset, so the asset and its textures can unload. asset_name_id is the
  // DagorAsset::getNameId() of the asset. Does nothing when the service has no entity for the asset.
  virtual void releaseIsolatedEntity(int asset_name_id) = 0;

  // Frees the entities. Call it before reset_game_resources().
  virtual void releaseIsolatedRenderData() = 0;

  static constexpr unsigned HUID = 0xBB1AE7D0u; // IDynModelIsolatedRender
};
