libcurl
=======

Upstream: curl, https://curl.se (curl license).
Vendored version: 8.22.0 (https://curl.se/download/curl-8.22.0.zip).

Only `COPYING`, `README`, `CHANGES.md`, `include/curl/*.h`, `include/README.md`
and the `.c`/`.h` files under `lib/` are kept; the upstream build systems, tests,
docs and the `curl` tool are dropped. `jamfile` builds the library instead.

TLS is always OpenSSL from `3rdPartyLibs/openssl-*` (also on Windows and
Apple, so Schannel and SecureTransport stay unused). The jamfile switches off
every protocol but HTTP, HTTPS and WebSockets; `HTTP_ONLY` is not used because
it would switch WebSockets off as well.

Local deviations
----------------

- `lib/curl_config.h` picks a per-platform `lib/curl_config_<platform>.h` for
  the targets that build with `HAVE_CONFIG_H`. Windows and Xbox instead use
  `lib/config-win32.h`, a local copy of upstream's.
- `lib/config-win32.h`: OpenSSL instead of Schannel, no `USE_WIN32_CRYPTO` on
  Xbox, no Unix sockets, and upstream's `#error` that limits the header to its
  own MSVC 2010-2013 project files is gone.
- `lib/curl_config_android.h` must keep the multi wakeup off an emulated
  socketpair: that one reaches netd through `connect()`, which can block inside
  `curl_multi_init` and gave ANRs. `HAVE_EVENTFD` decides it now, with
  `HAVE_PIPE` and `HAVE_SOCKETPAIR` behind it; keep one of them on.
- `lib/curl_setup.h`: `_TARGET_XBOX` implies `CURL_WINDOWS_UWP`, because GDK
  restricts the same Win32 APIs but is not caught by the `WINAPI_FAMILY` probe.
- `lib/curlx/nonblock.c` plus `lib/curlx/nonblock_sony.h`: PS4/PS5 set the
  non-blocking flag through `sceNetSetsockopt`.
- `lib/curl_config_sony.h` disables MIME, and with it the form API: 8.x moved
  `formdata.c` behind `CURL_DISABLE_FORM_API`, so both have to go off together
  or `formdata.c` calls a `mime.c` that compiled to nothing.
- `lib/vtls/keylog.c` reads SSLKEYLOGFILE through `curl_getenv` as 7.85.0 did,
  not the plain `getenv` 8.x switched to, because PS4/PS5 have no `getenv` and
  the wrapper already answers NULL for them.
- `lib/curl_config_sony.h` sets `_CURL_LOCAL_MEMZERO`, because Orbis and
  Prospero set `__FreeBSD__` and curl then picks an `explicit_bzero` their libc
  does not have. Upstream marks that switch as transitional, so when it goes
  the platform chain in `lib/curl_setup.h` needs the exclusion instead.

Regenerating a config header
----------------------------

`lib/curl_config_linux.h` is CMake output for the host, with `SIZEOF_LONG` and
`SIZEOF_SIZE_T` replaced by the compiler's `__SIZEOF_*__` so one header serves
LP64 and ILP32:

    cmake -S curl-8.22.0 -B build -DCURL_USE_OPENSSL=ON -DCURL_ZLIB=ON \
      -DCURL_ZSTD=ON -DCURL_BROTLI=ON -DUSE_NGHTTP2=OFF -DUSE_LIBIDN2=OFF \
      -DCURL_USE_LIBSSH2=OFF -DCURL_USE_LIBPSL=OFF -DCURL_USE_GSSAPI=OFF \
      -DENABLE_THREADED_RESOLVER=ON -DBUILD_CURL_EXE=OFF -DBUILD_TESTING=OFF

CMake answers for the host it runs on, which is newer than the build farm, so
a fresh `HAVE_*` needs a second look before it is kept: `HAVE_MEMSET_EXPLICIT`
is off because `memset_explicit` is C23 and older glibc does not declare it.

The macOS, Android and PS4/PS5 headers cannot be probed on the build host, so
they are the Linux header with the known per-platform answers applied. A
`HAVE_*` left off only costs curl a portable fallback, so anything unproven for
a platform stays off there. `lib/curl_config.template.h` is upstream
`lib/curl_config-cmake.h.in`, the list of every symbol a config header may set.
