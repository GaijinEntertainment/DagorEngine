# uuidv7

Small C++ library for generating and parsing UUID version 7 (RFC 9562)
identifiers: time-ordered UUIDs combining a millisecond timestamp with
random bits.

Upstream: nalgeon/uuidv7, https://github.com/nalgeon/uuidv7 (C++
implementation under src/uuidv7.cpp), public domain (Unlicense).
String formatting/parsing (uuidv7_snprintf, uuid7_from_string) were
added and optimized locally.
