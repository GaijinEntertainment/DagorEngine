# `application.blk`

The `application.blk` file is a key configuration file for the Dagor Engine
tools, detailing the setup and customization of various aspects of the editors
and builders, including rendering options, shader configurations, shadow
settings, asset management, and game-specific settings. This file contains
multiple blocks, each dedicated to specific functions and settings.

Below is a description of each block within the `application.blk` file:

## Root Level Parameters

- `collision`: Specifies the collision detection system to be used.
- `shaders`: Defines the path to the shaders.
- `useDynrend`: A boolean flag to enable or disable dynamic rendering.

## Blocks and Block Parameters

### `dynamicDeferred{}`

This block configures settings for the dynamic deferred rendering system,
including supported options, default rendering options, debug display options,
and various rendering formats.

- `supportsRopt{}`: Defines which rendering options are supported.
  - `render shadows`: Enables shadow rendering.
  - `render VSM shadows`: Enables variance shadow map shadows.
  - `use SSAO`: Enables screen space ambient occlusion.
  - `use SSR`: Enables screen space reflections.
  - `no sun, no amb, no envi refl, no transp, no postfx`: Enables sun,
    ambient, environmental reflections, transparency, and post-processing
    effects, respectively.
- `defaultRopt{}`: Sets the default rendering options.
  - `render VSM shadows`: Enables variance shadow map shadows.
  - `no sun, no amb, no envi refl, no transp, no postfx`: Enables sun,
    ambient, environmental reflections, transparency, and post-processing
    effects, respectively.
- `dbgShow{}`: Configures debug display options with various rendering
  indices for different components.
  - `diffuse`: Diffuse component.
  - `specular`: Specular component.
  - `normal`: Normal vector component.
  - `smoothness`: Smoothness (or roughness) component.
  - `base_color`: Base color of the material.
  - `metallness`: Metallic component.
  - `material`: Material ID.
  - `ssao`: Screen space ambient occlusion (SSAO).
  - `ao`: Ambient occlusion (AO).
  - `albedo_ao`: Maps the albedo multiplied by ambient occlusion.
  - `final_ao`: Maps the final ambient occlusion.
  - `preshadow`: Maps the pre-shadow (shadow map before applying shadows).
  - `translucency`: Maps the translucency component.
  - `translucency_color`: Maps the translucency color component.
  - `ssr`: Maps the screen space reflections (SSR).
  - `ssr_strength`: Maps the strength of the screen space reflections.
- `sceneFmt, postfxFmt, ssaoFmt`: Specifies formats for scene, post-processing
  effects, and SSAO.
- `postfx_sRGB_backbuf_write`: Enables/disables sRGB back buffer writing for
  post-processing effects.
- `gbufCount`: Sets the number of G-buffers.
- `gbufFmt0, gbufFmt1, gbufFmt2`: Defines formats for the G-buffers.
- `shadowPreset{}`: Defines presets for shadow mapping parameters. These
  presets are used to configure the resolution and distance for shadow maps, as
  well as the variance shadow maps (VSM).
  - `name`: Defines the name of the shadow preset, describing its
    configuration in terms of map size and maximum distance.
  - `csmSize`: Sets the size of the cascaded shadow map (CSM).
  - `csmMaxDist`: Sets the maximum distance for the cascaded shadow map.
  - `vsmSize`: Sets the size of the variance shadow map (VSM).
  - `vsmMaxDist`: Sets the maximum distance for the variance shadow map.
- `hdrMode, postfx, vsmAllowed, grassTranslucency`: Configures high dynamic
  range mode, post-processing effects, VSM allowance, and grass translucency.

### `clipmap{}`

Configures settings for clipmap rendering, which is typically used for large
terrains.

- `sideSize, stackCount, texelSize, step, finalSize, cacheCnt, bufferCnt`:
  Defines various parameters for the clipmap, including size, number of stacks,
  texel size, step size, final size, cache count, and buffer count.
- `cache_tex0, cache_tex1, cache_tex2, buf_tex0, buf_tex1, buf_tex2`:
  Specifies formats for cache textures and buffer textures.
- `useUAVFeedback, useToroidalHeightmap`: Enables UAV feedback and toroidal
  heightmap usage.

### `hdr_mode{}`

