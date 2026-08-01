# utf8

Header-only UTF-8 handling library for C++ (encoding/decoding, validation,
conversion between UTF-8/16/32). This copy is modified to use EASTL iterator
traits instead of the standard library.

Upstream: UTF8-CPP (utfcpp) by Nemanja Trifunovic,
https://github.com/nemtrif/utfcpp - vendored from the pre-3.0 API
(namespace `utf8`, ~v2.3.x era, before the 3.0 `utf8::unchecked` split).
