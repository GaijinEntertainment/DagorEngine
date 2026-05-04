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

bool write_to_text_file(auto blob, const eastl::wstring &path)
{
  FILE *fp = nullptr;
  _wfopen_s(&fp, path.c_str(), L"w");
  if (!fp)
    return false;

  const char *p = (const char *)blob->GetBufferPointer();
  size_t len = blob->GetBufferSize();
  while (len > 0 && p[len - 1] == '\0')
    --len;

  fwrite(p, len, 1, fp);
  fclose(fp);
  return true;
}

// turns profile string from char to wchar and ensures its 6.0 or later
// also returns version as fixed point 16bit.16bit value or 0 on error
uint32_t translate_profile(const char *in_profile, wchar_t *out_profile, uint32_t out_char_max)
{
  if (0 == strncmp("lib_", in_profile, 4))
  {
    if (out_char_max >= 8)
    {
      // lib is converted as is
      out_profile[0] = L'l';
      out_profile[1] = L'i';
      out_profile[2] = L'b';
      out_profile[3] = L'_';
      out_profile[4] = in_profile[4];
      out_profile[5] = L'_';
      out_profile[6] = in_profile[6];
      out_profile[7] = L'\0';

      return (uint32_t(in_profile[4] - '0') << 16) | uint32_t(in_profile[6] - '0');
    }
  }
  else
  {
    if (out_char_max >= 7)
    {
      out_profile[0] = in_profile[0];
      out_profile[1] = in_profile[1];
      out_profile[2] = L'_';
      // if 6.x or greater requested, overwrite defaults
      out_profile[3] = in_profile[3] >= '6' ? in_profile[3] : L'6';
      out_profile[4] = L'_';
      out_profile[5] = in_profile[3] >= '6' ? in_profile[5] : L'0';
      out_profile[6] = L'\0';

      return (uint32_t(in_profile[3] - '0') << 16) | uint32_t(in_profile[5] - '0');
    }
  }
  return 0;
}

