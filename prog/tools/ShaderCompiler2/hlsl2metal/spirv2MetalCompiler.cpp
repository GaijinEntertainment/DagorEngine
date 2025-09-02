// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "spirv2MetalCompiler.h"
#include "HLSL2MetalCommon.h"
#include "../debugSpitfile.h"

#include "buffBindPoints.h"

#include <ioSys/dag_fileIo.h>
#include <util/dag_string.h>
#include <drv/3d/dag_decl.h>

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

static std::vector<uint8_t> compileMSL(const std::string &source, bool use_ios_token, bool raytracing)
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

  cmdc += use_ios_token ? " -std=ios-metal2.4" : (raytracing ? " -std=metal3.1" : " -std=macos-metal2.2");
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

static bool HasResource(const spirv_cross::CompilerMSL &compiler, const spirv_cross::SmallVector<spirv_cross::Resource> &resources,
  const std::string &res)
{
  for (const auto &resource : resources)
    if (resource.name == res || res == compiler.get_name(resource.id))
      return true;
  return false;
}

static const spirv_cross::Resource *GetResource(const spirv_cross::CompilerMSL &compiler,
  const spirv_cross::SmallVector<spirv_cross::Resource> &resources, const std::string &res)
{
  for (const auto &resource : resources)
  {
    if (resource.name == res || res == compiler.get_name(resource.id))
      return &resource;
  }
  return nullptr;
}

static bool IsBindlessResource(spirv_cross::CompilerMSL &msl, const spirv_cross::Resource *resource)
{
  const spirv_cross::SPIRType &type = msl.get_type(resource->type_id);
  return (type.array.size() == 1) && (type.array[0] == 0);
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

static drv3d_metal::MetalImageType decodeImageType(spirv_cross::CompilerMSL &msl, int base_type_id)
{
  auto &type = msl.get_type(base_type_id);
  const auto &imageType = type.image;
  return translateImageType(imageType);
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
    auto &img_type = msl.get_type(imageType.type);

    drv3d_metal::EncodedMetalImageType img_tp;
    img_tp.type = translateImageType(imageType);
    img_tp.is_uint = img_type.basetype == spirv_cross::SPIRType::UInt;
    img_tp.is_int = img_type.basetype == spirv_cross::SPIRType::Int;

    int binding = msl.get_decoration(image.id, spv::DecorationBinding);
    if (IsBindlessResource(msl, &image))
      continue;
    int slot = offset + binding;
    int internalSlot = textureRemap[slot];
    G_ASSERT(internalSlot >= 0);

    // if its Buffer<> then we have lower 8 bit encoding actual shader slot
    // and next 8 bits are buffer index where it comes from
    if (img_tp.type == drv3d_metal::MetalImageType::TexBuffer)
    {
      slot = (internalSlot >> 8) & 0xff;
      internalSlot &= 0xff;
    }
    header.append(slot);
    header.append(internalSlot);
    header.append(img_tp.value);
  }
}

CompilerMSLlocal::CompilerMSLlocal(std::vector<uint32_t> spirv, bool use_ios_token) : CompilerMSL(spirv), use_ios_token(use_ios_token)
{
  this->set_enabled_interface_variables(this->get_active_interface_variables());
}

