// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "jsonDumpCommon.h"

#include <osApiWrappers/dag_files.h>
#include <util/dag_console.h>


namespace dafg::jsondump
{

extern Writer w;

static const char *compare_func_name(const uint8_t func)
{
  switch (func)
  {
    case CMPF::CMPF_NEVER: return "NEVER";
    case CMPF::CMPF_LESS: return "<";
    case CMPF::CMPF_EQUAL: return "=";
    case CMPF::CMPF_LESSEQUAL: return "<=";
    case CMPF::CMPF_GREATER: return ">";
    case CMPF::CMPF_NOTEQUAL: return "!=";
    case CMPF::CMPF_GREATEREQUAL: return ">=";
    case CMPF::CMPF_ALWAYS: return "ALWAYS";
    default: return "<invalid>";
  }
}

static const char *stencil_op_name(const uint8_t op)
{
  switch (op)
  {
    case STNCLOP_KEEP: return "KEEP";
    case STNCLOP_ZERO: return "ZERO";
    case STNCLOP_REPLACE: return "REPLACE";
    case STNCLOP_INCRSAT: return "INCRSAT";
    case STNCLOP_DECRSAT: return "DECRSAT";
    case STNCLOP_INVERT: return "INVERT";
    case STNCLOP_INCR: return "INCREASE";
    case STNCLOP_DECR: return "DECREASE";
    default: return "<invalid>";
  }
}

static const char *blend_op_name(const uint8_t op)
{
  switch (op)
  {
    case BLENDOP::BLENDOP_ADD: return "ADD";
    case BLENDOP::BLENDOP_SUBTRACT: return "SUB";
    case BLENDOP::BLENDOP_REVSUBTRACT: return "REV SUB";
    case BLENDOP::BLENDOP_MIN: return "MIN";
    case BLENDOP::BLENDOP_MAX: return "MAX";
    default: return "<invalid>";
  }
}

static const char *blend_factor_name(const uint8_t factor)
{
  switch (factor)
  {
    case BLEND_FACTOR::BLEND_ZERO: return "ZERO";
    case BLEND_FACTOR::BLEND_ONE: return "ONE";
    case BLEND_FACTOR::BLEND_SRCCOLOR: return "SRC COLOR";
    case BLEND_FACTOR::BLEND_INVSRCCOLOR: return "INV SRC COLOR";
    case BLEND_FACTOR::BLEND_SRCALPHA: return "SRC ALPHA";
    case BLEND_FACTOR::BLEND_INVSRCALPHA: return "INV SRC ALPHA";
    case BLEND_FACTOR::BLEND_DESTALPHA: return "DST ALPHA";
    case BLEND_FACTOR::BLEND_INVDESTALPHA: return "INV DST ALPHA";
    case BLEND_FACTOR::BLEND_DESTCOLOR: return "DST COLOR";
    case BLEND_FACTOR::BLEND_INVDESTCOLOR: return "INV DST COLOR";
    case BLEND_FACTOR::BLEND_SRCALPHASAT: return "SRC ALPHA SAT";
    case BLEND_FACTOR::BLEND_BOTHINVSRCALPHA: return "BOTH INV SRC ALPHA";
    case BLEND_FACTOR::BLEND_BLENDFACTOR: return "BLEND FACTOR";
    case BLEND_FACTOR::BLEND_INVBLENDFACTOR: return "INV BLEND FACTOR";

    case EXT_BLEND_FACTOR::EXT_BLEND_SRC1COLOR: return "SRC 1 COLOR";
    case EXT_BLEND_FACTOR::EXT_BLEND_INVSRC1COLOR: return "INV SRC 1 COLOR";
    case EXT_BLEND_FACTOR::EXT_BLEND_SRC1ALPHA: return "SRC 1 ALPHA";
    case EXT_BLEND_FACTOR::EXT_BLEND_INVSRC1ALPHA: return "INV SRC 1 ALPHA";

    default: return "<invalid>";
  }
}

void print_shaders_override_state(const shaders::OverrideState &state)
{
  using Bits = shaders::OverrideState::StateBits;

  w.Key("bits");
  w.String(detail::decode_flag_names(Bits(state.bits)));

  if (state.isOn(Bits::Z_FUNC))
  {
    w.Key("zFunc");
    w.String(compare_func_name(state.zFunc));
  }

  w.Key("forcedSampleCount");
  w.Uint(state.forcedSampleCount);

  if (state.isOn(Bits::BLEND_OP))
  {
    w.Key("blendOp");
    w.String(blend_op_name(state.blendOp));
  }

  if (state.isOn(Bits::BLEND_OP_A))
  {
    w.Key("blendOpA");
    w.String(blend_op_name(state.blendOpA));
  }

  if (state.isOn(Bits::BLEND_SRC_DEST))
  {
    w.Key("sblend");
    w.String(blend_factor_name(state.sblend));

    w.Key("dblend");
    w.String(blend_factor_name(state.dblend));
  }

  if (state.isOn(Bits::BLEND_SRC_DEST_A))
  {
    w.Key("sblenda");
    w.String(blend_factor_name(state.sblenda));

    w.Key("dblenda");
    w.String(blend_factor_name(state.dblenda));
  }

  w.Key("colorWr");
  write_string(w, String(0, "%#08X", state.colorWr));

  if (state.isOn(Bits::STENCIL))
  {
    w.Key("stencil");
    {
      OBJECT_SCOPED

      w.Key("func");
      w.String(compare_func_name(state.stencil.func));

      w.Key("fail");
      w.String(stencil_op_name(state.stencil.fail));

      w.Key("zFail");
      w.String(stencil_op_name(state.stencil.zFail));

      w.Key("pass");
      w.String(stencil_op_name(state.stencil.pass));

      w.Key("readMask");
      write_string(w, String(0, "%#02X", state.stencil.readMask));

      w.Key("writeMask");
      write_string(w, String(0, "%#02X", state.stencil.writeMask));
    }
  }

  if (state.isOn(Bits::Z_BIAS))
  {
    w.Key("zBias");
    w.Double(state.zBias);

    w.Key("slopeZBias");
    w.Double(state.slopeZBias);
  }
}


void print_intermediate_vrs_state(const intermediate::VrsState &vrs)
{
  w.Key("rateX");
  w.Uint(vrs.rateX);

  w.Key("rateY");
  w.Uint(vrs.rateY);

  w.Key("vertexCombiner");
  write_string(w, detail::get_enum_name(vrs.vertexCombiner));

  w.Key("pixelCombiner");
  write_string(w, detail::get_enum_name(vrs.pixelCombiner));
}

void print_intermediate_vertex_source(const intermediate::VertexSource &vtx_src, const intermediate::Graph &graph)
{
  w.Key("buffer");
  if (vtx_src.buffer)
    w.String(get_name(graph, *vtx_src.buffer));
  else
    w.String("");

  w.Key("stride");
  w.Uint(vtx_src.stride);
}

void print_intermediate_index_source(const intermediate::IndexSource &inx_src, const intermediate::Graph &graph)
{
  w.Key("buffer");
  if (inx_src.buffer)
    w.String(get_name(graph, *inx_src.buffer));
  else
    w.String("");
}


void write_to_file(const String &raw_filename, const Buffer &buffer)
{
  const char *platformPrefix = "";
#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_XBOX
  platformPrefix = "D:/";
#endif

  String filename = platformPrefix + raw_filename;
  if (!filename.suffix(".json"))
    filename += ".json";


  eastl::unique_ptr<void, DagorFileCloser> f(df_open(filename, DF_WRITE | DF_CREATE));
  if (!f)
  {
    console::error("daFG: failed to open '%s' for writing", filename.c_str());
    return;
  }

  int fileLen = static_cast<int>(buffer.GetSize());
  int bytesWritten = df_write(f.get(), buffer.GetString(), fileLen);
  if (bytesWritten == fileLen)
    console::print_d("daFG: dump saved to '%s'", filename.c_str());
  else
    console::error("daFG: dump to '%s' failed, wrote %d bytes instead of %d", filename.c_str(), bytesWritten, fileLen);
}

} // namespace dafg::jsondump
