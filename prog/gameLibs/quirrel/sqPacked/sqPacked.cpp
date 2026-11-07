// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/sqPacked/sqPacked_api.h>

#include <sqmodules/sqmodules.h>
#include <squirrel/sqpcheader.h>
#include <stdint.h>
#include <string.h>
#include <squirrel.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqtable.h>
#include <squirrel/sqclass.h>
#include <squirrel/sqarray.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>
#include <squirrel/sqstring.h>
#include <math.h>
#include <sqrat.h>
#include <osApiWrappers/dag_mutex.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/utility.h>
#include <EASTL/numeric_limits.h>

namespace
{

enum PkKind : uint8_t
{
  PK_CONST,
  PK_BOOL_BITS,
  PK_INT,
  PK_FLOAT,
  PK_STRING,
  PK_STRING_DICT,
  PK_ROW_KEY,
  PK_NULLABLE,
  PK_OPTIONAL,
  PK_UNION,
  PK_LIST,
  PK_MAP,
  PK_STRUCT,
  PK_INSTANCE,
  PK_NATIVE,
};

enum PkType : uint8_t
{
  PT_NULL,
  PT_BOOL,
  PT_INT,
  PT_FLOAT,
  PT_STRING,
  PT_TABLE,
  PT_ARRAY,
  PT_INSTANCE,
  PT_POINT2,
  PT_POINT3,
  PT_POINT4,
  PT_IPOINT2,
  PT_IPOINT3,
  PT_IPOINT4,
  PT_E3DCOLOR,
  PT_TMATRIX,
  PT_COUNT,
};

static_assert(PT_COUNT <= 16, "a union column keeps the type of a row in 4 bits");

struct PkNativeInfo
{
  const char *name;
  uint8_t components;
  PkType componentType;
  const char *fields[4];
};

const PkNativeInfo pk_native_infos[PT_COUNT - PT_POINT2] = {
  {"Point2", 2, PT_FLOAT, {"x", "y"}},
  {"Point3", 3, PT_FLOAT, {"x", "y", "z"}},
  {"Point4", 4, PT_FLOAT, {"x", "y", "z", "w"}},
  {"IPoint2", 2, PT_INT, {"x", "y"}},
  {"IPoint3", 3, PT_INT, {"x", "y", "z"}},
  {"IPoint4", 4, PT_INT, {"x", "y", "z", "w"}},
  {"E3DCOLOR", 4, PT_INT, {"r", "g", "b", "a"}},
  {"TMatrix", 4, PT_POINT3, {}},
};

inline bool pk_is_native(uint32_t type) { return type >= PT_POINT2 && type < PT_COUNT; }

inline const PkNativeInfo &pk_native(uint32_t type) { return pk_native_infos[type - PT_POINT2]; }

bool pk_native_field(uint32_t type, const char *name, size_t len, uint32_t &component)
{
  const PkNativeInfo &info = pk_native(type);
  if (info.componentType == PT_POINT3)
    return false;
  for (uint32_t k = 0; k < info.components; k++)
    if (strlen(info.fields[k]) == len && memcmp(info.fields[k], name, len) == 0)
    {
      component = k;
      return true;
    }
  return false;
}

bool pk_native_index(uint32_t type, int64_t index, uint32_t &component)
{
  const PkNativeInfo &info = pk_native(type);
  if (info.componentType != PT_POINT3 || index < 0 || index >= info.components)
    return false;
  component = uint32_t(index);
  return true;
}

bool pk_native_key(uint32_t type, const SQObject &key, uint32_t &component)
{
  if (sq_type(key) == OT_STRING)
    return pk_native_field(type, _string(key)->_val, size_t(_string(key)->_len), component);
  return sq_type(key) == OT_INTEGER && pk_native_index(type, _integer(key), component);
}

struct PkNativeRaw
{
  float f[12];
  int32_t i[4];
};

void pk_raw(const Point2 &p, PkNativeRaw &raw)
{
  raw.f[0] = p.x;
  raw.f[1] = p.y;
}

void pk_raw(const Point3 &p, PkNativeRaw &raw)
{
  raw.f[0] = p.x;
  raw.f[1] = p.y;
  raw.f[2] = p.z;
}

void pk_raw(const Point4 &p, PkNativeRaw &raw)
{
  raw.f[0] = p.x;
  raw.f[1] = p.y;
  raw.f[2] = p.z;
  raw.f[3] = p.w;
}

void pk_raw(const IPoint2 &p, PkNativeRaw &raw)
{
  raw.i[0] = p.x;
  raw.i[1] = p.y;
}

void pk_raw(const IPoint3 &p, PkNativeRaw &raw)
{
  raw.i[0] = p.x;
  raw.i[1] = p.y;
  raw.i[2] = p.z;
}

void pk_raw(const IPoint4 &p, PkNativeRaw &raw)
{
  raw.i[0] = p.x;
  raw.i[1] = p.y;
  raw.i[2] = p.z;
  raw.i[3] = p.w;
}

void pk_raw(const E3DCOLOR &c, PkNativeRaw &raw)
{
  raw.i[0] = c.r;
  raw.i[1] = c.g;
  raw.i[2] = c.b;
  raw.i[3] = c.a;
}

void pk_raw(const TMatrix &tm, PkNativeRaw &raw)
{
  for (uint32_t col = 0; col < 4; col++)
    for (uint32_t k = 0; k < 3; k++)
      raw.f[col * 3 + k] = tm.m[col][k];
}

struct PkHeader
{
  uint32_t size;
  uint32_t root;
  uint32_t stringCount;
  uint32_t stringOffsets;
  uint32_t stringBytes;
  uint32_t keyIndex;
  uint32_t keyIndexMask;
};

struct PkColumn
{
  uint8_t kind;
  uint8_t width;
  uint16_t extra;
  uint32_t rows;
};

struct PkValue
{
  SQInteger i;
  SQFloat f;
  uint32_t ref;
  uint32_t row;
  PkType type;
};

const uint32_t PK_NO_POS = 0xFFFFFFFFu;
const uint32_t PK_MAX_DENSE_STRUCT_FIELDS = 64;
const uint32_t PK_MAX_STRUCT_FIELDS = 0xFFFF;
const uint32_t PK_MIN_COLUMN_ROWS = 16;
const uint32_t PK_MAP_ENTRY_BYTES = 4;

inline bool pk_field_needs_column(uint32_t rows_with_field, uint32_t rows)
{
  uint32_t optionalColumnBytes = 24 + (rows + 63) / 64 * 12;
  return rows_with_field >= PK_MIN_COLUMN_ROWS &&
         (rows_with_field == rows || uint64_t(rows_with_field) * PK_MAP_ENTRY_BYTES >= optionalColumnBytes);
}
const uint32_t PK_MAX_DEPTH = 256;
const uint32_t PK_STRING_SLOTS = 2048;

inline uint64_t pk_read(const uint8_t *p, uint32_t width, uint32_t i)
{
  switch (width)
  {
    case 1: return p[i];
    case 2:
    {
      uint16_t x;
      memcpy(&x, p + size_t(i) * 2, 2);
      return x;
    }
    case 4:
    {
      uint32_t x;
      memcpy(&x, p + size_t(i) * 4, 4);
      return x;
    }
    default:
    {
      uint64_t x;
      memcpy(&x, p + size_t(i) * 8, 8);
      return x;
    }
  }
}

inline uint32_t pk_u32(const uint8_t *p)
{
  uint32_t x;
  memcpy(&x, p, 4);
  return x;
}

inline uint32_t pk_align4(uint32_t n) { return (n + 3) & ~3u; }

inline uint32_t pk_popcount(uint64_t x)
{
  x = x - ((x >> 1) & 0x5555555555555555ull);
  x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull);
  x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0Full;
  return uint32_t((x * 0x0101010101010101ull) >> 56);
}

inline uint8_t pk_width(uint64_t max_value)
{
  return max_value < 0x100 ? 1 : max_value < 0x10000 ? 2 : max_value < 0x100000000ull ? 4 : 8;
}

inline uint64_t pk_float_bits(double f)
{
  uint64_t bits;
  memcpy(&bits, &f, sizeof(bits));
  return bits;
}

inline bool pk_is_nan(double f) { return (pk_float_bits(f) & 0x7FFFFFFFFFFFFFFFull) > 0x7FF0000000000000ull; }

int pk_cmp_float_key(SQFloat a, SQFloat b)
{
  bool aNan = pk_is_nan(a), bNan = pk_is_nan(b);
  if (aNan != bNan)
    return aNan ? 1 : -1;
  if (!aNan && a < b)
    return -1;
  if (!aNan && a > b)
    return 1;
  uint64_t x = pk_float_bits(a), y = pk_float_bits(b);
  return x < y ? -1 : (x > y ? 1 : 0);
}

int pk_cmp_key(const PkValue &a, const PkValue &b)
{
  if (a.type != b.type)
    return a.type < b.type ? -1 : 1;
  if (a.type == PT_FLOAT)
    return pk_cmp_float_key(a.f, b.f);
  return a.i < b.i ? -1 : (a.i > b.i ? 1 : 0);
}


struct PqKey
{
  uint8_t type;
  int64_t v;
  bool operator<(const PqKey &o) const { return type != o.type ? type < o.type : v < o.v; }
  bool operator==(const PqKey &o) const { return type == o.type && v == o.v; }
};

struct PqKeyHash
{
  size_t operator()(const PqKey &k) const { return size_t((uint64_t(k.v) * 0x9E3779B97F4A7C15ull) >> 16 ^ k.type); }
};

using PqKeySet = eastl::unordered_set<PqKey, PqKeyHash>;

struct PqIndex
{
  uint32_t typeBegin[8] = {};
  uint8_t keyWidth = 4;
  uint8_t offsetWidth = 2;
  uint8_t posWidth = 2;
  eastl::vector<uint8_t> keys;
  eastl::vector<uint8_t> offsets;
  eastl::vector<uint8_t> positions;

  size_t bytes() const { return sizeof(*this) + keys.size() + offsets.size() + positions.size(); }

  int64_t key(uint32_t i) const
  {
    if (keyWidth == 4)
    {
      int32_t x;
      memcpy(&x, keys.data() + size_t(i) * 4, 4);
      return x;
    }
    int64_t x;
    memcpy(&x, keys.data() + size_t(i) * 8, 8);
    return x;
  }

  uint32_t position(uint32_t i) const { return uint32_t(pk_read(positions.data(), posWidth, i)); }

  bool find(const PqKey &k, uint32_t &begin, uint32_t &end) const
  {
    uint32_t lo = typeBegin[k.type], hi = typeBegin[k.type + 1];
    while (lo < hi)
    {
      uint32_t mid = (lo + hi) >> 1;
      int64_t x = key(mid);
      if (x == k.v)
      {
        begin = uint32_t(pk_read(offsets.data(), offsetWidth, mid));
        end = uint32_t(pk_read(offsets.data(), offsetWidth, mid + 1));
        return true;
      }
      if (x < k.v)
        lo = mid + 1;
      else
        hi = mid;
    }
    return false;
  }

  void build(eastl::vector<eastl::pair<PqKey, uint32_t>> &entries)
  {
    eastl::sort(entries.begin(), entries.end(),
      [](const auto &a, const auto &b) { return a.first < b.first || (!(b.first < a.first) && a.second < b.second); });
    entries.erase(eastl::unique(entries.begin(), entries.end(),
                    [](const auto &a, const auto &b) { return !(a.first < b.first) && !(b.first < a.first) && a.second == b.second; }),
      entries.end());
    uint32_t maxPos = 0;
    bool narrowKeys = true;
    for (auto &e : entries)
    {
      maxPos = eastl::max(maxPos, e.second);
      narrowKeys = narrowKeys && e.first.v >= INT32_MIN && e.first.v <= INT32_MAX;
    }
    keyWidth = narrowKeys ? 4 : 8;
    posWidth = maxPos < 0x10000 ? 2 : 4;
    offsetWidth = entries.size() < 0x10000 ? 2 : 4;
    eastl::vector<uint32_t> keyStarts;
    for (size_t i = 0; i < entries.size(); i++)
    {
      const PqKey &k = entries[i].first;
      if (i == 0 || entries[i - 1].first < k)
      {
        keyStarts.push_back(uint32_t(i));
        keys.insert(keys.end(), keyWidth, 0);
        if (keyWidth == 4)
        {
          int32_t x = int32_t(k.v);
          memcpy(keys.data() + keys.size() - 4, &x, 4);
        }
        else
          memcpy(keys.data() + keys.size() - 8, &k.v, 8);
        for (uint32_t t = k.type + 1; t < 8; t++)
          typeBegin[t] = uint32_t(keyStarts.size());
      }
      uint32_t pos = entries[i].second;
      positions.insert(positions.end(), (const uint8_t *)&pos, (const uint8_t *)&pos + posWidth);
    }
    keyStarts.push_back(uint32_t(entries.size()));
    for (uint32_t start : keyStarts)
      offsets.insert(offsets.end(), (const uint8_t *)&start, (const uint8_t *)&start + offsetWidth);
    keys.shrink_to_fit();
    positions.shrink_to_fit();
    offsets.shrink_to_fit();
  }
};

struct PqIndexKey
{
  uint32_t collRef = 0;
  uint32_t collRow = 0;
  eastl::string pathText;
};

struct PqIndexKeyRef
{
  uint32_t collRef;
  uint32_t collRow;
  const eastl::string &pathText;
};

struct PqIndexKeyHash
{
  static size_t combine(uint32_t coll_ref, uint32_t coll_row, const eastl::string &path_text)
  {
    return eastl::hash<eastl::string>()(path_text) ^ ((uint64_t(coll_ref) << 32 | coll_row) * 0x9E3779B97F4A7C15ull);
  }
  size_t operator()(const PqIndexKey &k) const { return combine(k.collRef, k.collRow, k.pathText); }
  size_t operator()(const PqIndexKeyRef &k) const { return combine(k.collRef, k.collRow, k.pathText); }
};

struct PqIndexKeyEqual
{
  bool operator()(const PqIndexKey &a, const PqIndexKey &b) const
  {
    return a.collRef == b.collRef && a.collRow == b.collRow && a.pathText == b.pathText;
  }
  bool operator()(const PqIndexKey &a, const PqIndexKeyRef &b) const
  {
    return a.collRef == b.collRef && a.collRow == b.collRow && a.pathText == b.pathText;
  }
};

uint64_t pk_next_tree_serial = 1;

struct PkClass
{
  SQObjectPtr cls;
  SQObjectPtr setState;
  bool setStateTakesClasses = false;
};

struct PkTree
{
  uint32_t refCount = 0;
  uint64_t serial = pk_next_tree_serial++;
  eastl::vector<uint8_t> buf;
  eastl::unordered_map<PqIndexKey, eastl::unique_ptr<PqIndex>, PqIndexKeyHash, PqIndexKeyEqual> indexes;
  eastl::vector<PkClass> classes;
  SQObjectPtr availableClasses;
  struct StringSlot
  {
    uint32_t id = ~0u;
    SQObjectPtr str;
  };
  mutable eastl::vector<StringSlot> stringSlots;

  const PkHeader &header() const { return *(const PkHeader *)buf.data(); }
  const PkColumn *column(uint32_t ref) const { return (const PkColumn *)(buf.data() + ref); }
  static const uint8_t *payload(const PkColumn *c) { return (const uint8_t *)(c + 1); }

  void string(uint32_t id, const char *&s, uint32_t &len) const
  {
    const PkHeader &h = header();
    const uint8_t *offsets = buf.data() + h.stringOffsets + size_t(id) * 4;
    uint32_t begin = pk_u32(offsets);
    s = (const char *)buf.data() + h.stringBytes + begin;
    len = pk_u32(offsets + 4) - begin;
  }

  bool stringEquals(uint32_t id, const char *str, SQInteger str_len) const
  {
    const char *s;
    uint32_t len;
    string(id, s, len);
    return SQInteger(len) == str_len && memcmp(s, str, len) == 0;
  }

  bool stringEquals(uint32_t id, const SQString *str) const { return stringEquals(id, str->_val, str->_len); }

  const SQObjectPtr &stringObject(HSQUIRRELVM v, uint32_t id) const
  {
    if (stringSlots.empty())
    {
      uint32_t slots = 1;
      while (slots < header().stringCount && slots < PK_STRING_SLOTS)
        slots <<= 1;
      stringSlots.resize(slots);
    }
    StringSlot &slot = stringSlots[id & (stringSlots.size() - 1)];
    if (slot.id != id)
    {
      const char *s;
      uint32_t len;
      string(id, s, len);
      slot.str = SQString::Create(_ss(v), s, SQInteger(len));
      slot.id = id;
    }
    return slot.str;
  }

  bool findKeyString(const char *str, SQInteger len, SQHash hash, uint32_t &id) const
  {
    const PkHeader &h = header();
    const uint8_t *index = buf.data() + h.keyIndex;
    for (uint32_t i = uint32_t(hash) & h.keyIndexMask;; i = (i + 1) & h.keyIndexMask)
    {
      uint32_t slot = pk_u32(index + size_t(i) * 4);
      if (!slot)
        return false;
      if (stringEquals(slot - 1, str, len))
      {
        id = slot - 1;
        return true;
      }
    }
  }

  bool findKeyString(const SQString *str, uint32_t &id) const { return findKeyString(str->_val, str->_len, str->_hash, id); }

  int64_t stringRank(const char *str, SQInteger len) const
  {
    uint32_t lo = 0, hi = header().stringCount;
    while (lo < hi)
    {
      uint32_t mid = (lo + hi) >> 1;
      const char *s;
      uint32_t slen;
      string(mid, s, slen);
      int r = memcmp(s, str, size_t(eastl::min(SQInteger(slen), len)));
      if (r == 0)
        r = SQInteger(slen) < len ? -1 : (SQInteger(slen) > len ? 1 : 0);
      if (r == 0)
        return int64_t(mid) * 2;
      if (r < 0)
        lo = mid + 1;
      else
        hi = mid;
    }
    return int64_t(lo) * 2 - 1;
  }

  static bool presence(const PkColumn *c, uint32_t row, uint32_t &rank)
  {
    const uint8_t *ranks = payload(c) + 4;
    const uint8_t *bits = ranks + size_t((c->rows + 63) >> 6) * 4;
    uint64_t word;
    memcpy(&word, bits + size_t(row >> 6) * 8, 8);
    uint64_t bit = uint64_t(1) << (row & 63);
    if (!(word & bit))
      return false;
    rank = pk_u32(ranks + size_t(row >> 6) * 4) + pk_popcount(word & (bit - 1));
    return true;
  }

  bool element(uint32_t ref, uint32_t row, PkValue &out) const
  {
    for (;;)
    {
      const PkColumn *c = column(ref);
      const uint8_t *p = payload(c);
      switch (c->kind)
      {
        case PK_CONST:
          out.type = PkType(c->extra);
          if (out.type == PT_FLOAT)
            memcpy(&out.f, p, sizeof(SQFloat));
          else
          {
            int64_t x;
            memcpy(&x, p, 8);
            out.i = SQInteger(x);
          }
          return true;
        case PK_BOOL_BITS:
          out.type = PT_BOOL;
          out.i = (p[row >> 3] >> (row & 7)) & 1;
          return true;
        case PK_INT:
        {
          uint64_t base;
          memcpy(&base, p, 8);
          out.type = PT_INT;
          out.i = SQInteger(base + pk_read(p + 8, c->width, row));
          return true;
        }
        case PK_FLOAT:
          out.type = PT_FLOAT;
          memcpy(&out.f, p + size_t(row) * sizeof(SQFloat), sizeof(SQFloat));
          return true;
        case PK_STRING:
          out.type = PT_STRING;
          out.i = SQInteger(pk_read(p, c->width, row));
          return true;
        case PK_STRING_DICT:
        {
          uint32_t dictCount = pk_u32(p);
          const uint8_t *indices = p + pk_align4(4 + dictCount * c->extra);
          out.type = PT_STRING;
          out.i = SQInteger(pk_read(p + 4, c->extra, uint32_t(pk_read(indices, c->width, row))));
          return true;
        }
        case PK_ROW_KEY: ref = pk_u32(p); continue;
        case PK_NULLABLE:
        case PK_OPTIONAL:
        {
          uint32_t rank;
          if (!presence(c, row, rank))
          {
            if (c->kind == PK_OPTIONAL)
              return false;
            out.type = PT_NULL;
            return true;
          }
          ref = pk_u32(p);
          row = rank;
          continue;
        }
        case PK_UNION:
        {
          const uint8_t *tags = p + PT_COUNT * 4;
          const uint8_t *positions = tags + pk_align4((c->rows + 1) / 2);
          uint32_t tag = (tags[row >> 1] >> ((row & 1) * 4)) & 15;
          ref = pk_u32(p + tag * 4);
          row = uint32_t(pk_read(positions, c->width, row));
          continue;
        }
        case PK_LIST:
          out.type = PT_ARRAY;
          out.ref = ref;
          out.row = row;
          return true;
        case PK_INSTANCE:
          out.type = PT_INSTANCE;
          out.ref = ref;
          out.row = row;
          return true;
        case PK_NATIVE:
          out.type = PkType(c->extra);
          out.ref = ref;
          out.row = row;
          return true;
        default:
          out.type = PT_TABLE;
          out.ref = ref;
          out.row = row;
          return true;
      }
    }
  }

  void nativeComponent(uint32_t ref, uint32_t row, uint32_t component, PkValue &out) const
  {
    element(pk_u32(payload(column(ref)) + size_t(component) * 4), row, out);
  }

  void nativeRaw(uint32_t ref, uint32_t row, PkNativeRaw &raw, uint32_t first_float) const
  {
    const PkColumn *c = column(ref);
    const PkNativeInfo &info = pk_native(c->extra);
    for (uint32_t k = 0; k < info.components; k++)
    {
      PkValue x;
      element(pk_u32(payload(c) + size_t(k) * 4), row, x);
      if (info.componentType == PT_POINT3)
        nativeRaw(x.ref, x.row, raw, k * 3);
      else if (info.componentType == PT_FLOAT)
        raw.f[first_float + k] = float(x.f);
      else
        raw.i[k] = int32_t(x.i);
    }
  }

  static uint32_t restOf(const PkColumn *c) { return c->width ? pk_u32(payload(c) + size_t(c->extra) * 8) : 0; }

  uint32_t restKeyId(const PkColumn *rest, uint32_t pos) const
  {
    PkValue k;
    element(pk_u32(payload(rest)), pos, k);
    return uint32_t(k.i);
  }

  uint32_t fieldsBefore(const PkColumn *c, const PkColumn *rest, uint32_t rb, uint32_t re, uint32_t pos) const
  {
    const uint8_t *ids = payload(c);
    uint32_t restCount = re - rb;
    uint32_t lo = pos > restCount ? pos - restCount : 0, hi = eastl::min(pos, uint32_t(c->extra));
    while (lo < hi)
    {
      uint32_t i = (lo + hi) >> 1;
      if (pk_u32(ids + size_t(i) * 4) < restKeyId(rest, rb + pos - i - 1))
        lo = i + 1;
      else
        hi = i;
    }
    return lo;
  }

