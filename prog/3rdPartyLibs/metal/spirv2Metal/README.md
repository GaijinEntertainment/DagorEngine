SPIRV-Cross: a library for parsing SPIR-V and converting it to MSL
(Metal Shading Language), used here for the Metal graphics backend.

Upstream: SPIRV-Cross, https://github.com/KhronosGroup/SPIRV-Cross
Vendored version: 0.67.0 (see SPVC_C_API_VERSION_* in spirv_cross_c.h),
with a local patch (../spirv-cross-no-fma.diff) gating invariant float
math behind msl_options.honor_no_contraction.
