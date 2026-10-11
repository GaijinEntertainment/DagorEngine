# ffx

Selected shader headers from AMD's FidelityFX SDK, used to implement
Stochastic Screen Space Reflections (SSSR) in `prog/gameLibs/render/shaders/ffx_sssr.dshl`.

Upstream: AMD FidelityFX SDK, https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK
(gpu/ and gpu/sssr/ headers). Vendored copyright dated 2024, includes the
SSSR `inv_direction` FFX_SELECT fix (post PR #106).

Local deviations in `ffx_sssr_intersect.h`, none of them upstream:

- `FFX_RaymarchResult` replaces the plain hit vector and carries the back
  surface entry point, which `FFX_SSSR_AdvanceRay` fills, and the trace end
  point, which `FFX_SSSR_HierarchicalRaymarch` writes.
- Mip ascent is clamped to `downsampled_depth_mip_count`, since the engine does
  not build the whole pyramid.
- A ray that goes under the surface on its first step is a hit at once. This
  is for the water SSR (`water_3d_ssr.dshl`), which starts on the water plane
  that the pyramid does not hold.
- `FFX_SSSR_ValidateHit` is removed. The SSR caller (`hierarchRayMarch` in
  `ssr_common.hlsl`) keeps its check that a hit must be 2 texels or more from
  the origin. Its `SSR_TRACEWATER` variant keeps the check too: there the
  origin is on the pyramid, and the water plane is tested separately.
- Traversal also stops when the ray leaves the screen, and a hit needs a
  non-sky depth in `downsampled_close_depth_tex`.
- A back surface entry less than 2 texels from the trace end is dropped, to
  stop an immediate self reflection.
- The low occupancy exit is compiled only with `FFW_WAVE`.

License: MIT (see LICENSE).
