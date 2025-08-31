// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <dxil/compiler.h>
#include <supp/dag_comPtr.h>

#include <windows.h>
#include <unknwn.h>
#include <dxc/dxcapi.h>
#include <hash/sha1.h>
#include <EASTL/variant.h>
#include <EASTL/vector.h>

#include <locale>
#include <codecvt>

using namespace dxil;

namespace
{
struct WrappedBlob final : public DxcText, public IDxcBlobEncoding
{
  template <typename T>
  WrappedBlob(dag::Span<T> s)
  {
    Ptr = s.data();
    Size = s.size();
    Encoding = CP_ACP;
  }
  template <typename T>
  WrappedBlob(eastl::span<T> s)
  {
    Ptr = s.data();
    Size = s.size();
    Encoding = CP_ACP;
  }
  WrappedBlob(eastl::string_view s)
  {
    Ptr = s.data();
    Size = s.size();
  }
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
      return NOERROR;
    }
    return E_NOINTERFACE;
  }
  LPVOID STDMETHODCALLTYPE GetBufferPointer(void) { return (void *)Ptr; }
  SIZE_T STDMETHODCALLTYPE GetBufferSize(void) { return Size; }

  HRESULT STDMETHODCALLTYPE GetEncoding(_Out_ BOOL *pKnown, _Out_ UINT32 *pCodePage)
  {
    *pKnown = TRUE;
    *pCodePage = Encoding;
    return 0;
  }
};

template <typename T>
T clamp(T v, T min, T max)
{
  return v > max ? max : v < min ? min : v;
}

bool write_to_file(auto blob, const eastl::wstring &path)
{
  FILE *fp = nullptr;
  _wfopen_s(&fp, path.c_str(), L"wb");
  if (!fp)
    return false;

  fwrite(blob->GetBufferPointer(), blob->GetBufferSize(), 1, fp);
  fclose(fp);
  return true;
}

