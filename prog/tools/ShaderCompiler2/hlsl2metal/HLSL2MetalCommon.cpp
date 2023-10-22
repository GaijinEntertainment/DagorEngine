
#include "HLSL2MetalCommon.h"
#include "../debugSpitfile.h"

#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_memIo.h>

#include <inttypes.h>

SemanticValue semanticTable[] = {{"POSITION", 0, VSDR_POS}, {"POSITION", 1, VSDR_POS2}, {"NORMAL", 0, VSDR_NORM},
  {"NORMAL", 1, VSDR_NORM2}, {"COLOR", 0, VSDR_DIFF}, {"COLOR", 1, VSDR_SPEC}, {"TEXCOORD", 0, VSDR_TEXC0},
  {"TEXCOORD", 1, VSDR_TEXC1}, {"TEXCOORD", 2, VSDR_TEXC2}, {"TEXCOORD", 3, VSDR_TEXC3}, {"TEXCOORD", 4, VSDR_TEXC4},
  {"TEXCOORD", 5, VSDR_TEXC5}, {"TEXCOORD", 6, VSDR_TEXC6}, {"TEXCOORD", 7, VSDR_TEXC7}, {"TEXCOORD", 8, VSDR_TEXC8},
  {"TEXCOORD", 9, VSDR_TEXC9}, {"TEXCOORD", 10, VSDR_TEXC10}, {"TEXCOORD", 11, VSDR_TEXC11}, {"TEXCOORD", 12, VSDR_TEXC12},
  {"TEXCOORD", 13, VSDR_TEXC13}, {"TEXCOORD", 14, VSDR_TEXC14}};

int semanticTableLength = sizeof(semanticTable) / sizeof(SemanticValue);

HashMD5::HashMD5()
{
  md5_init(&st);
  memset(&hash, 0, sizeof(hash));
}

void HashMD5::append(const void *ptr, int size) { md5_append(&st, (const md5_byte_t *)ptr, size); }

void HashMD5::calc()
{
  unsigned char tmp[16];
  md5_finish(&st, (md5_byte_t *)tmp);

  for (int i = 0; i < 16; i++)
  {
    sprintf(&hash[i * 2], "%02x", tmp[i]);
  }
}

const char *HashMD5::get() { return hash; }

SemanticValue &getSemanticValue(int index) { return semanticTable[index]; }

int getSemanticValueCount() { return semanticTableLength; }

void save2Lib(std::string &shader)
{
  if (!debug_output_dir)
  {
    return;
  }

  static bool first_time = true;

  FILE *f = NULL;

  char str[128];
  sprintf(str, "%s/shad_mtl.lib", debug_output_dir);

  if (first_time)
  {
    f = fopen(str, "wb");
    first_time = false;
  }
  else
  {
    f = fopen(str, "ab");
  }

  if (f)
  {
    int len = (int)shader.length();

    fwrite(&len, 4, 1, f);
    fwrite((void *)shader.c_str(), len, 1, f);

    fclose(f);
  }
}

MetalImageType translateImageType(const spirv_cross::SPIRType::ImageType &imgType)
{
  switch (imgType.dim)
  {
    case spv::Dim2D:
    {
      if (imgType.depth)
      {
        return Tex2DDepth;
      }
      else if (imgType.arrayed)
      {
        return Tex2DArray;
      }
      else
      {
        return Tex2D;
      }
    }
    case spv::Dim3D: return Tex3D;
    case spv::DimCube: return TexCube;
    case spv::DimBuffer: return Tex2D;
    default: return Tex2D;
  }
}

String prependMetaDataAndReplaceFuncs(std::string_view source, const char *shader_name, const char *entry,
  uint64_t shader_variant_hash)
{
  String sourceWithData = String(512, "// %s - %s\n", entry, shader_name);
  if (debug_output_dir)
  {
    sourceWithData += String(-1, "// hash - " PRIx64 "\n\n\n", shader_variant_hash);
  }

  sourceWithData.append(source.data(), source.size());

  sourceWithData.replaceAll("inversesqrt", "rsqrt");
  sourceWithData.replaceAll("dFdx", "dfdx");
  sourceWithData.replaceAll("dFdy", "dfdy");
  return sourceWithData;
}

void compressData(CompileResult &compile_result, const DataAccumulator &header, const std::string_view metal_src_code)
{
  const int dataSize = (int)::data_size(metal_src_code);
  const char *data = metal_src_code.data();

  DynamicMemGeneralSaveCB mcwr(tmpmem, 0, 128 << 10);
  mcwr.writeInt(_MAKE4C('MTLZ'));
  mcwr.beginBlock();
  ZlibSaveCB z_cwr(mcwr, ZlibSaveCB::CL_BestCompression);

  const int headerSize = header.size();
  constexpr int hashSize = 32;

  z_cwr.writeInt(headerSize + dataSize + hashSize);

  HashMD5 hash;
  hash.append(header.data(), headerSize);
  hash.append(data, dataSize);
  hash.calc();

  z_cwr.write(hash.get(), hashSize);
  z_cwr.write(header.data(), headerSize);
  z_cwr.write(data, dataSize);

  z_cwr.finish();

  mcwr.endBlock();
  mcwr.alignOnDword(mcwr.size());

  compile_result.bytecode.assign((const uint8_t *)mcwr.data(), (const uint8_t *)mcwr.data() + mcwr.size());
}
