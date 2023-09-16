// clang-format off
#pragma warning(disable : 4995)
#include <spirv/module_builder.h>

#if _TARGET_PC_WIN
#include <windows.h>
#include <unknwn.h>
#else
#include <folders/folders.h>
#endif
#include <dxc/dxcapi.h>
#include <spirv/compiler.h>
#include <spirv-tools/libspirv.hpp>
#include <string>
#include <osApiWrappers/dag_dynLib.h>
// clang-format on

using namespace spirv;

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

static const SemanticInfo shader_link_table[] = //
  {{"NORMAL", 0x0}, {"PSIZE", 0x1}, {"COLOR", 0x2}, {"TEXCOORD0", 0x3}, {"TEXCOORD1", 0x4}, {"TEXCOORD2", 0x5}, {"TEXCOORD3", 0x6},
    {"TEXCOORD4", 0x7}, {"TEXCOORD5", 0x8}, {"TEXCOORD6", 0x9}, {"TEXCOORD7", 0xA}, {"TEXCOORD8", 0xB}, {"TEXCOORD9", 0xC},
    {"TEXCOORD10", 0xD}, {"TEXCOORD11", 0xE}, {"TEXCOORD12", 0xF}, {"TEXCOORD13", 0x10}, {"TEXCOORD14", 0x11}, {"NORMAL0", 0x12},
    {"COLOR0", 0x13}, {"COLOR1", 0x14}, {"PSIZE0", 0x15}};

dag::ConstSpan<SemanticInfo> getInputSematicTable(const char *profile)
{
  // vs has special input
  if (profile[0] == 'v' && profile[1] == 's')
    return make_span(vs_input_table);
  return make_span(shader_link_table);
}

dag::ConstSpan<SemanticInfo> getOutputSematicTable(const char *profile)
{
  // ps has special output
  // spiregg guesses the output locations correctly already
  // TODO: verify this!
  if (profile[0] == 'p' && profile[1] == 's')
    return {};
  return make_span(shader_link_table);
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

CompileToSpirVResult spirv::compileHLSL_DXC(dag::ConstSpan<char> source, const char *entry, const char *profile, CompileFlags flags)
{
  CompileToSpirVResult result = {};
  DXCErrorHandler errorHandler{result};
  IDxcCompiler *compiler = nullptr;
#if _TARGET_PC_WIN
  const String libPath("dxcompiler.dll");
#else
  const String libPath = folders::get_exe_dir() + "libdxcompiler.dylib";
#endif
  eastl::unique_ptr<void, DagorDllCloser> library;
  library.reset(os_dll_load(libPath.c_str()));
  if (!library)
  {
    result.infoLog.emplace_back("Error: Unable to load DXC dll: " + eastl::string(os_dll_get_last_error_str()));
    return result;
  }
  DxcCreateInstanceProc DxcCreateInstance = (DxcCreateInstanceProc)os_dll_get_symbol(library.get(), "DxcCreateInstance");

  if (!DxcCreateInstance)
  {
    result.infoLog.emplace_back("Error: Missing entry point DxcCreateInstance in dxcompiler lib");
    return result;
  }

  DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

  if (!compiler)
  {
    result.infoLog.emplace_back("Error: unable to create compiler instance");
    return result;
  }

  // everything is wstring, entry is ascii, which is a subset so no conversion calls needed...
  std::wstring entryBuf;
  for (int i = 0; entry[i]; ++i)
    entryBuf.push_back(entry[i]);

  wchar_t targetProfile[] = L"xx_6_2";
  targetProfile[0] = profile[0];
  targetProfile[1] = profile[1];

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

  // can't use static const here, because compiler interface does not take const pointer to const
  // wchar pointer...
  eastl::vector<LPCWSTR> argBuf = //
    {L"-flegacy-macro-expansion", // currently not really needed though
      L"-fvk-use-dx-layout",      // needs either validation or extension to work
      // L"-enable-16bit-types",      // Does not much right now
      L"-spirv",
      // enable reflection for input laytout location remapping
      // depends on GOOGLE_* stuff, but ext specific opcodes should not end up in spirv dump
      // as this extensions are not supported by some devices causing pipeline compilation crash!
      L"-fspv-reflect", L"-fspv-extension=SPV_GOOGLE_hlsl_functionality1", L"-fspv-extension=SPV_GOOGLE_user_type",
      L"-fspv-extension=SPV_KHR_shader_draw_parameters",
      L"-fvk-use-dx-position-w", // ...
      // NOTE: currently needed to avoid stomping on each others toes
      L"-fvk-s-shift", spacingS, L"0", L"-fvk-t-shift", spacingT, L"0", L"-fvk-u-shift", spacingU, L"0",
      // use slot 0 in a never used descriptor set 9 to ensure no collisions
      L"-fvk-bind-globals", L"0", L"9",
      // L"-Vd"
      // force vulkan1.0 to avoid any problems with new spir-v on old devices
      L"-fspv-target-env=vulkan1.0", L"-fspv-extension=SPV_KHR_ray_tracing", L"-fspv-extension=SPV_KHR_ray_query"};

  if (bool(flags & CompileFlags::ENABLE_BINDLESS_SUPPORT))
  {
    argBuf.push_back(L"-fspv-extension=SPV_EXT_descriptor_indexing");
  }

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

        resolveSemantics(module, inputSemantics.data(), inLen, outputSemantics.data(), outLen, errorHandler);

        // patch atomic counter handling
        auto atomicMapping = resolveAtomicBuffers(module, AtomicResolveMode::SeparateBuffer);

        // re index to fill in possible holes
        // this can be avoided if nothing had changed (let the passes tell the caller if anything
        // had changed)
        // Not needed for now
        // reIndex(module);

        result.header = compileHeader(module, atomicMapping, flags, errorHandler);
        result.computeShaderInfo = resolveComputeShaderInfo(module);

        // All transform passes aim to remove the need for these, as features
        // of this extensions only add meta data for the user but not the driver
        cleanupReflectionLeftouts(module, errorHandler);
        module.disableExtension(Extension::GOOGLE_hlsl_functionality1);
        module.disableExtension(Extension::GOOGLE_user_type);

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