CompileResult compile(IDxcCompiler3 *compiler, UINT32 major, UINT32 minor, WrappedBlob blob, const char *entry, const char *profile,
  const DXCSettings &settings, DXCVersion dxc_version)
{
  // everything is wstring, entry is ascii, which is a subset so no conversion calls needed...
  eastl::vector<wchar_t> entryBuf;
  for (int i = 0; entry[i]; ++i)
    entryBuf.push_back(entry[i]);
  entryBuf.push_back('\0');

  wchar_t targetProfile[8];
  const uint32_t packedProfileVersion = translate_profile(profile, targetProfile, 8);
  const uint32_t minorProfileVersion = packedProfileVersion & 0xFFFF;
  const uint32_t majorProfileVersion = packedProfileVersion >> 16;

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

  if (settings.enableFp16 && (majorProfileVersion > 6 || (majorProfileVersion == 6 && minorProfileVersion >= 2)))
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

    if (write_to_text_file(&blob, hlslPath))
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
            // @NOTE: we can't consider this an error, or a warning (due to -wx), because dsc cache works on source
            // with hlsl macros, and different sources may become identical after preprocessing
            // @TODO: remove if we make preproc our own
            result.messageLog.append("dxil::compileHLSLWithDXC: Unable to store PDB at ");
            // truncates wchars to chars
            eastl::copy(begin(pdbPath), end(pdbPath), eastl::back_inserter(result.messageLog));
            result.messageLog.append(", probably due to identical compiled shaders");
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
            // @NOTE: we can't consider this an error, or a warning (due to -wx), because dsc cache works on source
            // with hlsl macros, and different sources may become identical after preprocessing
            // @TODO: remove if we make preproc our own
            result.messageLog.append("dxil::compileHLSLWithDXC: Unable to store CSO at ");
            // truncates wchars to chars
            eastl::copy(begin(csoPath), end(csoPath), eastl::back_inserter(result.messageLog));
            result.messageLog.append(", probably due to identical compiled shaders");
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

PreprocessResult dxil::preprocessHLSLWithDXC(const char *profile, dag::ConstSpan<char> src, dag::ConstSpan<DXCDefine> defines,
  void *dxc_lib)
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
  compilerParams.reserve(4 + 2 * defines.size());

  wchar_t targetProfile[8];
  translate_profile(profile, targetProfile, 8);

  compilerParams.emplace_back(L"-T");
  compilerParams.emplace_back(targetProfile);

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

namespace llvm
{
eastl::string_view parse_name(eastl::string_view &parse_range)
{
  parse_range = parse_range.substr(parse_range.find_first_not_of(" \t\t"));
  size_t ofs = 0;
  if ('@' == parse_range[ofs] || '%' == parse_range[ofs])
  {
    ++ofs;
  }
  if ('"' == parse_range[ofs] || '<' == parse_range[ofs])
  {
    auto terminating = '<' == parse_range[ofs] ? '>' : '"';
    for (++ofs; ofs < parse_range.size(); ++ofs)
    {
      if (terminating != parse_range[ofs])
      {
        continue;
      }
      if ('\\' == parse_range[ofs - 1])
      {
        continue;
      }
      auto name = parse_range.substr(0, ofs + 1);
      parse_range.remove_prefix(ofs + 1);
      return name;
    }
  }
  else
  {
    auto e = parse_range.find_first_of("*,(){} \n", ofs);
    auto name = parse_range.substr(0, e);
    parse_range.remove_prefix(e);
    return name;
  }
  return {};
}
class IL
{
public:
  struct MetaId
  {
    eastl::string_view value;
  };
  struct MetaInt
  {
    eastl::string_view value;
    uint32_t bits;
  };
  struct MetaString
  {
    eastl::string_view value;
  };
  struct MetaExpression
  {
    eastl::string_view value;
  };
  using MetaValue = eastl::variant<eastl::monostate, decltype(nullptr), MetaId, MetaInt, MetaString, MetaExpression>;
  // key = <value set>
  eastl::vector<MetaValue> &addMetaData(eastl::string_view key)
  {
    auto &e = metaData.emplace_back();
    e.key = key;
    return e.data;
  }
  const eastl::vector<MetaValue> *getMetaData(eastl::string_view key)
  {
    auto ref = eastl::find_if(metaData.begin(), metaData.end(), [key](auto &e) { return key == e.key; });
    if (metaData.end() != ref)
    {
      return &ref->data;
    }
    return nullptr;
  }

  void addGlobal(eastl::string_view name) { types.push_back(name); }
  void addType(eastl::string_view name) { types.push_back(name); }

  struct Expression
  {
    // may be empty, when no name was assigned (eg call without result, if statements and so on)
    eastl::string_view name;
    // empty when name is empty, otherwise has to be defined
    eastl::string_view type;
    // never be empty
    eastl::string_view op;
    // may be empty, depends on op
    eastl::vector<eastl::string_view> params;
  };
  struct Function
  {
    eastl::string type;
    eastl::vector<eastl::string_view> params;
    eastl::vector<Expression> expressions;
  };

  Function &addFunction(eastl::string_view name)
  {
    auto &e = functions.emplace_back();
    e.name = name;
    return e.function;
  }

  template <typename T>
  void visitFunctions(T clb)
  {
    for (auto &f : functions)
    {
      clb(f.name, f.function);
    }
  }

private:
  struct MetaDataEntry
  {
    eastl::string_view key;
    eastl::vector<MetaValue> data;
  };
  struct FunctionEntry
  {
    eastl::string_view name;
    Function function;
  };
  eastl::vector<MetaDataEntry> metaData;
  eastl::vector<eastl::string_view> types;
  eastl::vector<eastl::string_view> globals;
  eastl::vector<FunctionEntry> functions;
};
class AsmParser
{
  eastl::string_view assembly;
  eastl::string_view parseRange;
  eastl::string &log;

  bool expect(const char c)
  {
    skipSpaces();
    if (c != parseRange.front())
    {
      log.append_sprintf("expect %c but found: ", c);
      logLine();
      log.append("\n");
      return false;
    }
    parseRange.remove_prefix(1);
    return true;
  }
  bool expect(const char *str)
  {
    skipSpaces();
    if (!parseRange.starts_with(str))
    {
      log.append_sprintf("expect %s but found: ", str);
      logLine();
      log.append("\n");
      return false;
    }
    parseRange.remove_prefix(strlen(str));
    return true;
  }
  bool check(const char *str)
  {
    skipSpaces();
    if (parseRange.starts_with(str))
    {
      parseRange.remove_prefix(strlen(str));
      return true;
    }
    return false;
  }
  eastl::string_view parseName() { return parse_name(parseRange); }
  eastl::string_view peekName()
  {
    auto cpy = parseRange;
    return parse_name(cpy);
  }
  bool nextIsLocal()
  {
    skipSpaces();
    return '%' == parseRange.front();
  }
  bool nextIsGlobal()
  {
    skipSpaces();
    return '@' == parseRange.front();
  }

  IL::MetaValue parseMetaValue()
  {
    auto e = parseRange.find_first_of(",}");
    if (eastl::string_view::npos == e)
    {
      log.append("Unexpected end while parsing meta value: ");
      logLine();
      log.append("\n");
      return eastl::monostate{};
    }
    auto valueRange = parseRange.substr(0, e);
    if (',' == parseRange[e])
    {
      parseRange.remove_prefix(1);
    }
    parseRange.remove_prefix(e);
    if ('!' == valueRange.front())
    {
      if ('"' == valueRange[1])
      {
        return IL::MetaString{
          .value = valueRange.substr(2, valueRange.size() - 3),
        };
      }
      else
      {
        return IL::MetaId{
          .value = valueRange,
        };
      }
    }
    else if ('i' == valueRange.front())
    {
      // integer value
      valueRange.remove_prefix(1);
      auto be = valueRange.find(' ');
      auto bits = valueRange.substr(0, be);
      auto val = valueRange.find_first_not_of(' ', be);
      uint32_t bitCount = 1;
      if ("1" == bits)
      {
        bitCount = 1;
      }
      else if ("8" == bits)
      {
        bitCount = 8;
      }
      else if ("16" == bits)
      {
        bitCount = 16;
      }
      else if ("32" == bits)
      {
        bitCount = 32;
      }
      else if ("64" == bits)
      {
        bitCount = 64;
      }
      return IL::MetaInt{
        .value = valueRange.substr(val),
        .bits = bitCount,
      };
    }
    else if ("null" == valueRange)
    {
      return nullptr;
    }
    else
    {
      // everything else is a expression
      return IL::MetaExpression{
        .value = valueRange,
      };
    }
    return eastl::monostate{};
  }

  void logLine()
  {
    auto lineEnd = parseRange.find_first_of('\n');
    auto line = parseRange.substr(0, lineEnd + 1);
    log.append(line.begin(), line.end());
  }

  void skipSpaces() { parseRange.remove_prefix(parseRange.find_first_not_of(" \r\t")); }

  bool parseComment()
  {
    parseRange.remove_prefix(parseRange.find_first_of('\n') + 1);
    return true;
  }
  bool parseGlobalVariable(IL &target)
  {
    // types are:
    // @<name> = <definition>
    auto globalName = parseName();
    if (globalName.empty())
    {
      return false;
    }
    if (!expect('='))
    {
      return false;
    }

    // skip rest, don't care about actual variable declaration
    skipLine();
    target.addGlobal(globalName);
    return true;
  }
  bool parseType(IL &target)
  {
    // types are:
    // %<name> = type { <type decl> }
    auto typeName = parseName();
    if (typeName.empty())
    {
      return false;
    }
    if (!expect('='))
    {
      return false;
    }
    if (!expect("type"))
    {
      return false;
    }

    // skip rest, don't care about actual type decl
    skipLine();
    target.addType(typeName);
    return true;
  }
  bool parseComdat(IL &target)
  {
    skipLine();
    return true;
  }
  void logMetaValue(eastl::monostate) { log.append("<mono state>"); }
  void logMetaValue(decltype(nullptr)) { log.append("null"); }
  void logMetaValue(const IL::MetaId &id)
  {
    log.append("ID: '");
    log.append(id.value.begin(), id.value.end());
    log.append("'");
  }
  void logMetaValue(const IL::MetaInt &id)
  {
    log.append_sprintf("INT %u: '", id.bits);
    log.append(id.value.begin(), id.value.end());
    log.append("'");
  }
  void logMetaValue(const IL::MetaString &id)
  {
    log.append("String: '");
    log.append(id.value.begin(), id.value.end());
    log.append("'");
  }
  void logMetaValue(const IL::MetaExpression &id)
  {
    log.append("Expression: '");
    log.append(id.value.begin(), id.value.end());
    log.append("'");
  }
  void logMetaValue(size_t index, const IL::MetaValue &value)
  {
    log.append_sprintf(" [%u] ", index);
    eastl::visit([this](auto &e) { this->logMetaValue(e); }, value);
  }
  bool parseNamedMetaLine(IL &target)
  {
    auto name = parseName();
    if (!expect('='))
    {
      return false;
    }
    // distinct means that there could be multiple instances of the same definition
    check("distinct");
    if (!expect('!'))
    {
      return false;
    }
    if (!expect('{'))
    {
      return false;
    }

    auto &data = target.addMetaData(name);

    for (;;)
    {
      skipSpaces();
      if ('}' == parseRange.front())
      {
        break;
      }
      data.push_back(parseMetaValue());
    }

    skipLine();
    return true;
  }
  bool parseNamedMetaBlock(IL &target)
  {
    // the named meta block are lines starting with !<name> = !{<values>}
    while (!parseRange.empty())
    {
      skipSpaces();
      if ('!' != parseRange.front())
      {
        return true;
      }
      parseNamedMetaLine(target);
    }
    return true;
  }
  bool parseTarget(IL &target)
  {
    skipLine();
    return true;
  }
  bool parseDeclaration(IL &target)
  {
    skipLine();
    return true;
  }
  bool parseFunctionParameterList(IL::Function &target)
  {
    parseRange.remove_prefix(parseRange.find(')') + 1);
    return true;
  }
  bool parseLoad(IL::Expression &target)
  {
    if (!expect(','))
    {
      return false;
    }
    skipSpaces();
    auto e = parseRange.find(',');
    target.params.push_back(parseRange.substr(0, e));
    parseRange.remove_prefix(e);
    return true;
  }
  bool parseCall(IL::Expression &target)
  {
    if (!nextIsGlobal())
    {
      return false;
    }
    auto function = parseName();
    if (function.empty() || '@' != function.front())
    {
      return false;
    }
    if (!expect('('))
    {
      return false;
    }
    target.params.push_back(function);
    while (!parseRange.empty())
    {
      skipSpaces();
      if (')' == parseRange.front())
      {
        parseRange.remove_prefix(1);
        return true;
      }
      auto s = parseRange.find_first_of(",)");
      auto param = parseRange.substr(0, s);
      target.params.push_back(param);
      parseRange.remove_prefix(s);
      if (',' == parseRange.front())
      {
        parseRange.remove_prefix(1);
      }
    }
    return false;
  }
  bool isModifier(eastl::string_view mod) { return "fast" == mod || "nsw" == mod; }
  bool parseFunctionLine(IL::Function &target)
  {
    IL::Expression exp;
    if (nextIsLocal())
    {
      exp.name = parseName();
      if (!expect('='))
      {
        return false;
      }
      skipSpaces();
    }

    exp.op = parseName();

    auto mod = peekName();
    while (isModifier(mod))
    {
      parseRange.remove_prefix(mod.end() - parseRange.begin());
      mod = peekName();
    }

    exp.type = parseName();

    if ("call" == exp.op)
    {
      if (!parseCall(exp))
      {
        return false;
      }
    }
    else if ("load" == exp.op)
    {
      if (!parseLoad(exp))
      {
        return false;
      }
    }
    else
    {
      skipSpaces();
      // TODO parse args
    }
    target.expressions.push_back(exp);
    skipLine();
    return true;
  }
  bool parseFunction(IL &target)
  {
    // define <ret-type> @<name>(<params>) { <body> }

    // skip over define
    parseRange.remove_prefix(6);

    auto retType = parseName();

    // global name
    if (!nextIsGlobal())
    {
      return false;
    }

    auto name = parseName();

    if (!expect('('))
    {
      return false;
    }

    auto &func = target.addFunction(name);
    func.type = retType;

    if (!parseFunctionParameterList(func))
    {
      return false;
    }

    // skip over some optional stuff we don't care
    parseRange.remove_prefix(parseRange.find('{') + 1);
    skipLine();
    while (!parseRange.empty())
    {
      skipSpaces();
      if ('}' == parseRange.front())
      {
        skipLine();
        return true;
      }
      if (!parseFunctionLine(func))
      {
        log.append("!!parseFunctionLine failed\n");
        return false;
      }
    }

    return false;
  }
  bool parseAttributeDefinition(IL &target)
  {
    skipLine();
    return true;
  }
  void skipLine() { parseRange.remove_prefix(parseRange.find_first_of('\n') + 1); }

  bool parseGlobal(IL &target)
  {
    skipSpaces();
    if (parseRange.empty() || (0 == parseRange.front()))
    {
      parseRange = {};
      return false;
    }
    if ('\n' == parseRange.front())
    {
      parseRange.remove_prefix(1);
      return true;
    }
    if (';' == parseRange.front())
    {
      return parseComment();
    }
    else if ('@' == parseRange.front())
    {
      return parseGlobalVariable(target);
    }
    else if ('%' == parseRange.front())
    {
      return parseType(target);
    }
    else if ('$' == parseRange.front())
    {
      return parseComdat(target);
    }
    else if ('!' == parseRange.front())
    {
      return parseNamedMetaBlock(target);
    }
    else if (parseRange.starts_with("target"))
    {
      return parseTarget(target);
    }
    else if (parseRange.starts_with("define"))
    {
      return parseFunction(target);
    }
    else if (parseRange.starts_with("declare"))
    {
      return parseDeclaration(target);
    }
    else if (parseRange.starts_with("attributes"))
    {
      return parseAttributeDefinition(target);
    }
    log.append("Unexpected line: ");
    logLine();
    skipLine();
    log.append(assembly.begin(), assembly.end());

    return false;
  }

public:
  AsmParser(eastl::string_view asam, eastl::string &l) : assembly{asam}, log{l} {}

  IL parse()
  {
    parseRange = assembly;

    IL result;

    while (parseGlobal(result)) {}

    return result;
  }
};
} // namespace llvm

// DXIL specific interpreter of IL data in DXIL context
class DXILInterpreter
{
public:
  struct ShaderProfile
  {
    eastl::string_view name;
    uint32_t major = 0;
    uint32_t minor = 0;
  };
  struct ResourceInfo
  {
    uint32_t typeTableIndex;
    eastl::string_view nameSpaceIndex;
    eastl::string_view typeSlotIndex;
    eastl::string_view name;
    eastl::string_view mangledName;
    eastl::string_view type;
  };

private:
  llvm::IL &il;
  ShaderProfile profile;

  void loadProfile()
  {
    auto root = il.getMetaData("!dx.shaderModel");
    if (!root)
    {
      return;
    }

    auto modelDefName = eastl::get_if<llvm::IL::MetaId>(&root->at(0));
    if (!modelDefName)
    {
      return;
    }

    auto modelDef = il.getMetaData(modelDefName->value);
    if (!modelDef)
    {
      return;
    }

    auto modelName = eastl::get_if<llvm::IL::MetaString>(&modelDef->at(0));
    if (modelName)
    {
      profile.name = modelName->value;
    }

    auto modelMajor = eastl::get_if<llvm::IL::MetaInt>(&modelDef->at(1));
    if (modelMajor)
    {
      eastl::string tmp{modelMajor->value.begin(), modelMajor->value.end()};
      profile.major = atoi(tmp.c_str());
    }

    auto modelMinor = eastl::get_if<llvm::IL::MetaInt>(&modelDef->at(2));
    if (modelMinor)
    {
      eastl::string tmp{modelMinor->value.begin(), modelMinor->value.end()};
      profile.minor = atoi(tmp.c_str());
    }
  }

  // TODO: should not reimplement stuff from llvm parser
  static void parse_resource_def(eastl::string_view def, eastl::string_view &mangled_name, eastl::string_view &type)
  {
    if ('%' != def.front())
    {
      return;
    }

    type = llvm::parse_name(def);
    def.remove_prefix(def.find(' '));
    def.remove_prefix(def.find_first_not_of(' '));
    if (def.starts_with("bitcast"))
    {
      // <type>* bitcast(<source type>* <mangled name> to <type>)
      def.remove_prefix(def.find('*'));
      def.remove_prefix(def.find(' '));
      def.remove_prefix(def.find('@'));
      mangled_name = llvm::parse_name(def);
    }
    else
    {
      // <type>* <mangled name>
      def.remove_prefix(def.find('@'));
      mangled_name = llvm::parse_name(def);
    }
  }

public:
  DXILInterpreter(llvm::IL &i) : il{i} { loadProfile(); }

  const ShaderProfile &shaderProfile() const { return profile; }

  ResourceInfo getExtensionUnorderedResourceInfo(eastl::string_view space, eastl::string_view slot, eastl::string &log)
  {
    ResourceInfo result;
    auto root = il.getMetaData("!dx.resources");
    if (!root)
    {
      return result;
    }

    auto resourceTypeRootName = eastl::get_if<llvm::IL::MetaId>(&root->at(0));
    if (!resourceTypeRootName)
    {
      return result;
    }

    auto resourceTypeRoot = il.getMetaData(resourceTypeRootName->value);
    if (!resourceTypeRoot)
    {
      return result;
    }

    if (resourceTypeRoot->size() < 2)
    {
      return result;
    }

    auto unorderedResourceRootName = eastl::get_if<llvm::IL::MetaId>(&resourceTypeRoot->at(1));
    if (!unorderedResourceRootName)
    {
      return result;
    }

    auto unorderedResourceRoot = il.getMetaData(unorderedResourceRootName->value);
    if (!unorderedResourceRoot)
    {
      return result;
    }

    for (auto &r : *unorderedResourceRoot)
    {
      auto rName = eastl::get_if<llvm::IL::MetaId>(&r);
      if (!rName)
      {
        continue;
      }
      auto resInfo = il.getMetaData(rName->value);
      if (!resInfo)
      {
        continue;
      }

      if (resInfo->size() < 5)
      {
        continue;
      }

      auto nameSpaceIndex = eastl::get_if<llvm::IL::MetaInt>(&resInfo->at(3));
      if (!nameSpaceIndex)
      {
        continue;
      }
      if (space != nameSpaceIndex->value)
      {
        continue;
      }
      auto slotIndex = eastl::get_if<llvm::IL::MetaInt>(&resInfo->at(4));
      if (!slotIndex)
      {
        continue;
      }
      if (slot != slotIndex->value)
      {
        continue;
      }

      auto name = eastl::get_if<llvm::IL::MetaString>(&resInfo->at(2));
      if (!name)
      {
        continue;
      }
      auto resourceDef = eastl::get_if<llvm::IL::MetaExpression>(&resInfo->at(1));
      if (!resourceDef)
      {
        continue;
      }
      result.name = name->value;
      parse_resource_def(resourceDef->value, result.mangledName, result.type);
      result.typeTableIndex = eastl::distance(unorderedResourceRoot->data(), &r);
      result.nameSpaceIndex = space;
      result.typeSlotIndex = slot;

      break;
    }
    return result;
  }

  llvm::IL *operator->() { return &il; }
};

struct BaseSelector
{
  template <typename R, typename E>
  bool selectOpCodeAtomicCompareExchangeStoreOp(const R &res, const E &exp, const E &prev_exp) const
  {
    if ("call" != exp.op)
    {
      return false;
    }
    if (exp.params.size() < 3)
    {
      return false;
    }
    if ("@dx.op.atomicCompareExchange.i32" != exp.params[0])
    {
      return false;
    }
    if (!exp.params[2].ends_with(prev_exp.name))
    {
      return false;
    }
    return true;
  }

  template <typename R, typename E>
  bool selectOpCodeStoreOp(const R &res, const E &exp, const E &prev_exp) const
  {
    if ("call" != exp.op)
    {
      return false;
    }
    if (exp.params.size() < 3)
    {
      return false;
    }
    if ("@dx.op.rawBufferStore.i32" != exp.params[0] && "@dx.op.bufferStore.i32" != exp.params[0])
    {
      return false;
    }
    if (!exp.params[2].ends_with(prev_exp.name))
    {
      return false;
    }
    if ("i32 0" != exp.params[4])
    {
      return false;
    }
    return true;
  }

  template <typename R, typename E>
  bool selectCreateAnnotatedHandle(const R &res, const E &exp, const E &prev_exp) const
  {
    if ("call" != exp.op)
    {
      return false;
    }
    if (exp.params.size() < 3)
    {
      return false;
    }
    if ("@dx.op.annotateHandle" != exp.params[0])
    {
      return false;
    }
    if (!exp.params[2].ends_with(prev_exp.name))
    {
      return false;
    }
    return true;
  }

  template <typename R, typename E>
  bool selectLoadGlobal(const R &res, const E &exp, const E &prev_exp) const
  {
    if ("load" != exp.op)
    {
      return false;
    }
    if (exp.params.empty())
    {
      return false;
    }
    if (eastl::string_view::npos == exp.params[0].find(res.mangledName))
    {
      return false;
    }
    return true;
  }

  template <typename R, typename E>
  bool selectCreateHandle(const R &res, const E &exp, const E &prev_exp) const
  {
    if ("call" != exp.op)
    {
      return false;
    }
    if (exp.params.size() < 3)
    {
      return false;
    }
    if ("@dx.op.createHandle" != exp.params[0])
    {
      return false;
    }
    return true;
  }

  template <typename R, typename E>
  bool selectCreateHandleForLibTemplated(const R &res, const E &exp, const E &prev_exp) const
  {
    if ("call" != exp.op)
    {
      return false;
    }
    if (exp.params.size() < 3)
    {
      return false;
    }
    if (!exp.params[0].starts_with("@\"dx.op.createHandleForLib."))
    {
      return false;
    }
    if (!exp.params[2].starts_with(res.type))
    {
      return false;
    }
    if (!exp.params[2].ends_with(prev_exp.name))
    {
      return false;
    }
    return true;
  }

  template <typename R, typename E>
  bool selectCreateHandleForLib(const R &res, const E &exp, const E &prev_exp) const
  {
    if ("call" != exp.op)
    {
      return false;
    }
    if (exp.params.size() < 3)
    {
      return false;
    }
    if (!exp.params[0].starts_with("@dx.op.createHandleForLib"))
    {
      return false;
    }
    if (!exp.params[2].ends_with(prev_exp.name))
    {
      return false;
    }
    return true;
  }

  template <typename R, typename E>
  bool selectCreateHandleFromBinding(const R &res, const E &exp, const E &prev_exp) const
  {
    if ("call" != exp.op)
    {
      return false;
    }
    if (exp.params.size() < 3)
    {
      return false;
    }
    if (!exp.params[0].starts_with("@dx.op.createHandleFromBinding"))
    {
      return false;
    }
    if (res.typeSlotIndex != exp.params[3].substr(4))
    {
      return false;
    }
    if (res.nameSpaceIndex != exp.params[4].substr(4))
    {
      return false;
    }
    return true;
  }
};

struct NvidiaSelector : BaseSelector
{
  const bool isLib;
  const uint32_t minorVersion;

  uint32_t getPhaseCount() const
  {
    if (isLib)
    {
      return minorVersion < 6 ? 3 : 4;
    }
    else
    {
      return minorVersion < 6 ? 2 : 3;
    }
  }

  template <typename R, typename E>
  bool select(uint32_t phase, const R &res, const E &exp, const E &prev_exp) const
  {
    if (!isLib)
    {
      if (minorVersion < 6)
      {
        if (0 == phase)
        {
          return selectCreateHandle(res, exp, prev_exp);
        }
        if (1 == phase)
        {
          return selectOpCodeStoreOp(res, exp, prev_exp);
        }
      }
      else
      {
        if (0 == phase)
        {
          return selectCreateHandleFromBinding(res, exp, prev_exp);
        }
        if (1 == phase)
        {
          return selectCreateAnnotatedHandle(res, exp, prev_exp);
        }
        if (2 == phase)
        {
          return selectOpCodeStoreOp(res, exp, prev_exp);
        }
      }
    }
    else
    {
      if (minorVersion < 6)
      {
        if (0 == phase)
        {
          return selectLoadGlobal(res, exp, prev_exp);
        }
        if (1 == phase)
        {
          return selectCreateHandleForLibTemplated(res, exp, prev_exp);
        }
        if (2 == phase)
        {
          return selectOpCodeStoreOp(res, exp, prev_exp);
        }
      }
      else
      {
        if (0 == phase)
        {
          return selectLoadGlobal(res, exp, prev_exp);
        }
        if (1 == phase)
        {
          return selectCreateHandleForLib(res, exp, prev_exp);
        }
        if (2 == phase)
        {
          return selectCreateAnnotatedHandle(res, exp, prev_exp);
        }
        if (3 == phase)
        {
          return selectOpCodeStoreOp(res, exp, prev_exp);
        }
      }
    }
    return false;
  }

  template <typename E>
  eastl::optional<uint16_t> opCodeExtractor(const E &exp) const
  {
    auto val = exp.params[5].substr(3);
    auto opCode = atoi(val.begin());
    auto table = dxil::get_nvidia_extension_op_code_to_extension_bit_table();
    auto opCodeRef = eastl::find_if(table.begin(), table.end(), [opCode](auto &e) { return e.opCode == opCode; });
    if (table.end() == opCodeRef)
    {
      return {};
    }
    else
    {
      return {opCodeRef->extensionBit};
    }
  }
};

template <typename E>
void log_exp(eastl::string &log, const E &exp)
{
  log.append("<");
  log.append(exp.name.begin(), exp.name.end());
  log.append("> = <");
  log.append(exp.type.begin(), exp.type.end());
  log.append(">: ");
  log.append(exp.op.begin(), exp.op.end());
  log.append(" (");
  for (auto &p : exp.params)
  {
    log.append(" <");
    log.append(p.begin(), p.end());
    log.append(">");
  }
  log.append(" )\n");
}


template <typename T, typename I>
uint16_t extract_op_codes_phase(DXILInterpreter &dxil, const DXILInterpreter::ResourceInfo &res, eastl::string &log,
  const T &extractor, uint32_t phase, I base, I ed, const llvm::IL::Expression &prev_expr)
{
  const bool isFinalPhase = phase + 1 >= extractor.getPhaseCount();
  uint16_t value = 0;
  bool hadBadOpCode = false;
  for (auto at = base; at != ed; ++at)
  {
    if (!extractor.select(phase, res, *at, prev_expr))
    {
      continue;
    }
    if (!isFinalPhase)
    {
      value |= extract_op_codes_phase(dxil, res, log, extractor, phase + 1, at + 1, ed, *at);
    }
    else
    {
      auto opCode = extractor.opCodeExtractor(*at);
      if (!opCode)
      {
        log.append("Error decoding opcode\n");
        hadBadOpCode = true;
      }
      else
      {
        value |= *opCode;
      }
    }
  }
  if (hadBadOpCode)
  {
    value = 0;
  }
  return value;
}

template <typename T>
uint16_t extract_op_codes(DXILInterpreter &dxil, const DXILInterpreter::ResourceInfo &res, eastl::string &log, const T &extractor)
{
  uint16_t value = 0;
  dxil->visitFunctions([&dxil, &res, &log, &extractor, &value](auto name, auto &def) {
    value |= extract_op_codes_phase(dxil, res, log, extractor, 0, def.expressions.begin(), def.expressions.end(), {});
  });
  return value;
}

uint16_t dxil::parse_NVIDIA_extension_use(eastl::string_view dxil_asm, eastl::string &log)
{
  llvm::AsmParser parser{dxil_asm, log};
  auto il = parser.parse();
  DXILInterpreter dxil{il};
  auto profile = dxil.shaderProfile();
  auto res = dxil.getExtensionUnorderedResourceInfo("99", "0", log);
  if (res.type.empty())
  {
    log.append("failed to find res at space99 u0\n");
  }
  return extract_op_codes(dxil, res, log,
    NvidiaSelector{
      .isLib = "lib" == profile.name,
      .minorVersion = profile.minor,
    });
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

struct AMDSelector : BaseSelector
{
  const bool isLib;
  const uint32_t minorVersion;

  uint32_t getPhaseCount() const
  {
    if (isLib)
    {
      return minorVersion < 6 ? 3 : 4;
    }
    else
    {
      return minorVersion < 6 ? 2 : 3;
    }
  }

  template <typename R, typename E>
  bool select(uint32_t phase, const R &res, const E &exp, const E &prev_exp) const
  {
    if (!isLib)
    {
      if (minorVersion < 6)
      {
        if (0 == phase)
        {
          return selectCreateHandle(res, exp, prev_exp);
        }
        if (1 == phase)
        {
          return selectOpCodeAtomicCompareExchangeStoreOp(res, exp, prev_exp);
        }
      }
      else
      {
        if (0 == phase)
        {
          return selectCreateHandleFromBinding(res, exp, prev_exp);
        }
        if (1 == phase)
        {
          return selectCreateAnnotatedHandle(res, exp, prev_exp);
        }
        if (2 == phase)
        {
          return selectOpCodeAtomicCompareExchangeStoreOp(res, exp, prev_exp);
        }
      }
    }
    else
    {
      if (minorVersion < 6)
      {
        if (0 == phase)
        {
          return selectLoadGlobal(res, exp, prev_exp);
        }
        if (1 == phase)
        {
          return selectCreateHandleForLib(res, exp, prev_exp);
        }
        if (2 == phase)
        {
          return selectOpCodeAtomicCompareExchangeStoreOp(res, exp, prev_exp);
        }
      }
      else
      {
        if (0 == phase)
        {
          return selectLoadGlobal(res, exp, prev_exp);
        }
        if (1 == phase)
        {
          return selectCreateHandleForLib(res, exp, prev_exp);
        }
        if (2 == phase)
        {
          return selectCreateAnnotatedHandle(res, exp, prev_exp);
        }
        if (3 == phase)
        {
          return selectOpCodeAtomicCompareExchangeStoreOp(res, exp, prev_exp);
        }
      }
    }
    return false;
  }

  template <typename E>
  eastl::optional<uint16_t> opCodeExtractor(const E &exp) const
  {
    auto val = exp.params[3].substr(3);
    auto encodedOp = atoi(val.begin());
    uint32_t opCode = encodedOp & 0xFF;
    auto opCodeRef =
      eastl::find_if(eastl::begin(AmdOpCodeTable), eastl::end(AmdOpCodeTable), [opCode](auto &e) { return e.opCode == opCode; });
    if (eastl::end(AmdOpCodeTable) == opCodeRef)
    {
      return {};
    }
    else
    {
      return {opCodeRef->extensionBit};
    }
  }
};

// TODO: Currently this can not detect AMD_DXR_RAY_HIT_TOKEN
uint16_t dxil::parse_AMD_extension_use(eastl::string_view dxil_asm, eastl::string &log)
{
  llvm::AsmParser parser{dxil_asm, log};
  auto il = parser.parse();
  DXILInterpreter dxil{il};
  auto profile = dxil.shaderProfile();
  auto res = dxil.getExtensionUnorderedResourceInfo("2147420894", "0", log);
  if (res.type.empty())
  {
    log.append("failed to find res at space2147420894 u0\n");
  }
  return extract_op_codes(dxil, res, log,
    AMDSelector{
      .isLib = "lib" == profile.name,
      .minorVersion = profile.minor,
    });
}