Specifies the high dynamic range mode settings.

- `ps3, real`: Enables specific HDR modes.

### `SDK{}`

Defines settings related to the SDK (Software Development Kit).

- `sdk_folder, levels_folder`: Paths to the SDK directory and levels
  directory.
- `daeditor3x_additional_plugins`: Path to additional plugins for the editor.
- `exclude_dll`: Regex pattern to exclude specific `.dll` files.
- `allow_debug_bitmap_dump`: Enables debug bitmap dumping.
- `levelExpSHA1Tags{}`: Defines a list of SHA-1 tags associated with
  various elements within a level. Each tag represents a specific type of data
  or asset that is included in the level export process.
  - `tag`: Defines a certain tag.

### `game{}`

Contains game-specific settings and paths.

- `game_folder, levels_bin_folder`: Paths to the game directory and levels
  binary directory.
- `physmat, params, entities, scene, car_params`: Paths to various game
  configuration files.
- `texStreamingFile, texStreamingBlock, navmesh_layers`: Paths and settings
  related to texture streaming and navigation mesh layers.

### `levelsBlkPrefix{}`

Configures prefix settings for level blocks.

- `useExportBinPath`: Enables the use of export binary path.
- `try`: Path to try for level export.

### `assets{}`

Defines the parameters and settings related to the handling of game assets,
including their types, base paths, export settings, impostor rendering
parameters, and build configurations.

- `types{}`: Lists various asset types used in the game. Specifies
  various asset types such as textures (`tex`), prefabs (`prefab`), render
  instances (`rendInst`), effects (`fx`), and more.
  - `type`: Specifies the type of asset.
- `base`: Defines the base directory for asset storage.
- `export{}`: Contains settings for exporting assets.
  - `types{}`: Specifies the types of assets to be exported.
    - `type`: Specifies the type of asset.
  - `cache`: Defines the cache directory used during export.
  - `useCache2Fast`: Enables fast caching.
  - `profiles{}`: Defines export profiles.
    - `dedicatedServer{}`: A profile for dedicated servers.
      - `strip_d3d_res`: Removes Direct3D resources.
      - `collapse_packs`: Collapses packs.
      - `reject_exp`: Specifies asset types to reject during export.
  - `destination{}`: Specifies export destinations for different
    platforms.
      - `PC`: PC destination path.
      - `"PC~~dedicatedServer"`: PC dedicated server destination path.
      - `iOS`: iOS destination path.
      - `allow_patch`: Disables patching.
      - `additional{}`: Additional export settings.
  - `destGrpHdrCache{}`: Configures the group header cache in VROMFS for
    export.
  - `packages{}`: Manages asset packages.
    - `forceExplicitList`: Enables explicit package list usage.
      - `outer_space{}`: Specifies settings for the *OuterSpace*
        package.
      - `outer_space_hq{}`: Specifies settings for the high-quality
        *OuterSpace* package.
  - `stripPrefix`: Specifies the prefix to strip from all package file paths.
  - `addTexPrefix`: Defines the prefix to add for texture packages.
  - `packlist`: Specifies the pack list file to maintain the current list of
    valid packs.
  - `defaultDdsxTexPack`: Defines the default DDSX (DirectDraw Surface)
    texture pack file.
  - `rtMipGenAllowed`: Disables real-time mipmap generation.
  - `folderDefaults{}`: Specifies default settings for directories when
    certain properties are not explicitly defined for a specific directory.
    - `ddsxTexPack`: Defines that if a directory does not have an explicitly
      defined `ddsxTexPack`, it can inherit this property from its parent
      directory.
    - `gameResPack`: Defines that if a directory does not have an explicitly
      defined `gameResPack`, it can inherit this property from its parent
      directory.
  - `plugins{}`: Specifies the locations of export plugins.
    - `folder`: Path to the directory.
  - `perTypeDest{}`: Defines destination packages for specific asset
    types.
- `perTypeDest{}`: Manages destination settings per asset type.
  - `package{}`: Specifies package settings.
    - `pack{}`: Specifies pack settings.
      - `tifMask`: Indicates that assets related to mask textures are grouped
        into the certain package.
      - `land`: Indicates that assets related to landscape elements are
        grouped into the certain package.
      - `collision`: Indicates that assets related to collision detection are
        grouped into the certain package.
