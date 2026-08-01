# WebRTC Audio Processing Module (APM)

Echo cancellation (AEC3/AECM), noise suppression, AGC/AGC2, VAD and related
audio pipeline code extracted from the WebRTC native stack, plus the minimal
`api`/`rtc_base`/`system_wrappers`/`common_audio` support code it needs to
build standalone. The `avx2` subfolder is an AVX2-optimized build variant of
the same sources, not a separate upstream library.

Upstream: WebRTC (`webrtc/modules/audio_processing` and support code),
https://webrtc.googlesource.com/src
Vendored snapshot from mid-2025 or later (references the abseil LTS
`20250512` nullability-annotation break; includes agc2 input volume
controller/stats reporter and aec3 api_call_jitter_metrics).
Local changes are recorded in `gaijin.patch`.
License: BSD 3-Clause (see `LICENSE`).
