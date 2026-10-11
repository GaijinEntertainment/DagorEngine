# rnnoise

Recurrent neural network based noise suppression library for audio.

Upstream: RNNoise, https://gitlab.xiph.org/xiph/rnnoise (mirror: https://github.com/xiph/rnnoise)
Vendored version: 0.2-22-g70f1d25 (`PACKAGE_VERSION` in `config.h`).

## Local changes

- `src/vec_avx.h`: `real_m256i`, the real AVX type, comes before the macro that gives `__m256i` to the emulated type,
  as upstream has it; the `#ifdef __AVX__` around it is local, so that a build without AVX does not see it. Also local:
  `mm256_maddubs_epi16` is built only with SSE3. So the header compiles with SSE2 alone, and with AVX but no AVX2
  (PS4, Xbox One).
- `src/nnet.h`: the `Only SSE and SSE2 are available` warning is disabled (`#if 0`), because not every x86-64 platform
  can build with AVX2.
- `jamfile`: the build takes the little model (`rnnoise_data_little.c`) and leaves out the x86 run-time CPU dispatch
  (`src/x86`), so the SIMD level comes from the compiler flags of the platform.