- `impostor{}`: Defines settings for impostor rendering (simplified
  representations of complex objects to improve performance).
  - `data_folder`: Specifies the directory for impostor data.
  - `aoBrightness`: Adjusts ambient occlusion brightness.
  - `aoFalloffStart`: Sets the start distance for ambient occlusion falloff.
  - `aoFalloffStop`: Sets the stop distance for ambient occlusion falloff.
  - `preshadowEnabled`: Enables pre-shadowing.
  - `textureQualityLevels{}`: Defines texture quality levels.
    - `qualityLevel{}`: Quality levels based on:
      - `minHeight`: Defines minimum height.
      - `textureHeight`: Defines texture height.
- `build{}`: Configures general build settings.
  - `maxJobs`: Maximum number of build jobs.
  - `stateGraph{}`: Defines state graph settings.
    - `cppOutDir`: Specifies the output directory for state graph C++ files.
  - `dynModel{}`: Sets parameters for dynamic models.
    - `descListOutPath`: Specifies the output path for the description list of
      dynamic models.
    - `separateModelMatToDescBin`: Enables storing of model materials
      separately in the description binary file.
    - `ignoreMappingInPrepareBillboardMesh`: Enables ignoring texture mapping
      during the preparation of billboard meshes.
    - `enableMeshNodeCollapse`: Enables the collapsing of mesh nodes.
    - `maxBonesCount`: Sets the maximum number of bones that can be used in
      the skeletal animation of a dynamic model.
    - `setBonePerVertex`: Specifies the number of bones influencing each
      vertex.
    - `maxVPRConst`: Sets the maximum number of vertex shader constant
      registers.
    - `useDirectBones`: Enables the use of direct bone transformations.
    - `remapShaders{}`: Maps original shader names to new or alternative
      shaders, provides specific remapping rules, and optionally denies certain
      shaders from being used.
      - `denyShaders{}`: Specifies a regular expression to deny shaders
        whose names match the pattern.
      - `bat_shader`: Sets mapping for `bat_shader`.
      - `rendinst_simple`: Sets mapping for `rendinst_simple`.
      - `rendinst_simple_glass`: Sets mapping for `rendinst_simple_glass`.
      - `glass`: Sets mapping for `glass`.
      - `glass_refraction`: Sets mapping for `glass_refraction`.
      - `simple`: Sets mapping for `dynamic_simple`.
      - `dynamic_simple`: Sets mapping for `dynamic_simple`.
      - `dynamic_null`:: Sets mapping for `dynamic_null`.
      - `propeller_front`: Sets mapping for `propeller_front`.
      - `propeller_side`: Sets mapping for `propeller_side`.
      - `dynamic_glass_chrome`: Sets mapping for `dynamic_glass_chrome`.
      - `dynamic_masked_chrome`: Sets mapping for `dynamic_masked_chrome`.
      - `dynamic_masked_glass_chrome`: Sets mapping for
        `dynamic_masked_glass_chrome`.
      - `dynamic_illum`: Sets mapping for `dynamic_illum`.
      - `dynamic_mirror`: Sets mapping for `dynamic_mirror`.
      - `dynamic_alpha_blend`: Sets mapping for `dynamic_alpha_blend`.
      - `glass_crack`: Sets mapping for `glass_crack`.
      - `collimator`: Sets mapping for `collimator`.
      - `aces_weapon_fire`: Sets mapping for `aces_weapon_fire`.
      - `land_mesh_combined`: Sets mapping for `land_mesh_combined`.
      - `land_mesh_combined_2x`: Sets mapping for `land_mesh_combined_2x`.
      - `random_grass`: Sets mapping for `random_grass`.
      - `tracer_head`: Sets mapping for `tracer_head`.
      - `gi_black`: Sets mapping for `gi_black`.
  - `rendInst{}`: Configures render instance settings, shader remapping,
    and material processing.
    - `ignoreMappingInPrepareBillboardMesh`: Enables ignoring the mapping when
      preparing billboard meshes.
    - `descListOutPath`: Specifies the output path for the descriptor list of
      rendering instances.
    - `impostorParamsOutPath`: Specifies the output path for impostor
      parameters.
    - `separateModelMatToDescBin`: Enables separating the model materials into
      a descriptor binary.
    - `forceRiExtra`: Enables the inclusion of extra rendering instance data.
    - `remapShaders{}`: Maps original shader names to new or alternative
      shaders, similar to the `remapShaders{}` block in the `dynModel{}` block.
    - `processMat{}`: Specifies material processing settings.
  - `collision{}`: Defines settings for collision data.
    - `preferZSTD`: Enables a preference for ZSTD compression algorithm.
    - `writePrecookedFmt`: Enables writing the collision data in a pre-cooked
      format.
  - `skeleton{}`: Defines settings for skeleton data compression.
    - `preferZSTD`: Enables a preference for ZSTD compression algorithm.
  - `a2d{}`: Defines settings for 2D assets data compression.
    - `preferZSTD`: Enables a preference for ZSTD compression algorithm.
  - `tex{}`: Defines texture settings.
    - `defMaxTexSz`: Sets the default maximum texture size.
    - `iOS{}`: Configures settings for iOS platform.
      - `defMaxTexSz`: Sets the maximum texture size.
    - `and{}`: Configures settings for Android platform`
      - `defMaxTexSz`: Sets the maximum texture size.
  - `physObj{}`: Configures physical object properties.
    - `ragdoll{}`: Specifies ragdoll physics for characters.
      - `def_density`: Sets the default density.
      - `def_massType`: Defines the default mass type.
      - `def_collType`: Specifies the default collision type.
      - `name_full{}`: Defines the physical properties and constraints
        for the whole character.
      - `name_tail{}`: Specifies the joints for different parts of the
        characters body.
        - `" Pelvis"{}`
          - `Joint{}`
            - `type`: Defined joint type.
            - `minLimit`: Minimum rotational limits for the joint in X, Y, and
              Z axes.
            - `maxLimit`: Maximum rotational limits for the joint in X, Y, and
              Z axes.
            - `Damping`: Controls how quickly the joint's motion slows down.
            - `Twist Damping`: Specified damping applied to rotational movements.
        - `" Spine1"{}`
          - `Joint{}`
            - `type`: Defined joint type.
            - `minLimit`: Minimum rotational limits for the joint in X, Y, and
              Z axes.
            - `maxLimit`: Maximum rotational limits for the joint in X, Y, and
              Z axes.
            - `Damping`: Controls how quickly the joint's motion slows down.
            - `Twist Damping`: Specified damping applied to rotational
              movements.
        - `" R Calf"{}`
          - `Joint{}`
            - `type`: Defined joint type.
            - `minLimit`: Minimum rotational limits for the joint in X, Y, and
              Z axes.
            - `Damping`: Controls how quickly the joint's motion slows down.
        - `"@clone-last: L Calf"{}`: Clones the settings from the `"R
          Calf"` joint for the left calf.
        - `" R Foot"{}`
          - `Joint{}`
            - `type`: Defined joint type.
            - `minLimit`: Minimum rotational limits for the joint in X, Y, and
              Z axes.
            - `maxLimit`: Maximum rotational limits for the joint in X, Y, and
              Z axes.
            - `Damping`: Controls how quickly the joint's motion slows down.
            - `Twist Damping`: Specified damping applied to rotational
              movements.
        - `"@clone-last: L Foot"{}`: Clones the settings from the `"R
          Foot"` joint for the left foot.
          - `Override{}`: Overrides the parameters.
            - `swapLimZ`: Swaps the limits for the Z-axis.
        - `" Neck"{}`
          - `Joint{}`
            - `type`: Defined joint type.
            - `minLimit`: Minimum rotational limits for the joint in X, Y, and
              Z axes.
            - `maxLimit`: Maximum rotational limits for the joint in X, Y, and
              Z axes.
            - `Damping`: Controls how quickly the joint's motion slows down.
            - `Twist Damping`: Specified damping applied to rotational
              movements.
        - `" Head"{}`
          - `Reference`: References the neck bone for alignment.
          - `Joint{}`
            - `type`: Defined joint type.
            - `minLimit`: Minimum rotational limits for the joint in X, Y, and
              Z axes.
            - `maxLimit`: Maximum rotational limits for the joint in X, Y, and
              Z axes.
            - `Damping`: Controls how quickly the joint's motion slows down.
            - `Twist Damping`: Specified damping applied to rotational
              movements.
      - `def_helper{}`: Sets default helper settings.
        - `physObj`: Enables treating object as a physical object.
        - `animated_node`: Enables treating object as an animated node.
      - `twist_ctrl{}`: Specifies the configuration for controlling the
        twisting motion of certain bones in the character's rig.
        - `node0`: Specifies the starting bone or node for the twist control.
        - `node1`: Specifies the ending bone or node for the twist control.
        - `twistNode`: Defines the node that will handle the twisting effect
          between `node0` and `node1`.
        - `twistNode`: An additional twist node, which might be used to
          enhance or refine the twisting effect.
        - `angDiff:`: Specifies the angular difference or rotation applied to
          the twist nodes.
  - `lshader{}`: Manages the location of shader subgraphs.
    - `subgraphsFolder`: Specifies the folder path where shader subgraphs are
      stored.
  - `preferZSTD`: Enables settings for ZSTD compression.
  - `preferZLIB`: Enables settings for ZLIB compression.
  - `writeDdsxTexPackVer`: Sets the version for writing DDS texture packs.
  - `writeGameResPackVer`: Sets the version for writing game resource packs.

