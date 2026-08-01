# ONNX Runtime

Prebuilt [ONNX Runtime](https://onnxruntime.ai/) shared library (CPU), used by
`prog/gameLibs/textEmbed`'s `OrtEmbedder` to run the bge query encoder for
EdenDocs retrieval.

This directory holds **only the license** for source control. The runtime is
not built from source and its binaries are not committed: they are pulled from
the devtool package `$(_DEVTOOL)/onnxruntime-<ver>` and bundled next to the exe
by `prog/gameLibs/textEmbed/ort_bundle.jam` (the `.dll` / `.so` is loaded on
demand via `os_dll_load`, so there is no link-time dependency).

`ort_bundle.jam` registers this directory with `ExplicitLicenseUsed`, so the
generated `LICENSE-<exe>` carries the ONNX Runtime MIT license for every exe
that bundles the runtime.

- License: MIT (see `LICENSE`)
- Bundled version: pinned by `OrtVer` in `ort_bundle.jam` (currently 1.26.0)

When bumping `OrtVer`, re-check that upstream's `LICENSE` text still matches the
copy here.
