# Shader: rendinst_tree_perlin_layered

## Overview

The `rendinst_tree_perlin_layered` shader is designed for advanced rendering of
trees, using tessellation to enhance visual fidelity. This shader supports both
displacement and smoothing modes, allowing for detailed control over tree
geometry. Tessellation can be dynamically enabled or disabled at runtime, making
it adaptable for various performance settings.

## Tessellation Overview

Tessellation in this shader can be toggled at runtime via a shader variable. By
setting `pn_triangulation` to `1` in the console, all trees configured for
tessellation, as well as `rendinst_clipmap` objects with default settings, will
be tessellated. In the future, this feature will be incorporated into the
graphics settings.

## Enabling Tessellation in an Asset

1. **Default Setting:** Tessellation is disabled by default. To enable it, set
   `material_pn_triangulation` to `1` or `2`.

2. **Common Parameter:**
   - **max_tessellation_factor:** Controls the tessellation factor, which varies
     with distance. This parameter sets the maximum factor when the camera is
     close to the object.

3. **Tessellation Modes:**
   - **Mode 2:** Smoothing only. This mode does not have any additional
     configurable parameters.
   - **Mode 1:** Displacement mode. In this mode, the shader reads the
     displacement strength from the alpha component of the three diffuse
     textures. These textures are blended according to pixel shader blending
     rules.

   **Displacement Mode Parameters:**
   - **rendinst_displacement_min:** Defines the displacement when the diffuse
     texture's alpha is `0`. The `.xyz` components correspond to the three
     diffuse textures, while `.w` is unused.
   - **rendinst_displacement_max:** Defines the displacement when the diffuse
     texture's alpha is `1`. The `.xyz` components correspond to the three
     diffuse textures, while `.w` is unused.
   - **rendinst_displacement_lod:** Specifies the mip level used for sampling
     the diffuse textures during displacement. Lower values result in more
     noise, while higher values produce smoother displacement. The `.xyz`
     components correspond to the three diffuse textures, and `.w` is unused.
   - **rendinst_displacement_mod:** If `is_pivoted` is set to `1`, this shader
     variable allows multipliers to be specified based on the vertex hierarchy.
     The `.x` component is the multiplier for the trunk, while `.yzw` components
     apply to the branches in hierarchical order.

## Additional Notes

- **Tessellation Factor Limit:** The maximum tessellation factor is 12 due to
  API limitations. Setting `max_tessellation_factor` higher will simply reach
  this maximum at a greater distance. However, it is generally unnecessary to
  set it too high, as tree triangles are typically small.

- **rendinst_displacement_lod Setting:** This should be adjusted so that the
  texels sampled at the selected mip level correspond to a single triangle after
  tessellation, in terms of world size. Precise calculations aren't
  required – adjust until the visual result is satisfactory.

- **daEditor Configuration:** When using
  [*daEditor*](../../../dagor-tools/daeditor/daeditor/daeditor.md), ensure that
  the **PN triangulation** checkbox is selected to view the tessellation results
  immediately.



