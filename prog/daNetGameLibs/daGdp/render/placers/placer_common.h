// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

template <unsigned N = 256>
using TmpNameN = eastl::fixed_string<char, N>;
using TmpName = TmpNameN<>;

template <typename PersistentData>
dafg::NodeHandle create_indirect_args_node(const dafg::NameSpace &ns, const eastl::shared_ptr<PersistentData> &persistentData)
{
  return ns.registerNode("indirect_args", DAFG_PP_NODE_SRC, [persistentData](dafg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    const auto indirectArgsHandle = registry.create("indirect_args", dafg::History::No)
                                      .indirectBufferUa(d3d::buffers::Indirect::Dispatch, DAGDP_MAX_VIEWPORTS)
                                      .atStage(dafg::Stage::UNKNOWN) // TODO: d3d::zero_rwbufi semantics is not defined.
                                      .useAs(dafg::Usage::UNKNOWN)
                                      .handle();

    return [indirectArgsHandle] { d3d::zero_rwbufi(indirectArgsHandle.get()); };
  });
}

template <typename PersistentData>
dafg::NodeHandle create_cull_tiles_node(const dafg::NameSpace &ns, const eastl::shared_ptr<PersistentData> &persistentData)
{
  return ns.registerNode("cull_tiles", DAFG_PP_NODE_SRC, [persistentData](dafg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    const TmpNameN<32> viewResourceName(TmpNameN<32>::CtorSprintf(), "view@%s", constants.viewInfo.uniqueName.c_str());
    const auto viewHandle = registry.readBlob<ViewPerFrameData>(viewResourceName.c_str()).handle();

    registry.modify("indirect_args").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("dagdp_heightmap__indirect_args");

    registry.create("visible_tile_positions", dafg::History::No)
      .byteAddressBufferUaSr(constants.numTiles * 2) // int2
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("dagdp_heightmap__visible_tile_positions");

    return [persistentData, viewHandle, shader = ComputeShader("dagdp_heightmap_cull_tiles")] {
      const auto &constants = persistentData->constants;
      const auto &view = viewHandle.ref();

      ShaderGlobal::set_int(var::num_tiles, constants.numTiles);
      ShaderGlobal::set_real(var::tile_pos_delta, constants.tileWorldSize);
      ShaderGlobal::set_real(var::max_placeable_bounding_radius, constants.maxPlaceableBoundingRadius);

      ShaderGlobal::set_buffer(var::tile_positions, persistentData->tilePositionsBuffer.getBufId());

      for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
      {
        const auto &viewport = view.viewports[viewportIndex];
        ScopeFrustumPlanesShaderVars scopedFrustumVars(viewport.frustum);

        IPoint4 baseTileIntPos;
        baseTileIntPos.x = static_cast<int>(floorf(viewport.worldPos.x / constants.tileWorldSize));
        baseTileIntPos.y = static_cast<int>(floorf(viewport.worldPos.z / constants.tileWorldSize));

        ShaderGlobal::set_color4(var::base_tile_pos_xz, point4(baseTileIntPos) * constants.tileWorldSize);
        ShaderGlobal::set_color4(var::viewport_pos, viewport.worldPos);
        ShaderGlobal::set_int(var::viewport_index, viewportIndex);
        ShaderGlobal::set_real(var::viewport_max_distance,
          min(viewport.maxDrawDistance, constants.viewInfo.maxDrawDistance) * get_global_range_scale());

        const bool res = shader.dispatchThreads(constants.numTiles, 1, 1);
        G_ASSERT(res);
        G_UNUSED(res);
      }

      ShaderGlobal::set_buffer(var::tile_positions, BAD_D3DRESID);
    };
  });
}