  bool hasRow(uint32_t ref, uint32_t row) const
  {
    for (;;)
    {
      const PkColumn *c = column(ref);
      if (c->kind == PK_ROW_KEY)
      {
        ref = pk_u32(payload(c));
        continue;
      }
      uint32_t rank;
      return c->kind != PK_OPTIONAL || presence(c, row, rank);
    }
  }

  uint32_t nextPresent(const PkColumn *c, uint32_t row, uint32_t pos, uint32_t end, uint32_t *slot = nullptr, bool *in_rest = nullptr,
    const uint32_t *prev_slot = nullptr, bool prev_in_rest = false) const
  {
    uint32_t slotStore;
    bool inRestStore;
    slot = slot ? slot : &slotStore;
    in_rest = in_rest ? in_rest : &inRestStore;
    *slot = pos;
    *in_rest = false;
    if (c->kind != PK_STRUCT || pos >= end)
      return pos;
    const uint8_t *ids = payload(c);
    uint32_t fields = c->extra;
    if (!c->width)
    {
      for (uint32_t i = pos; i < fields; i++)
        if (hasRow(pk_u32(ids + (size_t(fields) + i) * 4), row))
        {
          *slot = i;
          return i;
        }
      return end;
    }
    uint32_t rest = restOf(c);
    const PkColumn *m = rest ? column(rest) : nullptr;
    uint32_t rb = 0, re = 0;
    if (m)
      range(m, row, rb, re);
    uint32_t i = !m ? pos : !prev_slot ? fieldsBefore(c, m, rb, re, pos) : prev_in_rest ? pos - (*prev_slot + 1 - rb) : *prev_slot + 1;
    uint32_t j = rb + (pos - i);
    uint32_t restId = j < re ? restKeyId(m, j) : PK_NO_POS;
    for (; i < fields; i++)
    {
      if (restId < pk_u32(ids + size_t(i) * 4))
        break;
      if (hasRow(pk_u32(ids + (size_t(fields) + i) * 4), row))
      {
        *slot = i;
        return i + (j - rb);
      }
    }
    if (j >= re)
      return end;
    *slot = j;
    *in_rest = true;
    return i + (j - rb);
  }

  uint32_t nextPresentAfter(const PkColumn *c, uint32_t row, uint32_t pos, uint32_t end, uint32_t &slot, bool &in_rest) const
  {
    uint32_t prevSlot = slot;
    bool prevInRest = in_rest;
    return nextPresent(c, row, pos + 1, end, &slot, &in_rest, &prevSlot, prevInRest);
  }

  bool slotKey(const PkColumn *c, uint32_t row, uint32_t slot, bool in_rest, PkValue &out) const
  {
    if (c->kind != PK_STRUCT)
      return keyAt(c, row, slot, out);
    if (in_rest)
      return element(pk_u32(payload(column(restOf(c)))), slot, out);
    out.type = PT_STRING;
    out.i = pk_u32(payload(c) + size_t(slot) * 4);
    return true;
  }

  bool slotValue(const PkColumn *c, uint32_t row, uint32_t slot, bool in_rest, PkValue &out) const
  {
    if (c->kind != PK_STRUCT)
      return valueAt(c, row, slot, out);
    if (in_rest)
      return element(pk_u32(payload(column(restOf(c))) + 4), slot, out);
    return element(pk_u32(payload(c) + (size_t(c->extra) + slot) * 4), row, out);
  }

  bool restSlot(const PkColumn *c, uint32_t row, uint32_t pos, uint32_t &slot) const
  {
    uint32_t rest = restOf(c);
    if (!rest)
    {
      slot = pos;
      return false;
    }
    const PkColumn *m = column(rest);
    uint32_t rb, re;
    range(m, row, rb, re);
    const uint8_t *ids = payload(c);
    uint32_t fields = c->extra, restCount = re - rb;
    uint32_t lo = fieldsBefore(c, m, rb, re, pos);
    uint32_t j = pos - lo;
    if (lo < fields && (j >= restCount || pk_u32(ids + size_t(lo) * 4) < restKeyId(m, rb + j)))
    {
      slot = lo;
      return false;
    }
    slot = rb + j;
    return true;
  }

  bool findStructKey(const PkColumn *c, uint32_t row, uint32_t key_id, uint32_t &pos) const
  {
    uint32_t field = 0;
    bool inColumns = findField(c, key_id, field);
    uint32_t rest = restOf(c);
    if (!rest)
    {
      pos = field;
      return inColumns;
    }
    const PkColumn *m = column(rest);
    uint32_t rb, re;
    range(m, row, rb, re);
    uint32_t lo = rb, hi = re;
    while (lo < hi)
    {
      uint32_t mid = (lo + hi) >> 1;
      if (restKeyId(m, mid) < key_id)
        lo = mid + 1;
      else
        hi = mid;
    }
    if (inColumns)
    {
      pos = field + (lo - rb);
      return true;
    }
    if (lo == re || restKeyId(m, lo) != key_id)
      return false;
    const uint8_t *ids = payload(c);
    uint32_t a = 0, b = c->extra;
    while (a < b)
    {
      uint32_t mid = (a + b) >> 1;
      if (pk_u32(ids + size_t(mid) * 4) < key_id)
        a = mid + 1;
      else
        b = mid;
    }
    pos = a + (lo - rb);
    return true;
  }

  void range(const PkColumn *c, uint32_t row, uint32_t &begin, uint32_t &end) const
  {
    if (c->kind == PK_STRUCT)
    {
      begin = 0;
      end = c->extra;
      if (c->width)
      {
        const PkColumn *m = column(restOf(c));
        const uint8_t *offsets = payload(m) + 8;
        end += uint32_t(pk_read(offsets, m->width, row + 1) - pk_read(offsets, m->width, row));
      }
      return;
    }
    const uint8_t *offsets = payload(c) + (c->kind == PK_LIST ? 4 : 8);
    begin = uint32_t(pk_read(offsets, c->width, row));
    end = uint32_t(pk_read(offsets, c->width, row + 1));
  }

  bool keyAt(const PkColumn *c, uint32_t row, uint32_t pos, PkValue &out) const
  {
    switch (c->kind)
    {
      case PK_STRUCT:
      {
        uint32_t slot;
        bool inRest = restSlot(c, row, pos, slot);
        return slotKey(c, row, slot, inRest, out);
      }
      case PK_MAP: return element(pk_u32(payload(c)), pos, out);
      default:
      {
        uint32_t begin, end;
        range(c, row, begin, end);
        out.type = PT_INT;
        out.i = SQInteger(pos - begin);
        return true;
      }
    }
  }

  bool valueAt(const PkColumn *c, uint32_t row, uint32_t pos, PkValue &out) const
  {
    switch (c->kind)
    {
      case PK_STRUCT:
      {
        uint32_t slot;
        bool inRest = restSlot(c, row, pos, slot);
        return slotValue(c, row, slot, inRest, out);
      }
      case PK_MAP: return element(pk_u32(payload(c) + 4), pos, out);
      default: return element(pk_u32(payload(c)), pos, out);
    }
  }

  bool keyEquals(const PkValue &k, const SQObject &key) const
  {
    switch (k.type)
    {
      case PT_STRING: return sq_type(key) == OT_STRING && stringEquals(uint32_t(k.i), _string(key));
      case PT_INT: return sq_type(key) == OT_INTEGER && _integer(key) == k.i;
      case PT_FLOAT: return sq_type(key) == OT_FLOAT && pk_cmp_float_key(_float(key), k.f) == 0;
      case PT_BOOL: return sq_type(key) == OT_BOOL && (_integer(key) != 0) == (k.i != 0);
      default: return false;
    }
  }

  bool keyValue(const SQObject &key, PkValue &out) const
  {
    switch (sq_type(key))
    {
      case OT_STRING:
      {
        uint32_t id;
        if (!findKeyString(_string(key), id))
          return false;
        out.type = PT_STRING;
        out.i = id;
        return true;
      }
      case OT_INTEGER:
        out.type = PT_INT;
        out.i = _integer(key);
        return true;
      case OT_FLOAT:
        out.type = PT_FLOAT;
        out.f = _float(key);
        return true;
      case OT_BOOL:
        out.type = PT_BOOL;
        out.i = _integer(key) != 0;
        return true;
      default: return false;
    }
  }

  bool findInMap(const PkColumn *c, uint32_t row, const PkValue &key, uint32_t &pos) const
  {
    uint32_t lo, hi;
    range(c, row, lo, hi);
    uint32_t keysRef = pk_u32(payload(c));
    const PkColumn *keys = column(keysRef);
    if (keys->kind == PK_STRING && key.type == PT_STRING)
    {
      const uint8_t *ids = payload(keys);
      uint64_t id = uint64_t(key.i);
      while (lo < hi)
      {
        uint32_t mid = (lo + hi) >> 1;
        uint64_t x = pk_read(ids, keys->width, mid);
        if (x == id)
        {
          pos = mid;
          return true;
        }
        if (x < id)
          lo = mid + 1;
        else
          hi = mid;
      }
      return false;
    }
    while (lo < hi)
    {
      uint32_t mid = (lo + hi) >> 1;
      PkValue k;
      element(keysRef, mid, k);
      int r = pk_cmp_key(k, key);
      if (r == 0)
      {
        pos = mid;
        return true;
      }
      if (r < 0)
        lo = mid + 1;
      else
        hi = mid;
    }
    return false;
  }

  bool findField(const PkColumn *c, uint32_t key_id, uint32_t &field) const
  {
    const uint8_t *ids = payload(c);
    uint32_t lo = 0, hi = c->extra;
    while (lo < hi)
    {
      uint32_t mid = (lo + hi) >> 1;
      uint32_t x = pk_u32(ids + size_t(mid) * 4);
      if (x == key_id)
      {
        field = mid;
        return true;
      }
      if (x < key_id)
        lo = mid + 1;
      else
        hi = mid;
    }
    return false;
  }

  bool find(const PkColumn *c, uint32_t row, const SQObject &key, uint32_t &pos) const
  {
    switch (c->kind)
    {
      case PK_STRUCT:
      {
        uint32_t id;
        return sq_type(key) == OT_STRING && findKeyString(_string(key), id) && findStructKey(c, row, id, pos);
      }
      case PK_MAP:
      {
        PkValue k;
        return keyValue(key, k) && findInMap(c, row, k, pos);
      }
      default:
      {
        if (!sq_isnumeric(key))
          return false;
        SQInteger idx = tointeger(key);
        uint32_t begin, end;
        range(c, row, begin, end);
        if (idx < 0 || idx >= SQInteger(end - begin))
          return false;
        pos = begin + uint32_t(idx);
        return true;
      }
    }
  }
};


int pk_count_of_params(const SQObjectPtr &f)
{
  if (sq_isclosure(f))
    return int(_closure(f)->_function->_nparameters);
  if (sq_isnativeclosure(f))
    return int(_nativeclosure(f)->_nparamscheck);
  return -1;
}

template <typename T>
void pk_raw_of_instance(const SQObject &o, PkNativeRaw &raw)
{
  if (const T *p = Sqrat::ClassType<T>::GetInstanceFromObj(o))
    pk_raw(*p, raw);
}

template <typename Src>
class PkStrMap
{
public:
  using Str = typename Src::Str;

  uint32_t size() const { return count; }
  bool empty() const { return count == 0; }

  uint32_t &operator[](const Str &s)
  {
    if ((count + 1) * 2 > uint32_t(slots.size()))
      grow();
    for (uint32_t i = Src::slotHash(s) & mask;; i = (i + 1) & mask)
    {
      Slot &slot = slots[i];
      if (!slot.used)
      {
        slot.key = s;
        slot.value = 0;
        slot.used = true;
        count++;
        return slot.value;
      }
      if (typename Src::StrEqual()(slot.key, s))
        return slot.value;
    }
  }

  template <typename F>
  void forEach(F f)
  {
    for (Slot &slot : slots)
      if (slot.used)
        f(slot.key, slot.value);
  }

private:
  struct Slot
  {
    Str key{};
    uint32_t value = 0;
    bool used = false;
  };
  eastl::vector<Slot> slots;
  uint32_t mask = 0;
  uint32_t count = 0;

  void grow()
  {
    eastl::vector<Slot> old;
    old.swap(slots);
    slots.resize(old.empty() ? 16 : old.size() * 2);
    mask = uint32_t(slots.size() - 1);
    for (const Slot &slot : old)
      if (slot.used)
      {
        uint32_t i = Src::slotHash(slot.key) & mask;
        while (slots[i].used)
          i = (i + 1) & mask;
        slots[i] = slot;
      }
  }
};

class PkSqSource
{
public:
  using Val = SQObject;
  using Str = const SQString *;
  using StrEqual = eastl::equal_to<const SQString *>;

  HSQUIRRELVM vm = nullptr;
  SQObjectPtr availableClasses;
  eastl::vector<SQObjectPtr> classes;

  static void chars(Str s, const char *&p, uint32_t &len)
  {
    p = s->_val;
    len = uint32_t(s->_len);
  }
  static SQHash hash(Str s) { return s->_hash; }
  static uint32_t slotHash(Str s) { return uint32_t((uint64_t(uintptr_t(s)) >> 4) * 0x9E3779B97F4A7C15ull >> 32); }

  int typeOf(const SQObject &o)
  {
    switch (sq_type(o))
    {
      case OT_NULL: return PT_NULL;
      case OT_BOOL: return PT_BOOL;
      case OT_INTEGER: return PT_INT;
      case OT_FLOAT: return PT_FLOAT;
      case OT_STRING: return PT_STRING;
      case OT_TABLE: return PT_TABLE;
      case OT_ARRAY: return PT_ARRAY;
      case OT_INSTANCE: return instanceType(o);
      default: return -1;
    }
  }
  static bool boolOf(const SQObject &o) { return _integer(o) != 0; }
  static int64_t intOf(const SQObject &o) { return _integer(o); }
  static SQFloat floatOf(const SQObject &o) { return _float(o); }
  static Str str(const SQObject &o) { return _string(o); }
  static uint32_t arrayLen(const SQObject &o) { return uint32_t(_array(o)->Size()); }
  static SQObject arrayAt(const SQObject &o, uint32_t i) { return _array(o)->_values[i]; }

  template <typename F>
  static void forEachEntry(const SQObject &table, F f)
  {
    SQObjectPtr walked(table);
    _table(table)->ForEachSlot(f);
  }

  template <typename F>
  static void forEachItem(const SQObject &arr, F f)
  {
    SQObjectPtr walked(arr);
    const SQArray *a = _array(arr);
    for (SQInteger i = 0; i < a->Size(); i++)
      f(SQObject(a->_values[i]));
  }

  static void nativeOf(const SQObject &o, uint32_t type, PkNativeRaw &raw)
  {
    switch (type)
    {
      case PT_POINT2: pk_raw_of_instance<Point2>(o, raw); break;
      case PT_POINT3: pk_raw_of_instance<Point3>(o, raw); break;
      case PT_POINT4: pk_raw_of_instance<Point4>(o, raw); break;
      case PT_IPOINT2: pk_raw_of_instance<IPoint2>(o, raw); break;
      case PT_IPOINT3: pk_raw_of_instance<IPoint3>(o, raw); break;
      case PT_IPOINT4: pk_raw_of_instance<IPoint4>(o, raw); break;
      case PT_E3DCOLOR: pk_raw_of_instance<E3DCOLOR>(o, raw); break;
      default: pk_raw_of_instance<TMatrix>(o, raw); break;
    }
  }

  bool prepareInstance(const SQObject &o, bool &fresh, const char *&error)
  {
    const SQInstance *inst = _instance(o);
    fresh = states.count(inst) == 0;
    if (!fresh)
      return true;
    uint32_t idx;
    SQObjectPtr state;
    if (!classIndex(inst->_class, idx, error) || !callGetState(o, state, error))
      return false;
    states[inst] = state;
    return true;
  }
  SQObject instanceState(const SQObject &o) { return states[_instance(o)]; }
  uint32_t instanceClass(const SQObject &o) { return classIds[_instance(o)->_class]; }

private:
  eastl::unordered_map<const SQClass *, uint32_t> classIds;
  eastl::unordered_map<const SQClass *, int> classTypes;
  eastl::unordered_map<const SQInstance *, SQObjectPtr> states;
  SQObjectPtr getStateName;

  static int nativeTypeOfTag(SQUserPointer tag)
  {
    if (tag == Sqrat::TypeTag<Point2>::get())
      return PT_POINT2;
    if (tag == Sqrat::TypeTag<Point3>::get())
      return PT_POINT3;
    if (tag == Sqrat::TypeTag<Point4>::get())
      return PT_POINT4;
    if (tag == Sqrat::TypeTag<IPoint2>::get())
      return PT_IPOINT2;
    if (tag == Sqrat::TypeTag<IPoint3>::get())
      return PT_IPOINT3;
    if (tag == Sqrat::TypeTag<IPoint4>::get())
      return PT_IPOINT4;
    if (tag == Sqrat::TypeTag<E3DCOLOR>::get())
      return PT_E3DCOLOR;
    if (tag == Sqrat::TypeTag<TMatrix>::get())
      return PT_TMATRIX;
    return PT_INSTANCE;
  }

  int instanceType(const SQObject &o)
  {
    const SQClass *cls = _instance(o)->_class;
    auto it = classTypes.find(cls);
    if (it != classTypes.end())
      return it->second;
    SQUserPointer tag = nullptr;
    int type = !cls->_base && SQ_SUCCEEDED(sq_getobjtypetag(&o, &tag)) ? nativeTypeOfTag(tag) : PT_INSTANCE;
    classTypes[cls] = type;
    return type;
  }

  bool classIndex(SQClass *cls, uint32_t &idx, const char *&error)
  {
    auto it = classIds.find(cls);
    if (it != classIds.end())
    {
      idx = it->second;
      return true;
    }
    bool available = false;
    if (sq_istable(availableClasses))
    {
      forEachEntry(availableClasses,
        [&](const SQObject &, const SQObject &val) { available = available || (sq_isclass(val) && _class(val) == cls); });
    }
    if (!available)
    {
      error = "instance of a class that is not in available_classes";
      return false;
    }
    if (classes.size() >= 0xFFFF)
    {
      error = "too many classes";
      return false;
    }
    idx = uint32_t(classes.size());
    classIds[cls] = idx;
    classes.push_back(SQObjectPtr(cls));
    return true;
  }

  bool callGetState(const SQObject &o, SQObjectPtr &state, const char *&error)
  {
    if (sq_isnull(getStateName))
      getStateName = SQString::Create(_ss(vm), "__getstate");
    SQObjectPtr closure;
    if (!_instance(o)->Get(getStateName, closure) || (!sq_isclosure(closure) && !sq_isnativeclosure(closure)))
    {
      error = "instance must have __getstate method";
      return false;
    }
    SQInteger top = sq_gettop(vm);
    bool takesClasses = pk_count_of_params(closure) > 1;
    sq_pushobject(vm, closure);
    sq_pushobject(vm, o);
    if (takesClasses)
      sq_pushobject(vm, availableClasses);
    if (SQ_FAILED(sq_call(vm, takesClasses ? 2 : 1, SQTrue, SQTrue)))
    {
      sq_settop(vm, top);
      error = "failed to call __getstate method";
      return false;
    }
    state = stack_get(vm, -1);
    sq_settop(vm, top);
    return true;
  }
};

class PkBlkSource
{
public:
  enum Kind : uint8_t
  {
    BLOCK,
    PARAM,
    GROUP,
    NAME,
  };
  struct Val
  {
    const DataBlock *blk;
    uint32_t indexOrStringSlot;
    Kind kind;
    uint8_t paramType;
  };
  struct Str
  {
    const char *s;
    uint32_t len;
    SQHash hash;
  };
  struct StrEqual
  {
    bool operator()(const Str &a, const Str &b) const { return a.len == b.len && (a.s == b.s || memcmp(a.s, b.s, a.len) == 0); }
  };

  static Val block(const DataBlock *blk) { return Val{blk, 0, BLOCK}; }

  static void chars(const Str &x, const char *&p, uint32_t &len)
  {
    p = x.s;
    len = x.len;
  }
  static SQHash hash(const Str &x) { return x.hash; }
  static uint32_t slotHash(const Str &x) { return uint32_t(x.hash); }

  int typeOf(const Val &v) const
  {
    switch (v.kind)
    {
      case BLOCK: return PT_TABLE;
      case GROUP: return PT_ARRAY;
      case NAME: return PT_STRING;
      default: break;
    }
    switch (v.paramType)
    {
      case DataBlock::TYPE_STRING: return PT_STRING;
      case DataBlock::TYPE_INT:
      case DataBlock::TYPE_INT64: return PT_INT;
      case DataBlock::TYPE_REAL: return PT_FLOAT;
      case DataBlock::TYPE_BOOL: return PT_BOOL;
      case DataBlock::TYPE_POINT2: return PT_POINT2;
      case DataBlock::TYPE_POINT3: return PT_POINT3;
      case DataBlock::TYPE_POINT4: return PT_POINT4;
      case DataBlock::TYPE_IPOINT2: return PT_IPOINT2;
      case DataBlock::TYPE_IPOINT3: return PT_IPOINT3;
      case DataBlock::TYPE_IPOINT4: return PT_IPOINT4;
      case DataBlock::TYPE_E3DCOLOR: return PT_E3DCOLOR;
      case DataBlock::TYPE_MATRIX: return PT_TMATRIX;
      default: return -1;
    }
  }
  static bool boolOf(const Val &v) { return v.blk->getBool(v.indexOrStringSlot); }
  static int64_t intOf(const Val &v)
  {
    return v.paramType == DataBlock::TYPE_INT64 ? v.blk->getInt64(v.indexOrStringSlot) : v.blk->getInt(v.indexOrStringSlot);
  }
  static SQFloat floatOf(const Val &v) { return SQFloat(v.blk->getReal(v.indexOrStringSlot)); }
  Str str(const Val &v)
  {
    if (v.kind != NAME)
      return valueStrs[v.indexOrStringSlot];
    if (int(v.indexOrStringSlot) < 0)
      return strOf(nullptr);
    if (v.indexOrStringSlot >= nameStrs.size())
      nameStrs.resize(v.indexOrStringSlot + 1, Str{nullptr, 0, 0});
    Str &cached = nameStrs[v.indexOrStringSlot];
    if (!cached.s)
      cached = strOf(v.blk->getName(int(v.indexOrStringSlot)));
    return cached;
  }
  uint32_t arrayLen(const Val &v) const { return uint32_t(groups[v.indexOrStringSlot].size()); }
  Val arrayAt(const Val &v, uint32_t i) const { return groups[v.indexOrStringSlot][i]; }

  template <typename F>
  void forEachEntry(const Val &v, F f)
  {
    for (const Entry &e : entriesOf(v.blk))
      f(e.key, e.value);
  }

  template <typename F>
  void forEachItem(const Val &v, F f) const
  {
    for (uint32_t i = 0, n = arrayLen(v); i < n; i++)
      f(arrayAt(v, i));
  }