CompileResult compile(IDxcCompiler3 *compiler, UINT32 major, UINT32 minor, WrappedBlob blob, const char *entry, const char *profile,
  const DXCSettings &settings, DXCVersion dxc_version)
{
  // everything is wstring, entry is ascii, which is a subset so no conversion calls needed...
  eastl::vector<wchar_t> entryBuf;
  for (int i = 0; entry[i]; ++i)
    entryBuf.push_back(entry[i]);
  entryBuf.push_back('\0');

  // min profile is 6.0
  wchar_t targetProfile[] = L"??_6_0";
  targetProfile[0] = profile[0];
  targetProfile[1] = profile[1];
  // if 6.x or greater requested, overwrite defaults
  if (profile[3] >= '6')
  {
    targetProfile[3] = profile[3];
    targetProfile[5] = profile[5];
  }

  wchar_t optimizeLevel[4] = L"-O0";
  optimizeLevel[2] = L'0' + clamp<uint32_t>(settings.optimizeLevel, 0, 3);

  wchar_t autoSpace[2] = L"0";
  autoSpace[0] = L'0' + clamp<uint32_t>(settings.autoBindSpace, 0, 9);

  Tab<LPCWSTR> compilerParams = {};
  compilerParams.reserve(64);

  compilerParams.emplace_back(L"-T");
  compilerParams.emplace_back(targetProfile);

  compilerParams.emplace_back(L"-E");
  compilerParams.emplace_back(entryBuf.data());

  compilerParams.emplace_back(optimizeLevel);
  // old versions don't know this arguments and will abort
  if (major > 1 || minor > 2)
  {
    compilerParams.emplace_back(L"-auto-binding-space");
    compilerParams.emplace_back(autoSpace);
  }

  if (settings.enableFp16 && (targetProfile[3] > L'6' || (targetProfile[3] == L'6' && targetProfile[5] >= L'2')))
  {
    compilerParams.emplace_back(L"-enable-16bit-types");
  }

  if (settings.pdbMode == PDBMode::FULL)
  {
    compilerParams.emplace_back(L"-Zss");
    compilerParams.emplace_back(L"-Zi");
  }
  else if (settings.pdbMode == PDBMode::SMALL)
  {
    compilerParams.emplace_back(L"-Zss");
    compilerParams.emplace_back(L"-Zs");
  }

  if (settings.saveHlslToBlob)
  {
    compilerParams.emplace_back(L"-Qembed_debug");
  }
  else
  {
    // strip everything from DXIL blob, only thing we need is reflection and debug, but for
    // those we use dedicated blob
    compilerParams.emplace_back(L"-Qstrip_debug");
    compilerParams.emplace_back(L"-Qstrip_reflect");
    compilerParams.emplace_back(L"-Qstrip_rootsignature");
  }


  if (settings.warningsAsErrors)
  {
    compilerParams.emplace_back(L"-WX");
  }
  if (settings.enforceIEEEStrictness)
  {
    compilerParams.emplace_back(L"-Gis");
  }
  if (settings.disableValidation)
  {
    compilerParams.emplace_back(L"-Vd");
  }
  static const wchar_t auto_generated_signature_name[] = L"_AUTO_GENERATED_ROOT_SIGNATURE";
  if (!settings.rootSignatureDefine.empty())
  {
    compilerParams.emplace_back(L"-rootsig-define");
    compilerParams.emplace_back(auto_generated_signature_name);
  }
  if (settings.assumeAllResourcesBound)
  {
    compilerParams.emplace_back(L"-all_resources_bound");
  }
  if (settings.hlsl2021)
  {
    compilerParams.emplace_back(L"-HV 2021");
  }
  else
  {
    compilerParams.emplace_back(L"-HV 2018");
  }

  wchar_t spaceName[] = L"AUTO_DX12_REGISTER_SPACE=space?";
  spaceName[30] = autoSpace[0];
  // AUTO_DX12_REGISTER_SPACE is used to automatically select the register space
  // for all register bindings
  // SHADER_COMPILER_DXC is a hint for the shaders, so that they know
  // which underlying shader compiler is running
  compilerParams.emplace_back(L"-D");
  compilerParams.emplace_back(spaceName);

  compilerParams.emplace_back(L"-D");
  compilerParams.emplace_back(L"SHADER_COMPILER_DXC=1");

  if (settings.xboxPhaseOne)
  {
    compilerParams.emplace_back(L"-D");
    compilerParams.emplace_back(L"__XBOX_DISABLE_PRECOMPILE=1");
  }
  else
  {
    if (!settings.rootSignatureDefine.empty() && !settings.pipelineHasStreamOutput)
    {
      // if we generate final binary, remove dxil (except shaders with stream output)
      // https://forums.xboxlive.com/questions/120453/streamout-mismatch.html
      if (recognizesXboxStripDxilMacro(dxc_version))
      {
        compilerParams.emplace_back(L"-D");
        compilerParams.emplace_back(L"__XBOX_STRIP_DXIL=1");
      }
    }

    if (DXCVersion::XBOX_ONE == dxc_version)
    {
      // NOTE: on XONE this defines are optional and can reduce final binary size
      if (settings.pipelineHasTesselationStage)
      {
        if (settings.pipelineHasGeoemetryStage)
        {
          // used stages:
          // LS HS ES GS VS PS
        }
        else
        {
          // used stages:
          // LS HS VS PS
          compilerParams.emplace_back(L"-D");
          compilerParams.emplace_back(L"__XBOX_DISABLE_PRECOMPILE_ES=1");
          compilerParams.emplace_back(L"-D");
          compilerParams.emplace_back(L"__XBOX_DISABLE_PRECOMPILE_GS=1");
        }
      }
      else if (settings.pipelineHasGeoemetryStage)
      {
        // used stages:
        // ES GS VS PS
        compilerParams.emplace_back(L"-D");
        compilerParams.emplace_back(L"__XBOX_DISABLE_PRECOMPILE_LS=1");
        compilerParams.emplace_back(L"-D");
        compilerParams.emplace_back(L"__XBOX_DISABLE_PRECOMPILE_HS=1");
      }
      else
      {
        // used stages:
        // VS PS
        compilerParams.emplace_back(L"-D");
        compilerParams.emplace_back(L"__XBOX_DISABLE_PRECOMPILE_LS=1");
        compilerParams.emplace_back(L"-D");
        compilerParams.emplace_back(L"__XBOX_DISABLE_PRECOMPILE_HS=1");
        compilerParams.emplace_back(L"-D");
        compilerParams.emplace_back(L"__XBOX_DISABLE_PRECOMPILE_ES=1");
        compilerParams.emplace_back(L"-D");
        compilerParams.emplace_back(L"__XBOX_DISABLE_PRECOMPILE_GS=1");
      }

      // Breaks tessellation shader output
      // tested with GDK 200500
      // compilerParams.emplace_back(L"-D");
      // compilerParams.emplace_back(L"__XBOX_ENABLE_PSPRIMID=1");
    }
    else if (DXCVersion::XBOX_SCARLETT == dxc_version)
    {
      if (settings.pipelineIsMesh)
      {
        if (settings.pipelineHasAmplification)
        {
          // currently no defines for mesh or amplification to optimize
        }
        else
        {
          // currently no defines for mesh or amplification to optimize
        }
      }
      else
      {
        // NOTE: on XSCARLETT this defines are mandatory to produce all needed stages!
        if (settings.pipelineHasTesselationStage)
        {
          compilerParams.emplace_back(L"-D");
          compilerParams.emplace_back(L"__XBOX_PRECOMPILE_VS_HS=1");
          compilerParams.emplace_back(L"-D");
          compilerParams.emplace_back(L"__XBOX_ENABLE_HSOFFCHIP=1");

          if (settings.pipelineHasGeoemetryStage)
          {
            compilerParams.emplace_back(L"-D");
            compilerParams.emplace_back(L"__XBOX_PRECOMPILE_DS_GS=1");
            compilerParams.emplace_back(L"-D");
            compilerParams.emplace_back(L"__XBOX_ENABLE_GSONCHIP=1");
          }
          else
          {
            compilerParams.emplace_back(L"-D");
            compilerParams.emplace_back(L"__XBOX_PRECOMPILE_DS_PS=1");
          }
        }
        else if (settings.pipelineHasGeoemetryStage)
        {
          compilerParams.emplace_back(L"-D");
          compilerParams.emplace_back(L"__XBOX_PRECOMPILE_VS_GS=1");
          compilerParams.emplace_back(L"-D");
          compilerParams.emplace_back(L"__XBOX_ENABLE_GSONCHIP=1");
        }
        else
        {
          compilerParams.emplace_back(L"-D");
          compilerParams.emplace_back(L"__XBOX_PRECOMPILE_VS_PS=1");
        }
      }

      // Breaks tessellation shader output
      // tested with GDK 200500
      // compilerParams.emplace_back(L"-D");
      // compilerParams.emplace_back(L"__XBOX_ENABLE_PSPRIMID=1");

      if (settings.scarlettWaveSize32)
      {
        compilerParams.emplace_back(L"-D");
        compilerParams.emplace_back(L"__XBOX_ENABLE_WAVE32=1");
      }
    }
  }

  // Custom defines
  dag::Vector<eastl::wstring> definesArgsStorage{};
  for (const auto &[def, val] : settings.defines)
  {
    definesArgsStorage.emplace_back(eastl::wstring::CtorSprintf{}, L"%.*ls=%.*ls", def.length(), def.data(), val.length(), val.data());
    compilerParams.emplace_back(L"-D");
    compilerParams.emplace_back(definesArgsStorage.back().c_str());
  }

  eastl::wstring sharedPath;
  size_t basePathLenght = 0;
  if ((settings.pdbMode != PDBMode::NONE) && !settings.PDBBasePath.empty())
  {
    auto &hlslPath = sharedPath;
    hlslPath.reserve(settings.PDBBasePath.length() + 70); // 64 for hashes 5 for extensions and +1 should be enough for anything
    hlslPath.assign(begin(settings.PDBBasePath), end(settings.PDBBasePath));
    if (auto lastChar = hlslPath.back(); lastChar != L'\\' && lastChar != L'/')
      hlslPath.push_back(L'\\');
    basePathLenght = hlslPath.length();

    static constexpr char hexChars[] = "0123456789ABCDEF";
    unsigned char hash[20];
    sha1_csum(static_cast<unsigned char *>(blob.GetBufferPointer()), blob.GetBufferSize(), hash);

    for (unsigned char byte : hash)
    {
      hlslPath.push_back(hexChars[byte >> 4]);
      hlslPath.push_back(hexChars[byte & 15]);
    }
    hlslPath.append(L".hlsl");

    if (write_to_file(&blob, hlslPath))
    {
      compilerParams.emplace_back(hlslPath.begin() + basePathLenght);
    }
  }

  CompileResult result;

#if DAGOR_DBGLEVEL > 0
  {
    result.messageLog += "DXC invoked with cmdline:";
    for (auto warg : compilerParams)
    {
      const auto argStdstr = std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(warg);
      result.messageLog += " ";
      result.messageLog += argStdstr.c_str();
    }

    result.messageLog += "\n";
  }
#endif

  ComPtr<IDxcResult> compileResult;
  auto compileEC = compiler->Compile(&blob, compilerParams.data(), compilerParams.size(), nullptr, IID_PPV_ARGS(&compileResult));

  if (FAILED(compileEC))
  {
    result.errorLog = "dxil::compileHLSLWithDXC: Error code from compiler: " + eastl::to_string(compileEC);
  }

  if (compileResult)
  {
    compileResult->GetStatus(&compileEC);
    if (FAILED(compileEC))
    {
      result.errorLog += "dxil::compileHLSLWithDXC: Error code from compile result: " + eastl::to_string(compileEC) + "\n";
    }
    ComPtr<IDxcBlobUtf8> errorMessages;
    compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorMessages), nullptr);
    if (errorMessages && errorMessages->GetStringLength() != 0)
    {
      // technically errorMessages is utf8, but there is hardly any chance to step outside of ascii
      result.errorLog.append(errorMessages->GetStringPointer(), errorMessages->GetStringPointer() + errorMessages->GetStringLength());
      result.errorLog += "\n";

      auto pos = result.errorLog.find("Parameter with semantic");
      if (pos != eastl::string::npos)
      {
        auto pos2 = result.errorLog.find("has overlapping semantic index at");
        if (pos2 != eastl::string::npos && pos2 > pos)
        {
          // give an additional hint, the error message is a bit cryptic
          result.errorLog.append("dxil::compileHLSLWithDXC: NOTE: The error message about "
                                 "overlapping semantics means  that the shader declares "
                                 "multiple variables with the same semantic name, remove "
                                 "the duplicates to fix this error. Even  unused ones "
                                 "count, DXC is more strict about such things than other "
                                 "compilers.");
        }
      }
    }
    if (SUCCEEDED(compileEC))
    {
      // Try to write out PDB if possible
      if (!sharedPath.empty())
      {
        ComPtr<IDxcBlob> pdb;
        ComPtr<IDxcBlobUtf16> pdbName;
        compileResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdb), &pdbName);
        if (pdb && pdbName)
        {
          sharedPath.resize(basePathLenght);
          auto &pdbPath = sharedPath;
          pdbPath.append(pdbName->GetStringPointer(), pdbName->GetStringPointer() + pdbName->GetStringLength());
          if (!write_to_file(pdb, pdbPath))
          {
            result.errorLog.append("dxil::compileHLSLWithDXC: Unable to store PDB at ");
            // truncates wchars to chars
            eastl::copy(begin(pdbPath), end(pdbPath), eastl::back_inserter(result.errorLog));
          }
        }

        ComPtr<IDxcBlob> cso;
        ComPtr<IDxcBlobUtf16> csoName;
        compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&cso), &csoName);

        // note: for some reason csoName is nullptr here, so pdb name is used instead
        if (cso && pdbName)
        {
          sharedPath.resize(basePathLenght);
          auto &csoPath = sharedPath;
          csoPath.append(pdbName->GetStringPointer(), pdbName->GetStringPointer() + pdbName->GetStringLength() - 4);
          csoPath.append(L".cso");
          if (!write_to_file(cso, csoPath))
          {
            result.errorLog.append("dxil::compileHLSLWithDXC: Unable to store CSO at ");
            // truncates wchars to chars
            eastl::copy(begin(csoPath), end(csoPath), eastl::back_inserter(result.errorLog));
          }
        }
      }

      ComPtr<IDxcBlob> dxil;
      ComPtr<IDxcBlobUtf16> shaderName;
      compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxil), &shaderName);
      if (dxil && dxil->GetBufferSize() > 0)
      {
        auto base = reinterpret_cast<const uint8_t *>(dxil->GetBufferPointer());
        result.byteCode.assign(base, base + dxil->GetBufferSize());
      }
      else
      {
        result.errorLog.append("dxil::compileHLSLWithDXC: No DXIL byte code object was generated!");
      }

      ComPtr<IDxcBlob> reflectionData;
      compileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionData), nullptr);
      if (reflectionData && reflectionData->GetBufferSize() > 0)
      {
        auto base = reinterpret_cast<const uint8_t *>(reflectionData->GetBufferPointer());
        result.reflectionData.assign(base, base + reflectionData->GetBufferSize());
      }
    }
  }
  return result;
}
} // namespace

