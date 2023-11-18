#pragma once


namespace drv3d_dx12
{


class ShaderByteCodeId
{
  union
  {
    uint32_t value;
    struct
    {
      uint32_t group : 3;
      uint32_t index : 29;
    };
  };

public:
  // ShaderByteCodeId() = default;
  // explicit ShaderByteCodeId(uint32_t value) : value{value} {}
  explicit operator bool() const { return -1 != value; }
  bool operator!() const { return -1 == value; }

  uint32_t getGroup() const { return group; }
  uint32_t getIndex() const { return index; }

  int32_t exportValue() const { return value; }

  static ShaderByteCodeId Null()
  {
    ShaderByteCodeId result;
    result.value = -1;
    return result;
  }

  static ShaderByteCodeId make(uint32_t group, uint32_t index)
  {
    ShaderByteCodeId result;
    result.group = group;
    result.index = index;
    return result;
  }

  friend bool operator<(const ShaderByteCodeId l, const ShaderByteCodeId r) { return l.value < r.value; }
  friend bool operator>(const ShaderByteCodeId l, const ShaderByteCodeId r) { return l.value > r.value; }
  friend bool operator!=(const ShaderByteCodeId l, const ShaderByteCodeId r) { return l.value != r.value; }
  friend bool operator==(const ShaderByteCodeId l, const ShaderByteCodeId r) { return l.value == r.value; }
};

} // namespace drv3d_dx12
