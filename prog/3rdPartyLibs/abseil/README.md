# Abseil (absl)

Google's C++ common libraries: base utilities, containers, strings, time,
synchronization, status, logging, etc. Used by Dagor for a small subset of
utility code (see `jamfile` for the sources actually compiled).

Upstream: abseil-cpp, https://github.com/abseil/abseil-cpp
Vendored from the head (non-LTS) branch, snapshot circa 2024 (includes
`absl/log`, `absl/crc`, `absl/base/nullability.h`, `absl/base/no_destructor.h`).
Local changes are recorded in `gaijin.patch`.
License: Apache License 2.0 (see `LICENSE`).