CompileResult dxil::compileHLSLWithDXC(dag::ConstSpan<char> src, const char *entry, const char *profile, const DXCSettings &settings,
  void *dxc_lib, DXCVersion dxc_version)
{
  CompileResult result;
  if (!dxc_lib)
  {
    result.errorLog = "dxil::compileHLSLWithDXC: no valid shader compiler library handle provided";
  }
  else
  {
    DxcCreateInstanceProc DxcCreateInstance = nullptr;
    reinterpret_cast<FARPROC &>(DxcCreateInstance) = GetProcAddress(static_cast<HMODULE>(dxc_lib), "DxcCreateInstance");

    if (!DxcCreateInstance)
    {
      result.errorLog = "dxil::compileHLSLWithDXC: Missing entry point in DxcCreateInstance "
                        "dxcompiler(_x/_xs) library";
    }
    else
    {
      ComPtr<IDxcCompiler3> compiler3;
      DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler3));

      if (!compiler3)
      {
        result.errorLog = "dxil::compileHLSLWithDXC: DxcCreateInstance failed";
      }
      else
      {
        // need to know which version we are on to decide if we have to pass some extra parameters
        // and omit some others
        ComPtr<IDxcVersionInfo> versionInfo;
        compiler3.As(&versionInfo);

        UINT32 major = 1;
        UINT32 minor = 0;
        if (versionInfo)
          versionInfo->GetVersion(&major, &minor);

        WrappedBlob sourceBlob{src};
        return compile(compiler3.Get(), major, minor, sourceBlob, entry, profile, settings, dxc_version);
      }
    }
  }

  return result;
}