### `level_metrics{}`

Defines metrics for levels and plugins.

- `whole_level{}`: Maximum sizes and counts for textures and level data.
  - `textures_size`: Maximum total size of textures allowed for the entire
    level.
  - `level_size`: Maximum total size of the level data.
  - `textures_count`: Maximum number of textures allowed in the level.
- `plugin{}`: Metrics for collision plugins.
  - `name:t="Collision"`: Name of the plugin or category. In this case, it
    pertains to `collision` data.
  - `max_size`: Maximum size of collision data allowed.
- `plugin{}`: Metrics for occluders plugins.
  - `name:t="Occluders"`: Name of the plugin or category related to
    `occluders`.
  - `max_triangles`: Maximum number of triangles allowed for occluder data.
- `plugin{}`: Metrics for environment plugins.
  - `name:t="Environment"`: Name of the plugin or category related to
    `environment` data.
  - `max_faces`: Maximum number of faces allowed in the environment data.
  - `znear_min`: Minimum value for the near clipping plane in the
    environment's rendering setup.
  - `znear_max`: Maximum value for the near clipping plane.
  - `zfar_min`: Minimum value for the far clipping plane in the environment's
    rendering setup.
  - `zfar_max`: Maximum value for the far clipping plane.
  - `z_ratio_max`: Maximum ratio of the distance between far and near clipping
    planes to the near clipping plane.

