// Copyright (C) Gaijin Games KFT.  All rights reserved.

#ifdef _MSC_VER
#pragma warning(disable : 4995)
#endif
#include "module_nodes.h"
#include <spirv/module_builder.h>

#if _TARGET_PC_WIN
#include <windows.h>
#include <unknwn.h>
#endif
#include <codecvt>
#include <dxc/dxcapi.h>
#include <locale>
#include <spirv/compiler.h>
#include <spirv-tools/libspirv.hpp>
#include <string>
#include <osApiWrappers/dag_dynLib.h>
#include <EASTL/array.h>

using namespace spirv;

struct spirv::DXCContext
{
  DagorDynLibHolder library;
  uint32_t verMajor = 0;
  uint32_t verMinor = 0;
  uint32_t verCommitsCount = 0;
  String verCommitHash;
  String verString;
  dag::Vector<std::wstring> extraParams;
  DxcCreateInstanceProc dxcCreateInstance = nullptr;
};

struct WrappedBlob final : public IDxcBlobEncoding
{
  WrappedBlob(dag::ConstSpan<char> s) : source{s} {}
  dag::ConstSpan<char> source;
  ULONG STDMETHODCALLTYPE AddRef()
  {
    // never do anything, stack managed
    return 1;
  }
  ULONG STDMETHODCALLTYPE Release()
  {
    // never do anything, stack managed
    return 1;
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(const IID &id, void **iface)
  {
    if (!iface)
      return E_INVALIDARG;
    if (id == __uuidof(IUnknown) || id == __uuidof(IDxcBlobEncoding) || id == __uuidof(IDxcBlob))
    {
      AddRef();
      *iface = this;
#if _TARGET_PC_WIN
      return NOERROR;
#else
      return 0;
#endif
    }
    return E_NOINTERFACE;
  }
  LPVOID STDMETHODCALLTYPE GetBufferPointer(void) { return (void *)source.data(); }
  SIZE_T STDMETHODCALLTYPE GetBufferSize(void) { return source.size(); }

  HRESULT STDMETHODCALLTYPE GetEncoding(_Out_ BOOL *pKnown, _Out_ UINT32 *pCodePage)
  {
    *pKnown = TRUE;
    *pCodePage = CP_ACP;
    return 0;
  }
};

static const SemanticInfo vs_input_table[] = //
  {{"POSITION", 0x0}, {"POSITION0", 0x0}, {"NORMAL", 0x2}, {"PSIZE", 0x5}, {"COLOR", 0x3}, {"TEXCOORD0", 0x8}, {"TEXCOORD1", 0x9},
    {"TEXCOORD2", 0xA}, {"TEXCOORD3", 0xB}, {"TEXCOORD4", 0xC}, {"TEXCOORD5", 0xD}, {"TEXCOORD6", 0xE}, {"TEXCOORD7", 0xF},
    {"TEXCOORD8", 0x5}, {"TEXCOORD9", 0x2}, {"TEXCOORD10", 0x3}, {"TEXCOORD11", 0x4}, {"TEXCOORD12", 0x1}, {"TEXCOORD13", 0x6},
    {"TEXCOORD14", 0x7}, {"NORMAL0", 0x2}, {"COLOR0", 0x3}, {"COLOR1", 0x4}, {"PSIZE0", 0x5}};

dag::ConstSpan<SemanticInfo> getInputSematicTable(const char *profile)
{
  // vs has special input
  if (profile[0] == 'v' && profile[1] == 's')
    return make_span(vs_input_table);
  return {};
}

dag::ConstSpan<SemanticInfo> getOutputSematicTable(const char *profile) { return {}; }

std::wstring getSpirvOptimizationConfigString(const eastl::vector<eastl::string_view> &disabledOptims)
{
  std::wstring result = L"-Oconfig=";
  if (disabledOptims.empty())
  {
    result += L"-O"; // optimize for performance
  }
  else if (eastl::find(disabledOptims.begin(), disabledOptims.end(), eastl::string_view("all")) != disabledOptims.end())
  {
    return L"-Od"; // disable all optimizations
  }
  else
  {
    eastl::vector<std::wstring> disabledOptimsWStr;
    for (const eastl::string_view &name : disabledOptims)
    {
      using namespace std;
      wstring nameWStr = wstring_convert<codecvt_utf8<wchar_t>>().from_bytes(name.data(), name.data() + name.size());
      disabledOptimsWStr.emplace_back(nameWStr);
    }

    auto checkOptPass = [&disabledOptimsWStr](const wchar_t *passName) -> const wchar_t * {
      for (const std::wstring &disabled : disabledOptimsWStr)
      {
        if (wcsstr(passName, disabled.c_str()) != nullptr)
          return L"";
      }
      return passName;
    };

    // take 'optimization for performance' config as basis and skip disabled passes
    // see https://github.com/KhronosGroup/SPIRV-Tools/blob/main/source/opt/optimizer.cpp#L169
    // or https://github.com/docd27/rollup-plugin-glsl-optimize/blob/master/doc/spirv-opt.md
    result += checkOptPass(L"--wrap-opkill,");
    result += checkOptPass(L"--eliminate-dead-branches,");
    result += checkOptPass(L"--merge-return,");
    result += checkOptPass(L"--inline-entry-points-exhaustive,");
    result += checkOptPass(L"--eliminate-dead-functions,");
    result += checkOptPass(L"--eliminate-dead-code-aggressive,");
    result += checkOptPass(L"--private-to-local,");
    result += checkOptPass(L"--eliminate-local-single-block,");
    result += checkOptPass(L"--eliminate-local-single-store,");
    result += checkOptPass(L"--eliminate-dead-code-aggressive,");
    result += checkOptPass(L"--scalar-replacement=100,");
    result += checkOptPass(L"--convert-local-access-chains,");
    result += checkOptPass(L"--eliminate-local-single-block,");
    result += checkOptPass(L"--eliminate-local-single-store,");
    result += checkOptPass(L"--eliminate-dead-code-aggressive,");
    result += checkOptPass(L"--ssa-rewrite,");
    result += checkOptPass(L"--eliminate-dead-code-aggressive,");
    result += checkOptPass(L"--ccp,");
    result += checkOptPass(L"--eliminate-dead-code-aggressive,");
    result += checkOptPass(L"--loop-unroll,");
    result += checkOptPass(L"--eliminate-dead-branches,");
    result += checkOptPass(L"--redundancy-elimination,");
    result += checkOptPass(L"--combine-access-chains,");
    result += checkOptPass(L"--simplify-instructions,");
    result += checkOptPass(L"--scalar-replacement=100,");
    result += checkOptPass(L"--convert-local-access-chains,");
    result += checkOptPass(L"--eliminate-local-single-block,");
    result += checkOptPass(L"--eliminate-local-single-store,");
    result += checkOptPass(L"--eliminate-dead-code-aggressive,");
    result += checkOptPass(L"--ssa-rewrite,");
    result += checkOptPass(L"--eliminate-dead-code-aggressive,");
    result += checkOptPass(L"--vector-dce,");
    result += checkOptPass(L"--eliminate-dead-inserts,");
    result += checkOptPass(L"--eliminate-dead-branches,");
    result += checkOptPass(L"--simplify-instructions,");
    result += checkOptPass(L"--if-conversion,");
    result += checkOptPass(L"--copy-propagate-arrays,");
    result += checkOptPass(L"--reduce-load-size,");
    result += checkOptPass(L"--eliminate-dead-code-aggressive,");
    result += checkOptPass(L"--merge-blocks,");
    result += checkOptPass(L"--redundancy-elimination,");
    result += checkOptPass(L"--eliminate-dead-branches,");
    result += checkOptPass(L"--merge-blocks,");
    result += checkOptPass(L"--simplify-instructions");
  }

  return result;
}

class DXCErrorHandler final : public spirv::ErrorHandler
{
public:
  DXCErrorHandler(CompileToSpirVResult &t) : target{t} {}
  CompileToSpirVResult &target;
  bool hadFatal = false;
  // called if an error is encountered with no possibility to recover from
  void onFatalError(const char *msg) override
  {
    target.infoLog.emplace_back("Fatal Error: ");
    target.infoLog.back() += msg;
    hadFatal = true;
  }
  // called if an error is encountered but it is possible to go on
  Action onError(const char *msg) override
  {
    target.infoLog.emplace_back("Error: ");
    target.infoLog.back() += msg;
    hadFatal = true;
    return Action::STOP;
  }
  // called if an incorrect behavior is detected that is not considered an error but bad practice
  Action onWarning(const char *msg) override
  {
    target.infoLog.emplace_back("Warning: ");
    target.infoLog.back() += msg;
    return Action::CONTINUE;
  }
  // called if something noteworthy happened
  void onMessage(const char *msg) override
  {
    target.infoLog.emplace_back("Info: ");
    target.infoLog.back() += msg;
  }