PreprocessResult dxil::preprocessHLSLWithDXC(dag::ConstSpan<char> src, dag::ConstSpan<DXCDefine> defines, void *dxc_lib)
{
  PreprocessResult result;
  if (!dxc_lib)
  {
    result.errorLog = "dxil::compileHLSLWithDXC: no valid shader compiler library handle provided";
    return result;
  }

  DxcCreateInstanceProc DxcCreateInstance = nullptr;
  reinterpret_cast<FARPROC &>(DxcCreateInstance) = GetProcAddress(static_cast<HMODULE>(dxc_lib), "DxcCreateInstance");

  if (!DxcCreateInstance)
  {
    result.errorLog = "dxil::compileHLSLWithDXC: Missing entry point in DxcCreateInstance "
                      "dxcompiler(_x/_xs) library";
    return result;
  }

  ComPtr<IDxcCompiler3> compiler3;
  DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler3));

  if (!compiler3)
  {
    result.errorLog = "dxil::compileHLSLWithDXC: DxcCreateInstance failed";
    return result;
  }

  WrappedBlob sourceBlob{src};

  dag::Vector<LPCWSTR> compilerParams;
  compilerParams.reserve(2 + 2 * defines.size());

  compilerParams.emplace_back(L"-P");

  // used in a great number of shaders
  // TODO: fix and get rid of this!!!
  compilerParams.emplace_back(L"-Wno-macro-redefined");

  dag::Vector<eastl::wstring> definesArgsStorage;
  for (const auto &[def, val] : defines)
  {
    definesArgsStorage.emplace_back(eastl::wstring::CtorSprintf{}, L"%.*ls=%.*ls", def.length(), def.data(), val.length(), val.data());
    compilerParams.emplace_back(L"-D");
    compilerParams.emplace_back(definesArgsStorage.back().c_str());
  }

  ComPtr<IDxcResult> compileResult;
  auto compileEC =
    compiler3->Compile(&sourceBlob, compilerParams.data(), compilerParams.size(), nullptr, IID_PPV_ARGS(&compileResult));

  if (FAILED(compileEC))
    result.errorLog.append_sprintf("dxil::compileHLSLWithDXC: Error code from compiler: %s\n", eastl::to_string(compileEC).c_str());

  if (!compileResult)
    return result;

  compileResult->GetStatus(&compileEC);
  if (FAILED(compileEC))
    result.errorLog.append_sprintf("dxil::compileHLSLWithDXC: Error code from compile result: %s\n",
      eastl::to_string(compileEC).c_str());

  ComPtr<IDxcBlobUtf8> errorMessages;
  compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorMessages), nullptr);
  if (errorMessages && errorMessages->GetStringLength() != 0)
  {
    // technically errorMessages is utf8, but there is hardly any chance to step outside of ascii
    result.errorLog.append(errorMessages->GetStringPointer(), errorMessages->GetStringPointer() + errorMessages->GetStringLength());
    result.errorLog += "\n";
  }

  if (SUCCEEDED(compileEC))
  {
    ComPtr<IDxcBlobUtf8> preprocessed;
    ComPtr<IDxcBlobUtf16> shaderName;
    compileResult->GetOutput(DXC_OUT_HLSL, IID_PPV_ARGS(&preprocessed), &shaderName);
    // NOTE: it is completely fine for preprocessing result to be empty.
    // Empty file in -- empty file out.
    auto base = reinterpret_cast<const char *>(preprocessed->GetBufferPointer());
    result.preprocessedSource.assign(base, base + preprocessed->GetBufferSize());
  }

  return result;
}

eastl::string dxil::disassemble(eastl::span<const uint8_t> data, void *dxc_lib)
{
  eastl::string result;

  if (!dxc_lib)
  {
    result = "dxil::disassemble: no valid shader compiler library handle provided";
    ;
  }
  else
  {
    DxcCreateInstanceProc DxcCreateInstance = nullptr;
    reinterpret_cast<FARPROC &>(DxcCreateInstance) = GetProcAddress(static_cast<HMODULE>(dxc_lib), "DxcCreateInstance");
    // on scdxil.dll this is named differently but anything else remains the same for disassembly
    if (!DxcCreateInstance)
      reinterpret_cast<FARPROC &>(DxcCreateInstance) = GetProcAddress(static_cast<HMODULE>(dxc_lib), "SCDxil_DxcCreateInstance");

    if (!DxcCreateInstance)
    {
      result = "dxil::disassemble: Missing entry point in DxcCreateInstance "
               "dxcompiler(_x) library";
    }
    else
    {

      ComPtr<IDxcCompiler> compiler;
      DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

      if (!compiler)
      {
        // error message
        result = "dxil::disassemble: DxcCreateInstance failed";
      }
      else
      {
        WrappedBlob moduleBlob{data};

        IDxcBlobEncoding *assembly = nullptr;
        compiler->Disassemble(&moduleBlob, &assembly);
        if (!assembly)
        {
          result = "dxil::disassemble: IDxcCompiler::Disassemble returned null";
        }
        else
        {
          result.assign((const char *)assembly->GetBufferPointer(), assembly->GetBufferSize());
        }
      }
    }
  }

  return result;
}

namespace
{
namespace llvm
{
struct MetaNameID
{
  eastl::string_view name;

  bool operator==(const MetaNameID &) const = default;
};
struct MetaID
{
  eastl::string_view id;

  bool operator==(const MetaID &) const = default;
};
struct MetaStringConstant
{
  eastl::string_view string;

  bool operator==(const MetaStringConstant &) const = default;
};
struct MetaBooleanConstant
{
  eastl::string_view value;

  bool operator==(const MetaBooleanConstant &) const = default;
};
struct MetaIntegerConstant
{
  eastl::string_view value;
  uint32_t bits;

  bool operator==(const MetaIntegerConstant &) const = default;
};
struct MetaTypedID
{
  eastl::string_view type;
  eastl::string_view id;

  bool operator==(const MetaTypedID &) const = default;
};
struct MetaSSAID
{
  eastl::string_view id;

  bool operator==(const MetaSSAID &) const = default;
};
struct MetaUnhandled
{
  eastl::string_view value;