### `genObjTypes{}`

Lists general object types.

- `type`: Various object types used by the tools.

### `dagored_visibility_tags{}`

Configures visibility tags for different plugins.

- `plugin{}`: Names of plugins and their associated tags for visibility.

### `dagored_disabled_plugins{}`

Lists plugins that are disabled.

- `disable`: Names of plugins that are disabled in the editor.

### `daEditorExportOrder{}`

Defines the order in which various game elements and assets are exported by the
editor. This ensures that dependencies are resolved and that assets are
processed in the correct sequence.

- `plugin`: Specifies the plugins used by the editor in a specific sequence.

### `lightmap{}`

Configures settings for generating lightmaps, which are precomputed lighting
information used for static objects.

- `High{}`: A quality preset defining the number of passes and settings
  per light.
  - `passes`: Number of passes or iterations for generating the lightmap.
  - `cub5`: Enables using the 5-face cubemaps.
  - `perLight`: Enables generating lightmaps per each light source.
  - `map{}`: Defines the settings for the lightmap textures, specifies
    the resolution of the lightmaps used.
    - `size`: Size of the lightmap texture.

### `projectDefaults{}`

Defines default project-wide settings.

- `disableGrass`: Enables grass rendering.
- `gpuGrass`: Enables grass rendering on the GPU
  - `riMgr{}`: Configures settings for render instance management.
    - `type`: Specifies the type of render instance manager to use.
    - `gridCellSize`: Sets the size of the grid cell for rendering instances.
    - `subGridSize`: Defines the size of the sub-grid within the main grid.
    - `minGridCellSize`: Sets the minimum size for a grid cell.
    - `minGridCellCount`: Specifies the minimum number of grid cells.
    - `writeOptTree`: Enables the writing of an optimized tree for rendering
      instances.
    - `packedRIBB`: Enables packed render instance bounding boxes.
    - `gen{}`: Configures generation settings for different platforms.
      - `PC`, `and`, `iOS`: Enables generation for PC, Android, and iOS.
    - `tmInst12x32bit`: Enables a specific rendering instance mode.
    - `maxRiGenPerCell`: Sets the maximum number of render instances generated
      per each cell.
    - `perInstDataDwords`: Specifies the number of DWORDs per instance data.
    - `perInstDataUseSeed`: Enables the use of seed data for instances.
  - `prefabMgr{}`: Configures the prefab manager for handling reusable
    game objects.
    - `clipmapShaderRE`: Defines a regular expression for selecting shaders
      related to clipmaps.
  - `envi{}`: Environment settings, including sky rendering and weather
    types.
    - `type`: Specifies the type of environment.
    - `znear`: Sets the near clipping plane distance.
    - `zfar`: Sets the far clipping plane distance.
    - `skiesFilePathPrefix`: Defines the file path prefix for sky-related
      files.
    - `skiesWeatherTypes`: Specifies the file for weather types.
    - `skiesPresets`: Defines the file pattern for weather presets.
    - `skiesGlobal`: Sets the global environment file.
    - `daSkiesDepthTex`: Specifies the depth texture for the skies.
    - `enviNames`: Enables specific environment names.
      - `morning`: Enables morning environment.
      - `day`: Enables day environment.
      - `evening`: Enables evening environment.
      - `night`: Enables night environment.
    - `skies`: Placeholder for additional sky settings.
  - `hmap{}`: Heightmap settings for terrain.
    - `type`: Specifies the type of heightmap to use.
  - `scnExport{}`: Export settings for scenes.
    - `removeInsideFaces`: Enables removing inner faces during scene export.
  - `preferZSTD`: Enables a preference for ZSTD compression algorithm.
  - `preferZLIB`: Enables a preference for ZLIB compression algorithm.