  void onFatalError(const eastl::string &msg) override
  {
    target.infoLog.emplace_back("Fatal Error: ");
    target.infoLog.back() += msg;
    hadFatal = true;
  }
  Action onError(const eastl::string &msg) override
  {
    target.infoLog.emplace_back("Error: ");
    target.infoLog.back() += msg;
    hadFatal = true;
    return Action::STOP;
  }
  Action onWarning(const eastl::string &msg) override
  {
    target.infoLog.emplace_back("Warning: ");
    target.infoLog.back() += msg;
    return Action::CONTINUE;
  }
  void onMessage(const eastl::string &msg) override
  {
    target.infoLog.emplace_back("Info: ");
    target.infoLog.back() += msg;
  }
};

static bool is_uav_resource(NodePointer<NodeOpVariable> var)
{
  auto ptrType = as<NodeOpTypePointer>(var->resultType);

  if (ptrType->storageClass == StorageClass::UniformConstant)
  {
    auto valueType = as<NodeTypedef>(ptrType->type);
    // Handles Texture{Type}<T>, Buffer<T>, RWTexture{Type}<T> and RWBuffer<T>
    if (is<NodeOpTypeSampledImage>(valueType) ||
        (is<NodeOpTypeImage>(valueType) && as<NodeOpTypeImage>(valueType)->sampled.value != 2) ||
        is<NodeOpTypeAccelerationStructureKHR>(valueType))
      return false;
    return true;
  }
  else if (ptrType->storageClass == StorageClass::Uniform)
  {
    if (get_buffer_kind(var) == BufferKind::Storage)
      return true;
  }
  return false;
}

static ReflectionInfo::Type convertReflectionType(NodePointer<NodeOpVariable> var)
{
  auto ptrType = as<NodeOpTypePointer>(var->resultType);
  if (is<NodeOpTypeSampler>(ptrType->type))
    return ReflectionInfo::Type::Sampler;
  if (ptrType->storageClass == StorageClass::Uniform) // const or struct
    return get_buffer_kind(var) == BufferKind::Uniform ? ReflectionInfo::Type::ConstantBuffer : ReflectionInfo::Type::StructuredBuffer;

  auto imageType = as<NodeOpTypeImage>(ptrType->type);
  if (is<NodeOpTypeAccelerationStructureKHR>(imageType))
    return ReflectionInfo::Type::TlasBuffer;
  return imageType->dim == Dim::Buffer ? ReflectionInfo::Type::Buffer : ReflectionInfo::Type::Texture;
}

static ReflectionInfo convertVariableInfo(NodePointer<NodeOpVariable> var, bool overwrite_sets)
{
  ReflectionInfo ret = {};

  PropertyName *name = find_property<PropertyName>(var);
  PropertyBinding *binding = find_property<PropertyBinding>(var);
  PropertyDescriptorSet *set = find_property<PropertyDescriptorSet>(var);
  ret.name = name ? name->name : "";
  ret.type = convertReflectionType(var);
  ret.uav = is_uav_resource(var);

  int type_set = int(ret.type) + (ret.uav ? 16 : 0);
  // special case for unbound arrays
  if (overwrite_sets && set && set->descriptorSet.value >= 32)
    type_set = set->descriptorSet.value;
  ret.set = overwrite_sets ? type_set : (set ? set->descriptorSet.value : -1);
  if (set)
    set->descriptorSet.value = ret.set;

  ret.binding = binding ? binding->bindingPoint.value % REGISTER_ENTRIES : -1;
  if (binding && set)
    binding->bindingPoint.value = ret.binding;
  return ret;
}

static eastl::vector<ReflectionInfo> compileReflection(ModuleBuilder &builder, bool overwrite_sets, bool &atomic_textures,
  bool &buff_length)
{
  eastl::vector<ReflectionInfo> ret;
  eastl::vector<NodePointer<NodeOpVariable>> uav_textures;
  eastl::vector<NodePointer<NodeOpImageTexelPointer>> uav_texture_pointers;
  eastl::vector<NodePointer<NodeOpVariable>> buffers;
  builder.enumerateAllGlobalVariables([&ret, &uav_textures, &buffers, overwrite_sets](auto var) //
    {
      auto ptrType = as<NodeOpTypePointer>(var->resultType);
      if (ptrType->storageClass == StorageClass::UniformConstant || ptrType->storageClass == StorageClass::Uniform)
      {
        ReflectionInfo info = convertVariableInfo(var, overwrite_sets);
        ret.push_back(info);

        if (info.uav && info.type == ReflectionInfo::Type::Texture)
        {
          uav_textures.push_back(var);
          return;
        }

        if (info.type == ReflectionInfo::Type::ConstantBuffer || info.type == ReflectionInfo::Type::StructuredBuffer ||
            info.type == ReflectionInfo::Type::Buffer || info.type == ReflectionInfo::Type::TlasBuffer)
          buffers.push_back(var);
      }
    });

  for (auto &var : uav_textures)
  {
    builder.enumerateConsumersOf(var, [&](auto tex_consumer, auto &) {
      if (tex_consumer->opCode == Op::OpImageTexelPointer)
        uav_texture_pointers.push_back(tex_consumer);
    });
  }
  for (auto &tex_consumer : uav_texture_pointers)
  {
    builder.enumerateConsumersOf(tex_consumer, [&](auto atomic_op, auto &) {
      if (atomic_op->opCode >= Op::OpAtomicLoad && atomic_op->opCode <= Op::OpAtomicXor)
        atomic_textures = true;
    });
  }

  for (auto &buf : buffers)
  {
    builder.enumerateConsumersOf(buf,
      [&buff_length](auto buf_consumer, auto &) { buff_length |= (buf_consumer->opCode == Op::OpArrayLength); });
  }

  return ret;
}

spirv::DXCContext *spirv::setupDXC(const char *path, const dag::Vector<String> &extra_params,
  int (*error_reporter)(const char *const, ...))
{
  spirv::DXCContext *ret = new spirv::DXCContext();
  for (const String &i : extra_params)
  {
    using namespace std;
    ret->extraParams.push_back(wstring_convert<codecvt_utf8<wchar_t>>().from_bytes(i.data(), i.data() + i.size()));
  }

  ret->verCommitHash = "<unknown hash>";

  String lookupPath(path);
  if (lookupPath.empty())
  {
    lookupPath = "./";
    eastl::array<char, DAGOR_MAX_PATH> dynlibPath{};
    bool getRes = os_dll_get_dll_name_from_addr(dynlibPath.data(), dynlibPath.size(), (const void *)&compileHLSL_DXC);
    if (getRes)
    {
#if _TARGET_PC_WIN
      const char *end = strrchr(dynlibPath.cbegin(), '\\');
#else
      const char *end = strrchr(dynlibPath.cbegin(), '/');
#endif

      if (end)
        lookupPath = String(dynlibPath.cbegin(), end - dynlibPath.cbegin() + 1);
      else
        lookupPath = String(dynlibPath.cbegin());
    }
  }

#if _TARGET_PC_WIN
  const String libPath = lookupPath + "dxcompiler.dll";
#elif _TARGET_PC_MACOSX
  const String libPath = lookupPath + "dxcompiler.dylib";
#elif _TARGET_PC_LINUX
  const String libPath = lookupPath + "libdxcompiler.so";
#endif
  ret->library.reset(os_dll_load_deep_bind(libPath.c_str()));

  if (!ret->library)
  {
    (*error_reporter)("\n[WARNING] Can't load dxc from '%s'\n", libPath.c_str());
    delete ret;
    return nullptr;
  }
  ret->dxcCreateInstance = (DxcCreateInstanceProc)os_dll_get_symbol(ret->library.get(), "DxcCreateInstance");

  if (!ret->dxcCreateInstance)
  {
    (*error_reporter)("\n[WARNING] Missing entry point DxcCreateInstance in dxcompiler lib\n");
    delete ret;
    return nullptr;
  }

  IDxcCompiler *compiler = nullptr;
  ret->dxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

  if (!compiler)
  {
    (*error_reporter)("\n[WARNING] Failed to create initial DXC instance\n");
    delete ret;
    return nullptr;
  }

  {
    IDxcVersionInfo *versionInfo = nullptr;
    compiler->QueryInterface(__uuidof(IDxcVersionInfo), (void **)&versionInfo);
    if (versionInfo)
    {
      versionInfo->GetVersion(&ret->verMajor, &ret->verMinor);
      versionInfo->Release();
    }
  }

  {
    IDxcVersionInfo2 *versionInfo2 = nullptr;
    compiler->QueryInterface(__uuidof(IDxcVersionInfo2), (void **)&versionInfo2);
    if (versionInfo2)
    {
      char *hash = nullptr;
      versionInfo2->GetCommitInfo(&ret->verCommitsCount, &hash);
      if (hash)
      {
        ret->verCommitHash = hash;
        CoTaskMemFree(hash);
      }
      versionInfo2->Release();
    }
  }

  compiler->Release();

  ret->verString = String(32, "%u.%u.%u.%s", ret->verMajor, ret->verMinor, ret->verCommitsCount, ret->verCommitHash.c_str());

  // printf("\n[OK] DXC loaded from %s ver %s \n", libPath.c_str(), ret->verString.c_str());
  return ret;
}

const char *spirv::getDXCVerString(const spirv::DXCContext *ctx) { return ctx ? ctx->verString : ""; }

void spirv::shutdownDXC(spirv::DXCContext *ctx)
{
  if (ctx)
    delete ctx;
}

CompileToSpirVResult spirv::compileHLSL_DXC(const spirv::DXCContext *dxc_ctx, dag::ConstSpan<char> source, const char *entry,
  const char *profile, CompileFlags flags, const eastl::vector<eastl::string_view> &disabledOptimizaions)
{
  CompileToSpirVResult result = {};
  DXCErrorHandler errorHandler{result};
  IDxcCompiler *compiler = nullptr;

  if (!dxc_ctx)
  {
    result.infoLog.emplace_back("Error: DXC failed to initialize");
    return result;
  }

  dxc_ctx->dxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
  if (!compiler)
  {
    result.infoLog.emplace_back("Error: unable to create compiler instance");
    return result;
  }

  // everything is wstring, entry is ascii, which is a subset so no conversion calls needed...
  std::wstring entryBuf;
  for (int i = 0; entry[i]; ++i)
    entryBuf.push_back(entry[i]);

  bool meshShader = (flags & CompileFlags::ENABLE_MESH_SHADER) == CompileFlags::ENABLE_MESH_SHADER;
  wchar_t targetProfile[7]{};
  targetProfile[0] = profile[0];
  targetProfile[1] = profile[1];
  targetProfile[2] = '_';
  targetProfile[3] = '6';
  targetProfile[4] = '_';
  targetProfile[5] = meshShader ? '5' : '2';

#if !_TARGET_PC_WIN
#define _MAX_ITOSTR_BASE10_COUNT 22
#endif
  wchar_t spacingS[_MAX_ITOSTR_BASE10_COUNT];
  wchar_t spacingT[_MAX_ITOSTR_BASE10_COUNT];
  wchar_t spacingU[_MAX_ITOSTR_BASE10_COUNT];
#if _TARGET_PC_WIN
  _itow_s(REGISTER_ENTRIES * 1, spacingS, 10);
  _itow_s(REGISTER_ENTRIES * 2, spacingT, 10);
  _itow_s(REGISTER_ENTRIES * 3, spacingU, 10);
#else
  std::swprintf(spacingS, _MAX_ITOSTR_BASE10_COUNT, L"%d", REGISTER_ENTRIES * 1);
  std::swprintf(spacingT, _MAX_ITOSTR_BASE10_COUNT, L"%d", REGISTER_ENTRIES * 2);
  std::swprintf(spacingU, _MAX_ITOSTR_BASE10_COUNT, L"%d", REGISTER_ENTRIES * 3);
#endif

  std::wstring optConfig = getSpirvOptimizationConfigString(disabledOptimizaions);

  // force vulkan1.0 to avoid any problems with new spir-v on old devices
  // or vulkan1.1 to support wave intrinsics
  const wchar_t *spirvVersion = L"-fspv-target-env=vulkan1.0";
  if (meshShader)
    spirvVersion = L"-fspv-target-env=vulkan1.1spirv1.4";
  else if ((flags & CompileFlags::ENABLE_WAVE_INTRINSICS) == CompileFlags::ENABLE_WAVE_INTRINSICS)
    spirvVersion = L"-fspv-target-env=vulkan1.1";

  // can't use static const here, because compiler interface does not take const pointer to const
  // wchar pointer...
  eastl::vector<LPCWSTR> argBuf = //
    {L"-flegacy-macro-expansion", // currently not really needed though
      L"-fvk-use-dx-layout",      // needs either validation or extension to work
      (flags & CompileFlags::ENABLE_HALFS) == CompileFlags::ENABLE_HALFS ? L"-enable-16bit-types" : L"", L"-spirv",
      // enable reflection for input laytout location remapping
      // depends on GOOGLE_* stuff, but ext specific opcodes should not end up in spirv dump
      // as this extensions are not supported by some devices causing pipeline compilation crash!
      L"-fspv-reflect", L"-fspv-extension=SPV_GOOGLE_hlsl_functionality1", L"-fspv-extension=SPV_GOOGLE_user_type",
      L"-fspv-extension=SPV_KHR_shader_draw_parameters",
      L"-fvk-use-dx-position-w", // ...
      // NOTE: currently needed to avoid stomping on each others toes
      L"-fvk-s-shift", spacingS, L"0", L"-fvk-t-shift", spacingT, L"0", L"-fvk-u-shift", spacingU, L"0",
      // use slot 0 in a never used descriptor set 9 to ensure no collisions
      L"-fvk-bind-globals", L"0", L"9", optConfig.c_str(),
      // L"-Vd"
      spirvVersion, L"-fspv-extension=SPV_KHR_ray_tracing", L"-fspv-extension=SPV_KHR_ray_query", L"-fspv-flatten-resource-arrays",
      (flags & CompileFlags::ENABLE_HALFS) == CompileFlags::ENABLE_HALFS ? L"-fspv-extension=SPV_KHR_16bit_storage" : L"",
      meshShader ? L"-fspv-extension=SPV_EXT_mesh_shader" : L""};

  if (bool(flags & CompileFlags::ENABLE_BINDLESS_SUPPORT))
  {
    argBuf.push_back(L"-fspv-extension=SPV_EXT_descriptor_indexing");
  }
  if (bool(flags & CompileFlags::ENABLE_HLSL21))
  {
    argBuf.push_back(L"-HV 2021");
  }
  else
  {
    argBuf.push_back(L"-HV 2018");
  }

  for (const std::wstring &i : dxc_ctx->extraParams)
    argBuf.push_back(i.c_str());

  WrappedBlob sourceBlob{source};

  IDxcOperationResult *compileResult = nullptr;
  auto compileEC = compiler->Compile(&sourceBlob, nullptr, entryBuf.c_str(), targetProfile, argBuf.data(), argBuf.size(), nullptr, 0,
    nullptr, &compileResult);
  compiler->Release();

  if (compileEC)
  {
    result.infoLog.push_back("Error code from compiler: " + eastl::to_string(compileEC));
  }

  if (compileResult)
  {
    compileResult->GetStatus(&compileEC);
    result.infoLog.push_back("Error code from compile result: " + eastl::to_string(compileEC));
    IDxcBlobEncoding *errors = nullptr;
    compileResult->GetErrorBuffer(&errors);
    if (errors)
    {
      UINT cp = CP_ACP;
      BOOL knownCP = FALSE;
      errors->GetEncoding(&knownCP, &cp);
#if _TARGET_PC_WIN
      if (!knownCP || cp == CP_ACP || cp == CP_UTF7 || cp == CP_UTF8)
#else
      if (!knownCP || cp == CP_ACP || cp == CP_UTF8)
#endif
      {
        result.infoLog.emplace_back((const char *)errors->GetBufferPointer(), errors->GetBufferSize());
      }
      errors->Release();
    }

    IDxcBlob *compiled = nullptr;
    compileResult->GetResult(&compiled);
    if (compiled)
    {
      result.infoLog.push_back("spir-v size " + eastl::to_string(compiled->GetBufferSize()));
      ModuleBuilder module;
      auto ptr = compiled->GetBufferPointer();
      if (ptr)
      {
        auto blobReadResult = read(module, (const unsigned *)ptr, (compiled->GetBufferSize() + 3) / 4);

        result.infoLog.insert(end(result.infoLog), begin(blobReadResult.errorLog), end(blobReadResult.errorLog));
        result.infoLog.insert(end(result.infoLog), begin(blobReadResult.warningLog), end(blobReadResult.warningLog));

        // first validate structure layouts and bail out if they are invalid
        if (!validateStructureLayouts(module, StructureValidationRules::VULKAN_1_0, errorHandler))
        {
          errorHandler.onMessage("DXC Produced the following SPIR-V Module");
          spvtools::SpirvTools tools{SPV_ENV_VULKAN_1_0};
          tools.SetMessageConsumer(
            [&](spv_message_level_t /* level */, const char * /* source */, const spv_position_t & /* position */,
              const char *message) //
            { result.infoLog.push_back(message); });
          std::string spirvAsm;
          tools.Disassemble((const unsigned *)ptr, (compiled->GetBufferSize() + 3) / 4, &spirvAsm, SPV_BINARY_TO_TEXT_OPTION_INDENT);
          if (!spirvAsm.empty())
          {
            result.infoLog.emplace_back(spirvAsm.c_str());
          }
          return result;
        }

        make_buffer_pointer_type_unique_per_buffer(module);

        // default renames anything to main
        renameEntryPoints(module);

        auto inputSemantics = getInputSematicTable(profile);
        auto outputSemantics = getOutputSematicTable(profile);
        auto inLen = inputSemantics.size();
        auto outLen = outputSemantics.size();
        if ((flags & CompileFlags::VARYING_PASS_THROUGH) == CompileFlags::VARYING_PASS_THROUGH)
        {
          // this just leads to dropping the semantic properties of the in/out vars if they have any
          inLen = 0;
          outLen = 0;
        }

        // patch atomic counter handling
        resolveAtomicBuffers(module, AtomicResolveMode::SeparateBuffer, errorHandler);

        // re index to fill in possible holes
        // this can be avoided if nothing had changed (let the passes tell the caller if anything
        // had changed)
        // Not needed for now
        // reIndex(module);

        result.computeShaderInfo = resolveComputeShaderInfo(module);
        if ((flags & CompileFlags::OUTPUT_REFLECTION) == CompileFlags::OUTPUT_REFLECTION)
        {
          bool atomic_textures = false;
          bool buff_length = false;
          result.header = {};
          result.reflection =
            compileReflection(module, (flags & CompileFlags::OVERWRITE_DESCRIPTOR_SETS) == CompileFlags::OVERWRITE_DESCRIPTOR_SETS,
              atomic_textures, buff_length);
          if (atomic_textures)
          {
            result.infoLog.emplace_back("Shader uses atomic ops for textures. this doesn't work on metal");
            return result;
          }

          if (buff_length)
          {
            result.infoLog.emplace_back("Shader uses array length ops for buffers. They don't work properly on some devices");
            return result;
          }
        }
        else
        {
          resolveSemantics(module, inputSemantics.data(), inLen, outputSemantics.data(), outLen, errorHandler);

          result.header = compileHeader(module, flags, errorHandler);

          fixDXCBugs(module, errorHandler);
          cleanupReflectionLeftouts(module, errorHandler);
          module.disableExtension(Extension::GOOGLE_hlsl_functionality1);
          module.disableExtension(Extension::GOOGLE_user_type);
        }

        // TODO: allow external control over this
        // TODO: fix writer to use this
        WriteConfig wcfg;
        wcfg.writeNames = true;
        wcfg.writeSourceExtensions = true;
        wcfg.writeLines = true;
        wcfg.writeDebugGrammar = true;

        auto resultBlob = write(module, wcfg, errorHandler);
        if (!errorHandler.hadFatal)
        {
          // eastl / std mixing...
          result.byteCode.assign(begin(resultBlob), end(resultBlob));
          // result.byteCode.assign((const unsigned *)ptr, ((const unsigned *)ptr) +
          // (compiled->GetBufferSize() + 3) / 4);
        }
        else
        {
          spvtools::SpirvTools tools{SPV_ENV_VULKAN_1_0};
          tools.SetMessageConsumer(
            [&](spv_message_level_t /* level */, const char * /* source */, const spv_position_t & /* position */,
              const char *message) //
            { result.infoLog.push_back(message); });
          std::string spirvAsm;
          tools.Disassemble(resultBlob.data(), resultBlob.size(), &spirvAsm, SPV_BINARY_TO_TEXT_OPTION_INDENT);
          if (!spirvAsm.empty())
          {
            result.infoLog.emplace_back(spirvAsm.c_str());
          }
        }
        compiled->Release();
      }
    }
    else
    {
      result.infoLog.push_back("GetResult returned null");
    }
  }

  return result;
}