  bool operator==(const MetaUnhandled &) const = default;
};
using MetaDataValue = eastl::variant<decltype(nullptr), MetaNameID, MetaID, MetaStringConstant, MetaBooleanConstant,
  MetaIntegerConstant, MetaUnhandled, MetaTypedID, MetaSSAID>;
using MetaDataList = eastl::vector<MetaDataValue>;
struct MetaData
{
  struct Entry
  {
    eastl::string_view name;
    MetaDataList list;
  };
  eastl::vector<Entry> entries;
  void add(eastl::string_view name, MetaDataList list) { entries.emplace_back(name, eastl::move(list)); }
  const MetaDataList *get(eastl::string_view name) const
  {
    auto at = eastl::find_if(entries.begin(), entries.end(), [name](auto &e) { return name == e.name; });
    if (entries.end() != at)
    {
      return &at->list;
    }
    return nullptr;
  }
};

void skip_spaces(eastl::string_view &view) { view.remove_prefix(view.find_first_not_of(' ')); }

bool is_num(char c) { return c >= '0' && c <= '9'; }

void log_line(eastl::string_view dasm, eastl::string &log, const char *info)
{
  auto lineEnd = dasm.find('\n');
  auto lineView = dasm.substr(0, lineEnd);
  eastl::string lineStr{lineView.begin(), lineView.end()};
  log.append_sprintf("llvm::log: %s '%s'\n", info, lineStr.c_str());
}

bool parse_meta_value(eastl::string_view &dasm, MetaDataValue &value, eastl::string &log)
{
  skip_spaces(dasm);

  if (dasm.starts_with("null"))
  {
    value = nullptr;
    dasm.remove_prefix(4);
    return true;
  }
  else if ('!' == dasm.front())
  {
    if ('"' == dasm[1])
    {
      auto sEnd = dasm.find('"', 2);
      if (eastl::string_view::npos != sEnd)
      {
        MetaStringConstant c;
        c.string = dasm.substr(0, sEnd + 1);
        dasm.remove_prefix(sEnd);
        value = c;
        return true;
      }
      auto lineEnd = dasm.find('\n');
      auto lineView = dasm.substr(0, lineEnd);
      log.append_sprintf("llvm::parse_meta_value: Parse error open string constant '");
      log.append(lineView.begin(), lineView.end());
      log.append("'\n'");
      return false;
    }
    else
    {
      if (is_num(dasm[1]))
      {
        auto numEnd = dasm.find_first_not_of("0123456789", 1);
        if (eastl::string_view::npos != numEnd)
        {
          MetaID id;
          id.id = dasm.substr(0, numEnd);
          dasm.remove_prefix(numEnd);
          value = id;
          return true;
        }
      }
      else
      {
        auto nameEnd = dasm.find_first_of(' =,}');
        if (eastl::string_view::npos != nameEnd)
        {
          MetaNameID id;
          id.name = dasm.substr(0, nameEnd);
          dasm.remove_prefix(nameEnd);
          value = id;
          return true;
        }
      }
    }
  }
  else if ('%' == dasm.front())
  {
    auto defEnd = dasm.find_first_of("=,}");
    auto secondDef = dasm.find('%', 1);
    if (secondDef < defEnd)
    {
      MetaTypedID typed;
      typed.type = dasm.substr(0, secondDef);
      typed.id = dasm.substr(secondDef, defEnd - secondDef);
      while (' ' == typed.id.back())
      {
        typed.id.remove_suffix(1);
      }
      while (' ' == typed.type.back())
      {
        typed.type.remove_suffix(1);
      }
      value = typed;
    }
    else
    {
      MetaSSAID u;
      u.id = dasm.substr(0, defEnd);
      dasm.remove_prefix(defEnd);
      while (' ' == u.id.back())
      {
        u.id.remove_suffix(1);
      }
      value = u;
    }
    return true;
  }
  else if ('i' == dasm.front())
  {
    dasm.remove_prefix(1);
    auto valueEnd = dasm.find_first_not_of("0123456789");
    auto valueType = dasm.substr(0, valueEnd);
    dasm.remove_prefix(valueEnd);
    skip_spaces(dasm);

    if (valueType == "1")
    {
      MetaBooleanConstant b;
      if (dasm.starts_with("true"))
      {
        b.value = dasm.substr(0, 4);
        dasm.remove_prefix(4);
      }
      else if (dasm.starts_with("false"))
      {
        b.value = dasm.substr(0, 5);
        dasm.remove_prefix(5);
      }
      else
      {
        auto lineEnd = dasm.find('\n');
        auto lineView = dasm.substr(0, lineEnd);
        log.append_sprintf("llvm::parse_meta_value: Parse error unknown bool value '");
        log.append(lineView.begin(), lineView.end());
        log.append("'\n'");
        return false;
      }

      value = b;
      return true;
    }
    else if (valueType == "8" || valueType == "32" || valueType == "64")
    {
      auto valueEnd = dasm.find_first_not_of("0123456789");
      MetaIntegerConstant ic;
      ic.bits = valueType == "8" ? 8 : valueType == "32" ? 32 : 64;
      ic.value = dasm.substr(0, valueEnd);
      dasm.remove_prefix(valueEnd);
      value = ic;
      return true;
    }
    else
    {
      log.append_sprintf("llvm::parse_meta_value: Parse error unknown value type '");
      log.append(valueType.begin(), valueType.end());
      log.append("'\n'");
      return false;
    }
  }
  else
  {
    auto defEnd = dasm.find_first_of("=,}");
    MetaUnhandled u;
    u.value = dasm.substr(0, defEnd);
    dasm.remove_prefix(defEnd);
    value = u;
    return true;
  }

  log_line(dasm, log, "llvm::parse_meta_value: unhandled\n");
  return false;
}
bool parse_meta_data_list(eastl::string_view &dasm, MetaDataList &list, eastl::string &log)
{
  auto defBegin = dasm.find('!');
  if (eastl::string_view::npos == defBegin)
  {
    log.append_sprintf("can't find meta data def start '!'");
    return false;
  }
  auto listBegin = dasm.find('{', defBegin);
  if (eastl::string_view::npos == listBegin)
  {
    log.append_sprintf("can't find meta data list start '{'");
    return false;
  }

  dasm.remove_prefix(listBegin + 1);

  for (;;)
  {
    MetaDataValue value;
    if (!parse_meta_value(dasm, value, log))
    {
      log.append_sprintf("parse_meta_value failed\n");
      return false;
    }
    auto loc = dasm.find_first_of(",}");
    if (eastl::string_view::npos == loc)
    {
      log.append_sprintf("unable to find either ',' or '}' for next list entry or end of list");
      return false;
    }
    auto c = dasm[loc];
    list.push_back(eastl::move(value));
    dasm = dasm.substr(loc + 1);
    if ('}' == c)
    {
      return true;
    }
  }
}
bool parse_meta_data_entry(eastl::string_view &dasm, MetaData &set, eastl::string &log)
{
  auto entryStart = dasm.find('!');
  if (eastl::string_view::npos == entryStart)
  {
    return false;
  }
  auto assignment = dasm.find('=', entryStart);
  if (eastl::string_view::npos == assignment)
  {
    return false;
  }
  auto entryName = dasm.substr(entryStart, assignment - entryStart);
  while (' ' == entryName.back())
  {
    entryName.remove_suffix(1);
  }
  dasm.remove_prefix(assignment + 1);
  MetaDataList lst;
  eastl::string entryNameString{entryName.begin(), entryName.end()};
  if (!parse_meta_data_list(dasm, lst, log))
  {
    return false;
  }
  set.add(entryName, eastl::move(lst));
  return true;
}

using NullPointerType = decltype(nullptr);

void print_meta_data_value(NullPointerType, eastl::string &log) { log.append("null"); }

void print_meta_data_value(const MetaNameID &value, eastl::string &log) { log.append(value.name.begin(), value.name.end()); }

void print_meta_data_value(const MetaID &value, eastl::string &log) { log.append(value.id.begin(), value.id.end()); }

void print_meta_data_value(const MetaStringConstant &value, eastl::string &log)
{
  log.append(value.string.begin(), value.string.end());
}

void print_meta_data_value(const MetaBooleanConstant &value, eastl::string &log)
{
  log.append(value.value.begin(), value.value.end());
}

void print_meta_data_value(const MetaIntegerConstant &value, eastl::string &log)
{
  log.append(value.value.begin(), value.value.end());
}

void print_meta_data_value(const MetaTypedID &value, eastl::string &log)
{
  log.append(value.type.begin(), value.type.end());
  log.append(" ");
  log.append(value.id.begin(), value.id.end());
}

void print_meta_data_value(const MetaSSAID &value, eastl::string &log) { log.append(value.id.begin(), value.id.end()); }

void print_meta_data_value(const MetaUnhandled &value, eastl::string &log) { log.append(value.value.begin(), value.value.end()); }

void print_meta_data_value(const MetaDataValue &value, eastl::string &log)
{
  eastl::visit([&log](auto &v) { print_meta_data_value(v, log); }, value);
}

void print_meta_data_list(const MetaDataList &list, eastl::string &log)
{
  for (auto &e : list)
  {
    print_meta_data_value(e, log);
    log += '\n';
  }
}

void print_meta_data(const MetaData &data, eastl::string &log)
{
  for (auto &e : data.entries)
  {
    log.append("Entries for '");
    log.append(e.name.begin(), e.name.end());
    log.append("':\n");
    print_meta_data_list(e.list, log);
  }
}

template <typename T>
bool equals(const MetaDataValue &value, const T &compare)
{
  auto castedValue = eastl::get_if<T>(&value);
  if (!castedValue)
  {
    return false;
  }
  return *castedValue == compare;
}

MetaData parse_meta_data(eastl::string_view dasm, eastl::string &log)
{
  MetaData data;

  auto startPoint = dasm.find("!llvm.ident");
  if (eastl::string_view::npos == startPoint)
  {
    log.append_sprintf("llvm::parse_meta_data: can't find meta data\n");
    return data;
  }

  auto workingRange = dasm.substr(startPoint);
  while (parse_meta_data_entry(workingRange, data, log)) {}

  return data;
}
bool parse_call_argument_list(eastl::string_view &dasm, MetaDataList &list, eastl::string &log)
{
  auto argStart = dasm.find('(');
  if (eastl::string_view::npos == argStart)
  {
    log_line(dasm, log, "parse_call_argument_list eastl::string_view::npos == argStart");
    return false;
  }
  dasm.remove_prefix(argStart + 1);

  for (;;)
  {
    MetaDataValue value;
    if (!parse_meta_value(dasm, value, log))
    {
      log.append_sprintf("parse_meta_value failed\n");
      return false;
    }
    auto loc = dasm.find_first_of(",)");
    if (eastl::string_view::npos == loc)
    {
      log.append_sprintf("unable to find either ',' or ')' for next list entry or end of list");
      return false;
    }
    auto c = dasm[loc];
    list.push_back(eastl::move(value));
    dasm = dasm.substr(loc + 1);
    if (')' == c)
    {
      return true;
    }
  }
}
// returns all the SSA values for all handles that match the resource type and index, most commonly this is just one value
MetaDataList gather_handle_creates_for_resource(eastl::string_view dasm, eastl::string_view resource_type, eastl::string_view index,
  eastl::string &log)
{
  MetaDataList result;
  eastl::string_view searchToken{"@dx.op.createHandle"};
  eastl::string_view callToken{"call"};
  eastl::string_view declareToken{"declare"};
  MetaIntegerConstant createHandleID{.value = "57", .bits = 32};
  MetaIntegerConstant resourceTypeID{.value = resource_type, .bits = 8};
  MetaIntegerConstant resourceIndexID{.value = index, .bits = 32};
  auto pos = dasm.find(searchToken);
  for (; pos != eastl::string_view::npos; pos = dasm.find(searchToken))
  {
    auto declarePos = dasm.rfind(declareToken, pos);
    auto callPos = dasm.rfind(callToken, pos);
    if (eastl::string_view::npos != declarePos && declarePos > callPos)
    {
      // found declaration, skip over it
      dasm = dasm.substr(pos + searchToken.length());
      continue;
    }
    auto equalPos = dasm.rfind('=', callPos);
    if (eastl::string_view::npos == equalPos)
    {
      log_line(dasm, log, "gather_handle_creates_for_resource eastl::string_view::npos == equalPos");
      break;
    }
    auto ssaaPos = dasm.rfind('%', equalPos);
    if (eastl::string_view::npos == ssaaPos)
    {
      log_line(dasm, log, "gather_handle_creates_for_resource eastl::string_view::npos == ssaaPos");
      break;
    }
    auto argListBegin = dasm.find('(', pos);
    if (eastl::string_view::npos == argListBegin)
    {
      log_line(dasm, log, "gather_handle_creates_for_resource eastl::string_view::npos == argListBegin");
      break;
    }

    auto defView = dasm.substr(ssaaPos);
    dasm.remove_prefix(argListBegin);

    MetaDataList argList;
    if (!parse_call_argument_list(dasm, argList, log))
    {
      break;
    }

    if (argList.size() < 5)
    {
      log_line(dasm, log, "gather_handle_creates_for_resource argList.size() < 5");
      break;
    }

    if (!equals(argList[0], createHandleID))
    {
      // sanity check
      log.append_sprintf("argList[0] was not 57");
      continue;
    }

    if (!equals(argList[1], resourceTypeID))
    {
      continue;
    }

    if (!equals(argList[2], resourceIndexID))
    {
      continue;
    }

    MetaDataValue value;
    if (!parse_meta_value(defView, value, log))
    {
      log.append("parse_meta_value failed\n");
      break;
    }
    result.push_back(eastl::move(value));
  }
  return result;
}
template <typename T>
void visit_calls_to(eastl::string_view dasm, eastl::string_view call_name, eastl::string &log, T visitor)
{
  eastl::string_view callToken{"call"};
  eastl::string_view declareToken{"declare"};
  auto pos = dasm.find(call_name);
  for (; pos != eastl::string_view::npos; pos = dasm.find(call_name))
  {
    auto declarePos = dasm.rfind(declareToken, pos);
    auto callPos = dasm.rfind(callToken, pos);
    if (eastl::string_view::npos != declarePos && declarePos > callPos)
    {
      // found declaration, skip over it
      dasm = dasm.substr(pos + call_name.length());
      continue;
    }

    auto argListBegin = dasm.find('(', pos);
    if (eastl::string_view::npos == argListBegin)
    {
      log_line(dasm, log, "visit_calls_to eastl::string_view::npos == argListBegin");
      break;
    }

    dasm.remove_prefix(argListBegin);

    MetaDataList argList;
    if (!parse_call_argument_list(dasm, argList, log))
    {
      break;
    }
    // should pass line begin not current parse location
    visitor(dasm, argList);
  }
}
} // namespace llvm
constexpr size_t invalid_uav_index = ~size_t(0);
size_t find_uav_resource_in_meta_data(const llvm::MetaData &data, eastl::string_view space_index, eastl::string &log)
{
  auto resourceTableRefList = data.get("!dx.resources");
  if (!resourceTableRefList)
  {
    log.append("resourceTableRefList not found\n");
    return invalid_uav_index;
  }
  auto resourceTableRef = eastl::get_if<llvm::MetaID>(&resourceTableRefList->at(0));
  if (!resourceTableRef)
  {
    log.append("resourceTableRef not found\n");
    return invalid_uav_index;
  }
  auto resourceTableList = data.get(resourceTableRef->id);
  if (!resourceTableList)
  {
    log.append("resourceTableList not found\n");
    return invalid_uav_index;
  }

  if (resourceTableList->size() < 2)
  {
    log.append("resourceTableList is invalid\n");
    return invalid_uav_index;
  }

  auto uavTableRef = eastl::get_if<llvm::MetaID>(&resourceTableList->at(1));
  if (!uavTableRef)
  {
    log.append("uavTableRef not found\n");
    return invalid_uav_index;
  }

  auto uavTableList = data.get(uavTableRef->id);
  if (!uavTableList)
  {
    log.append("uavTableRef not found\n");
    return invalid_uav_index;
  }

  size_t uavIndexInTable;
  for (uavIndexInTable = 0; uavIndexInTable < uavTableList->size(); ++uavIndexInTable)
  {
    auto &e = uavTableList->at(uavIndexInTable);
    auto eRef = eastl::get_if<llvm::MetaID>(&e);
    if (!eRef)
    {
      return invalid_uav_index;
    }
    auto eRefList = data.get(eRef->id);
    if (!eRefList)
    {
      return invalid_uav_index;
    }
    if (eRefList->size() < 5)
    {
      continue;
    }
    auto nameSpaceValue = eastl::get_if<llvm::MetaIntegerConstant>(&eRefList->at(3));
    if (!nameSpaceValue)
    {
      continue;
    }
    if (nameSpaceValue->value != space_index)
    {
      continue;
    }
    auto slotValue = eastl::get_if<llvm::MetaIntegerConstant>(&eRefList->at(4));
    if (!slotValue)
    {
      continue;
    }
    if (slotValue->value != "0")
    {
      continue;
    }
    break;
  }

  if (uavIndexInTable >= uavTableList->size())
  {
    return invalid_uav_index;
  }

  return uavIndexInTable;
}
} // namespace

