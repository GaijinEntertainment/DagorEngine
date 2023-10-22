#include "HLSL2SpirVCommon.h"
#include "pragmaScanner.h"
#include <regExp/regExp.h>
#include <EASTL/unique_ptr.h>

struct DXCVertexIDPatchRegExps
{
  RegExp patchRe;
  RegExp verifyRe;

  static thread_local eastl::unique_ptr<DXCVertexIDPatchRegExps> inst;

  static DXCVertexIDPatchRegExps *get()
  {
    if (inst)
      return inst.get();

    inst = eastl::make_unique<DXCVertexIDPatchRegExps>();

    if (!inst->patchRe.compile("\\s*([\\w_]+)\\s*\\:\\s*(SV_VertexID)\\s*.*\\).*\\n.*{", "m"))
    {
      inst.reset(nullptr);
      return nullptr;
    }

    if (!inst->verifyRe.compile("\\s*[\\w_]+\\s*\\:\\s*SV_VertexID\\s*\\;", "m"))
    {
      inst.reset(nullptr);
      return nullptr;
    }

    return inst.get();
  }
};

thread_local eastl::unique_ptr<DXCVertexIDPatchRegExps> DXCVertexIDPatchRegExps::inst;


bool fix_vertex_id_for_DXC(std::string &src, CompileResult &output)
{
  // replace every line with corrected construction
  // typical fragment
  //     type name(VsInput input, uint vertex_id : SV_VertexID)
  //     {
  // should be changed to
  //     type name(VsInput input, uint vertex_id : SV_VertexID, [[vk::builtin("BaseVertex")]] uint base_vertex_id_dxc)
  //     {
  //       vertex_id -= base_vertex_id_dxc;

  // fragments with struct declaration should be invalid treated and replaced by macro in shader
  //    uint field : SV_VertexID;

  // early exit to avoid burning CPU with regex
  const char *firstLine = strstr(src.c_str(), "SV_VertexID");
  if (!firstLine)
    return true;

  // move to line start
  while (*firstLine != '\n')
  {
    --firstLine;
    if (firstLine == src.c_str())
      break;
  }
  int sIdx = (int)(firstLine - src.c_str());

  auto *regExps = DXCVertexIDPatchRegExps::get();
  if (!regExps)
  {
    output.errors = "Failed to compile regexps for DXC SV_VertexID patch";
    return false;
  }

  std::string declPatch = ", [[vk::builtin(\"BaseVertex\")]] uint base_vertex_id_dxc : IGNORED_OUT_SEMANTIC";
  std::string opHead = "\n/*DXC_SPV: patch for SV_VertexID behavior*/\n";
  std::string opTail = " -= base_vertex_id_dxc;";

  if (regExps->verifyRe.test(src.c_str(), sIdx))
  {
    RegExp::Match &match = regExps->verifyRe.pmatch[0];

    std::string sourceLine(src.c_str() + match.rm_so, match.rm_eo - match.rm_so);
    output.errors.sprintf("DXC_SPV: SV_VertexID can't be used in struct, use HW macro instead\n"
                          "Line dump: %s",
      sourceLine.c_str());
    return false;
  }

  while (regExps->patchRe.test(src.c_str(), sIdx))
  {
    RegExp::Match &match = regExps->patchRe.pmatch[0];
    RegExp::Match &varNameMatch = regExps->patchRe.pmatch[1];
    RegExp::Match &declPosMatch = regExps->patchRe.pmatch[2];

    std::string varName(src.c_str() + varNameMatch.rm_so, varNameMatch.rm_eo - varNameMatch.rm_so);

    // make a patch string for neg op
    std::string opPatch;
    opPatch += opHead;
    opPatch += varName;
    opPatch += opTail;

    // patch data
    src.insert(match.rm_eo, opPatch);
    src.insert(declPosMatch.rm_eo, declPatch);

    sIdx = match.rm_eo;
    sIdx += opPatch.length();
    sIdx += declPatch.length();
  }

  return true;
}

eastl::vector<eastl::string_view> scanDisabledSpirvOptimizations(const char *source)
{
  eastl::vector<eastl::string_view> result;

  PragmaScanner scanner{source};
  while (auto pragma = scanner())
  {
    using namespace std::string_view_literals;
    auto from = pragma.tokens();

    if (*from == "spir-v"sv)
    {
      ++from;
      if (*from == "optimizer"sv)
      {
        ++from;
        if (*from == "disable"sv)
        {
          ++from;
          std::string_view optPassName = *from;
          if (!optPassName.empty())
            result.emplace_back(eastl::string_view(optPassName.data(), optPassName.size()));
        }
      }
    }
  }
  return result;
}