void CompilerMSLlocal::setOptions(bool has_raytracing)
{
  spirv_cross::CompilerGLSL::Options options_glsl;
  spirv_cross::CompilerMSL::Options options_msl;
  if (use_ios_token)
  {
    options_msl.platform = spirv_cross::CompilerMSL::Options::iOS;
    options_msl.set_msl_version(2, 4);
  }
  else
    // on macos raytracing requires msl 2.3 which corresponds to osx 11.0
    // so if we're compiling raytracing shaders its safe to assume its gonna be run on 11.0+
    options_msl.set_msl_version(2, has_raytracing ? 3 : 2);
  options_msl.pad_fragment_output_components = true;
  options_msl.enable_decoration_binding = true;
  options_msl.force_native_arrays = true;
  options_msl.ios_support_base_vertex_instance = true;
  options_msl.use_framebuffer_fetch_subpasses = true;
  // options_glsl.vertex.flip_vert_y = true;

  options_msl.argument_buffers_tier = spirv_cross::CompilerMSL::Options::ArgumentBuffersTier::Tier2;

  this->set_msl_options(options_msl);
  this->set_common_options(options_glsl);
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

bool CompilerMSLlocal::compileBinaryMSL(const String &mtl_src, std::vector<uint8_t> &mtl_bin, bool ios, bool raytracing,
  std::string_view &result)
{
#ifdef __APPLE__
  mtl_bin = compileMSL(mtl_src.data(), ios, raytracing);
  if (mtl_bin.size())
    result = {reinterpret_cast<const char *>(mtl_bin.data()), mtl_bin.size()};

  return !mtl_bin.empty();
#else
  return true;
#endif
}

static int EncodeBufferSlot(drv3d_metal::BufferType type, int slot)
{
  drv3d_metal::EncodedBufferRemap enc;
  enc.remap_type = drv3d_metal::EncodedBufferRemap::RemapType::Buffer;
  enc.buffer_type = type;
  enc.slot = slot;
  return enc.value;
}

static int EncodeTextureSlot(drv3d_metal::MetalImageType type, int slot)
{
  drv3d_metal::EncodedBufferRemap enc;
  enc.remap_type = drv3d_metal::EncodedBufferRemap::RemapType::Texture;
  enc.texture_type = type;
  enc.slot = slot;
  return enc.value;
}

static int EncodeSamplerSlot(int slot)
{
  drv3d_metal::EncodedBufferRemap enc;
  enc.remap_type = drv3d_metal::EncodedBufferRemap::RemapType::Sampler;
  enc.slot = slot;
  return enc.value;
}

static spv::ExecutionModel shader_type_to_execution_model(ShaderType type)
{
  G_ASSERT(type != ShaderType::Invalid);
  switch (type)
  {
    case ShaderType::Invalid: return spv::ExecutionModelVertex;
    case ShaderType::Vertex: return spv::ExecutionModelVertex;
    case ShaderType::Pixel: return spv::ExecutionModelFragment;
    case ShaderType::Compute: return spv::ExecutionModelGLCompute;
    case ShaderType::Mesh: return spv::ExecutionModelMeshEXT;
    case ShaderType::Amplification: return spv::ExecutionModelTaskEXT;
  }
  return spv::ExecutionModelVertex;
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

  bool has_raytracing = false;
  bool has_mesh = shaderType == ShaderType::Mesh || shaderType == ShaderType::Amplification;

  auto resources = this->get_shader_resources(this->get_active_interface_variables());
  for (auto resIt = resourceMap.begin(); resIt != resourceMap.end(); /*empty*/)
  {
    std::string name = resIt->name.c_str();
    bool has = name == "$Global" || name == "$Globals";
    has |= HasResource(*this, resources.uniform_buffers, name);
    has |= HasResource(*this, resources.storage_buffers, name);
    has |= HasResource(*this, resources.separate_images, name);
    has |= HasResource(*this, resources.storage_images, name);
    has |= HasResource(*this, resources.separate_samplers, name);

    bool has_acceleration_struct = HasResource(*this, resources.acceleration_structures, name);
    has_raytracing |= has_acceleration_struct;
    has |= has_acceleration_struct;

    if (has)
      resIt++;
    else
      resIt = resourceMap.erase(resIt);
  }

  eastl::vector<std::pair<eastl::string, spirv_cross::MSLResourceBinding>> binds;
  spirv_cross::MSLResourceBinding bind = {};
  bind.stage = shader_type_to_execution_model(shaderType);

  uint32_t sampler = 0, texture = 0, buffer_local = drv3d_metal::BIND_POINT + 1;
  uint32_t bindless_texture_id_count = 0, bindless_sampler_id_count = 0, bindless_buffer_id_count = 0;
  for (const auto &res : resourceMap)
  {
    bind.desc_set = res.set;
    bind.binding = res.binding;
    if (res.type == spirv::ReflectionInfo::Type::Sampler)
    {
      samplerRemap[bind.binding] = bind.msl_sampler = sampler++;
    }
    else if (res.type == spirv::ReflectionInfo::Type::Buffer)
    {
      bind.msl_texture = texture++;

      int slot = res.uav ? drv3d_metal::MAX_SHADER_TEXTURES + bind.binding : bind.binding;
      drv3d_metal::BufferType type = res.uav ? drv3d_metal::RW_BUFFER : drv3d_metal::STRUCT_BUFFER;
      textureRemap[slot] = bind.msl_texture | (drv3d_metal::RemapBufferSlot(type, bind.binding) << 8);
    }
    else if (res.type == spirv::ReflectionInfo::Type::Texture)
    {
      const spirv_cross::Resource *texture_resource = nullptr;
      const spirv_cross::Resource *sampler_resource = nullptr;
      const auto res_name = std::string(res.name.c_str());

      texture_resource = GetResource(*this, resources.separate_images, res_name);
      if (!texture_resource)
        texture_resource = GetResource(*this, resources.storage_images, res_name);
      sampler_resource = GetResource(*this, resources.separate_samplers, res_name);

      if (sampler_resource && IsBindlessResource(*this, sampler_resource))
      {
        if (bindless_sampler_id_count == drv3d_metal::BINDLESS_SAMPLER_ID_BUFFER_COUNT)
        {
          compile_result.errors.sprintf(
            "too many bindless sampler registration, max: %d. Increase BINDLESS_SAMPLER_ID_BUFFER_COUNT if possible\n",
            drv3d_metal::BINDLESS_SAMPLER_ID_BUFFER_COUNT);
          return compile_result;
        }

        bind.msl_sampler = buffer_local++;
        bufferRemap[drv3d_metal::RemapBufferSlot(drv3d_metal::BINDLESS_SAMPLER_ID_BUFFER, bindless_sampler_id_count)] =
          EncodeSamplerSlot(bind.msl_sampler);
        bindless_sampler_id_count++;
      }
      else if (texture_resource && IsBindlessResource(*this, texture_resource))
      {
        if (bindless_texture_id_count == drv3d_metal::BINDLESS_TEXTURE_ID_BUFFER_COUNT)
        {
          compile_result.errors.sprintf(
            "too many bindless texture registration, max: %d. Increase BINDLESS_TEXTURE_ID_BUFFER_COUNT if possible\n",
            drv3d_metal::BINDLESS_TEXTURE_ID_BUFFER_COUNT);
          return compile_result;
        }

        bind.msl_texture = buffer_local++;
        bufferRemap[drv3d_metal::RemapBufferSlot(drv3d_metal::BINDLESS_TEXTURE_ID_BUFFER, bindless_texture_id_count)] =
          EncodeTextureSlot(decodeImageType(*this, texture_resource->base_type_id), bind.msl_texture);
        bindless_texture_id_count++;
      }
      else
      {
        int slot = res.uav ? drv3d_metal::MAX_SHADER_TEXTURES + bind.binding : bind.binding;
        textureRemap[slot] = bind.msl_texture = texture++;
      }
    }
    else
    {
      const spirv_cross::Resource *buffer_resource = nullptr;
      const auto res_name = std::string(res.name.c_str());

      drv3d_metal::BufferType type =
        res.type == spirv::ReflectionInfo::Type::StructuredBuffer || res.type == spirv::ReflectionInfo::Type::TlasBuffer
          ? (res.uav ? drv3d_metal::RW_BUFFER : drv3d_metal::STRUCT_BUFFER)
          : drv3d_metal::CONST_BUFFER;

      buffer_resource = GetResource(*this, resources.uniform_buffers, res_name);
      if (!buffer_resource)
        buffer_resource = GetResource(*this, resources.storage_buffers, res_name);

      if (buffer_resource && IsBindlessResource(*this, buffer_resource))
      {
        if (bindless_buffer_id_count == drv3d_metal::BINDLESS_BUFFER_ID_BUFFER_COUNT)
        {
          compile_result.errors.sprintf(
            "too many bindless buffer registration, max: %d. Increase BINDLESS_BUFFER_ID_BUFFER_COUNT if possible\n",
            drv3d_metal::BINDLESS_BUFFER_ID_BUFFER_COUNT);
          return compile_result;
        }

        bind.msl_buffer = buffer_local++;
        bufferRemap[drv3d_metal::RemapBufferSlot(drv3d_metal::BINDLESS_BUFFER_ID_BUFFER, bindless_buffer_id_count)] =
          EncodeBufferSlot(drv3d_metal::STRUCT_BUFFER, bind.msl_buffer);
        bindless_buffer_id_count++;
      }
      else if (type != drv3d_metal::CONST_BUFFER || bind.binding)
      {
        bind.msl_buffer = buffer_local++;
        bufferRemap[drv3d_metal::RemapBufferSlot(type, bind.binding)] = EncodeBufferSlot(drv3d_metal::STRUCT_BUFFER, bind.msl_buffer);
      }
      else
        bind.msl_buffer = drv3d_metal::BIND_POINT;
    }

    this->add_msl_resource_binding(bind);
    binds.push_back(std::make_pair(res.name, bind));
  }

  if (shaderType == ShaderType::Vertex)
    this->position_invariant = true;

  this->setOptions(has_raytracing);

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

  if (shaderType == ShaderType::Pixel)
  {
    uint32_t output_mask = 0;
    for (const auto &stgOutput : res.stage_outputs)
    {
      int location = (int)this->get_decoration(stgOutput.id, spv::DecorationLocation);
      if (location < 0 || location >= Driver3dRenderTarget::MAX_SIMRT)
      {
        compile_result.errors.sprintf("Spir-V to MSL pixel shader output %s has wrong binding %d\n", stgOutput.name.c_str(), location);
        return compile_result;
      }
      output_mask |= 0xf << (4 * location);
    }
    header.append(output_mask);
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
      for (auto &[name, bind] : binds)
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

  uint32_t textureCount = 0;

  const auto countTextures = [this, &textureCount](spirv_cross::SmallVector<spirv_cross::Resource> &images) {
    for (const auto &image : images)
    {
      int binding = get_decoration(image.id, spv::DecorationBinding);
      if (IsBindlessResource(*this, &image))
        continue;
      textureCount++;
    }
  };

  countTextures(res.storage_images);
  countTextures(res.separate_images);

  header.append(textureCount);

  saveTexuredata(*this, header, res.storage_images, textureRemap, drv3d_metal::MAX_SHADER_TEXTURES);
  saveTexuredata(*this, header, res.separate_images, textureRemap, 0);

  uint32_t samplerCount = 0;
  for (const auto &sampler : res.separate_samplers)
  {
    int slot = this->get_decoration(sampler.id, spv::DecorationBinding);
    if (IsBindlessResource(*this, &sampler))
    {
      continue;
    }
    samplerCount++;
  }

  header.append(samplerCount);
  for (const auto &sampler : res.separate_samplers)
  {
    int slot = this->get_decoration(sampler.id, spv::DecorationBinding);
    if (IsBindlessResource(*this, &sampler))
    {
      continue;
    }
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
    if (!this->compileBinaryMSL(mtl_src, mtl_bin, use_ios_token, has_raytracing || has_mesh, sourceToCompress))
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