  static void nativeOf(const Val &v, uint32_t type, PkNativeRaw &raw)
  {
    uint32_t i = v.indexOrStringSlot;
    switch (type)
    {
      case PT_POINT2: pk_raw(v.blk->getPoint2(i), raw); break;
      case PT_POINT3: pk_raw(v.blk->getPoint3(i), raw); break;
      case PT_POINT4: pk_raw(v.blk->getPoint4(i), raw); break;
      case PT_IPOINT2: pk_raw(v.blk->getIPoint2(i), raw); break;
      case PT_IPOINT3: pk_raw(v.blk->getIPoint3(i), raw); break;
      case PT_IPOINT4: pk_raw(v.blk->getIPoint4(i), raw); break;
      case PT_E3DCOLOR: pk_raw(v.blk->getE3dcolor(i), raw); break;
      default: pk_raw(v.blk->getTm(i), raw); break;
    }
  }

  static bool prepareInstance(const Val &, bool &fresh, const char *&error)
  {
    fresh = false;
    error = "unsupported value type";
    return false;
  }
  static Val instanceState(const Val &v) { return v; }
  static uint32_t instanceClass(const Val &) { return 0; }

private:
  struct Entry
  {
    Val key;
    Val value;
  };
  eastl::unordered_map<const DataBlock *, eastl::vector<Entry>> entries;
  eastl::vector<eastl::vector<Val>> groups;
  eastl::vector<Str> nameStrs;
  eastl::vector<Str> valueStrs;
  eastl::vector<uint32_t> entryOfNameStamp;
  eastl::vector<uint32_t> entryOfName;
  uint32_t stamp = 0;

  static Str strOf(const char *s)
  {
    if (!s)
      s = "";
    uint32_t len = uint32_t(strlen(s));
    return Str{s, len, _hashstr(s, len)};
  }

  const eastl::vector<Entry> &entriesOf(const DataBlock *blk)
  {
    auto found = entries.find(blk);
    if (found != entries.end())
      return found->second;
    eastl::vector<Entry> &list = entries[blk];
    list.reserve(blk->blockCount() + blk->paramCount());
    stamp++;
    auto add = [&](int name_id, const Val &value) {
      uint32_t slot = uint32_t(name_id + 1);
      if (slot >= entryOfName.size())
      {
        entryOfName.resize(slot + 1);
        entryOfNameStamp.resize(slot + 1, 0);
      }
      if (entryOfNameStamp[slot] != stamp)
      {
        entryOfNameStamp[slot] = stamp;
        entryOfName[slot] = uint32_t(list.size());
        list.push_back(Entry{Val{blk, uint32_t(name_id), NAME}, value});
        return;
      }
      Entry &e = list[entryOfName[slot]];
      if (e.value.kind != GROUP)
      {
        groups.push_back(eastl::vector<Val>(1, e.value));
        e.value = Val{blk, uint32_t(groups.size() - 1), GROUP};
      }
      groups[e.value.indexOrStringSlot].push_back(value);
    };
    for (uint32_t i = 0; i < blk->blockCount(); i++)
      add(blk->getBlock(i)->getBlockNameId(), block(blk->getBlock(i)));
    for (uint32_t i = 0; i < blk->paramCount(); i++)
    {
      uint8_t type = uint8_t(blk->getParamType(i));
      uint32_t slot = i;
      if (type == DataBlock::TYPE_STRING)
      {
        slot = uint32_t(valueStrs.size());
        valueStrs.push_back(strOf(blk->getStr(i)));
      }
      add(blk->getParamNameId(i), Val{blk, slot, PARAM, type});
    }
    return list;
  }
};

template <typename Src>
class PkBuilder
{
public:
  using Val = typename Src::Val;
  using Str = typename Src::Str;

  eastl::vector<uint8_t> out;
  const char *error = nullptr;

  explicit PkBuilder(Src &source) : src(source) {}

  bool build(const Val &root)
  {
    if (!collect(root, 0))
      return false;
    assignStringIds();

    out.resize(sizeof(PkHeader));
    PkHeader h;
    h.stringCount = uint32_t(strings.size());

    h.stringOffsets = uint32_t(out.size());
    uint32_t offset = 0;
    for (const Str &s : strings)
    {
      const char *chars;
      uint32_t len;
      Src::chars(s, chars, len);
      putU32(offset);
      offset += len;
    }
    putU32(offset);
    h.stringBytes = uint32_t(out.size());
    for (const Str &s : strings)
    {
      const char *chars;
      uint32_t len;
      Src::chars(s, chars, len);
      put(chars, len);
    }
    alignOut(4);

    uint32_t keyCount = 0;
    for (uint8_t k : stringIsKey)
      keyCount += k;
    uint32_t capacity = 1;
    while (uint64_t(capacity) * 3 < uint64_t(keyCount) * 4)
      capacity <<= 1;
    eastl::vector<uint32_t> index(capacity, 0);
    for (uint32_t id = 0; id < strings.size(); id++)
    {
      if (!stringIsKey[id])
        continue;
      uint32_t i = uint32_t(Src::hash(strings[id])) & (capacity - 1);
      while (index[i])
        i = (i + 1) & (capacity - 1);
      index[i] = id + 1;
    }
    h.keyIndex = uint32_t(out.size());
    h.keyIndexMask = capacity - 1;
    put(index.data(), index.size() * 4);

    eastl::vector<Val> rootValues(1, root);
    h.root = column(rootValues, nullptr, 0, 0);
    alignOut(8);
    h.size = uint32_t(out.size());
    memcpy(out.data(), &h, sizeof(h));
    out.shrink_to_fit();
    return error == nullptr;
  }

private:
  using StrIds = PkStrMap<Src>;

  Src &src;
  StrIds stringIds;
  eastl::vector<Str> strings;
  eastl::vector<uint8_t> stringIsKey;

  void addString(const Str &s, bool is_key)
  {
    uint32_t &isKey = stringIds[s];
    if (is_key)
      isKey = 1;
  }

  bool collect(const Val &o, uint32_t depth)
  {
    if (depth > PK_MAX_DEPTH)
    {
      error = "nesting is too deep";
      return false;
    }
    int type = src.typeOf(o);
    switch (type)
    {
      case PT_NULL:
      case PT_BOOL:
      case PT_INT:
      case PT_FLOAT: return true;
      case PT_STRING: addString(src.str(o), false); return true;
      case PT_ARRAY:
      {
        bool ok = true;
        src.forEachItem(o, [&](const Val &item) { ok = ok && collect(item, depth + 1); });
        return ok;
      }
      case PT_TABLE:
      {
        bool ok = true;
        src.forEachEntry(o, [&](const Val &key, const Val &val) {
          if (!ok)
            return;
          switch (src.typeOf(key))
          {
            case PT_STRING: addString(src.str(key), true); break;
            case PT_INT:
            case PT_FLOAT:
            case PT_BOOL: break;
            default:
              error = "unsupported table key type";
              ok = false;
              return;
          }
          ok = collect(val, depth + 1);
        });
        return ok;
      }
      case PT_INSTANCE:
      {
        bool fresh = false;
        if (!src.prepareInstance(o, fresh, error))
          return false;
        return !fresh || collect(src.instanceState(o), depth + 1);
      }
      default:
        if (pk_is_native(uint32_t(type)))
          return true;
        error = "unsupported value type";
        return false;
    }
  }

  void assignStringIds()
  {
    eastl::vector<eastl::pair<uint64_t, Str>> byPrefix;
    byPrefix.reserve(stringIds.size());
    stringIds.forEach([&](const Str &s, uint32_t &) {
      const char *p;
      uint32_t len;
      Src::chars(s, p, len);
      uint64_t prefix = 0;
      for (uint32_t i = 0; i < 8; i++)
        prefix = (prefix << 8) | (i < len ? uint8_t(p[i]) : 0);
      byPrefix.push_back({prefix, s});
    });
    eastl::sort(byPrefix.begin(), byPrefix.end(), [](const auto &a, const auto &b) {
      if (a.first != b.first)
        return a.first < b.first;
      const char *pa, *pb;
      uint32_t la, lb;
      Src::chars(a.second, pa, la);
      Src::chars(b.second, pb, lb);
      int r = memcmp(pa, pb, eastl::min(la, lb));
      return r ? r < 0 : la < lb;
    });
    strings.reserve(byPrefix.size());
    for (const auto &it : byPrefix)
      strings.push_back(it.second);
    stringIsKey.resize(strings.size());
    for (uint32_t id = 0; id < strings.size(); id++)
    {
      uint32_t &v = stringIds[strings[id]];
      stringIsKey[id] = uint8_t(v);
      v = id;
    }
  }

  void put(const void *p, size_t n)
  {
    const uint8_t *b = (const uint8_t *)p;
    out.insert(out.end(), b, b + n);
  }
  void putU32(uint32_t x) { put(&x, 4); }
  void putUnsigned(uint64_t x, uint32_t width) { put(&x, width); }
  void alignOut(uint32_t a)
  {
    while (out.size() % a)
      out.push_back(0);
  }

  uint32_t beginColumn(PkKind kind, uint8_t width, uint32_t extra, uint32_t rows)
  {
    alignOut(4);
    uint32_t at = uint32_t(out.size());
    PkColumn c = {uint8_t(kind), width, uint16_t(extra), rows};
    put(&c, sizeof(c));
    return at;
  }

  bool same(const Val &a, const Val &b)
  {
    int type = src.typeOf(a);
    if (type != src.typeOf(b))
      return false;
    switch (type)
    {
      case PT_NULL: return true;
      case PT_BOOL: return src.boolOf(a) == src.boolOf(b);
      case PT_INT: return src.intOf(a) == src.intOf(b);
      case PT_FLOAT:
      {
        SQFloat x = src.floatOf(a), y = src.floatOf(b);
        return memcmp(&x, &y, sizeof(SQFloat)) == 0;
      }
      case PT_STRING: return typename Src::StrEqual()(src.str(a), src.str(b));
      default: return false;
    }
  }

  PkValue keyOrderValue(const Val &o)
  {
    PkValue v;
    v.type = PkType(src.typeOf(o));
    v.i = 0;
    v.f = 0;
    switch (v.type)
    {
      case PT_STRING: v.i = stringIds[src.str(o)]; break;
      case PT_FLOAT: v.f = src.floatOf(o); break;
      case PT_BOOL: v.i = src.boolOf(o); break;
      default: v.i = SQInteger(src.intOf(o)); break;
    }
    return v;
  }

  uint32_t column(const eastl::vector<Val> &vals, const eastl::vector<Val> *row_keys, uint32_t row_keys_ref, uint32_t depth)
  {
    if (depth > PK_MAX_DEPTH)
    {
      error = "nesting is too deep";
      return 0;
    }
    uint32_t n = uint32_t(vals.size());
    if (row_keys && n > 0)
    {
      bool sameAsKeys = true;
      for (uint32_t i = 0; i < n && sameAsKeys; i++)
        sameAsKeys = same(vals[i], (*row_keys)[i]);
      if (sameAsKeys)
      {
        uint32_t at = beginColumn(PK_ROW_KEY, 0, 0, n);
        putU32(row_keys_ref);
        return at;
      }
    }

    uint32_t typeCounts[PT_COUNT] = {};
    for (const Val &o : vals)
      typeCounts[src.typeOf(o)]++;
    uint32_t typesUsed = 0;
    for (uint32_t c : typeCounts)
      typesUsed += c ? 1 : 0;

    if (typesUsed > 1)
    {
      if (typesUsed == 2 && typeCounts[PT_NULL])
        return nullableColumn(vals, depth);
      return unionColumn(vals, typeCounts, depth);
    }

    PkType type = n ? PkType(src.typeOf(vals[0])) : PT_NULL;
    switch (type)
    {
      case PT_NULL: return constColumn(n, PT_NULL, 0, 0);
      case PT_BOOL: return boolColumn(n, [&](uint32_t i) { return src.boolOf(vals[i]); });
      case PT_INT: return intColumn(n, [&](uint32_t i) { return src.intOf(vals[i]); });
      case PT_FLOAT: return floatColumn(n, [&](uint32_t i) { return src.floatOf(vals[i]); });
      case PT_STRING: return stringColumn(n, [&](uint32_t i) { return stringIds[src.str(vals[i])]; });
      case PT_ARRAY: return listColumn(vals, depth);
      case PT_TABLE: return tableColumn(vals, row_keys, row_keys_ref, depth);
      case PT_INSTANCE: return instanceColumn(vals, depth);
      default:
      {
        eastl::vector<PkNativeRaw> raws(n);
        for (uint32_t i = 0; i < n; i++)
          src.nativeOf(vals[i], type, raws[i]);
        return nativeColumn(type, raws, 0);
      }
    }
  }

  uint32_t constColumn(uint32_t rows, PkType type, int64_t x, SQFloat f)
  {
    uint32_t at = beginColumn(PK_CONST, 0, type, rows);
    uint8_t raw[8] = {};
    if (type == PT_FLOAT)
      memcpy(raw, &f, sizeof(SQFloat));
    else
      memcpy(raw, &x, 8);
    put(raw, 8);
    return at;
  }

  template <typename Get>
  uint32_t boolColumn(uint32_t n, Get get)
  {
    bool first = get(0), allSame = true;
    for (uint32_t i = 1; i < n && allSame; i++)
      allSame = get(i) == first;
    if (allSame)
      return constColumn(n, PT_BOOL, first, 0);
    uint32_t at = beginColumn(PK_BOOL_BITS, 0, 0, n);
    eastl::vector<uint8_t> bits((n + 7) / 8, 0);
    for (uint32_t i = 0; i < n; i++)
      if (get(i))
        bits[i >> 3] |= uint8_t(1 << (i & 7));
    put(bits.data(), bits.size());
    return at;
  }

  template <typename Get>
  uint32_t intColumn(uint32_t n, Get get)
  {
    int64_t lo = get(0), hi = lo;
    for (uint32_t i = 1; i < n; i++)
    {
      int64_t x = get(i);
      lo = eastl::min(lo, x);
      hi = eastl::max(hi, x);
    }
    if (lo == hi)
      return constColumn(n, PT_INT, lo, 0);
    uint8_t width = pk_width(uint64_t(hi) - uint64_t(lo));
    uint32_t at = beginColumn(PK_INT, width, 0, n);
    put(&lo, 8);
    for (uint32_t i = 0; i < n; i++)
      putUnsigned(uint64_t(int64_t(get(i))) - uint64_t(lo), width);
    return at;
  }

  template <typename Get>
  uint32_t floatColumn(uint32_t n, Get get)
  {
    SQFloat first = get(0);
    bool allSame = true;
    for (uint32_t i = 1; i < n && allSame; i++)
    {
      SQFloat x = get(i);
      allSame = memcmp(&x, &first, sizeof(SQFloat)) == 0;
    }
    if (allSame)
      return constColumn(n, PT_FLOAT, 0, first);
    uint32_t at = beginColumn(PK_FLOAT, sizeof(SQFloat), 0, n);
    for (uint32_t i = 0; i < n; i++)
    {
      SQFloat x = get(i);
      put(&x, sizeof(SQFloat));
    }
    return at;
  }

  template <typename Get>
  uint32_t stringColumn(uint32_t n, Get get)
  {
    eastl::vector<uint32_t> ids(n);
    bool allSame = true;
    for (uint32_t i = 0; i < n; i++)
    {
      ids[i] = get(i);
      allSame = allSame && ids[i] == ids[0];
    }
    if (allSame)
      return constColumn(n, PT_STRING, ids[0], 0);
    eastl::vector<uint32_t> dict(ids);
    eastl::sort(dict.begin(), dict.end());
    dict.erase(eastl::unique(dict.begin(), dict.end()), dict.end());
    uint8_t idWidth = pk_width(dict.back());
    uint8_t indexWidth = pk_width(dict.size() - 1);
    uint64_t plainSize = uint64_t(n) * idWidth;
    uint64_t dictSize = pk_align4(4 + uint32_t(dict.size()) * idWidth) + uint64_t(n) * indexWidth;
    if (dictSize >= plainSize)
    {
      uint32_t at = beginColumn(PK_STRING, idWidth, 0, n);
      for (uint32_t id : ids)
        putUnsigned(id, idWidth);
      return at;
    }
    uint32_t at = beginColumn(PK_STRING_DICT, indexWidth, idWidth, n);
    putU32(uint32_t(dict.size()));
    for (uint32_t id : dict)
      putUnsigned(id, idWidth);
    alignOut(4);
    for (uint32_t id : ids)
      putUnsigned(uint64_t(eastl::lower_bound(dict.begin(), dict.end(), id) - dict.begin()), indexWidth);
    return at;
  }

  uint32_t nativeColumn(PkType type, const eastl::vector<PkNativeRaw> &raws, uint32_t first_float)
  {
    const PkNativeInfo &info = pk_native(type);
    uint32_t n = uint32_t(raws.size());
    uint32_t refs[4] = {};
    for (uint32_t k = 0; k < info.components; k++)
    {
      if (info.componentType == PT_POINT3)
        refs[k] = nativeColumn(PT_POINT3, raws, k * 3);
      else if (info.componentType == PT_FLOAT)
        refs[k] = floatColumn(n, [&](uint32_t i) { return SQFloat(raws[i].f[first_float + k]); });
      else
        refs[k] = intColumn(n, [&](uint32_t i) { return int64_t(raws[i].i[k]); });
    }
    uint32_t at = beginColumn(PK_NATIVE, 0, type, n);
    for (uint32_t k = 0; k < info.components; k++)
      putU32(refs[k]);
    return at;
  }

  uint32_t presenceColumn(PkKind kind, const eastl::vector<uint8_t> &present, uint32_t child)
  {
    uint32_t n = uint32_t(present.size());
    uint32_t blocks = (n + 63) / 64;
    uint32_t at = beginColumn(kind, 0, 0, n);
    putU32(child);
    eastl::vector<uint64_t> words(blocks, 0);
    uint32_t rank = 0;
    for (uint32_t b = 0; b < blocks; b++)
    {
      putU32(rank);
      for (uint32_t i = b * 64; i < eastl::min(n, b * 64 + 64); i++)
        if (present[i])
        {
          words[b] |= uint64_t(1) << (i & 63);
          rank++;
        }
    }
    put(words.data(), words.size() * 8);
    return at;
  }

  uint32_t nullableColumn(const eastl::vector<Val> &vals, uint32_t depth)
  {
    eastl::vector<uint8_t> present(vals.size());
    eastl::vector<Val> dense;
    for (size_t i = 0; i < vals.size(); i++)
      if (src.typeOf(vals[i]) != PT_NULL)
      {
        present[i] = 1;
        dense.push_back(vals[i]);
      }
    uint32_t child = column(dense, nullptr, 0, depth);
    return presenceColumn(PK_NULLABLE, present, child);
  }

  uint32_t unionColumn(const eastl::vector<Val> &vals, const uint32_t (&type_counts)[PT_COUNT], uint32_t depth)
  {
    uint32_t n = uint32_t(vals.size());
    uint32_t children[PT_COUNT] = {};
    eastl::vector<uint32_t> positions(n);
    eastl::vector<uint8_t> types(n);
    for (uint32_t i = 0; i < n; i++)
      types[i] = uint8_t(src.typeOf(vals[i]));
    uint32_t maxPos = 0;
    for (uint32_t t = 0; t < PT_COUNT; t++)
    {
      if (!type_counts[t])
        continue;
      eastl::vector<Val> subset;
      for (uint32_t i = 0; i < n; i++)
        if (types[i] == t)
        {
          positions[i] = uint32_t(subset.size());
          subset.push_back(vals[i]);
        }
      maxPos = eastl::max(maxPos, uint32_t(subset.size()) - 1);
      children[t] = column(subset, nullptr, 0, depth);
    }
    uint8_t width = pk_width(maxPos);
    uint32_t at = beginColumn(PK_UNION, width, 0, n);
    put(children, sizeof(children));
    eastl::vector<uint8_t> tags((n + 1) / 2, 0);
    for (uint32_t i = 0; i < n; i++)
      tags[i >> 1] |= uint8_t(types[i] << ((i & 1) * 4));
    put(tags.data(), tags.size());
    alignOut(4);
    for (uint32_t p : positions)
      putUnsigned(p, width);
    return at;
  }

  uint32_t listColumn(const eastl::vector<Val> &vals, uint32_t depth)
  {
    uint32_t n = uint32_t(vals.size());
    eastl::vector<Val> flat;
    eastl::vector<uint32_t> offsets(n + 1);
    for (uint32_t i = 0; i < n; i++)
    {
      offsets[i] = uint32_t(flat.size());
      uint32_t len = src.arrayLen(vals[i]);
      for (uint32_t j = 0; j < len; j++)
        flat.push_back(src.arrayAt(vals[i], j));
    }
    offsets[n] = uint32_t(flat.size());
    uint32_t child = column(flat, nullptr, 0, depth + 1);
    uint8_t width = pk_width(flat.size());
    uint32_t at = beginColumn(PK_LIST, width, 0, n);
    putU32(child);
    for (uint32_t o : offsets)
      putUnsigned(o, width);
    return at;
  }

  uint32_t instanceColumn(const eastl::vector<Val> &vals, uint32_t depth)
  {
    uint32_t n = uint32_t(vals.size());
    eastl::vector<Val> stateVals(n);
    eastl::vector<uint32_t> ids(n);
    bool mixed = false;
    for (uint32_t i = 0; i < n; i++)
    {
      stateVals[i] = src.instanceState(vals[i]);
      ids[i] = src.instanceClass(vals[i]);
      mixed = mixed || ids[i] != ids[0];
    }
    uint32_t child = column(stateVals, nullptr, 0, depth + 1);
    uint32_t at = beginColumn(PK_INSTANCE, mixed ? 2 : 0, mixed ? 0 : ids[0], n);
    putU32(child);
    if (mixed)
      for (uint32_t id : ids)
        putUnsigned(id, 2);
    return at;
  }

  uint32_t tableColumn(const eastl::vector<Val> &vals, const eastl::vector<Val> *row_keys, uint32_t row_keys_ref, uint32_t depth)
  {
    StrIds keyCounts;
    bool allStringKeys = true;
    uint64_t totalKeys = 0;
    for (const Val &t : vals)
      src.forEachEntry(t, [&](const Val &key, const Val &) {
        totalKeys++;
        if (src.typeOf(key) == PT_STRING)
          keyCounts[src.str(key)]++;
        else
          allStringKeys = false;
      });
    uint32_t maxCount = 0;
    keyCounts.forEach([&](const Str &, uint32_t &count) { maxCount = eastl::max(maxCount, count); });
    uint32_t n = uint32_t(vals.size());
    if (!allStringKeys || keyCounts.empty() || (n > 1 && maxCount < 2))
      return mapColumn(vals, nullptr, depth);
    if (keyCounts.size() <= PK_MAX_DENSE_STRUCT_FIELDS && totalKeys * 2 >= uint64_t(n) * keyCounts.size())
      return structColumn(vals, keyCounts, true, row_keys, row_keys_ref, depth);
    uint32_t columnFields = 0;
    keyCounts.forEach([&](const Str &, uint32_t &count) { columnFields += pk_field_needs_column(count, n) ? 1 : 0; });
    if (columnFields == 0 || columnFields > PK_MAX_STRUCT_FIELDS)
      return mapColumn(vals, nullptr, depth);
    return structColumn(vals, keyCounts, false, row_keys, row_keys_ref, depth);
  }

