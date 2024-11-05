#pragma once
#include <EASTL/vector_map.h>
#include <math/integer/dag_IPoint4.h>
#include <shaders/dag_shaderVar.h>
#include <EASTL/variant.h>

class PriorityManagedShadervar
{
public:
  PriorityManagedShadervar() = default; // Just for vector_map
  PriorityManagedShadervar(int id);
  void clear(int priority);
  template <typename T>
  void update(int priority, T value)
  {
    G_ASSERT(eastl::holds_alternative<T>(defaultValue));
    values[priority] = value;
    setHighestPriority();
  }
  int getVarId();

private:
  int varId;
  int type;
  using ShadervarUnion = eastl::variant<float, int, Point4, IPoint4>;
  ShadervarUnion defaultValue;
  eastl::vector_map<int, ShadervarUnion> values;
  void setHighestPriority();
  ShadervarUnion get();
  void set(ShadervarUnion v);
};

class PriorityManagedShadervarMap
{
public:
  PriorityManagedShadervar &getEntry(int id);

private:
  eastl::vector_map<int, PriorityManagedShadervar> map;
};

namespace PriorityShadervar
{
void set_real(int id, int prio, float value);
void set_int(int id, int prio, int value);
void set_color4(int id, int prio, Point4 value);
void set_int4(int id, int prio, IPoint4 value);
void clear(int id, int prio);
} // namespace PriorityShadervar