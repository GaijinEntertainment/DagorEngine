// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cstdint>


namespace drv3d_dx12
{
class ShaderID
{
  union
  {
    int32_t value;
    struct
    {
      uint32_t group : 3;
      uint32_t index : 29;
    };
  };

public:
  // ShaderID() = default;
  // explicit ShaderID(uint32_t value) : value{value} {}
  explicit operator bool() const { return -1 != value; }
  bool operator!() const { return -1 == value; }

  uint32_t getGroup() const { return group; }
  uint32_t getIndex() const { return index; }

  int32_t exportValue() const { return value; }

  static ShaderID Null()
  {
    ShaderID result;
    result.value = -1;
    return result;
  }

  static ShaderID importValue(int32_t value)
  {
    ShaderID result;
    result.value = value;
    return result;
  }

  static ShaderID make(uint32_t group, uint32_t index)
  {
    ShaderID result;
    result.group = group;
    result.index = index;
    return result;
  }

  friend bool operator<(const ShaderID l, const ShaderID r) { return l.value < r.value; }

  friend bool operator>(const ShaderID l, const ShaderID r) { return l.value > r.value; }

  friend bool operator!=(const ShaderID l, const ShaderID r) { return l.value != r.value; }

  friend bool operator==(const ShaderID l, const ShaderID r) { return l.value == r.value; }
};

// comes from ShaderID::group member having 3 bits -> 0-7
inline constexpr uint32_t max_scripted_shaders_bin_groups = 8;

class ProgramID
{
  union
  {
    int32_t value;
    struct
    {
      uint32_t type : 2;
      uint32_t group : 3;
      uint32_t index : 27;
    };
  };

public:
  bool operator!() const { return -1 == value; }
  explicit operator bool() const { return -1 == value; }

  uint32_t getType() const { return type; }
  uint32_t getIndex() const { return index; }
  uint32_t getGroup() const { return group; }

  int32_t exportValue() const { return value; }

  static constexpr uint32_t type_graphics = 0;
  static constexpr uint32_t type_compute = 1;

  bool isGraphics() const { return type_graphics == type; }
  bool isCompute() const { return type_compute == type; }

  static ProgramID Null()
  {
    ProgramID result;
    result.value = -1;
    return result;
  }

  static ProgramID importValue(int32_t value)
  {
    ProgramID result;
    result.value = value;
    return result;
  }

  static ProgramID asGraphicsProgram(uint32_t group, uint32_t index)
  {
    ProgramID result;
    result.type = type_graphics;
    result.group = group;
    result.index = index;
    return result;
  }

  static ProgramID asComputeProgram(uint32_t group, uint32_t index)
  {
    ProgramID result;
    result.type = type_compute;
    result.group = group;
    result.index = index;
    return result;
  }

  friend bool operator<(const ProgramID l, const ProgramID r) { return l.value < r.value; }

  friend bool operator>(const ProgramID l, const ProgramID r) { return l.value > r.value; }

  friend bool operator!=(const ProgramID l, const ProgramID r) { return l.value != r.value; }

  friend bool operator==(const ProgramID l, const ProgramID r) { return l.value == r.value; }
};

class GraphicsProgramID
{
  union
  {
    int32_t value;
    struct
    {
      uint32_t group : 3;
      uint32_t index : 29;
    };
  };

public:
  bool operator!() const { return -1 == value; }
  explicit operator bool() const { return -1 == value; }

  uint32_t getIndex() const { return index; }
  uint32_t getGroup() const { return group; }

  int32_t exportValue() const { return value; }

  static GraphicsProgramID Null()
  {
    GraphicsProgramID result;
    result.value = -1;
    return result;
  }

  static GraphicsProgramID importValue(int32_t value)
  {
    GraphicsProgramID result;
    result.value = value;
    return result;
  }

  static GraphicsProgramID make(uint32_t group, uint32_t index)
  {
    GraphicsProgramID result;
    result.group = group;
    result.index = index;
    return result;
  }

  friend bool operator<(const GraphicsProgramID l, const GraphicsProgramID r) { return l.value < r.value; }

  friend bool operator>(const GraphicsProgramID l, const GraphicsProgramID r) { return l.value > r.value; }

  friend bool operator!=(const GraphicsProgramID l, const GraphicsProgramID r) { return l.value != r.value; }

  friend bool operator==(const GraphicsProgramID l, const GraphicsProgramID r) { return l.value == r.value; }
};

} // namespace drv3d_dx12
