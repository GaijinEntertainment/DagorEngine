# ffx

Selected shader headers from AMD's FidelityFX SDK, used to implement
Stochastic Screen Space Reflections (SSSR) in `prog/gameLibs/render/shaders/ffx_sssr.dshl`.

Upstream: AMD FidelityFX SDK, https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK
(gpu/ and gpu/sssr/ headers). Vendored copyright dated 2024, includes the
SSSR `inv_direction` FFX_SELECT fix (post PR #106).

License: MIT (see LICENSE).