### `shader_glob_vars_scheme{}`

Global shader variables configuration.

- `common{}`: Common settings for shaders.
  - `assetViewer{}`: Specific settings for the *Asset Viewer*, including
    anti-aliasing options.
    - `caption`: Defines the caption or label for a setting within the *Asset
      Viewer*.
    - `show_no_aa`: Controls the display of assets without anti-aliasing.

### `heightMap{}`

Configures settings for heightmaps, which define terrain elevation and other
surface details.

- `useMeshSurface`: Enables mesh surface using for height map generation.
- `requireTileTexe`: Enables tile textures required for height maps.
- `hasColorTex`: Enables color textures presenting in the height map.
- `hasLightmapTex`: Enables lightmap textures presenting in the height map.
- `storeNxzInLtmapTex`: Enables storing normal and height (NxZ) data in the
  lightmap texture.
- `genFwdRadius`: Defines the forward radius for height map generation.
- `genBackRadius`: Defines the backward radius for height map generation.
- `genDiscardRadius`: Defines the discard radius for height map generation.
- `landClassLayers{}`: Defines various layers used in height maps. Each
  layer specifies its properties and attributes.
  - `layer{}`: Represents a distinct layer in the height map.
    - `name`: The name of the layer.
    - `bits`: The bit width for this layer.
    - `attr`: The attribute associated with this layer.
- `lightmapCvtProps{}`: Defines properties for converting lightmaps.
  This includes format settings, swizzling, addressing, mipmap levels, and gamma
  correction.
  - `fmt`: The format for the lightmap texture.
  - `swizzleARGB`: The swizzling pattern for the lightmap texture.
  - `addr`: The addressing mode for the lightmap texture.
  - `hqMip`; `mqMip`; `lqMip`: Mipmap levels for high, medium, and low
    quality.
  - `gamma`: The gamma correction value.
  - `iOS{}`, `and{}`: Platform-specific format settings.

### `additional_platforms{}`

Lists additional platforms for which the tools are configured.

- `platform`: Specifies the platform.

### `parameters{}`

Defines general parameters for the tools.

