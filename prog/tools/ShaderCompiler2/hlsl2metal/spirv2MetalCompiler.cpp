#include "spirv2MetalCompiler.h"
#include "HLSL2MetalCommon.h"

#include <debug/dag_assert.h>
#include <sstream>

#include <unordered_map>

uint32_t CompilerMSLlocal::getTypeSize(const spirv_cross::SPIRType &type, uint32_t &align) const
{
  using namespace spirv_cross;

  uint32_t array_size = 1;
  for (auto a : type.array)
    array_size *= a;
  if (type.basetype == SPIRType::Struct)
  {
    uint32_t struct_size = 0;
    for (const auto &memb : type.member_types)
    {
      const SPIRType &mtype = get_type(memb);
      uint32_t local_align = 0;
      const uint32_t msize = getTypeSize(mtype, local_align);
      const uint32_t offset = struct_size % local_align ? align - struct_size % local_align : 0;
      struct_size += offset + msize;
    }
    align = 16;
    return array_size * struct_size;
  }
  else if (type.basetype == SPIRType::Int || type.basetype == SPIRType::UInt || type.basetype == SPIRType::Float)
  {
    const int elements = type.vecsize == 1 ? 1 : (type.vecsize == 2 ? 2 : 4);
    align = elements * 4;
    return 4 * elements * array_size;
  }
  G_ASSERT(0 && "Unsupported type used for LDS data");
  return 0;
}

bool CompilerMSLlocal::validate(std::stringstream &errors)
{
  using namespace spirv_cross;

  std::unordered_map<std::string, uint32_t> structCache;

  uint32_t totalLDSSize = 0, align = 0;
  ir.for_each_typed_id<SPIRVariable>([&](uint32_t vid, const SPIRVariable &var) {
    if (var.storage == spv::StorageClassWorkgroup)
    {
      const std::string &name = get_name(vid);
      const SPIRType &type = get_type(var.basetype);
      const uint32_t struct_size = getTypeSize(type, align);
      structCache[name] = struct_size;
      totalLDSSize += struct_size;
    }
  });

  // Comes from Direct3D
  constexpr uint32_t MaxLDSPerThreadGroup = 32 * 1024; // in bytes
  if (totalLDSSize > MaxLDSPerThreadGroup)
  {
    for (const auto &[name, size] : structCache)
    {
      errors << name << " " << size << std::endl;
    }
  }

  return totalLDSSize <= MaxLDSPerThreadGroup;
}
