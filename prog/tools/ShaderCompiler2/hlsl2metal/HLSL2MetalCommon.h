#include <hash/md5.h>
#include <3d/dag_3dConst_base.h>
#include <util/dag_string.h>
#include <string>
#include <string_view>

#include <spirv2Metal/spirv_common.hpp>

#include "metalShaderType.h"
#include "dataAccumulator.h"
#include "../compileResult.h"

enum MetalImageType : int
{
  Tex2D = 0,
  Tex2DArray = 1,
  Tex2DDepth = 2,
  TexCube = 3,
  Tex3D = 4
};

MetalImageType translateImageType(const spirv_cross::SPIRType::ImageType &imgType);

struct HashMD5
{
  md5_state_t st;
  char hash[33];

  HashMD5();

  void append(const void *ptr, int size);
  void calc();
  const char *get();
};

struct SemanticValue
{
  const char *name;
  int index;
  int vsdr;
};

SemanticValue &getSemanticValue(int index);
int getSemanticValueCount();

void save2Lib(std::string &shader);

String prependMetaDataAndReplaceFuncs(std::string_view source, const char *shader_name, const char *entry,
  uint64_t shader_variant_hash);

void compressData(CompileResult &compile_result, const DataAccumulator &header, const std::string_view metal_src_code);