  uint32_t structColumn(const eastl::vector<Val> &vals, StrIds &fields, bool all_fields_in_columns, const eastl::vector<Val> *row_keys,
    uint32_t row_keys_ref, uint32_t depth)
  {
    uint32_t n = uint32_t(vals.size());
    eastl::vector<eastl::pair<uint32_t, Str>> order;
    fields.forEach([&](const Str &key, uint32_t &count) {
      if (all_fields_in_columns || pk_field_needs_column(count, n))
        order.push_back({stringIds[key], key});
      else
        count = PK_NO_POS;
    });
    bool hasRest = order.size() < fields.size();
    eastl::sort(order.begin(), order.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    uint32_t fieldCount = uint32_t(order.size());
    for (uint32_t f = 0; f < fieldCount; f++)
      fields[order[f].second] = f;

    eastl::vector<eastl::vector<Val>> fieldValues(fieldCount);
    eastl::vector<eastl::vector<uint8_t>> present(fieldCount, eastl::vector<uint8_t>(n, 0));
    for (uint32_t row = 0; row < n; row++)
      src.forEachEntry(vals[row], [&](const Val &key, const Val &val) {
        uint32_t f = fields[src.str(key)];
        if (f == PK_NO_POS)
          return;
        fieldValues[f].push_back(val);
        present[f][row] = 1;
      });

    eastl::vector<uint32_t> refs(fieldCount);
    for (uint32_t f = 0; f < fieldCount; f++)
    {
      if (fieldValues[f].size() == n)
        refs[f] = column(fieldValues[f], row_keys, row_keys_ref, depth + 1);
      else
      {
        uint32_t child = column(fieldValues[f], nullptr, 0, depth + 1);
        refs[f] = presenceColumn(PK_OPTIONAL, present[f], child);
      }
    }
    uint32_t restRef = hasRest ? mapColumn(vals, &fields, depth) : 0;
    uint32_t at = beginColumn(PK_STRUCT, hasRest ? 1 : 0, fieldCount, n);
    for (auto &it : order)
      putU32(it.first);
    for (uint32_t r : refs)
      putU32(r);
    if (hasRest)
      putU32(restRef);
    return at;
  }

  uint32_t mapColumn(const eastl::vector<Val> &vals, StrIds *column_fields, uint32_t depth)
  {
    uint32_t n = uint32_t(vals.size());
    eastl::vector<Val> keys, values;
    eastl::vector<uint32_t> offsets(n + 1);
    eastl::vector<eastl::pair<PkValue, eastl::pair<Val, Val>>> entries;
    for (uint32_t row = 0; row < n; row++)
    {
      offsets[row] = uint32_t(keys.size());
      entries.clear();
      src.forEachEntry(vals[row], [&](const Val &key, const Val &val) {
        if (!column_fields || (*column_fields)[src.str(key)] == PK_NO_POS)
          entries.push_back({keyOrderValue(key), {key, val}});
      });
      eastl::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) { return pk_cmp_key(a.first, b.first) < 0; });
      for (auto &e : entries)
      {
        keys.push_back(e.second.first);
        values.push_back(e.second.second);
      }
    }
    offsets[n] = uint32_t(keys.size());
    uint32_t keysRef = column(keys, nullptr, 0, depth + 1);
    uint32_t valuesRef = column(values, &keys, keysRef, depth + 1);
    uint8_t width = pk_width(keys.size());
    uint32_t at = beginColumn(PK_MAP, width, 0, n);
    putU32(keysRef);
    putU32(valuesRef);
    for (uint32_t o : offsets)
      putUnsigned(o, width);
    return at;
  }
};


struct PkView
{
  PkTree *tree;
  uint32_t ref;
  uint32_t row;
  uint32_t iterPos;
  uint32_t iterSlot;
};

const uint32_t PK_SLOT_IN_REST = 0x80000000u;

SQInteger pk_view_release(HSQUIRRELVM, SQUserPointer p, SQInteger)
{
  PkView *view = (PkView *)p;
  if (view->tree && --view->tree->refCount == 0)
    delete view->tree;
  return 1;
}

PkView *pk_get_view(HSQUIRRELVM v, SQInteger idx)
{
  const SQObjectPtr &o = stack_get(v, idx);
  if (sq_type(o) != OT_INSTANCE || _instance(o)->_hook != pk_view_release)
    return nullptr;
  return (PkView *)_instance(o)->_userpointer;
}

bool pk_push_view(HSQUIRRELVM v, PkTree *tree, uint32_t ref, uint32_t row, SQClass *cls)
{
  SQInstance *inst = cls->CreateInstance(v);
  if (!inst)
    return false;
  PkView *view = (PkView *)inst->_userpointer;
  view->tree = tree;
  view->ref = ref;
  view->row = row;
  view->iterPos = PK_NO_POS;
  tree->refCount++;
  inst->_hook = pk_view_release;
  v->Push(SQObjectPtr(inst));
  return true;
}

void pk_push_scalar(HSQUIRRELVM v, const PkTree *tree, const PkValue &val)
{
  switch (val.type)
  {
    case PT_BOOL: sq_pushbool(v, val.i != 0); break;
    case PT_INT: sq_pushinteger(v, val.i); break;
    case PT_FLOAT: sq_pushfloat(v, val.f); break;
    case PT_STRING: v->Push(tree->stringObject(v, uint32_t(val.i))); break;
    default: sq_pushnull(v); break;
  }
}

bool pk_unpack_value(HSQUIRRELVM v, const PkTree *tree, const PkValue &val);

bool pk_push_instance(HSQUIRRELVM v, const PkTree *tree, const PkValue &val)
{
  const PkColumn *c = tree->column(val.ref);
  const uint8_t *p = PkTree::payload(c);
  const PkClass &cls = tree->classes[c->width ? uint32_t(pk_read(p + 4, c->width, val.row)) : c->extra];
  SQInteger top = sq_gettop(v);
  sq_pushobject(v, cls.cls);
  sq_pushnull(v);
  if (SQ_FAILED(sq_call(v, 1, SQTrue, SQTrue)))
  {
    sq_settop(v, top);
    return false;
  }
  sq_remove(v, -2);
  sq_pushobject(v, cls.setState);
  sq_push(v, -2);
  PkValue state;
  tree->element(pk_u32(p), val.row, state);
  if (!pk_unpack_value(v, tree, state))
  {
    sq_settop(v, top);
    return false;
  }
  if (cls.setStateTakesClasses)
    sq_pushobject(v, tree->availableClasses);
  if (SQ_FAILED(sq_call(v, cls.setStateTakesClasses ? 3 : 2, SQFalse, SQTrue)))
  {
    sq_settop(v, top);
    return false;
  }
  sq_settop(v, top + 1);
  return true;
}

template <typename T>
bool pk_push_copy(HSQUIRRELVM v, const T &value, const char *class_name)
{
  if (!Sqrat::ClassType<T>::hasClassData(v))
  {
    eastl::string msg(eastl::string::CtorSprintf(), "class %s of a packed value is not bound in this VM", class_name);
    (void)sq_throwerror(v, msg.c_str());
    return false;
  }
  return Sqrat::ClassType<T>::PushInstanceCopy(v, value);
}

bool pk_push_native(HSQUIRRELVM v, const PkTree *tree, const PkValue &val)
{
  PkNativeRaw raw;
  tree->nativeRaw(val.ref, val.row, raw, 0);
  const char *name = pk_native(val.type).name;
  switch (val.type)
  {
    case PT_POINT2: return pk_push_copy(v, Point2(raw.f[0], raw.f[1]), name);
    case PT_POINT3: return pk_push_copy(v, Point3(raw.f[0], raw.f[1], raw.f[2]), name);
    case PT_POINT4: return pk_push_copy(v, Point4(raw.f[0], raw.f[1], raw.f[2], raw.f[3]), name);
    case PT_IPOINT2: return pk_push_copy(v, IPoint2(raw.i[0], raw.i[1]), name);
    case PT_IPOINT3: return pk_push_copy(v, IPoint3(raw.i[0], raw.i[1], raw.i[2]), name);
    case PT_IPOINT4: return pk_push_copy(v, IPoint4(raw.i[0], raw.i[1], raw.i[2], raw.i[3]), name);
    case PT_E3DCOLOR:
      return pk_push_copy(v, E3DCOLOR(uint8_t(raw.i[0]), uint8_t(raw.i[1]), uint8_t(raw.i[2]), uint8_t(raw.i[3])), name);
    default:
    {
      TMatrix tm;
      for (uint32_t col = 0; col < 4; col++)
        for (uint32_t k = 0; k < 3; k++)
          tm.m[col][k] = raw.f[col * 3 + k];
      return pk_push_copy(v, tm, name);
    }
  }
}

bool pk_push_value(HSQUIRRELVM v, PkTree *tree, const PkValue &val, SQClass *cls)
{
  if (val.type == PT_TABLE || val.type == PT_ARRAY)
    return pk_push_view(v, tree, val.ref, val.row, cls);
  if (val.type == PT_INSTANCE)
    return pk_push_instance(v, tree, val);
  if (pk_is_native(val.type))
    return pk_push_native(v, tree, val);
  pk_push_scalar(v, tree, val);
  return true;
}

bool pk_view_find(PkView *view, const PkColumn *c, const SQObject &key, uint32_t &pos, uint32_t &slot, bool &in_rest)
{
  const PkTree *tree = view->tree;
  PkValue k;
  if (view->iterPos != PK_NO_POS && c->kind != PK_LIST &&
      tree->slotKey(c, view->row, view->iterSlot & ~PK_SLOT_IN_REST, (view->iterSlot & PK_SLOT_IN_REST) != 0, k) &&
      tree->keyEquals(k, key))
  {
    pos = view->iterPos;
    slot = view->iterSlot & ~PK_SLOT_IN_REST;
    in_rest = (view->iterSlot & PK_SLOT_IN_REST) != 0;
    return true;
  }
  if (!tree->find(c, view->row, key, pos))
    return false;
  slot = pos;
  in_rest = c->kind == PK_STRUCT && tree->restSlot(c, view->row, pos, slot);
  return true;
}

SQInteger pk_view_get(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 1);
  if (!view)
    return sq_throwerror(v, "invalid packed view");
  const PkColumn *c = view->tree->column(view->ref);
  uint32_t pos, slot;
  bool inRest;
  PkValue val;
  if (!pk_view_find(view, c, stack_get(v, 2), pos, slot, inRest) || !view->tree->slotValue(c, view->row, slot, inRest, val))
  {
    const SQObjectPtr &key = stack_get(v, 2);
    if (sq_type(key) == OT_STRING && sq_gettop(v) >= 4)
    {
      const SQObjectPtr &methods = stack_get(v, c->kind == PK_LIST ? 3 : 4);
      SQObjectPtr method;
      if (sq_type(methods) == OT_TABLE && _table(methods)->Get(key, method))
      {
        v->Push(method);
        return 1;
      }
    }
    sq_pushnull(v);
    return sq_throwobject(v);
  }
  if (!pk_push_value(v, view->tree, val, _instance(stack_get(v, 1))->_class))
    return SQ_ERROR;
  return 1;
}

SQInteger pk_view_nexti(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 1);
  if (!view)
    return sq_throwerror(v, "invalid packed view");
  const PkTree *tree = view->tree;
  const PkColumn *c = tree->column(view->ref);
  uint32_t begin, end, pos, slot = 0;
  bool inRest = false;
  tree->range(c, view->row, begin, end);
  const SQObjectPtr &prev = stack_get(v, 2);
  bool first = sq_type(prev) == OT_NULL;
  if (first)
    pos = begin;
  else
  {
    if (!pk_view_find(view, c, prev, pos, slot, inRest))
      return sq_throwerror(v, "invalid iterator key");
    pos++;
  }
  pos = first ? tree->nextPresent(c, view->row, pos, end, &slot, &inRest)
              : tree->nextPresentAfter(c, view->row, pos - 1, end, slot, inRest);
  if (pos >= end)
  {
    view->iterPos = PK_NO_POS;
    sq_pushnull(v);
    return 1;
  }
  view->iterPos = pos;
  view->iterSlot = inRest ? slot | PK_SLOT_IN_REST : slot;
  PkValue key;
  tree->slotKey(c, view->row, slot, inRest, key);
  pk_push_scalar(v, tree, key);
  return 1;
}

SQInteger pk_view_cloned(HSQUIRRELVM v)
{
  PkView *src = pk_get_view(v, 2);
  const SQObjectPtr &self = stack_get(v, 1);
  if (!src || sq_type(self) != OT_INSTANCE || _instance(self)->_class != _instance(stack_get(v, 2))->_class)
    return sq_throwerror(v, "invalid packed view");
  PkView *dst = (PkView *)_instance(self)->_userpointer;
  PkTree *held = _instance(self)->_hook == pk_view_release ? dst->tree : nullptr;
  *dst = *src;
  dst->iterPos = PK_NO_POS;
  dst->tree->refCount++;
  if (held && --held->refCount == 0)
    delete held;
  _instance(self)->_hook = pk_view_release;
  return 0;
}

SQInteger pk_view_constructor(HSQUIRRELVM v) { return sq_throwerror(v, "packed views are made by packed.pack()"); }

bool pk_push_datablock_view(HSQUIRRELVM v, const DataBlock &blk, SQClass *view_class)
{
  PkBlkSource src;
  PkBuilder<PkBlkSource> builder(src);
  if (!builder.build(PkBlkSource::block(&blk)))
  {
    (void)sq_throwerror(v, builder.error);
    return false;
  }
  PkTree *tree = new PkTree;
  tree->buf.swap(builder.out);
  if (!pk_push_view(v, tree, tree->header().root, 0, view_class))
  {
    delete tree;
    return false;
  }
  return true;
}

SQInteger pk_pack(HSQUIRRELVM v)
{
  SQInteger viewClassIdx = sq_gettop(v);
  SQObjectPtr root = stack_get(v, 2);
  if (sq_type(root) == OT_INSTANCE)
  {
    const DataBlock *blk = Sqrat::ClassType<DataBlock>::GetInstanceFromObj(root);
    if (!blk)
      return sq_throwerror(v, "pack() expects a table, an array or a DataBlock");
    return pk_push_datablock_view(v, *blk, _class(stack_get(v, viewClassIdx))) ? 1 : SQ_ERROR;
  }
  if (sq_type(root) != OT_TABLE && sq_type(root) != OT_ARRAY)
    return sq_throwerror(v, "pack() expects a table, an array or a DataBlock");
  PkSqSource src;
  src.vm = v;
  if (viewClassIdx > 3)
    src.availableClasses = stack_get(v, 3);
  PkBuilder<PkSqSource> builder(src);
  if (!builder.build(root))
    return sq_throwerror(v, builder.error);
  PkTree *tree = new PkTree;
  tree->buf.swap(builder.out);
  tree->availableClasses = src.availableClasses;
  SQObjectPtr setStateName(SQString::Create(_ss(v), "__setstate"));
  for (const SQObjectPtr &cls : src.classes)
  {
    SQObjectPtr setState;
    if (!_class(cls)->Get(setStateName, setState) || (!sq_isclosure(setState) && !sq_isnativeclosure(setState)))
    {
      delete tree;
      return sq_throwerror(v, "class must have __setstate method");
    }
    tree->classes.push_back({cls, setState, pk_count_of_params(setState) > 2});
  }
  if (!pk_push_view(v, tree, tree->header().root, 0, _class(stack_get(v, viewClassIdx))))
  {
    delete tree;
    return SQ_ERROR;
  }
  return 1;
}

bool pk_unpack_value(HSQUIRRELVM v, const PkTree *tree, const PkValue &val)
{
  if (val.type == PT_INSTANCE)
    return pk_push_instance(v, tree, val);
  if (pk_is_native(val.type))
    return pk_push_native(v, tree, val);
  if (val.type != PT_TABLE && val.type != PT_ARRAY)
  {
    pk_push_scalar(v, tree, val);
    return true;
  }
  const PkColumn *c = tree->column(val.ref);
  uint32_t begin, end;
  tree->range(c, val.row, begin, end);
  if (c->kind == PK_LIST)
    sq_newarray(v, 0);
  else
    sq_newtableex(v, SQInteger(end - begin));
  uint32_t slot;
  bool inRest;
  for (uint32_t pos = tree->nextPresent(c, val.row, begin, end, &slot, &inRest); pos < end;
       pos = tree->nextPresentAfter(c, val.row, pos, end, slot, inRest))
  {
    PkValue item;
    if (!tree->slotValue(c, val.row, slot, inRest, item))
      continue;
    if (c->kind != PK_LIST)
    {
      PkValue key;
      tree->slotKey(c, val.row, slot, inRest, key);
      pk_push_scalar(v, tree, key);
    }
    if (!pk_unpack_value(v, tree, item))
      return false;
    if (SQ_FAILED(c->kind == PK_LIST ? sq_arrayappend(v, -2) : sq_rawset(v, -3)))
      return false;
  }
  return true;
}

SQInteger pk_unpack(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 2);
  if (!view)
    return sq_throwerror(v, "unpack() expects a packed view");
  PkValue val;
  val.type = view->tree->column(view->ref)->kind == PK_LIST ? PT_ARRAY : PT_TABLE;
  val.ref = view->ref;
  val.row = view->row;
  return pk_unpack_value(v, view->tree, val) ? 1 : SQ_ERROR;
}

SQInteger pk_get(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 2);
  if (!view)
    return sq_throwerror(v, "get() expects a packed view");
  const PkTree *tree = view->tree;
  PkValue val;
  val.type = PT_TABLE;
  val.ref = view->ref;
  val.row = view->row;
  SQInteger top = sq_gettop(v);
  for (SQInteger i = 3; i <= top; i++)
  {
    uint32_t component;
    if (pk_is_native(val.type) && pk_native_key(val.type, stack_get(v, i), component))
    {
      tree->nativeComponent(val.ref, val.row, component, val);
      continue;
    }
    if (val.type != PT_TABLE && val.type != PT_ARRAY)
    {
      sq_pushnull(v);
      return 1;
    }
    const PkColumn *c = tree->column(val.ref);
    uint32_t pos;
    if (!tree->find(c, val.row, stack_get(v, i), pos) || !tree->valueAt(c, val.row, pos, val))
    {
      sq_pushnull(v);
      return 1;
    }
  }
  return pk_push_value(v, view->tree, val, _instance(stack_get(v, 2))->_class) ? 1 : SQ_ERROR;
}

SQInteger pk_count(const PkView *view)
{
  const PkTree *tree = view->tree;
  const PkColumn *c = tree->column(view->ref);
  uint32_t begin, end;
  tree->range(c, view->row, begin, end);
  SQInteger count = SQInteger(end - begin);
  if (c->kind == PK_STRUCT)
  {
    count = 0;
    uint32_t slot;
    bool inRest;
    for (uint32_t pos = tree->nextPresent(c, view->row, begin, end, &slot, &inRest); pos < end;
         pos = tree->nextPresentAfter(c, view->row, pos, end, slot, inRest))
      count++;
  }
  return count;
}

SQInteger pk_len(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 2);
  if (!view)
    return sq_throwerror(v, "len() expects a packed view");
  sq_pushinteger(v, pk_count(view));
  return 1;
}

bool pk_push_shallow(HSQUIRRELVM v, PkView *view, SQClass *cls, bool lists_as_arrays = false)
{
  PkTree *tree = view->tree;
  const PkColumn *c = tree->column(view->ref);
  uint32_t begin, end;
  tree->range(c, view->row, begin, end);
  const bool isList = c->kind == PK_LIST;
  if (isList)
    sq_newarray(v, 0);
  else
    sq_newtableex(v, SQInteger(end - begin));
  uint32_t slot;
  bool inRest;
  for (uint32_t pos = tree->nextPresent(c, view->row, begin, end, &slot, &inRest); pos < end;
       pos = tree->nextPresentAfter(c, view->row, pos, end, slot, inRest))
  {
    PkValue item;
    if (!tree->slotValue(c, view->row, slot, inRest, item))
      continue;
    if (!isList)
    {
      PkValue key;
      tree->slotKey(c, view->row, slot, inRest, key);
      pk_push_scalar(v, tree, key);
    }
    PkView inner = {tree, item.ref, item.row, PK_NO_POS, 0};
    if (lists_as_arrays && item.type == PT_ARRAY ? !pk_push_shallow(v, &inner, cls) : !pk_push_value(v, tree, item, cls))
      return false;
    if (SQ_FAILED(isList ? sq_arrayappend(v, -2) : sq_rawset(v, -3)))
      return false;
  }
  return true;
}

SQInteger pk_view_len(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 1);
  if (!view)
    return sq_throwerror(v, "invalid packed view");
  sq_pushinteger(v, pk_count(view));
  return 1;
}

const SQInteger PK_NATIVE_METHOD_FREE_VARS = 1;

SQInteger pk_view_merge(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 1);
  if (!view)
    return sq_throwerror(v, "invalid packed view");
  SQInteger top = sq_gettop(v) - PK_NATIVE_METHOD_FREE_VARS;
  for (SQInteger arg = 2; arg <= top; arg++)
  {
    SQObjectType argType = sq_gettype(v, arg);
    if (!(argType & (_RT_TABLE | _RT_CLASS | _RT_INSTANCE)))
    {
      v->Raise_ParamTypeError(arg, _RT_TABLE | _RT_CLASS | _RT_INSTANCE, argType);
      return SQ_ERROR;
    }
  }
  if (!pk_push_shallow(v, view, _instance(stack_get(v, 1))->_class))
    return SQ_ERROR;
  SQInteger res = sq_gettop(v);
  for (SQInteger arg = 2; arg <= top; arg++)
  {
    sq_pushnull(v);
    while (SQ_SUCCEEDED(sq_next(v, arg)))
      sq_newslot(v, res, SQFalse);
    sq_pop(v, 1);
  }
  return 1;
}

bool pk_value_equals(const PkTree *tree, const PkValue &val, const SQObject &o)
{
  SQObjectType t = sq_type(o);
  switch (val.type)
  {
    case PT_NULL: return t == OT_NULL;
    case PT_BOOL: return t == OT_BOOL && (_integer(o) != 0) == (val.i != 0);
    case PT_INT: return t == OT_INTEGER ? _integer(o) == SQInteger(val.i) : t == OT_FLOAT && SQFloat(val.i) == _float(o);
    case PT_FLOAT: return t == OT_FLOAT ? SQFloat(val.f) == _float(o) : t == OT_INTEGER && SQFloat(val.f) == SQFloat(_integer(o));
    case PT_STRING: return t == OT_STRING && tree->stringEquals(uint32_t(val.i), _string(o));
    default: return false;
  }
}

