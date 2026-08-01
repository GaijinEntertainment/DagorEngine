This folder vendors PCRE (Perl Compatible Regular Expressions), the classic
PCRE 1.x C library by Philip Hazel, University of Cambridge, including the
JIT compiler (Zoltan Herczeg) and the C++ wrapper (Google Inc.).

Upstream: PCRE1 (classic PCRE, now end-of-life; superseded by PCRE2), https://www.pcre.org
Sources: https://ftp.exim.org/pub/pcre/ , https://github.com/PCRE2Project/pcre2
Vendored version: 8.31 (2012-07-06)

The `pcre/` subdirectory holds the unmodified upstream source tree; the
`win32/`, `linux32/`, `linux64/`, `macosx/` subdirectories hold
platform-specific generated `pcre_chartables.c` files.