template <typename PersistentData>
dafg::NodeHandle create_place_node(
  const dafg::NameSpace &ns, const eastl::shared_ptr<PersistentData> &persistentData, bool is_optimistic)
{
  TmpName name(TmpName::CtorSprintf(), "place_stage%d", is_optimistic ? 0 : 1);

  return ns.registerNode(name.c_str(), DAFG_PP_NODE_SRC, [persistentData, is_optimistic](dafg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
      .modify("instance_data")
      .buffer()
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("dagdp__instance_data");

#if IS_GRASS
    ComputeShader placeShader = ComputeShader(is_optimistic ? "dagdp_grass_place_stage0" : "dagdp_grass_place_stage1");
#else
    ComputeShader placeShader = ComputeShader(is_optimistic ? "dagdp_heightmap_place_stage0" : "dagdp_heightmap_place_stage1");
#endif

    (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
      .read(is_optimistic ? "dyn_allocs_stage0" : "dyn_allocs_stage1")
      .buffer()
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("dagdp__dyn_allocs");

    (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
      .modify(is_optimistic ? "dyn_counters_stage0" : "dyn_counters_stage1")
      .buffer()
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("dagdp__dyn_counters");

    eastl::fixed_string<char, 32> viewResourceName;
    viewResourceName.append_sprintf("view@%s", constants.viewInfo.uniqueName.c_str());
    const auto viewHandle = registry.readBlob<ViewPerFrameData>(viewResourceName.c_str()).handle();

    const auto indirectArgsHandle =
      registry.read("indirect_args").buffer().atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::INDIRECTION_BUFFER).handle();

    registry.read("visible_tile_positions")
      .buffer()
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("dagdp_heightmap__visible_tile_positions");

    return [persistentData, viewHandle, indirectArgsHandle, shader = eastl::move(placeShader)] {
      const auto &constants = persistentData->constants;
      const auto &view = viewHandle.ref();
      G_ASSERT(view.viewports.size() <= constants.viewInfo.maxViewports);

      ShaderGlobal::set_buffer(var::draw_ranges, persistentData->commonBuffers.drawRangesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placeables, persistentData->commonBuffers.placeablesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placeable_weights, persistentData->commonBuffers.placeableWeightsBuffer.getBufId());
      ShaderGlobal::set_buffer(var::renderable_indices, persistentData->commonBuffers.renderableIndicesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::biomes, persistentData->biomesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::variants, persistentData->commonBuffers.variantsBuffer.getBufId());

      ShaderGlobal::set_int(var::num_renderables, constants.numRenderables);
      ShaderGlobal::set_int(var::num_placeables, constants.numPlaceables);
      ShaderGlobal::set_int(var::num_biomes, constants.numBiomes);
      ShaderGlobal::set_int(var::num_tiles, constants.numTiles);

      ShaderGlobal::set_real(var::max_placeable_bounding_radius, constants.maxPlaceableBoundingRadius);
      ShaderGlobal::set_real(var::tile_pos_delta, constants.tileWorldSize);
      ShaderGlobal::set_real(var::instance_pos_delta, constants.tileWorldSize / TILE_INSTANCE_COUNT_1D);
      ShaderGlobal::set_real(var::debug_frustum_culling_bias, get_frustum_culling_bias());

      ShaderGlobal::set_int(var::prng_seed_jitter_x, constants.prngSeed + 0x4272ECD4u);
      ShaderGlobal::set_int(var::prng_seed_jitter_z, constants.prngSeed + 0x86E5A4D2u);
      ShaderGlobal::set_int(var::prng_seed_placeable, constants.prngSeed + 0x08C2592Cu);
      ShaderGlobal::set_int(var::prng_seed_scale, constants.prngSeed + 0xDF3069FFu);
      ShaderGlobal::set_int(var::prng_seed_slope, constants.prngSeed + 0x3C1385DBu);
      ShaderGlobal::set_int(var::prng_seed_occlusion, constants.prngSeed + 0x93C0FB91u);
      ShaderGlobal::set_int(var::prng_seed_yaw, constants.prngSeed + 0x71F23960u);
      ShaderGlobal::set_int(var::prng_seed_pitch, constants.prngSeed + 0xDEB40CF0u);
      ShaderGlobal::set_int(var::prng_seed_roll, constants.prngSeed + 0xF6A38A81u);
      ShaderGlobal::set_int(var::prng_seed_density, constants.prngSeed + 0x54F2A367u);

#if IS_GRASS
      ShaderGlobal::set_real(var::grass_max_range, constants.grassMaxRange);
      ShaderGlobal::set_int(var::prng_seed_decal, constants.prngSeed + 0x45F9668Eu);
      ShaderGlobal::set_int(var::prng_seed_height, constants.prngSeed + 0x682A73E1u);
#else
      ShaderGlobal::set_texture(var::density_mask, persistentData->densityMask);
      ShaderGlobal::set_color4(var::density_mask_scale_offset, persistentData->densityMaskScaleOffset);
      ShaderGlobal::set_buffer(var::density_mask_channel_weights, persistentData->densityMaskChannelWeightsBuffer.getBufId());

      ShaderGlobal::set_real(var::grid_jitter, constants.gridJitter);
      ShaderGlobal::set_real(var::displacement_noise_scale, constants.displacementNoiseScale);
      ShaderGlobal::set_real(var::displacement_strength, constants.displacementStrength);
      ShaderGlobal::set_real(var::placement_noise_scale, constants.placementNoiseScale);
      ShaderGlobal::set_real(var::sample_range, constants.sampleRange);
      ShaderGlobal::set_int(var::lower_level, constants.lowerLevel);
      ShaderGlobal::set_int(var::use_decals, constants.useDecals);
      ShaderGlobal::set_int(var::discard_on_grass_erasure, constants.discardOnGrassErasure);
#endif

      // TODO: the way culling variables are bound right now (see frustum.dshl), we need a dispatch per viewport.
      // Otherwise they could be merged into a single dispatch.
      for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
      {
        const auto &viewport = view.viewports[viewportIndex];
        ScopeFrustumPlanesShaderVars scopedFrustumVars(viewport.frustum);

        IPoint4 baseTileIntPos;
        baseTileIntPos.x = static_cast<int>(floorf(viewport.worldPos.x / constants.tileWorldSize));
        baseTileIntPos.y = static_cast<int>(floorf(viewport.worldPos.z / constants.tileWorldSize));

        ShaderGlobal::set_color4(var::base_tile_pos_xz, point4(baseTileIntPos) * constants.tileWorldSize);
        ShaderGlobal::set_int4(var::base_tile_int_pos_xz, baseTileIntPos);
        ShaderGlobal::set_color4(var::viewport_pos, viewport.worldPos);
        ShaderGlobal::set_real(var::viewport_max_distance,
          min(viewport.maxDrawDistance, constants.viewInfo.maxDrawDistance) * get_global_range_scale());
        ShaderGlobal::set_int(var::viewport_index, viewportIndex);

        bool res = shader.dispatchIndirect(indirectArgsHandle.view().getBuf(), viewportIndex * DISPATCH_INDIRECT_BUFFER_SIZE);
        G_ASSERT(res);
        G_UNUSED(res);
      }

      ShaderGlobal::set_buffer(var::draw_ranges, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::placeables, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::placeable_weights, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::renderable_indices, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::tile_positions, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::biomes, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::variants, BAD_D3DRESID);
    };
  });
}
