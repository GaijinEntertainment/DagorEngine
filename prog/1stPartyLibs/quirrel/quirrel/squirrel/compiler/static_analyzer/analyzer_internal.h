#pragma once

namespace SQCompilation
{


enum ReturnTypeBits
{
  RT_NOTHING = 1 << 0,
  RT_NULL = 1 << 1,
  RT_BOOL = 1 << 2,
  RT_NUMBER = 1 << 3,
  RT_STRING = 1 << 4,
  RT_TABLE = 1 << 5,
  RT_ARRAY = 1 << 6,
  RT_CLOSURE = 1 << 7,
  RT_FUNCTION_CALL = 1 << 8,
  RT_UNRECOGNIZED = 1 << 9,
  RT_THROW = 1 << 10,
  RT_CLASS = 1 << 11,
};


inline const char *enumFqn(Arena *arena, const char *enumName, const char *cname) {
  int32_t l1 = strlen(enumName);
  int32_t l2 = strlen(cname);
  int32_t l = l1 + 1 + l2 + 1;
  char *r = (char *)arena->allocate(l);
  snprintf(r, l, "%s.%s", enumName, cname);
  return r;
}


inline int32_t strhash(const char *s) {
  int32_t r = 0;
  while (*s) {
    r *= 31;
    r += *s;
    ++s;
  }

  return r;
}

struct StringHasher {
  int32_t operator()(const char *s) const {
    return strhash(s);
  }
};

struct StringEqualer {
  int32_t operator()(const char *a, const char *b) const {
    return strcmp(a, b) == 0;
  }
};

}