uint16_t dxil::parse_NVIDIA_extension_use(eastl::string_view dxil_asm, eastl::string &log)
{
  uint16_t result = 0;

  auto metaData = llvm::parse_meta_data(dxil_asm, log);
  auto uavIndexInTable = find_uav_resource_in_meta_data(metaData, "99", log);

  if (invalid_uav_index == uavIndexInTable)
  {
    log.append("Can't find NVIDIA magic uav resource in LLVM meta data\n");
    return result;
  }

  log.append_sprintf("Found NVIDIA magic UAV resource in UAV table at index %u\n", uint32_t(uavIndexInTable));

  eastl::string tableIndexString;
  tableIndexString.sprintf("%u", uint32_t(uavIndexInTable));
  auto handleIds = llvm::gather_handle_creates_for_resource(dxil_asm, "1", tableIndexString, log);

  if (handleIds.empty())
  {
    log.append("Can't locate create handle calls for NVIDIA magic UAV resource\n");
    return result;
  }

  bool hadBadOpCode = false;
  // Nvidia surrounds the opcode recording with '@dx.op.bufferUpdateCounter' calls.
  // The instruction Id will be encoded with 'call void @dx.op.rawBufferStore.i32(i32 140, <id of the UAV handle>, <beginning
  // bufferUpdateCounter call ID>, <member index has to be i32 0>, <opcode as i32>, i32 undef, i32 undef, i32 undef, i8 1, i32 4)'
  // There are also instructions with sub opcodes, like atomic ops, but we don't need to know which kind of atomic op is used only that
  // a atomic op of a certain kind was used
  auto visitor = [&handleIds, &log, &hadBadOpCode, &result](auto line, auto &param_list) {
    if (param_list.size() < 5)
    {
      return;
    }

    auto resourcRef = eastl::get_if<llvm::MetaTypedID>(&param_list[1]);
    if (!resourcRef)
    {
      log.append("resourcRef failed\n");
      return;
    }

    auto idRef = eastl::find_if(handleIds.begin(), handleIds.end(), [resourcRef, &log](auto &value) {
      auto valueID = eastl::get_if<llvm::MetaSSAID>(&value);
      if (!valueID)
      {
        log.append("valueID failed\n");
        return false;
      }
      return valueID->id == resourcRef->id;
    });
    if (handleIds.end() == idRef)
    {
      return;
    }

    llvm::MetaIntegerConstant opCodeLocation{.value = "0", .bits = 32};
    if (!equals(param_list[3], opCodeLocation))
    {
      return;
    }

    auto endcodedOp = eastl::get_if<llvm::MetaIntegerConstant>(&param_list[4]);
    if (!endcodedOp)
    {
      log.append("endcodedOp failed\n");
      return;
    }

    eastl::string numberValueString{endcodedOp->value.begin(), endcodedOp->value.end()};
    uint32_t nvidiaOpCode = 0;
    // FIXME: don't use scanf!
    sscanf(numberValueString.c_str(), "%u", &nvidiaOpCode);
    auto table = dxil::get_nvidia_extension_op_code_to_extension_bit_table();
    auto opCodeRef = eastl::find_if(table.begin(), table.end(), [nvidiaOpCode](auto &e) { return e.opCode == nvidiaOpCode; });
    if (table.end() == opCodeRef)
    {
      log.append_sprintf("Error: Unknown NVIDIA OpCode 0x%02X found\n", nvidiaOpCode);
      hadBadOpCode = true;
    }
    else
    {
      result |= opCodeRef->extensionBit;
    }
  };
  llvm::visit_calls_to(dxil_asm, "@dx.op.bufferStore", log, visitor);
  llvm::visit_calls_to(dxil_asm, "@dx.op.rawBufferStore", log, visitor);
  // set to 0 to force error
  if (hadBadOpCode)
  {
    result = 0;
  }

  return result;
}