bool pk_view_find_value(PkView *view, const SQObject &needle, uint32_t &index)
{
  SQObjectType needleType = sq_type(needle);
  if (needleType != OT_NULL && !(needleType & (_RT_BOOL | _RT_INTEGER | _RT_FLOAT | _RT_STRING)))
    return false;
  const PkTree *tree = view->tree;
  const PkColumn *c = tree->column(view->ref);
  uint32_t begin, end, slot;
  bool inRest;
  tree->range(c, view->row, begin, end);
  for (uint32_t pos = tree->nextPresent(c, view->row, begin, end, &slot, &inRest); pos < end;
       pos = tree->nextPresentAfter(c, view->row, pos, end, slot, inRest))
  {
    PkValue item;
    if (tree->slotValue(c, view->row, slot, inRest, item) && pk_value_equals(tree, item, needle))
    {
      index = pos - begin;
      return true;
    }
  }
  return false;
}

SQInteger pk_view_contains(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 1);
  if (!view)
    return sq_throwerror(v, "invalid packed view");
  uint32_t index;
  sq_pushbool(v, pk_view_find_value(view, stack_get(v, 2), index));
  return 1;
}

SQInteger pk_view_indexof(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 1);
  if (!view)
    return sq_throwerror(v, "invalid packed view");
  uint32_t index;
  if (!pk_view_find_value(view, stack_get(v, 2), index))
    return 0;
  sq_pushinteger(v, SQInteger(index));
  return 1;
}

SQInteger pk_view_list_hasindex(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 1);
  if (!view)
    return sq_throwerror(v, "invalid packed view");
  SQInteger idx = tointeger(stack_get(v, 2));
  sq_pushbool(v, idx >= 0 && idx < SQInteger(pk_count(view)));
  return 1;
}

SQInteger pk_view_table_hasindex(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 1);
  if (!view)
    return sq_throwerror(v, "invalid packed view");
  const PkColumn *c = view->tree->column(view->ref);
  uint32_t pos, slot;
  bool inRest;
  PkValue val;
  sq_pushbool(v, pk_view_find(view, c, stack_get(v, 2), pos, slot, inRest) && view->tree->slotValue(c, view->row, slot, inRest, val));
  return 1;
}

SQInteger pk_view_is_frozen(HSQUIRRELVM v)
{
  sq_pushbool(v, SQTrue);
  return 1;
}

template <bool lists_as_arrays>
SQInteger pk_view_method(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 1);
  if (!view)
    return sq_throwerror(v, "invalid packed view");
  SQInteger top = sq_gettop(v);
  sq_push(v, top);
  if (!pk_push_shallow(v, view, _instance(stack_get(v, 1))->_class, lists_as_arrays))
    return SQ_ERROR;
  for (SQInteger i = 2; i < top; i++)
    sq_push(v, i);
  if (SQ_FAILED(sq_call(v, top - 1, SQTrue, SQFalse)))
    return SQ_ERROR;
  return 1;
}

SQInteger pk_view_write(HSQUIRRELVM v)
{
  const char *name = "";
  sq_getstring(v, sq_gettop(v), &name);
  eastl::string msg(eastl::string::CtorSprintf(), "packed view is read-only, %s() cannot change it", name);
  return sq_throwerror(v, msg.c_str());
}

void pk_add_view_method(HSQUIRRELVM v, const char *name, SQFUNCTION f, const SQObjectPtr &outer)
{
  sq_pushstring(v, name, -1);
  v->Push(outer);
  sq_newclosure(v, f, 1);
  sq_setnativeclosurename(v, -1, name);
  sq_rawset(v, -3);
}

struct PkNativeMethod
{
  const char *name;
  SQFUNCTION f;
  SQInteger nparamscheck;
  const char *typemask;
};

template <size_t R, size_t W, size_t N>
void pk_push_view_methods(HSQUIRRELVM v, SQClass *builtin_class, const char *const (&reads)[R], const char *const (&writes)[W],
  const PkNativeMethod (&natives)[N])
{
  sq_newtable(v);
  pk_add_view_method(v, "len", pk_view_len, SQObjectPtr());
  pk_add_view_method(v, "is_frozen", pk_view_is_frozen, SQObjectPtr());
  for (const char *name : reads)
  {
    SQObjectPtr builtin;
    if (builtin_class->Get(SQObjectPtr(SQString::Create(_ss(v), name)), builtin))
      pk_add_view_method(v, name, strcmp(name, "totable") == 0 ? pk_view_method<true> : pk_view_method<false>, builtin);
  }
  for (const char *name : writes)
    pk_add_view_method(v, name, pk_view_write, SQObjectPtr(SQString::Create(_ss(v), name)));
  for (const PkNativeMethod &m : natives)
  {
    sq_pushstring(v, m.name, -1);
    sq_pushnull(v);
    sq_newclosure(v, m.f, PK_NATIVE_METHOD_FREE_VARS);
    sq_setnativeclosurename(v, -1, m.name);
    sq_setparamscheck(v, m.nparamscheck, m.typemask);
    sq_rawset(v, -3);
  }
}

const char *const pk_table_reads[] = {
  "keys", "values", "map", "filter", "reduce", "each", "findindex", "findvalue", "topairs", "rawget"};
const PkNativeMethod pk_table_natives[] = {{"__merge", pk_view_merge, -2, "xt|y|x"}, {"hasindex", pk_view_table_hasindex, 2, "x."},
  {"rawin", pk_view_table_hasindex, 2, "x."}, {"hasvalue", pk_view_contains, 2, "x."}};
const char *const pk_table_writes[] = {"rawset", "rawdelete", "clear", "__update", "replace_with", "swap"};
const char *const pk_list_reads[] = {"map", "filter", "reduce", "each", "findindex", "findvalue", "totable", "slice", "top"};
const PkNativeMethod pk_list_natives[] = {{"indexof", pk_view_indexof, 2, "x."}, {"contains", pk_view_contains, 2, "x."},
  {"hasvalue", pk_view_contains, 2, "x."}, {"hasindex", pk_view_list_hasindex, 2, "xn"}};
const char *const pk_list_writes[] = {"append", "extend", "pop", "insert", "remove", "resize", "reverse", "sort", "sortby",
  "sort_reversed", "sortby_reversed", "apply", "replace", "replace_with", "clear", "swap"};

SQInteger pk_is_view(HSQUIRRELVM v)
{
  sq_pushbool(v, pk_get_view(v, 2) != nullptr);
  return 1;
}

void pk_set_int_slot(HSQUIRRELVM v, const char *name, SQInteger value)
{
  sq_pushstring(v, name, -1);
  sq_pushinteger(v, value);
  sq_rawset(v, -3);
}

SQInteger pk_stats(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 2);
  if (!view)
    return sq_throwerror(v, "stats() expects a packed view");
  const PkHeader &h = view->tree->header();
  uint32_t keyIndexBytes = (h.keyIndexMask + 1) * 4;
  sq_newtable(v);
  pk_set_int_slot(v, "total", h.size);
  pk_set_int_slot(v, "strings", h.keyIndex - h.stringOffsets);
  pk_set_int_slot(v, "stringCount", h.stringCount);
  pk_set_int_slot(v, "keyIndex", keyIndexBytes);
  pk_set_int_slot(v, "columns", h.size - (h.keyIndex + keyIndexBytes));
  pk_set_int_slot(v, "views", view->tree->refCount);
  size_t indexBytes = 0;
  for (auto &it : view->tree->indexes)
    indexBytes += it.second->bytes();
  pk_set_int_slot(v, "indexes", SQInteger(view->tree->indexes.size()));
  pk_set_int_slot(v, "indexBytes", SQInteger(indexBytes));
  SQInteger cachedStrings = 0;
  size_t stringCacheBytes = view->tree->stringSlots.capacity() * sizeof(PkTree::StringSlot);
  for (const PkTree::StringSlot &slot : view->tree->stringSlots)
    if (sq_type(slot.str) == OT_STRING)
    {
      cachedStrings++;
      stringCacheBytes += sizeof(SQString) + size_t(_string(slot.str)->_len);
    }
  pk_set_int_slot(v, "cachedStrings", cachedStrings);
  pk_set_int_slot(v, "stringCacheBytes", SQInteger(stringCacheBytes));
  return 1;
}


enum PqCmp : uint8_t
{
  PQ_EQ,
  PQ_NE,
  PQ_LT,
  PQ_LE,
  PQ_GT,
  PQ_GE,
};

struct PqVal
{
  PkType type = PT_NULL;
  int64_t i = 0;
  double f = 0;
  uint32_t ref = 0;
  uint32_t row = 0;
};

PqVal pq_from(const PkValue &x)
{
  PqVal v;
  v.type = x.type;
  switch (x.type)
  {
    case PT_NULL:
    case PT_BOOL:
    case PT_INT: v.i = x.i; break;
    case PT_FLOAT: v.f = x.f; break;
    case PT_STRING: v.i = int64_t(x.i) * 2; break;
    default:
      v.ref = x.ref;
      v.row = x.row;
      break;
  }
  return v;
}

bool pq_is_nan(const PqVal &v) { return v.type == PT_FLOAT && pk_is_nan(v.f); }

int pq_order(const PqVal &a, const PqVal &b)
{
  bool aNum = a.type == PT_INT || a.type == PT_FLOAT, bNum = b.type == PT_INT || b.type == PT_FLOAT;
  if (aNum && bNum)
  {
    if (a.type == PT_INT && b.type == PT_INT)
      return a.i < b.i ? -1 : (a.i > b.i ? 1 : 0);
    if (pq_is_nan(a) || pq_is_nan(b))
      return -2;
    SQFloat x = a.type == PT_INT ? SQFloat(a.i) : SQFloat(a.f), y = b.type == PT_INT ? SQFloat(b.i) : SQFloat(b.f);
    return x < y ? -1 : (x > y ? 1 : 0);
  }
  if (a.type != b.type)
    return -2;
  switch (a.type)
  {
    case PT_NULL: return 0;
    case PT_BOOL:
    case PT_STRING: return a.i < b.i ? -1 : (a.i > b.i ? 1 : 0);
    default: return a.ref == b.ref && a.row == b.row ? 0 : -2;
  }
}

bool pq_test_order(PqCmp op, int o)
{
  switch (op)
  {
    case PQ_EQ: return o == 0;
    case PQ_NE: return o != 0;
    case PQ_LT: return o == -1;
    case PQ_LE: return o == -1 || o == 0;
    case PQ_GT: return o == 1;
    default: return o == 1 || o == 0;
  }
}

bool pq_test(PqCmp op, const PqVal &a, const PqVal &b) { return pq_test_order(op, pq_order(a, b)); }

bool pq_test_float(PqCmp op, SQFloat a, SQFloat b)
{
  if (pk_is_nan(a) || pk_is_nan(b))
    return op == PQ_NE;
  return pq_test_order(op, a < b ? -1 : (a > b ? 1 : 0));
}

bool pq_key(const PqVal &v, PqKey &k)
{
  switch (v.type)
  {
    case PT_NULL: k = {PT_NULL, 0}; return true;
    case PT_BOOL:
    case PT_INT:
    case PT_STRING: k = {uint8_t(v.type), v.i}; return true;
    case PT_FLOAT:
      if (!pk_is_nan(v.f) && v.f == floor(v.f) && fabs(v.f) < 9.0e18)
        k = {PT_INT, int64_t(v.f)};
      else
      {
        int64_t bits;
        memcpy(&bits, &v.f, 8);
        k = {PT_FLOAT, bits};
      }
      return true;
    default: return false;
  }
}

const int64_t PQ_EXACT_FLOAT_INT = int64_t(1) << eastl::numeric_limits<SQFloat>::digits;

bool pq_is_inexact_number(const PqVal &v)
{
  if (v.type == PT_INT)
    return v.i >= PQ_EXACT_FLOAT_INT || v.i <= -PQ_EXACT_FLOAT_INT;
  return v.type == PT_FLOAT && (pk_is_nan(v.f) || fabs(v.f) >= double(PQ_EXACT_FLOAT_INT));
}

bool pq_truthy(const PqVal &v)
{
  switch (v.type)
  {
    case PT_NULL: return false;
    case PT_BOOL:
    case PT_INT: return v.i != 0;
    case PT_FLOAT: return pk_is_nan(v.f) || SQFloat(v.f) != 0;
    default: return true;
  }
}

int pq_sort_order(const PqVal &a, const PqVal &b)
{
  bool aNan = pq_is_nan(a), bNan = pq_is_nan(b);
  if ((aNan || bNan) && (a.type == PT_INT || a.type == PT_FLOAT) && (b.type == PT_INT || b.type == PT_FLOAT))
    return aNan == bNan ? 0 : (aNan ? 1 : -1);
  int o = pq_order(a, b);
  if (o == -2)
    o = a.type < b.type ? -1 : (a.type > b.type ? 1 : 0);
  return o;
}

PqKey pq_identity(const PqVal &v)
{
  PqKey k;
  if (!pq_key(v, k))
    k = {uint8_t(v.type), int64_t(uint64_t(v.ref) << 32 | v.row)};
  return k;
}

PkValue pq_to_pk(const PqVal &v)
{
  PkValue x = {};
  x.type = v.type;
  x.ref = v.ref;
  x.row = v.row;
  x.i = SQInteger(v.type == PT_STRING ? v.i / 2 : v.i);
  x.f = SQFloat(v.f);
  return x;
}

void pq_push_key(HSQUIRRELVM v, const PkTree *tree, const PqKey &k)
{
  switch (k.type)
  {
    case PT_BOOL: sq_pushbool(v, k.v != 0); break;
    case PT_INT: sq_pushinteger(v, SQInteger(k.v)); break;
    case PT_FLOAT:
    {
      double f;
      memcpy(&f, &k.v, 8);
      sq_pushfloat(v, SQFloat(f));
      break;
    }
    default:
    {
      const char *s;
      uint32_t len;
      tree->string(uint32_t(k.v / 2), s, len);
      sq_pushstring(v, s, SQInteger(len));
      break;
    }
  }
}

enum PqAgg : uint8_t
{
  AGG_NONE,
  AGG_COUNT,
  AGG_MIN,
  AGG_MAX,
  AGG_SUM,
  AGG_AVG,
};

struct PqAcc
{
  int64_t count = 0;
  int64_t intSum = 0;
  double floatSum = 0;
  bool isFloat = false;
  bool has = false;
  PqVal best;

  void add(PqAgg agg, const PqVal &x)
  {
    switch (agg)
    {
      case AGG_COUNT: count += x.type != PT_NULL ? 1 : 0; return;
      case AGG_MIN:
      case AGG_MAX:
        if (x.type != PT_INT && x.type != PT_FLOAT && x.type != PT_STRING)
          return;
        if (!has || (agg == AGG_MIN ? pq_sort_order(x, best) < 0 : pq_sort_order(x, best) > 0))
        {
          best = x;
          has = true;
        }
        return;
      default:
        if (x.type == PT_INT)
        {
          intSum = int64_t(uint64_t(intSum) + uint64_t(x.i));
          floatSum += double(x.i);
        }
        else if (x.type == PT_FLOAT)
        {
          isFloat = true;
          floatSum += x.f;
        }
        else
          return;
        count++;
    }
  }

  void push(HSQUIRRELVM v, PkTree *tree, PqAgg agg, SQClass *cls) const
  {
    switch (agg)
    {
      case AGG_COUNT: sq_pushinteger(v, SQInteger(count)); return;
      case AGG_MIN:
      case AGG_MAX:
        if (has)
          pk_push_value(v, tree, pq_to_pk(best), cls);
        else
          sq_pushnull(v);
        return;
      case AGG_SUM:
        if (isFloat)
          sq_pushfloat(v, SQFloat(floatSum));
        else
          sq_pushinteger(v, SQInteger(intSum));
        return;
      default:
        if (count)
          sq_pushfloat(v, SQFloat(floatSum / double(count)));
        else
          sq_pushnull(v);
        return;
    }
  }
};

struct PqExpr;

struct PqStep
{
  enum Kind : uint8_t
  {
    FIELD,
    INDEX,
    ANY,
    FILTER,
  };
  int64_t index = 0;
  uint32_t slot = 0;
  Kind kind = FIELD;
  eastl::string name;
  eastl::shared_ptr<PqExpr> filter;
};

struct PqPath
{
  eastl::vector<PqStep> steps;
  eastl::string text;
  bool fromKey = false;
  bool hasAny = false;
  bool hasFilter = false;
  bool missingAsNull = false;
};

void pq_append_field_text(eastl::string &text, const eastl::string &name)
{
  text += ".";
  text += eastl::to_string(name.size());
  text += ":";
  text += name;
}

struct PqOperand
{ //-V802
  enum Kind : uint8_t
  {
    PATH,
    LITERAL,
    PARAM,
  };
  PqVal literal;
  uint32_t slot = 0;
  Kind kind = LITERAL;
  PqPath path;
};

struct PqExpr
{
  enum Op : uint8_t
  {
    OR,
    AND,
    NOT,
    CMP,
    TRUTHY,
    IN,
  };
  Op op = OR;
  PqCmp cmp = PQ_EQ;
  uint32_t setSlot = 0;
  eastl::unique_ptr<PqExpr> a, b;
  PqOperand l, r;
  eastl::unique_ptr<PqExpr> necessaryConditions;
};

struct PqSetSource
{
  bool fromParam = false;
  uint32_t paramSlot = 0;
  eastl::vector<PqOperand> items;
};

struct PqSet
{
  PqKeySet keys;
  eastl::vector<PqVal> inexactNumbers;
  bool hasNull = false;
};

struct PqQuery
{
  PqPath source;
  eastl::unique_ptr<PqExpr> where;
  bool whereReadsKey = false;
  PqPath groupPath;
  bool hasGroup = false;
  PqPath orderPath;
  bool hasOrder = false;
  bool orderDesc = false;
  int64_t limit = -1;
  PqPath selectPath;
  bool hasSelect = false;
  bool distinct = false;
  PqAgg agg = AGG_NONE;
  bool aggHasPath = false;
  eastl::vector<PqSetSource> sets;
  eastl::vector<eastl::string> fieldNames;
  eastl::vector<eastl::string> stringLiterals;
  eastl::vector<eastl::string> params;
  uint64_t resolvedSerial = 0;
  eastl::vector<int32_t> fieldIds;
  eastl::vector<int64_t> stringRanks;
};

class PqParser
{
public:
  const char *error = nullptr;

  bool parse(const char *text, PqQuery &q)
  {
    s = text;
    query = &q;
    skipWs();
    if (!parsePath(q.source, true))
      return false;
    if (q.source.steps.empty() || q.source.steps.back().kind != PqStep::ANY)
      return fail("the source path must end with [*]");
    q.source.steps.pop_back();
    for (const PqStep &st : q.source.steps)
      if (st.kind == PqStep::ANY || st.kind == PqStep::FILTER)
        return fail("only the last step of the source path can be [*]");
    skipWs();
    if (*s == '?')
    {
      s++;
      q.where = parseOr();
      if (!q.where)
        return false;
      q.whereReadsKey = readsKey(q.where.get());
    }
    skipWs();
    if (word("group"))
    {
      skipWs();
      if (!word("by"))
        return fail("'by' expected after 'group'");
      skipWs();
      if (!parsePath(q.groupPath, false))
        return false;
      q.hasGroup = true;
      skipWs();
    }
    if (word("order"))
    {
      skipWs();
      if (!word("by"))
        return fail("'by' expected after 'order'");
      skipWs();
      if (!parsePath(q.orderPath, false))
        return false;
      q.hasOrder = true;
      skipWs();
      if (word("desc"))
        q.orderDesc = true;
      else
        word("asc");
      skipWs();
    }
    if (word("limit"))
    {
      skipWs();
      PqVal n;
      if (*s < '0' || *s > '9' || !parseNumber(n) || n.type != PT_INT)
        return fail("a non-negative integer expected after 'limit'");
      q.limit = n.i;
      skipWs();
    }
    if (word("select"))
    {
      skipWs();
      if (!parseSelect(q))
        return false;
      skipWs();
    }
    if (*s)
      return fail("unexpected text at the end of the query");
    if (q.hasGroup && q.agg == AGG_NONE)
      return fail("group by needs an aggregate in select");
    if (q.hasGroup && (q.hasOrder || q.limit >= 0))
      return fail("order by and limit do not go with group by");
    addNecessaryConditions(q.where.get());
    return true;
  }

private:
  const char *s = nullptr;
  PqQuery *query = nullptr;

  bool fail(const char *msg)
  {
    if (!error)
      error = msg;
    return false;
  }

  void skipWs()
  {
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
      s++;
  }

