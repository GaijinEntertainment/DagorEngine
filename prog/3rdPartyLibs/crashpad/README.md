# Crashpad

Crashpad is Google's crash-reporting system: it captures process state at
crash time and writes minidumps for later upload/analysis. Used here for
client-side crash report collection.

Upstream: Crashpad, https://chromium.googlesource.com/crashpad/crashpad
(GitHub mirror: https://github.com/chromium/crashpad)
Vendored version: 0.8.0 (see package.h)

Bundles a few upstream third_party pieces as-is under third_party/
(getopt, lss, mini_chromium, xnu headers, zlib shim).
