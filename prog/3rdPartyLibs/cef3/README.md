# CEF3 (Chromium Embedded Framework)

Vendored subset of CEF used to embed a Chromium-based browser (in-game UI,
tools). Only the libcef_dll wrapper sources and public C++/C API headers are
kept here; the prebuilt libcef binaries are fetched separately via devtools
(see select.jam).

Upstream: https://github.com/chromiumembedded/cef (BSD license, see the
LICENSE file under each version's libcef_dll directory).

Three versions are vendored side by side, selected per-platform in
select.jam:

- v2623 - CEF/Chromium branch 2623 (legacy layout, headers supplied by
  devtool, not vendored here).
- v4896 - CEF 100.0.24, Chromium 100.0.4896.127 (branch 4896).
- v4951 - CEF 101.0.15, Chromium 101.0.4951.54 (branch 4951).