  static bool isIdentStart(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
  static bool isIdentChar(char c) { return isIdentStart(c) || (c >= '0' && c <= '9'); }

  bool parseSelect(PqQuery &q)
  {
    q.hasSelect = true;
    if (word("distinct"))
    {
      skipWs();
      q.distinct = true;
      return parsePath(q.selectPath, false);
    }
    static const struct
    {
      const char *name;
      PqAgg agg;
    } aggs[] = {{"count", AGG_COUNT}, {"min", AGG_MIN}, {"max", AGG_MAX}, {"sum", AGG_SUM}, {"avg", AGG_AVG}};
    const char *start = s;
    for (auto &a : aggs)
    {
      if (!word(a.name))
        continue;
      skipWs();
      if (*s != '(')
      {
        s = start;
        break;
      }
      s++;
      skipWs();
      q.agg = a.agg;
      if (*s == ')')
      {
        if (a.agg != AGG_COUNT)
          return fail("only count() goes with no path");
      }
      else
      {
        if (!parsePath(q.selectPath, false))
          return false;
        q.aggHasPath = true;
        skipWs();
        if (*s != ')')
          return fail("')' expected");
      }
      s++;
      return true;
    }
    return parsePath(q.selectPath, false);
  }

  bool parseSet(PqExpr &e)
  {
    skipWs();
    PqSetSource src;
    if (*s == '$')
    {
      PqOperand o;
      if (!parseOperand(o))
        return false;
      src.fromParam = true;
      src.paramSlot = o.slot;
    }
    else if (*s == '[')
    {
      s++;
      skipWs();
      while (*s != ']')
      {
        PqOperand o;
        if (!parseOperand(o))
          return false;
        if (o.kind != PqOperand::LITERAL)
          return fail("the list after 'in' takes only literals");
        src.items.push_back(o);
        skipWs();
        if (*s == ',')
        {
          s++;
          skipWs();
        }
      }
      s++;
    }
    else
      return fail("'[' or a parameter expected after 'in'");
    e.setSlot = uint32_t(query->sets.size());
    query->sets.push_back(eastl::move(src));
    return true;
  }

  static const PqOperand *leafPath(const PqExpr &e)
  {
    if (e.op == PqExpr::TRUTHY || e.op == PqExpr::IN)
      return e.l.kind == PqOperand::PATH ? &e.l : nullptr;
    if (e.op != PqExpr::CMP || (e.l.kind == PqOperand::PATH) == (e.r.kind == PqOperand::PATH))
      return nullptr;
    return e.l.kind == PqOperand::PATH ? &e.l : &e.r;
  }

  static eastl::unique_ptr<PqExpr> leafWithPath(const PqExpr &e, const PqOperand &po, const eastl::vector<PqStep> &steps)
  {
    eastl::unique_ptr<PqExpr> x(new PqExpr);
    x->op = e.op;
    x->cmp = e.cmp;
    x->setSlot = e.setSlot;
    x->l = e.l;
    x->r = e.r;
    PqOperand &o = &po == &e.l ? x->l : x->r;
    o.path.steps = steps;
    o.path.hasAny = true;
    o.path.hasFilter = false;
    o.path.missingAsNull = true;
    o.path.text = o.path.fromKey ? "@key" : "@";
    for (const PqStep &st : steps)
      if (st.kind == PqStep::FIELD)
        pq_append_field_text(o.path.text, st.name);
      else
        o.path.text += st.kind == PqStep::INDEX ? "[" + eastl::to_string(st.index) + "]" : eastl::string("[*]");
    o.path.text += " or null";
    return x;
  }

  static void conjuncts(const PqExpr &e, eastl::vector<const PqExpr *> &out)
  {
    if (e.op == PqExpr::AND)
    {
      conjuncts(*e.a, out);
      conjuncts(*e.b, out);
    }
    else
      out.push_back(&e);
  }

  static void addNecessaryConditions(PqExpr *e)
  {
    if (!e)
      return;
    if (e->op == PqExpr::AND || e->op == PqExpr::OR || e->op == PqExpr::NOT)
    {
      addNecessaryConditions(e->a.get());
      addNecessaryConditions(e->b.get());
      return;
    }
    const PqOperand *po = leafPath(*e);
    if (!po || !po->path.hasFilter || po->path.fromKey)
      return;
    eastl::vector<eastl::unique_ptr<PqExpr>> parts;
    eastl::vector<PqStep> base;
    for (const PqStep &st : po->path.steps)
    {
      if (st.kind != PqStep::FILTER)
      {
        base.push_back(st);
        continue;
      }
      PqStep any;
      any.kind = PqStep::ANY;
      base.push_back(any);
      eastl::vector<const PqExpr *> conds;
      conjuncts(*st.filter, conds);
      for (const PqExpr *c : conds)
      {
        const PqOperand *cp = leafPath(*c);
        if (!cp || cp->path.fromKey || cp->path.hasFilter || (c->op == PqExpr::CMP && c->cmp == PQ_NE))
          continue;
        eastl::vector<PqStep> steps(base);
        steps.insert(steps.end(), cp->path.steps.begin(), cp->path.steps.end());
        parts.push_back(leafWithPath(*c, *cp, steps));
      }
    }
    if (e->op == PqExpr::TRUTHY || e->cmp != PQ_NE)
      parts.push_back(leafWithPath(*e, *po, base));
    for (auto &part : parts)
    {
      if (!e->necessaryConditions)
      {
        e->necessaryConditions = eastl::move(part);
        continue;
      }
      eastl::unique_ptr<PqExpr> both(new PqExpr);
      both->op = PqExpr::AND;
      both->a = eastl::move(e->necessaryConditions);
      both->b = eastl::move(part);
      e->necessaryConditions = eastl::move(both);
    }
  }

  bool word(const char *w)
  {
    size_t n = strlen(w);
    if (strncmp(s, w, n) != 0 || isIdentChar(s[n]))
      return false;
    s += n;
    return true;
  }

  bool parseIdent(eastl::string &out)
  {
    if (!isIdentStart(*s))
      return fail("identifier expected");
    const char *b = s;
    while (isIdentChar(*s))
      s++;
    out.assign(b, s);
    return true;
  }

  bool parseString(eastl::string &out)
  {
    char quote = *s++;
    out.clear();
    while (*s && *s != quote)
    {
      if (*s == '\\' && s[1])
        s++;
      out.push_back(*s++);
    }
    if (*s != quote)
      return fail("unterminated string");
    s++;
    return true;
  }

  bool parseNumber(PqVal &out)
  {
    char *end = nullptr;
    const char *b = s;
    bool isFloat = false;
    const char *p = s + (*s == '-' ? 1 : 0);
    while ((*p >= '0' && *p <= '9') || *p == '.' || *p == 'e' || *p == 'E')
    {
      if (*p == '.' || *p == 'e' || *p == 'E')
        isFloat = true;
      p++;
    }
    if (isFloat)
    {
      out.type = PT_FLOAT;
      out.f = SQFloat(strtod(b, &end));
    }
    else
    {
      out.type = PT_INT;
      out.i = strtoll(b, &end, 10);
    }
    if (end == b)
      return fail("number expected");
    s = end;
    return true;
  }

  static bool readsKey(const PqExpr *e)
  {
    return e && ((e->l.kind == PqOperand::PATH && e->l.path.fromKey) || (e->r.kind == PqOperand::PATH && e->r.path.fromKey) ||
                  readsKey(e->a.get()) || readsKey(e->b.get()));
  }

  void addFieldStep(PqPath &path, const eastl::string &name)
  {
    PqStep st;
    st.name = name;
    st.slot = uint32_t(query->fieldNames.size());
    query->fieldNames.push_back(name);
    path.steps.push_back(st);
    pq_append_field_text(path.text, name);
  }

  bool parsePath(PqPath &path, bool is_source)
  {
    if (*s == '@')
    {
      s++;
      if (strncmp(s, "key", 3) == 0 && !isIdentChar(s[3]))
      {
        s += 3;
        path.fromKey = true;
        path.text = "@key";
      }
      else
        path.text = "@";
    }
    else if (isIdentStart(*s))
    {
      eastl::string name;
      parseIdent(name);
      addFieldStep(path, name);
    }
    else if (!(is_source && *s == '['))
      return fail("path expected");
    for (;;)
    {
      if (*s == '.')
      {
        s++;
        eastl::string name;
        if (!parseIdent(name))
          return false;
        addFieldStep(path, name);
      }
      else if (*s == '[')
      {
        s++;
        skipWs();
        if (*s == '*')
        {
          s++;
          PqStep st;
          st.kind = PqStep::ANY;
          path.steps.push_back(st);
          path.hasAny = true;
          path.text += "[*]";
        }
        else if (*s == '?')
        {
          s++;
          PqStep st;
          st.kind = PqStep::FILTER;
          st.filter = parseOr();
          if (!st.filter)
            return false;
          path.steps.push_back(st);
          path.hasAny = true;
          path.hasFilter = true;
          path.text += "[?]";
        }
        else if (*s == '\'' || *s == '"')
        {
          eastl::string name;
          if (!parseString(name))
            return false;
          addFieldStep(path, name);
        }
        else
        {
          PqVal num;
          if (!parseNumber(num) || num.type != PT_INT)
            return fail("integer index expected");
          PqStep st;
          st.kind = PqStep::INDEX;
          st.index = num.i;
          path.steps.push_back(st);
          path.text += "[" + eastl::to_string(num.i) + "]";
        }
        skipWs();
        if (*s != ']')
          return fail("']' expected");
        s++;
      }
      else
        return true;
    }
  }

  bool parseOperand(PqOperand &o)
  {
    skipWs();
    if (*s == '$')
    {
      s++;
      eastl::string name;
      if (!parseIdent(name))
        return false;
      auto &params = query->params;
      auto it = eastl::find(params.begin(), params.end(), name);
      o.kind = PqOperand::PARAM;
      o.slot = uint32_t(it - params.begin());
      if (it == params.end())
        params.push_back(name);
      return true;
    }
    if (*s == '\'' || *s == '"')
    {
      o.kind = PqOperand::LITERAL;
      o.literal.type = PT_STRING;
      o.slot = uint32_t(query->stringLiterals.size());
      eastl::string str;
      if (!parseString(str))
        return false;
      query->stringLiterals.push_back(eastl::move(str));
      return true;
    }
    if (*s == '-' || (*s >= '0' && *s <= '9'))
    {
      o.kind = PqOperand::LITERAL;
      return parseNumber(o.literal);
    }
    static const struct
    {
      const char *word;
      PkType type;
      int64_t i;
    } keywords[] = {{"true", PT_BOOL, 1}, {"false", PT_BOOL, 0}, {"null", PT_NULL, 0}};
    for (auto &kw : keywords)
    {
      size_t n = strlen(kw.word);
      if (strncmp(s, kw.word, n) == 0 && !isIdentChar(s[n]))
      {
        s += n;
        o.kind = PqOperand::LITERAL;
        o.literal.type = kw.type;
        o.literal.i = kw.i;
        return true;
      }
    }
    o.kind = PqOperand::PATH;
    return parsePath(o.path, false);
  }

  eastl::unique_ptr<PqExpr> parseOr()
  {
    eastl::unique_ptr<PqExpr> left = parseAnd();
    while (left)
    {
      skipWs();
      if (s[0] != '|' || s[1] != '|')
        break;
      s += 2;
      eastl::unique_ptr<PqExpr> e(new PqExpr);
      e->op = PqExpr::OR;
      e->a = eastl::move(left);
      e->b = parseAnd();
      if (!e->b)
        return nullptr;
      left = eastl::move(e);
    }
    return left;
  }

  eastl::unique_ptr<PqExpr> parseAnd()
  {
    eastl::unique_ptr<PqExpr> left = parseUnary();
    while (left)
    {
      skipWs();
      if (s[0] != '&' || s[1] != '&')
        break;
      s += 2;
      eastl::unique_ptr<PqExpr> e(new PqExpr);
      e->op = PqExpr::AND;
      e->a = eastl::move(left);
      e->b = parseUnary();
      if (!e->b)
        return nullptr;
      left = eastl::move(e);
    }
    return left;
  }

  eastl::unique_ptr<PqExpr> parseUnary()
  {
    skipWs();
    if (*s == '!' && s[1] != '=')
    {
      s++;
      eastl::unique_ptr<PqExpr> e(new PqExpr);
      e->op = PqExpr::NOT;
      e->a = parseUnary();
      return e->a ? eastl::move(e) : nullptr;
    }
    if (*s == '(')
    {
      s++;
      eastl::unique_ptr<PqExpr> e = parseOr();
      skipWs();
      if (!e || *s != ')')
      {
        fail("')' expected");
        return nullptr;
      }
      s++;
      return e;
    }
    eastl::unique_ptr<PqExpr> e(new PqExpr);
    if (!parseOperand(e->l))
      return nullptr;
    skipWs();
    static const struct
    {
      const char *text;
      PqCmp cmp;
    } ops[] = {{"==", PQ_EQ}, {"!=", PQ_NE}, {"<=", PQ_LE}, {">=", PQ_GE}, {"<", PQ_LT}, {">", PQ_GT}};
    for (auto &op : ops)
    {
      size_t n = strlen(op.text);
      if (strncmp(s, op.text, n) == 0)
      {
        s += n;
        e->op = PqExpr::CMP;
        e->cmp = op.cmp;
        return parseOperand(e->r) ? eastl::move(e) : nullptr;
      }
    }
    if (word("in"))
    {
      e->op = PqExpr::IN;
      return parseSet(*e) ? eastl::move(e) : nullptr;
    }
    e->op = PqExpr::TRUTHY;
    return e;
  }
};

const PqVal pq_null_value;
const PqPath pq_empty_path;

enum PqPlan
{
  PLAN_AUTO,
  PLAN_COLUMNS,
  PLAN_ROWS,
};

struct PqBits
{
  eastl::vector<uint64_t> words;
  uint32_t size = 0;

  explicit PqBits(uint32_t n = 0, bool ones = false) { reset(n, ones); }

  void reset(uint32_t n, bool ones)
  {
    size = n;
    words.assign((n + 63) / 64, ones ? ~uint64_t(0) : 0);
    trim();
  }
  void trim()
  {
    if ((size & 63) && !words.empty())
      words.back() &= (uint64_t(1) << (size & 63)) - 1;
  }
  void setAll()
  {
    for (uint64_t &w : words)
      w = ~uint64_t(0);
    trim();
  }
  void set(uint32_t i) { words[i >> 6] |= uint64_t(1) << (i & 63); }
  bool get(uint32_t i) const { return (words[i >> 6] >> (i & 63)) & 1; }
  bool any() const
  {
    for (uint64_t w : words)
      if (w)
        return true;
    return false;
  }
  void invert()
  {
    for (uint64_t &w : words)
      w = ~w;
    trim();
  }
  void andWith(const PqBits &o)
  {
    for (size_t i = 0; i < words.size(); i++)
      words[i] &= o.words[i];
  }
  void andNot(const PqBits &o)
  {
    for (size_t i = 0; i < words.size(); i++)
      words[i] &= ~o.words[i];
  }
  void orWith(const PqBits &o)
  {
    for (size_t i = 0; i < words.size(); i++)
      words[i] |= o.words[i];
  }
  template <typename F>
  bool forEach(F f) const
  {
    for (size_t w = 0; w < words.size(); w++)
      for (uint64_t x = words[w]; x; x &= x - 1)
        if (f(uint32_t(w * 64 + pk_popcount((x & (0 - x)) - 1))))
          return true;
    return false;
  }
};

bool pq_in_set(const PqSet &set, const PqVal &v)
{
  PqKey k;
  if (pq_is_nan(v) || !pq_key(v, k))
    return false;
  if (set.keys.count(k) != 0)
    return true;
  if (pq_is_inexact_number(v))
    for (const PqVal &x : set.inexactNumbers)
      if (pq_order(v, x) == 0)
        return true;
  return false;
}

struct PqLeaf
{
  const PqPath *path = nullptr;
  const PqSet *set = nullptr;
  PqVal value;
  PqCmp op = PQ_EQ;
  bool truthy = false;
  bool negate = false;
  bool sameType = false;
};

template <typename T>
inline bool pq_cmp_num(PqCmp op, T a, T b)
{
  switch (op)
  {
    case PQ_EQ: return a == b;
    case PQ_NE: return a != b;
    case PQ_LT: return a < b;
    case PQ_LE: return a <= b;
    case PQ_GT: return a > b;
    default: return a >= b;
  }
}

template <typename T, typename P>
void pq_scan_typed(const uint8_t *data, uint32_t lo, uint32_t hi, P pred, PqBits &out)
{
  for (uint32_t r = lo; r < hi; r++)
  {
    T x;
    memcpy(&x, data + size_t(r) * sizeof(T), sizeof(T));
    if (pred(uint64_t(x)))
      out.set(r - lo);
  }
}

template <typename P>
void pq_scan(const uint8_t *data, uint32_t width, uint32_t lo, uint32_t hi, P pred, PqBits &out)
{
  switch (width)
  {
    case 1: pq_scan_typed<uint8_t>(data, lo, hi, pred, out); break;
    case 2: pq_scan_typed<uint16_t>(data, lo, hi, pred, out); break;
    case 4: pq_scan_typed<uint32_t>(data, lo, hi, pred, out); break;
    default: pq_scan_typed<uint64_t>(data, lo, hi, pred, out); break;
  }
}

class PqRunner
{
public:
  PqRunner(PqQuery &q, const PkTree &t) : query(q), tree(t)
  {
    if (query.resolvedSerial == tree.serial)
      return;
    query.fieldIds.resize(query.fieldNames.size());
    for (size_t i = 0; i < query.fieldNames.size(); i++)
    {
      const eastl::string &name = query.fieldNames[i];
      uint32_t id;
      SQInteger len = SQInteger(name.size());
      query.fieldIds[i] = tree.findKeyString(name.c_str(), len, _hashstr(name.c_str(), size_t(len)), id) ? int32_t(id) : -1;
    }
    query.stringRanks.resize(query.stringLiterals.size());
    for (size_t i = 0; i < query.stringLiterals.size(); i++)
      query.stringRanks[i] = tree.stringRank(query.stringLiterals[i].c_str(), SQInteger(query.stringLiterals[i].size()));
    query.resolvedSerial = tree.serial;
  }

  eastl::vector<SQObjectPtr> paramObjects;
  eastl::vector<PqVal> params;
  eastl::vector<PqSet> sets;

  bool makeSets()
  {
    sets.resize(query.sets.size());
    for (size_t i = 0; i < query.sets.size(); i++)
    {
      const PqSetSource &src = query.sets[i];
      PqSet &set = sets[i];
      auto add = [&set](const PqVal &x) {
        PqKey k;
        if (pq_key(x, k))
          set.keys.insert(k);
        if (pq_is_inexact_number(x))
          set.inexactNumbers.push_back(x);
        set.hasNull = set.hasNull || x.type == PT_NULL;
      };
      if (!src.fromParam)
      {
        for (const PqOperand &o : src.items)
          add(operandValue(o));
        continue;
      }
      const SQObjectPtr &arr = paramObjects[src.paramSlot];
      if (sq_type(arr) != OT_ARRAY)
        return false;
      const SQObjectPtrVec &values = _array(arr)->_values;
      for (SQUnsignedInteger j = 0; j < values.size(); j++)
        add(paramFromObject(values[j]));
    }
    return true;
  }

  PqVal paramFromObject(const SQObject &o) const
  {
    PqVal v;
    switch (sq_type(o))
    {
      case OT_BOOL:
        v.type = PT_BOOL;
        v.i = _integer(o) != 0;
        break;
      case OT_INTEGER:
        v.type = PT_INT;
        v.i = _integer(o);
        break;
      case OT_FLOAT:
        v.type = PT_FLOAT;
        v.f = _float(o);
        break;
      case OT_STRING:
        v.type = PT_STRING;
        v.i = tree.stringRank(_string(o)->_val, _string(o)->_len);
        break;
      default: break;
    }
    return v;
  }

  PqVal operandValue(const PqOperand &o) const
  {
    if (o.kind == PqOperand::PARAM)
      return params[o.slot];
    PqVal v = o.literal;
    if (v.type == PT_STRING)
      v.i = query.stringRanks[o.slot];
    return v;
  }

  static bool nativeStep(uint32_t type, const PqStep &st, uint32_t &component)
  {
    if (st.kind == PqStep::FIELD)
      return pk_native_field(type, st.name.c_str(), st.name.size(), component);
    return st.kind == PqStep::INDEX && pk_native_index(type, st.index, component);
  }

  bool step(const PqStep &st, const PqVal &v, PqVal &out) const
  {
    uint32_t component;
    if (pk_is_native(v.type))
    {
      if (!nativeStep(v.type, st, component))
        return false;
      PkValue x;
      tree.nativeComponent(v.ref, v.row, component, x);
      out = pq_from(x);
      return true;
    }
    if (v.type != PT_TABLE && v.type != PT_ARRAY)
      return false;
    const PkColumn *c = tree.column(v.ref);
    uint32_t pos = 0;
    bool found = false;
    if (st.kind == PqStep::FIELD)
    {
      int32_t id = query.fieldIds[st.slot];
      if (id >= 0 && c->kind == PK_STRUCT)
      {
        uint32_t field;
        if (tree.findField(c, uint32_t(id), field))
        {
          PkValue x;
          if (!tree.element(pk_u32(PkTree::payload(c) + (size_t(c->extra) + field) * 4), v.row, x))
            return false;
          out = pq_from(x);
          return true;
        }
        uint32_t rest = PkTree::restOf(c);
        if (!rest)
          return false;
        c = tree.column(rest);
        PkValue k;
        k.type = PT_STRING;
        k.i = SQInteger(id);
        found = tree.findInMap(c, v.row, k, pos);
      }
      else if (id >= 0 && c->kind == PK_MAP)
      {
        PkValue k;
        k.type = PT_STRING;
        k.i = SQInteger(id);
        found = tree.findInMap(c, v.row, k, pos);
      }
    }
    else if (c->kind == PK_LIST)
    {
      uint32_t b, e;
      tree.range(c, v.row, b, e);
      if (st.index >= 0 && st.index < int64_t(e - b))
      {
        pos = b + uint32_t(st.index);
        found = true;
      }
    }
    else if (c->kind == PK_MAP)
    {
      PkValue k;
      k.type = PT_INT;
      k.i = SQInteger(st.index);
      found = tree.findInMap(c, v.row, k, pos);
    }
    PkValue x;
    if (!found || !tree.valueAt(c, v.row, pos, x))
      return false;
    out = pq_from(x);
    return true;
  }

  template <typename F>
  bool walk(const PqPath &path, size_t i, const PqVal &v, F &f) const
  {
    for (; i < path.steps.size(); i++)
    {
      const PqStep &st = path.steps[i];
      if (st.kind == PqStep::ANY || st.kind == PqStep::FILTER)
      {
        if (v.type != PT_TABLE && v.type != PT_ARRAY)
          return false;
        const PkColumn *c = tree.column(v.ref);
        uint32_t b, e;
        tree.range(c, v.row, b, e);
        uint32_t slot;
        bool inRest;
        for (uint32_t pos = tree.nextPresent(c, v.row, b, e, &slot, &inRest); pos < e;
             pos = tree.nextPresentAfter(c, v.row, pos, e, slot, inRest))
        {
          PkValue x;
          if (!tree.slotValue(c, v.row, slot, inRest, x))
            continue;
          PqVal xv = pq_from(x);
          if (st.kind == PqStep::FILTER)
          {
            PkValue k;
            tree.slotKey(c, v.row, slot, inRest, k);
            if (!eval(*st.filter, xv, pq_from(k)))
              continue;
          }
          if (walk(path, i + 1, xv, f))
            return true;
        }
        return false;
      }
      PqVal next;
      if (!step(st, v, next))
        return path.hasAny && !path.missingAsNull ? false : f(pq_null_value);
      return walk(path, i + 1, next, f);
    }
    return f(v);
  }

  template <typename F>
  bool anyValue(const PqOperand &o, const PqVal &row, const PqVal &key, F f) const
  {
    if (o.kind == PqOperand::PATH)
      return walk(o.path, 0, o.path.fromKey ? key : row, f);
    return f(operandValue(o));
  }

  bool constantText(const PqOperand &o, const char *&s, SQInteger &len) const
  {
    if (o.kind == PqOperand::LITERAL && o.literal.type == PT_STRING)
    {
      s = query.stringLiterals[o.slot].c_str();
      len = SQInteger(query.stringLiterals[o.slot].size());
      return true;
    }
    if (o.kind != PqOperand::PARAM || sq_type(paramObjects[o.slot]) != OT_STRING)
      return false;
    s = _stringval(paramObjects[o.slot]);
    len = _string(paramObjects[o.slot])->_len;
    return true;
  }

  static int textOrder(const char *a, SQInteger a_len, const char *b, SQInteger b_len)
  {
    int r = memcmp(a, b, size_t(eastl::min(a_len, b_len)));
    if (r == 0)
      return a_len < b_len ? -1 : (a_len > b_len ? 1 : 0);
    return r < 0 ? -1 : 1;
  }

  int constantOrder(const PqOperand &a, const PqOperand &b) const
  {
    const char *as, *bs;
    SQInteger al, bl;
    if (constantText(a, as, al) && constantText(b, bs, bl))
      return textOrder(as, al, bs, bl);
    return pq_order(operandValue(a), operandValue(b));
  }