#define AmdExtD3DShaderIntrinsicsOpcode_Readfirstlane       0x01
#define AmdExtD3DShaderIntrinsicsOpcode_Readlane            0x02
#define AmdExtD3DShaderIntrinsicsOpcode_LaneId              0x03
#define AmdExtD3DShaderIntrinsicsOpcode_Swizzle             0x04
#define AmdExtD3DShaderIntrinsicsOpcode_Ballot              0x05
#define AmdExtD3DShaderIntrinsicsOpcode_MBCnt               0x06
#define AmdExtD3DShaderIntrinsicsOpcode_Min3U               0x07
#define AmdExtD3DShaderIntrinsicsOpcode_Min3F               0x08
#define AmdExtD3DShaderIntrinsicsOpcode_Med3U               0x09
#define AmdExtD3DShaderIntrinsicsOpcode_Med3F               0x0a
#define AmdExtD3DShaderIntrinsicsOpcode_Max3U               0x0b
#define AmdExtD3DShaderIntrinsicsOpcode_Max3F               0x0c
#define AmdExtD3DShaderIntrinsicsOpcode_BaryCoord           0x0d
#define AmdExtD3DShaderIntrinsicsOpcode_VtxParam            0x0e
#define AmdExtD3DShaderIntrinsicsOpcode_Reserved1           0x0f
#define AmdExtD3DShaderIntrinsicsOpcode_Reserved2           0x10
#define AmdExtD3DShaderIntrinsicsOpcode_Reserved3           0x11
#define AmdExtD3DShaderIntrinsicsOpcode_WaveReduce          0x12
#define AmdExtD3DShaderIntrinsicsOpcode_WaveScan            0x13
#define AmdExtD3DShaderIntrinsicsOpcode_LoadDwAtAddr        0x14
#define AmdExtD3DShaderIntrinsicsOpcode_DrawIndex           0x17
#define AmdExtD3DShaderIntrinsicsOpcode_AtomicU64           0x18
#define AmdExtD3DShaderIntrinsicsOpcode_GetWaveSize         0x19
#define AmdExtD3DShaderIntrinsicsOpcode_BaseInstance        0x1a
#define AmdExtD3DShaderIntrinsicsOpcode_BaseVertex          0x1b
#define AmdExtD3DShaderIntrinsicsOpcode_FloatConversion     0x1c
#define AmdExtD3DShaderIntrinsicsOpcode_ReadlaneAt          0x1d
#define AmdExtD3DShaderIntrinsicsOpcode_ShaderClock         0x1f
#define AmdExtD3DShaderIntrinsicsOpcode_ShaderRealtimeClock 0x20

