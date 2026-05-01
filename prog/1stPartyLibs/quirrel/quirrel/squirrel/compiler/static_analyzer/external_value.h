#pragma once

#include <assert.h>
#include <squirrel.h>
#include "sqobject.h"

#include <vector>


class Arena;


namespace SQCompilation
{

// Records an SQObject value coming from outside the script being analyzed:
// host-provided bindings, or modules resolved by the analyzer calling require_optional().
// Holds the value (refcounted) and where the analyzer learned about it.
struct ExternalValue {
  SQObjectPtr value;
  int line = -1;
  int column = -1;
  int width = 0;

  ExternalValue() = default;
  ExternalValue(const SQObject &v, int l, int c, int w)
    : value(v), line(l), column(c), width(w) {}
};


// Owns ExternalValue instances for the lifetime of one analyzer pass.
// Arena-allocates the storage so layout is cheap; tracks pointers in a vector
// so the destructor can run ~ExternalValue() (the arena does not).
class ExternalValueTable {
  std::vector<ExternalValue*> _values;
  Arena *_arena;

public:
  explicit ExternalValueTable(Arena *a) : _arena(a) {}
  ~ExternalValueTable();

  ExternalValueTable(const ExternalValueTable&) = delete;
  ExternalValueTable& operator=(const ExternalValueTable&) = delete;

  ExternalValue *make(const SQObject &v, int line, int col, int width);
};

}
