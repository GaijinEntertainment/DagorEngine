#ifndef _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_AOTJSONWRITER_H_
#define _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_AOTJSONWRITER_H_
#pragma once

#include <daScript/daScript.h>


class RapidJsonWriter;

namespace das_jsonwriter
{
  typedef bool (* TInteropProc)(RapidJsonWriter &, das::Context &, das::SimNode_CallBase *, vec4f *);

  template <TInteropProc P, int N>
  vec4f checked_interop(das::Context & context, das::SimNode_CallBase * call, vec4f * args)
  {
    G_ASSERT(call);
    G_ASSERT(call->nArguments == N);
    G_ASSERT(args);
    RapidJsonWriter & jw = das::cast<RapidJsonWriter &>::to(args[0]);
    return das::cast<bool>::from(P(jw, context, call, args));
  }

  bool jw_start_obj(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args);
  bool jw_end_obj(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args);
  bool jw_start_array(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args);
  bool jw_end_array(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args);
  bool jw_reset(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args);
  bool jw_key(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args);
  bool jw_value(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args);
  bool jw_append(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args);

  vec4f jw_writer(das::Context & context, das::SimNode_CallBase * call, vec4f * args);
}


#endif // _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_AOTJSONWRITER_H_