  bool constantIn(const PqOperand &o, uint32_t set_slot) const
  {
    const char *s, *x;
    SQInteger len, xlen;
    if (!constantText(o, s, len))
      return pq_in_set(sets[set_slot], operandValue(o));
    const PqSetSource &src = query.sets[set_slot];
    if (!src.fromParam)
    {
      for (const PqOperand &item : src.items)
        if (constantText(item, x, xlen) && textOrder(s, len, x, xlen) == 0)
          return true;
      return false;
    }
    const SQObjectPtrVec &values = _array(paramObjects[src.paramSlot])->_values;
    for (SQUnsignedInteger j = 0; j < values.size(); j++)
      if (sq_type(values[j]) == OT_STRING && textOrder(s, len, _stringval(values[j]), _string(values[j])->_len) == 0)
        return true;
    return false;
  }

  bool eval(const PqExpr &e, const PqVal &row, const PqVal &key) const
  {
    switch (e.op)
    {
      case PqExpr::OR: return eval(*e.a, row, key) || eval(*e.b, row, key);
      case PqExpr::AND: return eval(*e.a, row, key) && eval(*e.b, row, key);
      case PqExpr::NOT: return !eval(*e.a, row, key);
      case PqExpr::TRUTHY: return anyValue(e.l, row, key, pq_truthy);
      case PqExpr::IN:
        if (e.l.kind != PqOperand::PATH)
          return constantIn(e.l, e.setSlot);
        return anyValue(e.l, row, key, [&](const PqVal &v) { return pq_in_set(sets[e.setSlot], v); });
      default:
        if (e.l.kind != PqOperand::PATH && e.r.kind != PqOperand::PATH)
          return pq_test_order(e.cmp, constantOrder(e.l, e.r));
        return anyValue(e.l, row, key,
          [&](const PqVal &lv) { return anyValue(e.r, row, key, [&](const PqVal &rv) { return pq_test(e.cmp, lv, rv); }); });
    }
  }

  const PqExpr *findIndexable(const PqExpr *e) const
  {
    if (!e)
      return nullptr;
    if (e->op == PqExpr::AND)
    {
      const PqExpr *x = findIndexable(e->a.get());
      return x ? x : findIndexable(e->b.get());
    }
    if (e->necessaryConditions && (e->op == PqExpr::TRUTHY || e->op == PqExpr::CMP || e->op == PqExpr::IN))
      return findIndexable(e->necessaryConditions.get());
    if (e->op == PqExpr::IN)
      return e->l.kind == PqOperand::PATH && sets[e->setSlot].inexactNumbers.empty() ? e : nullptr;
    if (e->op != PqExpr::CMP || e->cmp != PQ_EQ)
      return nullptr;
    bool lPath = e->l.kind == PqOperand::PATH, rPath = e->r.kind == PqOperand::PATH;
    if (lPath == rPath)
      return nullptr;
    if ((lPath ? e->l : e->r).path.hasFilter)
      return findIndexable(e->necessaryConditions.get());
    return pq_is_inexact_number(operandValue(lPath ? e->r : e->l)) ? nullptr : e;
  }

  const PqIndex &index(uint32_t coll_ref, uint32_t coll_row, const PqPath &path, PkTree &mutable_tree) const
  {
    auto it = mutable_tree.indexes.find_as(PqIndexKeyRef{coll_ref, coll_row, path.text}, PqIndexKeyHash(), PqIndexKeyEqual());
    if (it != mutable_tree.indexes.end())
      return *it->second;
    eastl::unique_ptr<PqIndex> idx(new PqIndex);
    const PkColumn *c = tree.column(coll_ref);
    uint32_t b, e;
    tree.range(c, coll_row, b, e);
    eastl::vector<eastl::pair<PqKey, uint32_t>> entries;
    for (uint32_t pos = b; pos < e; pos++)
    {
      PkValue x;
      if (!tree.valueAt(c, coll_row, pos, x))
        continue;
      if (path.fromKey)
        tree.keyAt(c, coll_row, pos, x);
      auto add = [&](const PqVal &v) {
        PqKey key;
        if (pq_key(v, key))
          entries.push_back({key, pos});
        return false;
      };
      walk(path, 0, pq_from(x), add);
    }
    idx->build(entries);
    const PqIndex &res = *idx;
    mutable_tree.indexes.emplace(PqIndexKey{coll_ref, coll_row, path.text}, eastl::move(idx));
    return res;
  }

  bool leafFor(const PqExpr &e, PqLeaf &leaf) const
  {
    if (e.op == PqExpr::TRUTHY)
    {
      if (e.l.kind != PqOperand::PATH || e.l.path.fromKey || e.l.path.hasFilter)
        return false;
      leaf.path = &e.l.path;
      leaf.truthy = true;
      return true;
    }
    if (e.op == PqExpr::IN)
    {
      if (e.l.kind != PqOperand::PATH || e.l.path.fromKey || e.l.path.hasFilter || sets[e.setSlot].hasNull)
        return false;
      leaf.path = &e.l.path;
      leaf.set = &sets[e.setSlot];
      return true;
    }
    if (e.op != PqExpr::CMP)
      return false;
    bool lPath = e.l.kind == PqOperand::PATH, rPath = e.r.kind == PqOperand::PATH;
    if (lPath == rPath)
      return false;
    const PqOperand &po = lPath ? e.l : e.r;
    if (po.path.fromKey || po.path.hasFilter)
      return false;
    PqCmp op = e.cmp;
    if (!lPath)
      op = op == PQ_LT ? PQ_GT : op == PQ_GT ? PQ_LT : op == PQ_LE ? PQ_GE : op == PQ_GE ? PQ_LE : op;
    leaf.value = operandValue(lPath ? e.r : e.l);
    if (leaf.value.type == PT_NULL || leaf.value.type == PT_TABLE || leaf.value.type == PT_ARRAY)
      return false;
    if (op == PQ_NE)
    {
      if (po.path.hasAny)
        return false;
      op = PQ_EQ;
      leaf.negate = true;
    }
    leaf.path = &po.path;
    leaf.op = op;
    return true;
  }

  bool isColumnar(const PqExpr &e) const
  {
    if (e.op == PqExpr::AND || e.op == PqExpr::OR)
      return isColumnar(*e.a) && isColumnar(*e.b);
    if (e.op == PqExpr::NOT)
      return isColumnar(*e.a);
    PqLeaf leaf;
    return leafFor(e, leaf);
  }

  static uint32_t rankBefore(const PkColumn *c, uint32_t row)
  {
    const uint8_t *ranks = PkTree::payload(c) + 4;
    uint32_t blocks = (c->rows + 63) >> 6;
    const uint8_t *bits = ranks + size_t(blocks) * 4;
    if (row >= c->rows)
    {
      if (!blocks)
        return 0;
      uint64_t w;
      memcpy(&w, bits + size_t(blocks - 1) * 8, 8);
      return pk_u32(ranks + size_t(blocks - 1) * 4) + pk_popcount(w);
    }
    uint64_t w;
    memcpy(&w, bits + size_t(row >> 6) * 8, 8);
    return pk_u32(ranks + size_t(row >> 6) * 4) + pk_popcount(w & ((uint64_t(1) << (row & 63)) - 1));
  }

  static void setMatch(const PkColumn *c, uint32_t lo, uint32_t hi, const PqSet &set, PqBits &out)
  {
    if (set.keys.empty())
      return;
    const uint8_t *p = PkTree::payload(c);
    auto has = [&set](PkType type, int64_t v) { return set.keys.count(PqKey{uint8_t(type), v}) != 0; };
    switch (c->kind)
    {
      case PK_BOOL_BITS:
      {
        bool withFalse = has(PT_BOOL, 0), withTrue = has(PT_BOOL, 1);
        for (uint32_t r = lo; r < hi; r++)
          if (((p[r >> 3] >> (r & 7)) & 1) ? withTrue : withFalse)
            out.set(r - lo);
        return;
      }
      case PK_INT:
      {
        int64_t base;
        memcpy(&base, p, 8);
        if (set.inexactNumbers.empty())
          pq_scan(p + 8, c->width, lo, hi, [&](uint64_t x) { return has(PT_INT, int64_t(uint64_t(base) + x)); }, out);
        else
          pq_scan(
            p + 8, c->width, lo, hi,
            [&](uint64_t x) {
              PqVal v;
              v.type = PT_INT;
              v.i = int64_t(uint64_t(base) + x);
              return pq_in_set(set, v);
            },
            out);
        return;
      }
      case PK_FLOAT:
        for (uint32_t r = lo; r < hi; r++)
        {
          SQFloat x;
          memcpy(&x, p + size_t(r) * sizeof(SQFloat), sizeof(SQFloat));
          PqVal v;
          v.type = PT_FLOAT;
          v.f = x;
          if (pq_in_set(set, v))
            out.set(r - lo);
        }
        return;
      case PK_STRING: pq_scan(p, c->width, lo, hi, [&](uint64_t x) { return has(PT_STRING, int64_t(x) * 2); }, out); return;
      case PK_STRING_DICT:
      {
        uint32_t dictCount = pk_u32(p);
        const uint8_t *indices = p + pk_align4(4 + dictCount * c->extra);
        eastl::vector<uint8_t> inDict(dictCount);
        for (uint32_t d = 0; d < dictCount; d++)
          inDict[d] = has(PT_STRING, int64_t(pk_read(p + 4, c->extra, d)) * 2) ? 1 : 0;
        pq_scan(indices, c->width, lo, hi, [&](uint64_t x) { return inDict[size_t(x)] != 0; }, out);
        return;
      }
      default: return;
    }
  }

  void scalarMatch(const PkColumn *c, uint32_t ref, uint32_t lo, uint32_t hi, const PqLeaf &leaf, PqBits &out) const
  {
    const uint8_t *p = PkTree::payload(c);
    if (c->kind == PK_CONST)
    {
      PkValue x;
      tree.element(ref, lo, x);
      PqVal v = pq_from(x);
      bool match = leaf.truthy ? pq_truthy(v) : leaf.set ? pq_in_set(*leaf.set, v) : pq_test(leaf.op, v, leaf.value);
      if (match && (!leaf.sameType || v.type == leaf.value.type))
        out.setAll();
      return;
    }
    if (leaf.set)
    {
      setMatch(c, lo, hi, *leaf.set, out);
      return;
    }
    if (leaf.truthy)
    {
      switch (c->kind)
      {
        case PK_BOOL_BITS:
          for (uint32_t r = lo; r < hi; r++)
            if ((p[r >> 3] >> (r & 7)) & 1)
              out.set(r - lo);
          return;
        case PK_INT:
        {
          int64_t base;
          memcpy(&base, p, 8);
          pq_scan(p + 8, c->width, lo, hi, [&](uint64_t x) { return int64_t(uint64_t(base) + x) != 0; }, out);
          return;
        }
        case PK_FLOAT:
          for (uint32_t r = lo; r < hi; r++)
          {
            SQFloat x;
            memcpy(&x, p + size_t(r) * sizeof(SQFloat), sizeof(SQFloat));
            if (pk_is_nan(x) || x != 0)
              out.set(r - lo);
          }
          return;
        default: out.setAll(); return;
      }
    }
    const PqVal &t = leaf.value;
    bool numeric = t.type == PT_INT || t.type == PT_FLOAT;
    if (leaf.sameType && ((c->kind == PK_INT && t.type != PT_INT) || (c->kind == PK_FLOAT && t.type != PT_FLOAT)))
      return;
    switch (c->kind)
    {
      case PK_BOOL_BITS:
        if (t.type == PT_BOOL)
          for (uint32_t r = lo; r < hi; r++)
            if (pq_cmp_num<int64_t>(leaf.op, (p[r >> 3] >> (r & 7)) & 1, t.i))
              out.set(r - lo);
        return;
      case PK_INT:
      {
        if (!numeric)
          return;
        int64_t base;
        memcpy(&base, p, 8);
        if (t.type == PT_INT)
          pq_scan(
            p + 8, c->width, lo, hi, [&](uint64_t x) { return pq_cmp_num<int64_t>(leaf.op, int64_t(uint64_t(base) + x), t.i); }, out);
        else
          pq_scan(
            p + 8, c->width, lo, hi,
            [&](uint64_t x) { return pq_test_float(leaf.op, SQFloat(int64_t(uint64_t(base) + x)), SQFloat(t.f)); }, out);
        return;
      }
      case PK_FLOAT:
      {
        if (!numeric)
          return;
        SQFloat target = t.type == PT_INT ? SQFloat(t.i) : SQFloat(t.f);
        for (uint32_t r = lo; r < hi; r++)
        {
          SQFloat x;
          memcpy(&x, p + size_t(r) * sizeof(SQFloat), sizeof(SQFloat));
          if (pq_test_float(leaf.op, x, target))
            out.set(r - lo);
        }
        return;
      }
      case PK_STRING:
        if (t.type == PT_STRING)
          pq_scan(p, c->width, lo, hi, [&](uint64_t x) { return pq_cmp_num<int64_t>(leaf.op, int64_t(x) * 2, t.i); }, out);
        return;
      case PK_STRING_DICT:
      {
        if (t.type != PT_STRING)
          return;
        uint32_t dictCount = pk_u32(p);
        const uint8_t *indices = p + pk_align4(4 + dictCount * c->extra);
        uint32_t a = 0, b = dictCount;
        while (a < b)
        {
          uint32_t mid = (a + b) >> 1;
          if (int64_t(pk_read(p + 4, c->extra, mid)) * 2 < t.i)
            a = mid + 1;
          else
            b = mid;
        }
        bool exact = a < dictCount && int64_t(pk_read(p + 4, c->extra, a)) * 2 == t.i;
        int64_t at = int64_t(a);
        switch (leaf.op)
        {
          case PQ_EQ:
            if (exact)
              pq_scan(indices, c->width, lo, hi, [&](uint64_t x) { return int64_t(x) == at; }, out);
            return;
          case PQ_LT: pq_scan(indices, c->width, lo, hi, [&](uint64_t x) { return int64_t(x) < at; }, out); return;
          case PQ_GE: pq_scan(indices, c->width, lo, hi, [&](uint64_t x) { return int64_t(x) >= at; }, out); return;
          case PQ_LE: pq_scan(indices, c->width, lo, hi, [&](uint64_t x) { return int64_t(x) < at + (exact ? 1 : 0); }, out); return;
          default: pq_scan(indices, c->width, lo, hi, [&](uint64_t x) { return int64_t(x) >= at + (exact ? 1 : 0); }, out); return;
        }
      }
      default: return;
    }
  }

  void colMatch(uint32_t ref, uint32_t lo, uint32_t hi, size_t step, const PqLeaf &leaf, PqBits &out) const
  {
    if (lo >= hi)
      return;
    const PkColumn *c = tree.column(ref);
    const uint8_t *p = PkTree::payload(c);
    const PqPath &path = *leaf.path;
    bool atEnd = step == path.steps.size();
    switch (c->kind)
    {
      case PK_ROW_KEY: colMatch(pk_u32(p), lo, hi, step, leaf, out); return;
      case PK_NULLABLE:
      case PK_OPTIONAL:
      {
        uint32_t clo = rankBefore(c, lo), chi = rankBefore(c, hi);
        PqBits sub(chi - clo);
        colMatch(pk_u32(p), clo, chi, step, leaf, sub);
        if (!sub.any())
          return;
        for (uint32_t r = lo; r < hi; r++)
        {
          uint32_t rank;
          if (PkTree::presence(c, r, rank) && sub.get(rank - clo))
            out.set(r - lo);
        }
        return;
      }
      case PK_UNION:
      {
        const uint8_t *tags = p + PT_COUNT * 4;
        const uint8_t *positions = tags + pk_align4((c->rows + 1) / 2);
        auto tagAt = [tags](uint32_t r) { return (tags[r >> 1] >> ((r & 1) * 4)) & 15; };
        uint32_t firstChildRow[PT_COUNT] = {}, rowsOfType[PT_COUNT] = {};
        for (uint32_t r = lo; r < hi; r++)
        {
          uint32_t t = tagAt(r);
          if (rowsOfType[t]++ == 0)
            firstChildRow[t] = uint32_t(pk_read(positions, c->width, r));
        }
        for (uint32_t t = 0; t < PT_COUNT; t++)
        {
          uint32_t child = pk_u32(p + t * 4);
          if (!child || !rowsOfType[t])
            continue;
          PqBits sub(rowsOfType[t]);
          colMatch(child, firstChildRow[t], firstChildRow[t] + rowsOfType[t], step, leaf, sub);
          if (!sub.any())
            continue;
          uint32_t k = 0;
          for (uint32_t r = lo; r < hi; r++)
            if (tagAt(r) == t && sub.get(k++))
              out.set(r - lo);
        }
        return;
      }
      case PK_STRUCT:
      {
        if (atEnd && leaf.truthy)
          out.setAll();
        if (atEnd || path.steps[step].kind == PqStep::INDEX)
          return;
        uint32_t fieldCount = c->extra;
        uint32_t rest = PkTree::restOf(c);
        const PqStep &st = path.steps[step];
        if (st.kind == PqStep::FIELD)
        {
          int32_t id = query.fieldIds[st.slot];
          uint32_t f;
          if (id < 0)
            return;
          if (tree.findField(c, uint32_t(id), f))
            colMatch(pk_u32(p + (size_t(fieldCount) + f) * 4), lo, hi, step + 1, leaf, out);
          else if (rest)
            colMatch(rest, lo, hi, step, leaf, out);
          return;
        }
        for (uint32_t f = 0; f < fieldCount; f++)
        {
          PqBits sub(hi - lo);
          colMatch(pk_u32(p + (size_t(fieldCount) + f) * 4), lo, hi, step + 1, leaf, sub);
          out.orWith(sub);
        }
        if (rest)
        {
          PqBits sub(hi - lo);
          colMatch(rest, lo, hi, step, leaf, sub);
          out.orWith(sub);
        }
        return;
      }
      case PK_LIST:
      case PK_MAP:
      {
        if (atEnd)
        {
          if (leaf.truthy)
            out.setAll();
          return;
        }
        const PqStep &st = path.steps[step];
        if (c->kind == PK_LIST && st.kind == PqStep::FIELD)
          return;
        const uint8_t *offsets = p + (c->kind == PK_LIST ? 4 : 8);
        uint32_t child = pk_u32(p + (c->kind == PK_LIST ? 0 : 4));
        uint32_t elo = uint32_t(pk_read(offsets, c->width, lo)), ehi = uint32_t(pk_read(offsets, c->width, hi));
        PqBits sub(ehi - elo);
        if (st.kind != PqStep::ANY)
        {
          PqBits selected(ehi - elo);
          if (c->kind == PK_LIST)
          {
            if (st.index < 0)
              return;
            for (uint32_t row = lo; row < hi; row++)
            {
              uint64_t j = pk_read(offsets, c->width, row) + uint64_t(st.index);
              if (j < pk_read(offsets, c->width, row + 1))
                selected.set(uint32_t(j) - elo);
            }
          }
          else
          {
            PqLeaf keyLeaf;
            keyLeaf.path = &pq_empty_path;
            keyLeaf.sameType = true;
            keyLeaf.value.type = st.kind == PqStep::FIELD ? PT_STRING : PT_INT;
            keyLeaf.value.i = st.kind == PqStep::FIELD ? int64_t(query.fieldIds[st.slot]) * 2 : st.index;
            if (st.kind == PqStep::FIELD && query.fieldIds[st.slot] < 0)
              return;
            colMatch(pk_u32(p), elo, ehi, 0, keyLeaf, selected);
          }
          if (!selected.any())
            return;
          colMatch(child, elo, ehi, step + 1, leaf, sub);
          sub.andWith(selected);
        }
        else
          colMatch(child, elo, ehi, step + 1, leaf, sub);
        uint32_t r = lo;
        sub.forEach([&](uint32_t j) {
          while (uint32_t(pk_read(offsets, c->width, r + 1)) <= elo + j)
            r++;
          out.set(r - lo);
          return false;
        });
        return;
      }
      case PK_NATIVE:
      {
        if (atEnd)
        {
          if (leaf.truthy)
            out.setAll();
          return;
        }
        uint32_t component;
        if (nativeStep(c->extra, path.steps[step], component))
          colMatch(pk_u32(p + size_t(component) * 4), lo, hi, step + 1, leaf, out);
        return;
      }
      default:
        if (atEnd)
          scalarMatch(c, ref, lo, hi, leaf, out);
        return;
    }
  }

  struct Scope
  {
    const PkColumn *c;
    uint32_t collRow;
    uint32_t begin, end;
    uint32_t valuesRef;
  };

  PqVal whereKey(const PkColumn *c, uint32_t coll_row, uint32_t pos) const
  {
    PkValue k;
    return query.whereReadsKey && tree.keyAt(c, coll_row, pos, k) ? pq_from(k) : PqVal();
  }

  PqVal pathStart(const PqPath &path, const PkColumn *c, uint32_t coll_row, uint32_t pos) const
  {
    PkValue x;
    bool found = path.fromKey ? tree.keyAt(c, coll_row, pos, x) : tree.valueAt(c, coll_row, pos, x);
    return found ? pq_from(x) : PqVal();
  }

  void rowsLeaf(const PqExpr &e, const Scope &s, const PqBits &cand, PqBits &out) const
  {
    cand.forEach([&](uint32_t i) {
      PkValue x;
      if (tree.valueAt(s.c, s.collRow, s.begin + i, x) && eval(e, pq_from(x), whereKey(s.c, s.collRow, s.begin + i)))
        out.set(i);
      return false;
    });
  }

  void evalSet(const PqExpr &e, const Scope &s, const PqBits &cand, PqBits &out) const
  {
    out.reset(cand.size, false);
    switch (e.op)
    {
      case PqExpr::AND:
      {
        const PqExpr *first = e.a.get(), *second = e.b.get();
        if (!isColumnar(*first) && isColumnar(*second))
          eastl::swap(first, second);
        PqBits t;
        evalSet(*first, s, cand, t);
        if (t.any())
          evalSet(*second, s, t, out);
        return;
      }
      case PqExpr::OR:
      {
        PqBits t, rest(cand), u;
        evalSet(*e.a, s, cand, t);
        rest.andNot(t);
        evalSet(*e.b, s, rest, u);
        out = t;
        out.orWith(u);
        return;
      }
      case PqExpr::NOT:
      {
        PqBits t;
        evalSet(*e.a, s, cand, t);
        out = cand;
        out.andNot(t);
        return;
      }
      default:
      {
        PqLeaf leaf;
        if (leafFor(e, leaf))
        {
          PqBits all(cand.size);
          colMatch(s.valuesRef, s.begin, s.end, 0, leaf, all);
          if (leaf.negate)
            all.invert();
          out = all;
          out.andWith(cand);
          return;
        }
        if (e.necessaryConditions)
        {
          PqBits narrowed;
          evalSet(*e.necessaryConditions, s, cand, narrowed);
          rowsLeaf(e, s, narrowed, out);
          return;
        }
        rowsLeaf(e, s, cand, out);
      }
    }
  }