using AmdOpCodeTableEntry = dxil::NvidiaExtensionOpCodeToExtensionBitEntry;

static const AmdOpCodeTableEntry AmdOpCodeTable[] = {
  {AmdExtD3DShaderIntrinsicsOpcode_Readfirstlane, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_Readlane, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_LaneId, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_Swizzle, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_Ballot, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_MBCnt, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_Min3U, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_Min3F, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_Med3U, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_Med3F, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_Max3U, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_Max3F, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_BaryCoord, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_VtxParam, dxil::VendorExtensionBits::AMD_INTRISICS_16},
  {AmdExtD3DShaderIntrinsicsOpcode_WaveReduce, dxil::VendorExtensionBits::AMD_INTRISICS_17},
  {AmdExtD3DShaderIntrinsicsOpcode_WaveScan, dxil::VendorExtensionBits::AMD_INTRISICS_17},
  {AmdExtD3DShaderIntrinsicsOpcode_LoadDwAtAddr, dxil::VendorExtensionBits::AMD_INTRISICS_19},
  {AmdExtD3DShaderIntrinsicsOpcode_DrawIndex, dxil::VendorExtensionBits::AMD_INTRISICS_19},
  {AmdExtD3DShaderIntrinsicsOpcode_AtomicU64, dxil::VendorExtensionBits::AMD_INTRISICS_19},
  {AmdExtD3DShaderIntrinsicsOpcode_GetWaveSize, dxil::VendorExtensionBits::AMD_GET_WAVE_SIZE},
  {AmdExtD3DShaderIntrinsicsOpcode_BaseInstance, dxil::VendorExtensionBits::AMD_GET_BASE_INSTANCE},
  {AmdExtD3DShaderIntrinsicsOpcode_BaseVertex, dxil::VendorExtensionBits::AMD_GET_BASE_VERTEX},
  {AmdExtD3DShaderIntrinsicsOpcode_FloatConversion, dxil::VendorExtensionBits::AMD_FLOAT_CONVERSION},
  {AmdExtD3DShaderIntrinsicsOpcode_ReadlaneAt, dxil::VendorExtensionBits::AMD_READ_LINE_AT},
  {AmdExtD3DShaderIntrinsicsOpcode_ShaderClock, dxil::VendorExtensionBits::AMD_SHADER_CLOCK},
  {AmdExtD3DShaderIntrinsicsOpcode_ShaderRealtimeClock, dxil::VendorExtensionBits::AMD_SHADER_CLOCK},
};

// TODO: Currently this can not detect AMD_DXR_RAY_HIT_TOKEN
uint16_t dxil::parse_AMD_extension_use(eastl::string_view dxil_asm, eastl::string &log)
{
  uint16_t result = 0;

  auto metaData = llvm::parse_meta_data(dxil_asm, log);
  auto uavIndexInTable = find_uav_resource_in_meta_data(metaData, "2147420894", log);

  if (invalid_uav_index == uavIndexInTable)
  {
    log.append("Can't find AMD magic uav resource in LLVM meta data\n");
    return result;
  }

  log.append_sprintf("Found AMD magic UAV resource in UAV table at index %u\n", uint32_t(uavIndexInTable));

  eastl::string tableIndexString;
  tableIndexString.sprintf("%u", uint32_t(uavIndexInTable));
  auto handleIds = llvm::gather_handle_creates_for_resource(dxil_asm, "1", tableIndexString, log);

  if (handleIds.empty())
  {
    log.append("Can't locate create handle calls for AMD magic UAV resource\n");
    return result;
  }

  bool hadBadOpCode = false;
  // AMD encodes recording of opcodes with @dx.op.atomicCompareExchange.i32 with the first parameter being the encoded opcode which has
  // to be decoded The base opcode is the lower byte (eg & 0xFF to retrieve).
  llvm::visit_calls_to(dxil_asm, "@dx.op.atomicCompareExchange", log,
    [&handleIds, &log, &hadBadOpCode, &result](auto line, auto &param_list) {
      if (param_list.size() < 3)
      {
        return;
      }

      auto resourcRef = eastl::get_if<llvm::MetaTypedID>(&param_list[1]);
      if (!resourcRef)
      {
        log.append("resourcRef failed\n");
        return;
      }

      auto idRef = eastl::find_if(handleIds.begin(), handleIds.end(), [resourcRef, &log](auto &value) {
        auto valueID = eastl::get_if<llvm::MetaSSAID>(&value);
        if (!valueID)
        {
          log.append("valueID failed\n");
          return false;
        }
        return valueID->id == resourcRef->id;
      });
      if (handleIds.end() == idRef)
      {
        return;
      }

      auto endcodedOp = eastl::get_if<llvm::MetaIntegerConstant>(&param_list[2]);
      if (!endcodedOp)
      {
        log.append("endcodedOp failed\n");
        return;
      }

      eastl::string numberValueString{endcodedOp->value.begin(), endcodedOp->value.end()};
      uint32_t endocedOpValue = 0;
      // FIXME: don't use scanf!
      sscanf(numberValueString.c_str(), "%u", &endocedOpValue);
      uint32_t amdOpCode = endocedOpValue & 0xFF;
      auto opCodeRef = eastl::find_if(eastl::begin(AmdOpCodeTable), eastl::end(AmdOpCodeTable),
        [amdOpCode](auto &e) { return e.opCode == amdOpCode; });
      if (eastl::end(AmdOpCodeTable) == opCodeRef)
      {
        log.append_sprintf("Error: Unknown AMD OpCode 0x%02X found\n", amdOpCode);
        hadBadOpCode = true;
      }
      else
      {
        result |= opCodeRef->extensionBit;
      }
    });

  // set to 0 to force error
  if (hadBadOpCode)
  {
    result = 0;
  }

  return result;
}
