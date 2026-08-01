# posix

Dagor's internal POSIX-compatibility shim: platform-specific implementations
of POSIX functions missing on PS4/PS5, Nintendo Switch, and Windows/Xbox
(socket helpers, `access`, `timegm`, `inet_ntop`/`inet_pton`, etc). Mostly
original glue code written against each platform's native SDK, with a few
small snippets adapted from external sources:

- `src/inet_pton.c`, `src/inet_ntop.c`: classic BSD `inet_pton`/`inet_ntop`
  (originally by Paul Vixie for BIND), adapted from Android Bionic libc
  (BSD license). https://cs.android.com/android/platform/superproject/+/master:bionic/libc/
- `include/stl/jthread.h`: `std::jthread` drop-in replacement from
  https://codereview.stackexchange.com/questions/282843/jthread-drop-in-replacement
- `src/nswitch/gmtime_s.cpp`: light embedded `gmtime_s()` based on
  http://www.ethernut.de/api/gmtime_8c_source.html
