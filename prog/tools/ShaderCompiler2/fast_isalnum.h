#pragma once
inline bool fast_isalnum(char c_)
{
  unsigned char c = (unsigned char)c_;
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool fast_isalnum_or_(char c) { return c == '_' || fast_isalnum(c); }

inline bool fast_isspace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }