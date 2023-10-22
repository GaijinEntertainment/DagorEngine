#include "spirv2MetalCompiler.h"
#include "HLSL2MetalCommon.h"
#include "../debugSpitfile.h"

#include "buffBindPoints.h"

#include <ioSys/dag_fileIo.h>
#include <util/dag_string.h>

#include <debug/dag_assert.h>
#include <sstream>
#include <fstream>

#include <unordered_map>
#ifdef __APPLE__
#include <unistd.h>
#endif

#ifdef __APPLE__
static std::string getTempFile()
{
  char path[] = "/tmp/shader-XXXXXXX";
  int fd = mkstemp(path);

  if (fd == -1)
    return "";

  close(fd);

  return path;
}

static int bin_system(const char *command)
{
  pid_t pid = fork();

  if (pid > 0)
  {
    int status;
    waitpid(pid, &status, 0);

    return status;
  }
  else
  {
    _exit(execl("/bin/sh", "sh", "-c", command, NULL));
  }
}

static std::vector<uint8_t> readFile(const std::string &path)
{
  std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
  if (!in)
    return std::vector<uint8_t>();

  std::vector<uint8_t> result;

  while (in)
  {
    uint8_t buf[4096];

    in.read((char *)buf, sizeof(buf));
    result.insert(result.end(), buf, buf + in.gcount());
  }

  return result;
}

static std::vector<uint8_t> compileMSL(const std::string &source, bool use_ios_token)
{
  std::string bindir = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/";
  if (use_ios_token)
    bindir = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/metal/ios/bin/";

  std::string tmps = getTempFile();
  std::string tmpc = getTempFile();

  {
    std::ofstream out(tmps);
    out << source;
  }

  std::string cmdc = bindir + "metal";

  cmdc += use_ios_token ? " -std=ios-metal2.4" : " -std=macos-metal2.2";
  cmdc += use_ios_token ? " -miphoneos-version-min=15.0" : " -mmacos-version-min=10.15";
  cmdc += " -Wno-uninitialized -Wno-unused-variable";
  cmdc += " -x metal -Os";
  cmdc += " -o " + tmpc;
  cmdc += " " + tmps;

  int rc = bin_system(cmdc.c_str());

  if (rc != 0)
    return std::vector<uint8_t>();

  std::vector<uint8_t> result = readFile(tmpc);

  unlink(tmps.c_str());
  unlink(tmpc.c_str());

  return result;
}
#endif

static bool HasResource(const spirv_cross::SmallVector<spirv_cross::Resource> &resources, const std::string &res)
{
  for (const auto &resource : resources)
    if (resource.name == res || resource.name == ("type." + res))
      return true;
  return false;
}

static SemanticValue *lookupSemantic(const std::string &key)
{
  for (int k = 0; k < getSemanticValueCount(); k++)
  {
    SemanticValue &val = getSemanticValue(k);

    if (key == std::string(val.name) + std::to_string(val.index) || (val.index == 0 && key == std::string(val.name)))
      return &val;
  }
  return nullptr;
}

void saveTexuredata(spirv_cross::CompilerMSL &msl, DataAccumulator &header, spirv_cross::SmallVector<spirv_cross::Resource> &images,
  int *textureRemap, int offset)
{
  if (images.size() == 0)
    return;

  for (const auto &image : images)
  {
    auto &type = msl.get_type(image.base_type_id);
    const auto &imageType = type.image;
    MetalImageType img_tp = translateImageType(imageType);

    int slot = offset + msl.get_decoration(image.id, spv::DecorationBinding);

    header.append(slot);

    int internalSlot = textureRemap[slot];
    G_ASSERT(internalSlot >= 0);
    header.append(internalSlot);

    header.append(img_tp);
  }
}

CompilerMSLlocal::CompilerMSLlocal(std::vector<uint32_t> spirv, bool use_ios_token) : CompilerMSL(spirv), use_ios_token(use_ios_token)
{
  spirv_cross::CompilerGLSL::Options options_glsl;
  spirv_cross::CompilerMSL::Options options_msl;
  if (use_ios_token)
  {
    options_msl.platform = spirv_cross::CompilerMSL::Options::iOS;
    options_msl.set_msl_version(2, 4);
  }
  else
    options_msl.set_msl_version(2, 2);
  options_msl.pad_fragment_output_components = true;
  options_msl.enable_decoration_binding = true;
  options_msl.force_native_arrays = true;
  options_msl.ios_support_base_vertex_instance = true;
  // options_glsl.vertex.flip_vert_y = true;

  this->set_msl_options(options_msl);
  this->set_common_options(options_glsl);

  this->set_enabled_interface_variables(this->get_active_interface_variables());
}

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

bool CompilerMSLlocal::compileBinaryMSL(const String &mtl_src, std::vector<uint8_t> &mtl_bin, bool ios, std::string_view &result)
{
#ifdef __APPLE__
  mtl_bin = compileMSL(mtl_src.data(), ios);
  if (mtl_bin.size())
    result = {reinterpret_cast<const char *>(mtl_bin.data()), mtl_bin.size()};

  return !mtl_bin.empty();
#else
  return true;
#endif
}

CompileResult CompilerMSLlocal::convertToMSL(CompileResult &compile_result, eastl::vector<spirv::ReflectionInfo> &resourceMap,
  const char *source, ShaderType shaderType, const char *shader_name, const char *entry, uint64_t shader_variant_hash,
  bool use_binary_msl)
{
  int bufferRemap[drv3d_metal::BUFFER_POINT_COUNT];
  int textureRemap[drv3d_metal::MAX_SHADER_TEXTURES * 2];
  int samplerRemap[drv3d_metal::MAX_SHADER_TEXTURES];

  std::fill(bufferRemap, bufferRemap + drv3d_metal::BUFFER_POINT_COUNT, -1);
  std::fill(textureRemap, textureRemap + drv3d_metal::MAX_SHADER_TEXTURES * 2, -1);
  std::fill(samplerRemap, samplerRemap + drv3d_metal::MAX_SHADER_TEXTURES, -1);

  auto resources = this->get_shader_resources(this->get_active_interface_variables());
  for (auto resIt = resourceMap.begin(); resIt != resourceMap.end(); /*empty*/)
  {
    std::string name = resIt->name.c_str();
    bool has = name == "$Global" || name == "$Globals";
    has |= HasResource(resources.uniform_buffers, name);
    has |= HasResource(resources.storage_buffers, name);
    has |= HasResource(resources.separate_images, name);
    has |= HasResource(resources.storage_images, name);
    has |= HasResource(resources.separate_samplers, name);
    has |= HasResource(resources.acceleration_structures, name);
    if (has)
      resIt++;
    else
      resIt = resourceMap.erase(resIt);
  }

  eastl::vector<spirv_cross::MSLResourceBinding> binds;
  spirv_cross::MSLResourceBinding bind = {};
  bind.stage = shaderType == ShaderType::Pixel
                 ? spv::ExecutionModelFragment
                 : (shaderType == ShaderType::Compute ? spv::ExecutionModelGLCompute : spv::ExecutionModelVertex);

  uint32_t sampler = 0, texture = 0, buffer_local = drv3d_metal::BIND_POINT + 1;
  for (const auto &res : resourceMap)
  {
    bind.desc_set = res.set;
    bind.binding = res.binding;
    if (res.type == spirv::ReflectionInfo::Type::Sampler)
    {
      samplerRemap[bind.binding] = bind.msl_sampler = sampler++;
    }
    else if (res.type == spirv::ReflectionInfo::Type::Texture || res.type == spirv::ReflectionInfo::Type::Buffer)
    {
      int slot = res.uav ? drv3d_metal::MAX_SHADER_TEXTURES + bind.binding : bind.binding;
      textureRemap[slot] = bind.msl_texture = texture++;
    }
    else
    {
      drv3d_metal::BufferType type =
        res.type == spirv::ReflectionInfo::Type::StructuredBuffer || res.type == spirv::ReflectionInfo::Type::TlasBuffer
          ? (res.uav ? drv3d_metal::RW_BUFFER : drv3d_metal::STRUCT_BUFFER)
          : drv3d_metal::CONST_BUFFER;
      if (type != drv3d_metal::CONST_BUFFER || bind.binding)
        bufferRemap[drv3d_metal::RemapBufferSlot(type, bind.binding)] = bind.msl_buffer = buffer_local++;
      else
        bind.msl_buffer = drv3d_metal::BIND_POINT;
    }

    this->add_msl_resource_binding(bind);
    binds.push_back(bind);
  }

  std::string msource;
  try
  {
    msource = this->compile();
  }
  catch (std::exception &e)
  {
    compile_result.errors.sprintf("Spir-V to MSL compilation failed:\n %s", e.what());
    return compile_result;
  }

  if (shaderType == ShaderType::Compute)
  {
    std::stringstream errors;
    if (!this->validate(errors))
    {
      compile_result.errors.sprintf("Spir-V to MSL LDS size validation failed \n%s\n", errors.str().c_str());
      return compile_result;
    }
  }

  String mtl_src = prependMetaDataAndReplaceFuncs(msource, shader_name, entry, shader_variant_hash);

  char local_entry[96] = {};
  strncpy(local_entry, entry, sizeof(local_entry));

  DataAccumulator header;
  header.append(shaderType).appendData(local_entry, sizeof(local_entry)).appendData(bufferRemap, sizeof(bufferRemap));

  spirv_cross::ShaderResources res = this->get_shader_resources(this->get_active_interface_variables());

  if (shaderType == ShaderType::Vertex)
  {
    struct VsInputs
    {
      int reg = -1;
      int vsdr = -1;
      int vecsize = 0;
    };

    std::vector<VsInputs> inputs;
    inputs.reserve(res.stage_inputs.size());

    for (const auto &stgInput : res.stage_inputs)
    {
      const auto &membertype = this->get_type(stgInput.base_type_id);
      std::string semantic_str = this->get_decoration_string(stgInput.id, spv::DecorationHlslSemanticGOOGLE);
      if (semantic_str.empty())
      {
        compile_result.errors.sprintf("Spir-V to MSL input %s has empty semantic\n", stgInput.name.c_str());
        return compile_result;
      }
      int vecsize = (int)membertype.vecsize;
      int location = (int)this->get_decoration(stgInput.id, spv::DecorationLocation);
      auto semantic = lookupSemantic(semantic_str);
      if (semantic == nullptr)
      {
        compile_result.errors.sprintf("Spir-V to MSL input %s can't lookup semantic %s\n", stgInput.name.c_str(),
          semantic_str.c_str());
        return compile_result;
      }
      inputs.push_back({location, semantic->vsdr, vecsize});
    }

    header.append((int)inputs.size());
    if (!inputs.empty())
    {
      sort(begin(inputs), end(inputs), [](auto &a, auto &b) { return a.reg < b.reg; });
      for (const auto &i : inputs)
        header.append(i.reg).append(i.vecsize).append(i.vsdr);
    }
  }

  if (shaderType == ShaderType::Compute)
  {
    header.append(compile_result.computeShaderInfo.threadGroupSizeX);
    header.append(compile_result.computeShaderInfo.threadGroupSizeY);
    header.append(compile_result.computeShaderInfo.threadGroupSizeZ);

    const int accStructCount = res.acceleration_structures.size();
    header.append(accStructCount);

    for (const auto &accStruct : res.acceleration_structures)
    {
      const int driverSlot = this->get_decoration(accStruct.id, spv::DecorationBinding);
      const int driverSet = this->get_decoration(accStruct.id, spv::DecorationDescriptorSet);

      int shaderSlot = -1;
      for (auto &bind : binds)
        if (bind.desc_set == driverSet && bind.binding == driverSlot)
        {
          shaderSlot = bind.msl_buffer;
          break;
        }

      if (shaderSlot == -1 || driverSlot < 0)
      {
        compile_result.errors.sprintf("Spir-V to MSL wrong acceleration slots, driver %d or shader %d\n", driverSlot, shaderSlot);
        return compile_result;
      }

      header.append(driverSlot).append(shaderSlot);
    }
  }

  int num_reg = 0;
  if (res.uniform_buffers.size() > 0)
  {
    for (const auto &buff : res.uniform_buffers)
    {
      auto &type = this->get_type(buff.base_type_id);
      auto slot = this->get_decoration(buff.id, spv::DecorationBinding);
      if (slot != 0)
        continue;

      num_reg = this->get_member_decoration(type.self, int(type.member_types.size() - 1), spv::DecorationOffset) / 16;

      auto &membertype = this->get_type(type.member_types.back());

      int sz = membertype.columns;
      if (membertype.basetype == spirv_cross::SPIRType::Struct)
      {
        auto &lm = this->get_type(membertype.member_types.back());
        sz = this->get_member_decoration(membertype.self, int(membertype.member_types.size() - 1), spv::DecorationOffset) / 16;
        sz += lm.array.size() == 0 ? lm.columns : lm.columns * lm.array[0];
      }

      if (membertype.array.size() == 0)
      {
        num_reg += sz;
      }
      else
      {
        num_reg += sz * membertype.array[0];
      }
    }
  }

  header.append(num_reg);
  header.append((int)res.separate_images.size() + (int)res.storage_images.size());

  saveTexuredata(*this, header, res.storage_images, textureRemap, drv3d_metal::MAX_SHADER_TEXTURES);
  saveTexuredata(*this, header, res.separate_images, textureRemap, 0);

  header.append((int)res.separate_samplers.size());
  for (const auto &sampler : res.separate_samplers)
  {
    int slot = this->get_decoration(sampler.id, spv::DecorationBinding);
    int internalSlot = samplerRemap[slot];
    if (slot == -1 || internalSlot < 0)
    {
      compile_result.errors.sprintf("Spir-V to MSL wrong sampler slots, driver %d or shader %d\n", internalSlot, slot);
      return compile_result;
    }
    header.append(slot).append(internalSlot);
  }

  spitfile(shader_name, entry, "metal", shader_variant_hash, (void *)mtl_src.data(), (int)data_size(mtl_src));

  msource.assign(mtl_src.data());

  save2Lib(msource);

  std::string_view sourceToCompress = {mtl_src.c_str(), mtl_src.size()};

  std::vector<uint8_t> mtl_bin;
  if (use_binary_msl)
  {
    if (!this->compileBinaryMSL(mtl_src, mtl_bin, use_ios_token, sourceToCompress))
    {
      debug("metal source:\n%s\n", mtl_src.c_str());
      eastl::string str;
      str.sprintf("\nMSL binary compilation failed:%s\n", mtl_src.c_str());
      compile_result.errors += str;
      return compile_result;
    }
  }

  compressData(compile_result, header, sourceToCompress);
  return compile_result;
}