  template <typename F>
  void run(uint32_t coll_ref, uint32_t coll_row, PqPlan plan, PkTree &mutable_tree, F emit) const
  {
    const PkColumn *c = tree.column(coll_ref);
    if (c->kind != PK_STRUCT && c->kind != PK_MAP && c->kind != PK_LIST)
      return;
    const PqExpr *indexed = plan == PLAN_AUTO ? findIndexable(query.where.get()) : nullptr;
    if (indexed)
    {
      bool pathLeft = indexed->l.kind == PqOperand::PATH;
      const PqIndex &idx = index(coll_ref, coll_row, pathLeft ? indexed->l.path : indexed->r.path, mutable_tree);
      bool onlyCondition = indexed == query.where.get();
      auto visit = [&](uint32_t pos) {
        PkValue x;
        if (!tree.valueAt(c, coll_row, pos, x))
          return false;
        if (!onlyCondition && !eval(*query.where, pq_from(x), whereKey(c, coll_row, pos)))
          return false;
        return emit(c, pos);
      };
      uint32_t ib, ie;
      if (indexed->op == PqExpr::IN)
      {
        eastl::vector<uint32_t> positions;
        for (const PqKey &key : sets[indexed->setSlot].keys)
          if (idx.find(key, ib, ie))
            for (uint32_t i = ib; i < ie; i++)
              positions.push_back(idx.position(i));
        eastl::sort(positions.begin(), positions.end());
        positions.erase(eastl::unique(positions.begin(), positions.end()), positions.end());
        for (uint32_t pos : positions)
          if (visit(pos))
            return;
        return;
      }
      PqKey key;
      if (!pq_key(operandValue(pathLeft ? indexed->r : indexed->l), key) || !idx.find(key, ib, ie))
        return;
      for (uint32_t i = ib; i < ie; i++)
        if (visit(idx.position(i)))
          return;
      return;
    }
    uint32_t b, e;
    tree.range(c, coll_row, b, e);
    uint32_t valuesRef = c->kind == PK_STRUCT ? 0 : pk_u32(PkTree::payload(c) + (c->kind == PK_LIST ? 0 : 4));
    if (plan == PLAN_AUTO && valuesRef && tree.column(valuesRef)->kind == PK_MAP)
      plan = PLAN_ROWS;
    if (plan != PLAN_ROWS && c->kind != PK_STRUCT && query.where)
    {
      Scope s = {c, coll_row, b, e, valuesRef};
      PqBits cand(e - b, true), res;
      evalSet(*query.where, s, cand, res);
      res.forEach([&](uint32_t i) { return emit(c, b + i); });
      return;
    }
    for (uint32_t pos = b; pos < e; pos++)
    {
      PkValue x;
      if (!tree.valueAt(c, coll_row, pos, x))
        continue;
      if (query.where && !eval(*query.where, pq_from(x), whereKey(c, coll_row, pos)))
        continue;
      if (emit(c, pos))
        return;
    }
  }

  template <typename F>
  void records(uint32_t coll_ref, uint32_t coll_row, PqPlan plan, PkTree &mutable_tree, bool ordered, size_t needed, F f) const
  {
    size_t keep = query.limit < 0 ? needed : eastl::min(size_t(query.limit), needed);
    if (keep == 0)
      return;
    if (ordered && query.hasOrder)
    {
      const PkColumn *c = tree.column(coll_ref);
      eastl::vector<uint32_t> positions;
      run(coll_ref, coll_row, plan, mutable_tree, [&](const PkColumn *, uint32_t pos) {
        positions.push_back(pos);
        return false;
      });
      sortByOrderPath(c, coll_row, positions, keep);
      for (uint32_t pos : positions)
        if (f(c, pos))
          return;
      return;
    }
    size_t n = 0;
    run(coll_ref, coll_row, plan, mutable_tree, [&](const PkColumn *c, uint32_t pos) { return f(c, pos) || ++n >= keep; });
  }

  void sortByOrderPath(const PkColumn *c, uint32_t coll_row, eastl::vector<uint32_t> &positions, size_t keep) const
  {
    struct Item
    {
      uint32_t pos;
      uint32_t order;
      PqVal key;
      bool has() const { return key.type != PT_NULL; }
    };
    const PqPath &path = query.orderPath;
    bool desc = query.orderDesc;
    auto cmp = [desc](const PqVal &a, const PqVal &b) { return desc ? -pq_sort_order(a, b) : pq_sort_order(a, b); };
    eastl::vector<Item> items;
    items.reserve(positions.size());
    for (uint32_t pos : positions)
    {
      Item it = {pos, uint32_t(items.size()), PqVal()};
      auto take = [&](const PqVal &v) {
        if (v.type != PT_NULL && (!it.has() || cmp(v, it.key) < 0))
          it.key = v;
        return false;
      };
      walk(path, 0, pathStart(path, c, coll_row, pos), take);
      items.push_back(it);
    }
    auto before = [&](const Item &a, const Item &b) {
      if (a.has() != b.has())
        return a.has();
      int o = a.has() ? cmp(a.key, b.key) : 0;
      return o != 0 ? o < 0 : a.order < b.order;
    };
    if (keep == 1)
    {
      size_t best = 0;
      for (size_t i = 1; i < items.size(); i++)
        if (before(items[i], items[best]))
          best = i;
      if (!items.empty())
        positions.assign(1, items[best].pos);
      return;
    }
    if (keep < items.size())
    {
      eastl::partial_sort(items.begin(), items.begin() + keep, items.end(), before);
      items.resize(keep);
    }
    else
      eastl::sort(items.begin(), items.end(), before);
    positions.resize(items.size());
    for (size_t i = 0; i < items.size(); i++)
      positions[i] = items[i].pos;
  }

private:
  PqQuery &query;
  const PkTree &tree;
};

SQInteger pq_query_release(HSQUIRRELVM, SQUserPointer p, SQInteger)
{
  delete *(PqQuery **)p;
  return 1;
}

PqQuery *pq_get_query(HSQUIRRELVM v, SQInteger idx)
{
  const SQObjectPtr &o = stack_get(v, idx);
  if (sq_type(o) != OT_INSTANCE || _instance(o)->_hook != pq_query_release)
    return nullptr;
  return *(PqQuery **)_instance(o)->_userpointer;
}

SQInteger pq_compile(HSQUIRRELVM v)
{
  const char *text = nullptr;
  sq_getstring(v, 2, &text);
  eastl::unique_ptr<PqQuery> q(new PqQuery);
  PqParser parser;
  if (!parser.parse(text, *q))
    return sq_throwerror(v, parser.error);
  SQInstance *inst = _class(stack_get(v, 3))->CreateInstance(v);
  if (!inst)
    return SQ_ERROR;
  *(PqQuery **)inst->_userpointer = q.release();
  inst->_hook = pq_query_release;
  v->Push(SQObjectPtr(inst));
  return 1;
}

SQInteger pq_query_constructor(HSQUIRRELVM v) { return sq_throwerror(v, "queries are made by packed.query()"); }

enum PqMode
{
  PQ_FIRST,
  PQ_FIRST_KEY,
  PQ_ALL,
  PQ_KEYS,
  PQ_COUNT,
  PQ_ANY,
  PQ_VALUE,
};

void pq_push_aggregate(HSQUIRRELVM v, const PqQuery &q, const PqRunner &runner, PkTree &tree, const PqVal &coll, PqPlan plan,
  SQClass *view_class)
{
  bool isCollection = coll.type == PT_TABLE || coll.type == PT_ARRAY;
  bool countRecords = q.agg == AGG_COUNT && !q.aggHasPath;
  eastl::vector<PqVal> values;
  auto collect = [&](const PkColumn *c, uint32_t pos) {
    values.clear();
    if (countRecords)
      return;
    auto take = [&](const PqVal &val) {
      values.push_back(val);
      return false;
    };
    runner.walk(q.selectPath, 0, runner.pathStart(q.selectPath, c, coll.row, pos), take);
  };
  auto addTo = [&](PqAcc &acc) {
    if (countRecords)
      acc.count++;
    for (const PqVal &val : values)
      acc.add(q.agg, val);
  };
  if (!q.hasGroup)
  {
    PqAcc acc;
    if (isCollection)
      runner.records(coll.ref, coll.row, plan, tree, true, SIZE_MAX, [&](const PkColumn *c, uint32_t pos) {
        collect(c, pos);
        addTo(acc);
        return false;
      });
    acc.push(v, &tree, q.agg, view_class);
    return;
  }
  eastl::vector<eastl::pair<PqKey, PqAcc>> groups;
  eastl::unordered_map<PqKey, uint32_t, PqKeyHash> groupIndex;
  eastl::vector<uint32_t> recordGroups;
  if (isCollection)
    runner.records(coll.ref, coll.row, plan, tree, true, SIZE_MAX, [&](const PkColumn *c, uint32_t pos) {
      recordGroups.clear();
      auto group = [&](const PqVal &val) {
        PqKey key;
        if (val.type != PT_NULL && pq_key(val, key))
        {
          auto it = groupIndex.insert(eastl::make_pair(key, uint32_t(groups.size())));
          if (it.second)
            groups.push_back(eastl::make_pair(key, PqAcc()));
          if (eastl::find(recordGroups.begin(), recordGroups.end(), it.first->second) == recordGroups.end())
            recordGroups.push_back(it.first->second);
        }
        return false;
      };
      runner.walk(q.groupPath, 0, runner.pathStart(q.groupPath, c, coll.row, pos), group);
      if (recordGroups.empty())
        return false;
      collect(c, pos);
      for (uint32_t g : recordGroups)
        addTo(groups[g].second);
      return false;
    });
  sq_newtableex(v, SQInteger(groups.size()));
  for (const auto &g : groups)
  {
    pq_push_key(v, &tree, g.first);
    g.second.push(v, &tree, q.agg, view_class);
    sq_newslot(v, -3, SQFalse);
  }
}

template <PqMode mode>
SQInteger pq_execute(HSQUIRRELVM v)
{
  PqQuery *q = pq_get_query(v, 1);
  PkView *view = pk_get_view(v, 2);
  if (!q || !view)
    return sq_throwerror(v, "expected query.method(packed_view, [params, plan])");
  if (q->agg != AGG_NONE && mode != PQ_VALUE && mode != PQ_ANY)
    return sq_throwerror(v, "the query has an aggregate in select, use value()");
  if (q->agg == AGG_NONE && mode == PQ_VALUE)
    return sq_throwerror(v, "value() needs an aggregate in select");
  if (q->hasSelect && (mode == PQ_FIRST_KEY || mode == PQ_KEYS))
    return sq_throwerror(v, "keys() and firstKey() take no select, use select @key");
  PkTree &tree = *view->tree;
  PqRunner runner(*q, tree);
  SQInteger top = sq_gettop(v);
  runner.params.resize(q->params.size());
  eastl::vector<SQObjectPtr> &paramObjects = runner.paramObjects;
  paramObjects.resize(q->params.size());
  if (!q->params.empty())
  {
    if (top < 3 || sq_gettype(v, 3) != OT_TABLE)
      return sq_throwerror(v, "the query has parameters, a table of them is expected");
    for (size_t i = 0; i < q->params.size(); i++)
    {
      sq_pushstring(v, q->params[i].c_str(), SQInteger(q->params[i].size()));
      if (SQ_FAILED(sq_rawget(v, 3)))
        return sq_throwerror(v, "a query parameter is missing");
      paramObjects[i] = stack_get(v, -1);
      runner.params[i] = runner.paramFromObject(paramObjects[i]);
      sq_pop(v, 1);
    }
  }
  if (!runner.makeSets())
    return sq_throwerror(v, "the parameter after 'in' must be an array");
  PqPlan plan = PLAN_AUTO;
  if (top >= 4)
  {
    const char *name = nullptr;
    sq_getstring(v, 4, &name);
    if (strcmp(name, "columns") == 0)
      plan = PLAN_COLUMNS;
    else if (strcmp(name, "rows") == 0)
      plan = PLAN_ROWS;
    else if (strcmp(name, "auto") != 0)
      return sq_throwerror(v, "plan must be 'auto', 'columns' or 'rows'");
  }

  PqVal coll;
  coll.type = PT_TABLE;
  coll.ref = view->ref;
  coll.row = view->row;
  for (const PqStep &st : q->source.steps)
    if (!runner.step(st, coll, coll))
    {
      coll.type = PT_NULL;
      break;
    }

  bool isCollection = coll.type == PT_TABLE || coll.type == PT_ARRAY;
  SQClass *viewClass = _instance(stack_get(v, 2))->_class;
  if (mode == PQ_ANY)
  {
    bool found = false;
    if (isCollection && q->limit != 0)
      runner.run(coll.ref, coll.row, plan, tree, [&](const PkColumn *, uint32_t) { return found = true; });
    sq_pushbool(v, found);
    return 1;
  }
  if (mode == PQ_VALUE)
  {
    pq_push_aggregate(v, *q, runner, tree, coll, plan, viewClass);
    return 1;
  }
  if (mode == PQ_ALL || mode == PQ_KEYS)
    sq_newarray(v, 0);
  SQInteger count = 0;
  bool pushed = false;
  bool failed = false;
  auto put = [&](const PkValue &x) {
    count++;
    if (mode == PQ_COUNT)
      return false;
    if (!pk_push_value(v, &tree, x, viewClass))
    {
      failed = true;
      return true;
    }
    if (mode == PQ_FIRST || mode == PQ_FIRST_KEY)
    {
      pushed = true;
      return true;
    }
    sq_arrayappend(v, -2);
    return false;
  };
  PqKeySet seen;
  auto take = [&](const PqVal &val) { return q->distinct && !seen.insert(pq_identity(val)).second ? false : put(pq_to_pk(val)); };
  auto emit = [&](const PkColumn *c, uint32_t pos) {
    PkValue x;
    if (mode == PQ_COUNT && !q->hasSelect)
    {
      count++;
      return false;
    }
    if (mode == PQ_FIRST_KEY || mode == PQ_KEYS)
    {
      tree.keyAt(c, coll.row, pos, x);
      return put(x);
    }
    if (q->hasSelect)
      return runner.walk(q->selectPath, 0, runner.pathStart(q->selectPath, c, coll.row, pos), take);
    tree.valueAt(c, coll.row, pos, x);
    return put(x);
  };
  bool oneRecord = (mode == PQ_FIRST || mode == PQ_FIRST_KEY) && !q->selectPath.hasAny;
  if (isCollection)
    runner.records(coll.ref, coll.row, plan, tree, mode != PQ_COUNT || q->hasSelect, oneRecord ? 1 : SIZE_MAX, emit);
  if (failed)
    return SQ_ERROR;
  if (mode == PQ_COUNT)
    sq_pushinteger(v, count);
  else if ((mode == PQ_FIRST || mode == PQ_FIRST_KEY) && !pushed)
    sq_pushnull(v);
  return 1;
}

const SQRegFunctionFromStr pq_query_methods[] = {
  {pq_query_constructor, "constructor()", SQ_DOC("Queries are made by packed.query(), not by the class")},
  {pq_query_constructor, "instance._cloned(other: instance)", SQ_DOC("Queries are made by packed.query(), not by clone")},
  {pq_execute<PQ_FIRST>, "instance.first(view: instance, [params: table|null, plan: string]): any",
    SQ_DOC("Returns the first matching value, or null")},
  {pq_execute<PQ_FIRST_KEY>, "instance.firstKey(view: instance, [params: table|null, plan: string]): any",
    SQ_DOC("Returns the key of the first matching value, or null")},
  {pq_execute<PQ_ALL>, "instance.all(view: instance, [params: table|null, plan: string]): array",
    SQ_DOC("Returns all matching values")},
  {pq_execute<PQ_KEYS>, "instance.keys(view: instance, [params: table|null, plan: string]): array",
    SQ_DOC("Returns the keys of all matching values")},
  {pq_execute<PQ_COUNT>, "instance.count(view: instance, [params: table|null, plan: string]): int",
    SQ_DOC("Returns the number of matching values")},
  {pq_execute<PQ_ANY>, "instance.any(view: instance, [params: table|null, plan: string]): bool",
    SQ_DOC("Returns true when a record matches the condition, ignores order by and select")},
  {pq_execute<PQ_VALUE>, "instance.value(view: instance, [params: table|null, plan: string]): any",
    SQ_DOC("Returns the aggregate of select, or a table of them by the group by values")},
  {NULL, NULL, NULL}};

const SQRegFunctionFromStr pk_view_methods[] = {
  {pk_view_constructor, "constructor()", SQ_DOC("Packed views are made by packed.pack(), not by the class")},
  {pk_view_nexti, "instance._nexti(prev): any", SQ_DOC("Iterator support: returns the next key or null")},
  {pk_view_cloned, "instance._cloned(other: instance)", SQ_DOC("Makes the clone refer to the same packed data")}, {NULL, NULL, NULL}};

const int pk_view_class_registry_key = 0;

SQUserPointer pk_view_class_key() { return (SQUserPointer)&pk_view_class_registry_key; }

SQClass *pk_registered_view_class(HSQUIRRELVM v)
{
  SQInteger top = sq_gettop(v);
  sq_pushregistrytable(v);
  sq_pushuserpointer(v, pk_view_class_key());
  SQClass *cls = nullptr;
  if (SQ_SUCCEEDED(sq_rawget(v, -2)) && sq_gettype(v, -1) == OT_CLASS)
    cls = _class(stack_get(v, -1));
  sq_settop(v, top);
  return cls;
}

struct PkSavedBuffer
{
  uint32_t ref;
  uint32_t row;
  eastl::vector<uint8_t> buf;
};

dag::mutex pk_saved_mutex;
eastl::unordered_map<SQInteger, PkSavedBuffer> pk_saved_buffers;
SQInteger pk_next_saved_token = 1;

SQInteger pk_save(HSQUIRRELVM v)
{
  PkView *view = pk_get_view(v, 2);
  if (!view)
    return sq_throwerror(v, "save() expects a packed view");
  const PkTree *tree = view->tree;
  if (!tree->classes.empty())
    return sq_throwerror(v, "save() cannot store class instances, their classes live in one VM");
  SQInteger token;
  {
    dag::lock_guard<dag::mutex> lock(pk_saved_mutex);
    token = pk_next_saved_token++;
    pk_saved_buffers[token] = PkSavedBuffer{view->ref, view->row, tree->buf};
  }
  sq_pushinteger(v, token);
  return 1;
}

SQInteger pk_load(HSQUIRRELVM v)
{
  SQInteger token = 0;
  sq_getinteger(v, 2, &token);
  SQClass *viewClass = pk_registered_view_class(v);
  if (!viewClass)
    return sq_throwerror(v, "the packed module is not registered in this VM");
  PkSavedBuffer saved;
  {
    dag::lock_guard<dag::mutex> lock(pk_saved_mutex);
    auto it = pk_saved_buffers.find(token);
    if (it == pk_saved_buffers.end())
      return sq_throwerror(v, "load() expects a token of packed.save() that is not loaded yet");
    saved = eastl::move(it->second);
    pk_saved_buffers.erase(it);
  }
  PkTree *tree = new PkTree;
  tree->buf.swap(saved.buf);
  if (!pk_push_view(v, tree, saved.ref, saved.row, viewClass))
  {
    delete tree;
    return SQ_ERROR;
  }
  return 1;
}

const SQRegFunctionFromStr pk_funcs[] = {
  {pk_save, "save(view: instance): int",
    SQ_DOC("Keeps a copy of the buffer of a packed view in the process and returns its token; load() of the token in this VM "
           "or another one gives the view again")},
  {pk_load, "load(token: int): instance", SQ_DOC("Gives a view of the buffer that save() kept under the token; a token loads once")},
  {pk_unpack, "unpack(view: instance): table|array", SQ_DOC("Makes ordinary tables and arrays from a packed view")},
  {pk_get, "get(view: instance, ...): any", SQ_DOC("Reads a value on a path of keys, or null when a key is missing")},
  {pk_len, "len(view: instance): int", SQ_DOC("Returns the number of keys or elements of a packed view")},
  {pk_is_view, "pure is_view(x): bool", SQ_DOC("Returns true when x is a packed view")},
  {pk_stats, "stats(view: instance): table", SQ_DOC("Returns the sizes of the parts of the packed buffer")}, {NULL, NULL, NULL}};

SQRESULT register_packed_lib(HSQUIRRELVM v)
{
  sq_newclass(v, SQFalse);
  sq_setclassudsize(v, -1, sizeof(PkView));
  for (const SQRegFunctionFromStr *f = pk_view_methods; f->f; f++)
    sq_new_closure_slot_from_decl_string(v, f->f, 0, f->declstring, f->docstring);
  pk_push_view_methods(v, _class(_ss(v)->_table_class), pk_table_reads, pk_table_writes, pk_table_natives);
  pk_push_view_methods(v, _class(_ss(v)->_array_class), pk_list_reads, pk_list_writes, pk_list_natives);
  sq_new_closure_slot_from_decl_string(v, pk_view_get, 2, "instance._get(key): any",
    SQ_DOC("Reads one key of a packed table or array; a missing key that names a table or array method gives that method"));
  sq_pushregistrytable(v);
  sq_pushuserpointer(v, pk_view_class_key());
  sq_push(v, -3);
  sq_rawset(v, -3);
  sq_pop(v, 1);
  sq_new_closure_slot_from_decl_string(v, pk_pack, 1, "pack(data: table|array|instance, [available_classes: table|null]): instance",
    SQ_DOC("Packs a table, an array or a DataBlock into one immutable buffer and returns a view of its root. An instance of "
           "a script class is stored through its __getstate and needs its class in available_classes"));

  sq_newclass(v, SQFalse);
  sq_setclassudsize(v, -1, sizeof(PqQuery *));
  for (const SQRegFunctionFromStr *f = pq_query_methods; f->f; f++)
    sq_new_closure_slot_from_decl_string(v, f->f, 0, f->declstring, f->docstring);
  sq_new_closure_slot_from_decl_string(v, pq_compile, 1, "query(text: string): instance",
    SQ_DOC("Compiles a query; its methods run it on a packed view"));

  for (const SQRegFunctionFromStr *f = pk_funcs; f->f; f++)
    sq_new_closure_slot_from_decl_string(v, f->f, 0, f->declstring, f->docstring);
  return SQ_OK;
}

} // namespace

void bindquirrel::register_packed_module(SqModules *module_mgr)
{
  HSQUIRRELVM v = module_mgr->getVM();
  HSQOBJECT hModule;
  sq_newtable(v);
  sq_getstackobj(v, -1, &hModule);
  register_packed_lib(v);
  module_mgr->addNativeModule("packed", Sqrat::Object(hModule, v));
  sq_pop(v, 1);
}

SQRESULT bindquirrel::push_packed_datablock(HSQUIRRELVM vm, const DataBlock &blk)
{
  SQClass *viewClass = pk_registered_view_class(vm);
  if (!viewClass)
    return sq_throwerror(vm, "the packed module is not registered in this VM");
  return pk_push_datablock_view(vm, blk, viewClass) ? SQ_OK : SQ_ERROR;
}
