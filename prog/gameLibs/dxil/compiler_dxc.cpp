// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <dxil/compiler.h>
#include <supp/dag_comPtr.h>

#include <windows.h>
#include <unknwn.h>
#include <dxc/dxcapi.h>

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

CompileResult compile(IDxcCompiler3 *compiler, UINT32 major, UINT32 minor, WrappedBlob blob, const char *entry, const char *profile,
  DXCSettings settings, DXCVersion dxc_version)
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

  LPCWSTR compilerParams[64] = {};
  uint32_t compilerParamsCount = 0;

  compilerParams[compilerParamsCount++] = L"-T";
  compilerParams[compilerParamsCount++] = targetProfile;

  compilerParams[compilerParamsCount++] = L"-E";
  compilerParams[compilerParamsCount++] = entryBuf.data();

  compilerParams[compilerParamsCount++] = optimizeLevel;
  // old versions don't know this arguments and will abort
  if (major > 1 || minor > 2)
  {
    compilerParams[compilerParamsCount++] = L"-auto-binding-space";
    compilerParams[compilerParamsCount++] = autoSpace;
  }

  if (settings.enableFp16 && (targetProfile[3] > L'6' || (targetProfile[3] == L'6' && targetProfile[5] >= L'2')))
  {
    compilerParams[compilerParamsCount++] = L"-enable-16bit-types";
  }

  if (settings.pdbMode == PDBMode::FULL)
  {
    compilerParams[compilerParamsCount++] = L"-Zss";
    compilerParams[compilerParamsCount++] = L"-Zi";
  }
  else if (settings.pdbMode == PDBMode::SMALL)
  {
    compilerParams[compilerParamsCount++] = L"-Zss";
    compilerParams[compilerParamsCount++] = L"-Zs";
  }

  if (settings.saveHlslToBlob)
  {
    compilerParams[compilerParamsCount++] = L"-Qembed_debug";
  }
  else
  {
    // strip everything from DXIL blob, only thing we need is reflection and debug, but for
    // those we use dedicated blob
    compilerParams[compilerParamsCount++] = L"-Qstrip_debug";
    compilerParams[compilerParamsCount++] = L"-Qstrip_reflect";
    compilerParams[compilerParamsCount++] = L"-Qstrip_rootsignature";
  }


  if (settings.warningsAsErrors)
  {
    compilerParams[compilerParamsCount++] = L"-WX";
  }
  if (settings.enforceIEEEStrictness)
  {
    compilerParams[compilerParamsCount++] = L"-Gis";
  }
  if (settings.disableValidation)
  {
    compilerParams[compilerParamsCount++] = L"-Vd";
  }
  static const wchar_t auto_generated_signature_name[] = L"_AUTO_GENERATED_ROOT_SIGNATURE";
  static const wchar_t auto_generated_signature_define[] = L"_AUTO_GENERATED_ROOT_SIGNATURE=";
  if (!settings.rootSignatureDefine.empty())
  {
    compilerParams[compilerParamsCount++] = L"-rootsig-define";
    compilerParams[compilerParamsCount++] = auto_generated_signature_name;
  }
  if (settings.assumeAllResourcesBound)
  {
    compilerParams[compilerParamsCount++] = L"-all_resources_bound";
  }
  if (settings.hlsl2021)
  {
    compilerParams[compilerParamsCount++] = L"-HV 2021";
  }
  else
  {
    compilerParams[compilerParamsCount++] = L"-HV 2018";
  }

  wchar_t spaceName[] = L"AUTO_DX12_REGISTER_SPACE=space?";
  spaceName[30] = autoSpace[0];
  // AUTO_DX12_REGISTER_SPACE is used to automatically select the register space
  // for all register bindings
  // SHADER_COMPILER_DXC is a hint for the shaders, so that they know
  // which underlying shader compiler is running
  compilerParams[compilerParamsCount++] = L"-D";
  compilerParams[compilerParamsCount++] = spaceName;

  compilerParams[compilerParamsCount++] = L"-D";
  compilerParams[compilerParamsCount++] = L"SHADER_COMPILER_DXC=1";

  if (settings.xboxPhaseOne)
  {
    compilerParams[compilerParamsCount++] = L"-D";
    compilerParams[compilerParamsCount++] = L"__XBOX_DISABLE_PRECOMPILE=1";
  }
  else
  {
    if (!settings.rootSignatureDefine.empty())
    {
      // if we generate final binary, remove dxil
      if (recognizesXboxStripDxilMacro(dxc_version))
      {
        compilerParams[compilerParamsCount++] = L"-D";
        compilerParams[compilerParamsCount++] = L"__XBOX_STRIP_DXIL=1";
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
          compilerParams[compilerParamsCount++] = L"-D";
          compilerParams[compilerParamsCount++] = L"__XBOX_DISABLE_PRECOMPILE_ES=1";
          compilerParams[compilerParamsCount++] = L"-D";
          compilerParams[compilerParamsCount++] = L"__XBOX_DISABLE_PRECOMPILE_GS=1";
        }
      }
      else if (settings.pipelineHasGeoemetryStage)
      {
        // used stages:
        // ES GS VS PS
        compilerParams[compilerParamsCount++] = L"-D";
        compilerParams[compilerParamsCount++] = L"__XBOX_DISABLE_PRECOMPILE_LS=1";
        compilerParams[compilerParamsCount++] = L"-D";
        compilerParams[compilerParamsCount++] = L"__XBOX_DISABLE_PRECOMPILE_HS=1";
      }
      else
      {
        // used stages:
        // VS PS
        compilerParams[compilerParamsCount++] = L"-D";
        compilerParams[compilerParamsCount++] = L"__XBOX_DISABLE_PRECOMPILE_LS=1";
        compilerParams[compilerParamsCount++] = L"-D";
        compilerParams[compilerParamsCount++] = L"__XBOX_DISABLE_PRECOMPILE_HS=1";
        compilerParams[compilerParamsCount++] = L"-D";
        compilerParams[compilerParamsCount++] = L"__XBOX_DISABLE_PRECOMPILE_ES=1";
        compilerParams[compilerParamsCount++] = L"-D";
        compilerParams[compilerParamsCount++] = L"__XBOX_DISABLE_PRECOMPILE_GS=1";
      }

      // Breaks tessellation shader output
      // tested with GDK 200500
      // compilerParams[compilerParamsCount++] = L"-D";
      // compilerParams[compilerParamsCount++] = L"__XBOX_ENABLE_PSPRIMID=1";
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
          compilerParams[compilerParamsCount++] = L"-D";
          compilerParams[compilerParamsCount++] = L"__XBOX_PRECOMPILE_VS_HS=1";
          compilerParams[compilerParamsCount++] = L"-D";
          compilerParams[compilerParamsCount++] = L"__XBOX_ENABLE_HSOFFCHIP=1";

          if (settings.pipelineHasGeoemetryStage)
          {
            compilerParams[compilerParamsCount++] = L"-D";
            compilerParams[compilerParamsCount++] = L"__XBOX_PRECOMPILE_DS_GS=1";
            compilerParams[compilerParamsCount++] = L"-D";
            compilerParams[compilerParamsCount++] = L"__XBOX_ENABLE_GSONCHIP=1";
          }
          else
          {
            compilerParams[compilerParamsCount++] = L"-D";
            compilerParams[compilerParamsCount++] = L"__XBOX_PRECOMPILE_DS_PS=1";
          }
        }
        else if (settings.pipelineHasGeoemetryStage)
        {
          compilerParams[compilerParamsCount++] = L"-D";
          compilerParams[compilerParamsCount++] = L"__XBOX_PRECOMPILE_VS_GS=1";
          compilerParams[compilerParamsCount++] = L"-D";
          compilerParams[compilerParamsCount++] = L"__XBOX_ENABLE_GSONCHIP=1";
        }
        else
        {
          compilerParams[compilerParamsCount++] = L"-D";
          compilerParams[compilerParamsCount++] = L"__XBOX_PRECOMPILE_VS_PS=1";
        }
      }

      // Breaks tessellation shader output
      // tested with GDK 200500
      // compilerParams[compilerParamsCount++] = L"-D";
      // compilerParams[compilerParamsCount++] = L"__XBOX_ENABLE_PSPRIMID=1";

      if (settings.scarlettWaveSize32)
      {
        compilerParams[compilerParamsCount++] = L"-D";
        compilerParams[compilerParamsCount++] = L"__XBOX_ENABLE_WAVE32=1";
      }
    }
  }

  ComPtr<IDxcResult> compileResult;
  auto compileEC = compiler->Compile(&blob, compilerParams, compilerParamsCount, nullptr, IID_PPV_ARGS(&compileResult));

  CompileResult result;
  if (FAILED(compileEC))
  {
    result.logMessage = "dxil::compileHLSLWithDXC: Error code from compiler: " + eastl::to_string(compileEC);
  }

  if (compileResult)
  {
    compileResult->GetStatus(&compileEC);
    if (FAILED(compileEC))
    {
      result.logMessage += "dxil::compileHLSLWithDXC: Error code from compile result: " + eastl::to_string(compileEC) + "\n";
    }
    ComPtr<IDxcBlobUtf8> errorMessages;
    compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorMessages), nullptr);
    if (errorMessages && errorMessages->GetStringLength() != 0)
    {
      // technically errorMessages is utf8, but there is hardly any chance to step outside of ascii
      result.logMessage.append(errorMessages->GetStringPointer(),
        errorMessages->GetStringPointer() + errorMessages->GetStringLength());
      result.logMessage += "\n";

      auto pos = result.logMessage.find("Parameter with semantic");
      if (pos != eastl::string::npos)
      {
        auto pos2 = result.logMessage.find("has overlapping semantic index at");
        if (pos2 != eastl::string::npos && pos2 > pos)
        {
          // give an additional hint, the error message is a bit cryptic
          result.logMessage.append("dxil::compileHLSLWithDXC: NOTE: The error message about "
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
      if ((settings.pdbMode != PDBMode::NONE) && !settings.PDBBasePath.empty())
      {
        ComPtr<IDxcBlob> pdb;
        ComPtr<IDxcBlobUtf16> pdbName;
        compileResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdb), &pdbName);
        if (pdb && pdbName)
        {
          eastl::wstring pdbPath;
          pdbPath.reserve(settings.PDBBasePath.length() + 1 + pdbName->GetStringLength());
          pdbPath.assign(begin(settings.PDBBasePath), end(settings.PDBBasePath));
          if (pdbPath.back() != L'\\' && pdbPath.back() != L'/')
            pdbPath.push_back(L'\\');
          pdbPath.append(pdbName->GetStringPointer(), pdbName->GetStringPointer() + pdbName->GetStringLength());
          FILE *fp = nullptr;
          _wfopen_s(&fp, pdbPath.c_str(), L"wb");
          if (fp)
          {
            fwrite(pdb->GetBufferPointer(), pdb->GetBufferSize(), 1, fp);
            fclose(fp);
          }
          else
          {
            result.logMessage.append("dxil::compileHLSLWithDXC: Unable to store PDB at ");
            // truncates wchars to chars
            eastl::copy(begin(pdbPath), end(pdbPath), eastl::back_inserter(result.logMessage));
          }
        }

        ComPtr<IDxcBlob> cso;
        ComPtr<IDxcBlobUtf16> csoName;
        compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&cso), &csoName);

        // note: for some reason csoName is nullptr here, so pdb name is used instead
        if (cso && pdbName)
        {
          eastl::wstring csoPath;
          csoPath.reserve(settings.PDBBasePath.length() + 1 + pdbName->GetStringLength());
          csoPath.assign(begin(settings.PDBBasePath), end(settings.PDBBasePath));
          if (csoPath.back() != L'\\' && csoPath.back() != L'/')
            csoPath.push_back(L'\\');
          csoPath.append(pdbName->GetStringPointer(), pdbName->GetStringPointer() + pdbName->GetStringLength() - 4);
          csoPath.append(L".cso");
          FILE *fp = nullptr;
          _wfopen_s(&fp, csoPath.c_str(), L"wb");
          if (fp)
          {
            fwrite(cso->GetBufferPointer(), cso->GetBufferSize(), 1, fp);
            fclose(fp);
          }
          else
          {
            result.logMessage.append("dxil::compileHLSLWithDXC: Unable to store CSO at ");
            // truncates wchars to chars
            eastl::copy(begin(csoPath), end(csoPath), eastl::back_inserter(result.logMessage));
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
        result.logMessage.append("dxil::compileHLSLWithDXC: No DXIL byte code object was generated!");
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

CompileResult dxil::compileHLSLWithDXC(dag::ConstSpan<char> src, const char *entry, const char *profile, DXCSettings settings,
  void *dxc_lib, DXCVersion dxc_version)
{
  CompileResult result;
  if (!dxc_lib)
  {
    result.logMessage = "dxil::compileHLSLWithDXC: no valid shader compiler library handle provided";
  }
  else
  {
    DxcCreateInstanceProc DxcCreateInstance = nullptr;
    reinterpret_cast<FARPROC &>(DxcCreateInstance) = GetProcAddress(static_cast<HMODULE>(dxc_lib), "DxcCreateInstance");

    if (!DxcCreateInstance)
    {
      result.logMessage = "dxil::compileHLSLWithDXC: Missing entry point in DxcCreateInstance "
                          "dxcompiler(_x/_xs) library";
    }
    else
    {
      ComPtr<IDxcCompiler3> compiler3;
      DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler3));

      if (!compiler3)
      {
        result.logMessage = "dxil::compileHLSLWithDXC: DxcCreateInstance failed";
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

        // need to inject some things at the top of the shader
        // - bindless expand macro
        // - root signature for xbox
        static const char bindless_macro[] = "#define BINDLESS_REGISTER(type, index) register(type##0, space##index)\n";
        static const char root_sign_def_line[] = "#define _AUTO_GENERATED_ROOT_SIGNATURE ";
        eastl::vector<char> temp;
        size_t reserveSize =
          settings.rootSignatureDefine.size() + src.size() + 1 + sizeof(bindless_macro) + sizeof(root_sign_def_line) - 1;
        temp.insert(end(temp), eastl::begin(bindless_macro), eastl::end(bindless_macro) - 1);
        if (!settings.rootSignatureDefine.empty())
        {
          temp.insert(end(temp), eastl::begin(root_sign_def_line), eastl::end(root_sign_def_line) - 1);
          temp.insert(end(temp), settings.rootSignatureDefine.begin(), settings.rootSignatureDefine.end());
        }
        temp.push_back('\n');
        temp.insert(end(temp), src.begin(), src.end());
        WrappedBlob sourceBlob{dag::ConstSpan<char>(temp.data(), temp.size())};

        return compile(compiler3.Get(), major, minor, sourceBlob, entry, profile, settings, dxc_version);
      }
    }
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