- `maxTraceDistance`: Maximum distance for tracing operations, such as ray
  casting.

### `AssetLight{}`

Configures lighting assets, including dynamic cube textures and level of detail
(LOD) settings.

- `shader_var`: Shader variable for dynamic cube textures.
- `blur_lods`: Enables LOD blurring.
- `bound_with_camera`: Enables binding the lighting to the camera.

### `defProjectLocal{}`

Defines local project settings, particularly camera control settings for
different types of camera modes.

- `freeCamera{}`: Configures settings for a free-moving camera,
  typically used for debugging or exploration purposes.
  - `move_step`: The step size for camera movement.
  - `strife_step`: The step size for strafing movement.
  - `control_multiplier`: The multiplier for camera control, affecting how
    responsive the camera is to input.
- `maxCamera{}`: Configures settings for a maximum control camera, which
  might be used for more precise control.
  - `move_step`: The step size for camera movement.
  - `strife_step`: The step size for strafing movement.
  - `control_multiplier`: The multiplier for camera control, offering more
    fine-grained control compared to the `freeCamera` settings.

### `animCharView{}`

Defines variables and first-person shooter (FPS) views for character animation.

- `vars{}`: Contains various enumerations and ranges for character
  states and animations. These settings allow for detailed control over
  character behavior and animation.
  - `weapon_mod_state{}`: Enumeration for weapon modification states.
  - `weapon_selected{}`: Enumeration for selected weapons.
  - `vehicle_selected{}`: Enumeration for selected vehicles.
  - `seat_type{}`: Enumeration for seat types.
  - `heal_item_selected{}`: Enumeration for selected healing items.
  - `weap_state{}`: Enumeration for weapon states.
  - `pos_state{}`: Enumeration for position states.
  - `move_state{}`: Enumeration for movement states.
  - `move_ang{}`: Defines the angle range for movement with minimum,
    maximum, and step values.
  - `pers_course{}`: Defines the personal course angle with minimum,
    maximum, and step values.
  - `aim_pitch{}`: Defines the aim pitch angle with minimum, maximum,
    and step values.
  - `torso_lean{}`: Defines the torso lean angle with minimum, maximum,
    and step values.
  - `torso_rotate{}`: Defines the torso rotation angle with minimum,
    maximum, and step values.
  - `bolt_action{}`: Progress for bolt action with a trackbar for UI
    representation.
  - `reload_progress{}`: Progress for reloading with a trackbar for UI
    representation.
  - `reload_style{}`: Enumeration for reload styles.
  - `single_reload_progress{}`: Progress for single reload with a
    trackbar for UI representation.
  - `single_reload_state{}`: Enumeration for single reload states.
  - `device_progress{}`: Progress for device usage with a trackbar for
    UI representation.
  - `device_state{}`: Enumeration for device states.
  - `deflection_progress{}`: Progress for deflection with a trackbar for
    UI representation.
  - `deflect_angle{}`: Angle for deflection with a trackbar for UI
    representation.
  - `l.hand_ik_mul{}`: Multiplier for left-hand IK with a trackbar for
    UI representation.
  - `r.hand_ik_mul{}`: Multiplier for right-hand IK with a trackbar for
    UI representation.
  - `changeweapon_progress{}`: Progress for changing weapons with a
    trackbar for UI representation.
  - `throw_progress{}`: Progress for throwing with a trackbar for UI
    representation.
  - `action_progress{}`: Progress for actions with a trackbar for UI
    representation.
  - `recover_progress{}`: Progress for recovery with a trackbar for UI
    representation.
  - `gesture_progress{}`: Progress for gestures with a trackbar for UI
    representation.
- `fps_view{}`: Define the first-person shooter views for different
  scenarios.
  - `name`: Name of the view.
  - `node`: Node to attach the camera.
  - `camOfs`: Camera offset.
  - `camAxis`: Camera axis.
  - `camUpAxis`: Camera up axis.
  - `fixed_dir`: Whether the direction is fixed.
  - `pitch_param`: Parameter for pitch.
  - `yaw_param`: Parameter for yaw.
  - `znear`: Near clipping plane.
  - `zfar`: Far clipping plane.
  - `hideNodes{}`: Nodes to hide during rendering.


