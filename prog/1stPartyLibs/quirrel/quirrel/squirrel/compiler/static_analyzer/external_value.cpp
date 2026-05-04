#include "external_value.h"
#include "compiler/arena.h"

#include <new>


namespace SQCompilation
{

ExternalValueTable::~ExternalValueTable() {
  for (auto *v : _values)
    v->~ExternalValue();
}

ExternalValue *ExternalValueTable::make(const SQObject &v, int line, int col, int width) {
  void *mem = _arena->allocate(sizeof(ExternalValue));
  auto *ev = new (mem) ExternalValue(v, line, col, width);
  _values.push_back(ev);
  return ev;
}

}
