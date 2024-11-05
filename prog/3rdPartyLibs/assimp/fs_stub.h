#pragma once

inline int chdir(const char *) { return -1; }
inline char *realpath(const char *, const char *) { return nullptr; }
