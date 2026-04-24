// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shAssertPrivate.h"
#include "shadersBinaryData.h"
#include "shBindumpsPrivate.h"
#include <math/dag_hlsl_floatx.h>
#include <shaders/assert.hlsli>
#include <shaders/dag_shAssert.h>
#include <shaders/dag_linkedListOfShadervars.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <drv/3d/dag_shaderConstants.h>
#include <3d/dag_lockSbuffer.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <EASTL/vector.h>
#include <EASTL/vector_set.h>
#include <EASTL/array.h>
#include <EASTL/fixed_string.h>
#include <util/dag_console.h>
#include <ioSys/dag_dataBlock.h>

namespace shader_assert
{

#if DAGOR_DBGLEVEL < 1

void readback_all() {}
void reset_all() {}

#else

#define SHADER_VAR(var) static shadervars::Node var(#var);

SHADER_VAR(assertion_buffer_slot);
SHADER_VAR(assertion_buffer_info_slot);

void AssertionContext::init(ScriptedShadersBinDumpOwner const &dump_owner)
{
  bool enabledBySettings = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("enableShaderAsserts", false);
  if (!dump_owner.getDumpV2() || !VariableMap::isVariablePresent(assertion_buffer_slot.shadervarId))
  {
    if (enabledBySettings)
      logerr("Shader asserts are enabled by settings (graphics/enableShaderAsserts:b=true), but shader bindump doesn't have asserts");
    return;
  }

  if (!enabledBySettings)
    return;

  G_ASSERT(VariableMap::isVariablePresent(assertion_buffer_info_slot.shadervarId));

  failedByClass.clear();
  stacksOnFrames.fill({});
  currentFrame = 0;

  {
    eastl::fixed_string<char, 128> namebuf{};
    namebuf.sprintf("%s-%x", "assertion_ring_buffer", dump_owner.selfHandle);
    assertionRingBuffer.init(sizeof(AssertEntry), dump_owner.getDump()->classes.size(), RING_BUFFER_SIZE, namebuf.c_str(),
      SBCF_UA_STRUCTURED_READBACK, 0, false);
    namebuf.sprintf("%s-%x", "assertion_buffer", dump_owner.selfHandle);
    assertionBuffer = dag::buffers::create_ua_sr_structured(sizeof(AssertEntry), dump_owner.getDump()->classes.size(), namebuf.c_str(),
      dag::buffers::Init::Zero, RESTAG_ENGINE);
    namebuf.sprintf("%s-%x", "assertion_info_buffer", dump_owner.selfHandle);
    assertionInfoBuffer = dag::buffers::create_one_frame_cb(1, namebuf.c_str(), RESTAG_ENGINE);
  }

  debug("shaders::assert is inited");
}

void AssertionContext::close()
{
  if (!assertionBuffer)
    return;

  assertionRingBuffer.reset();
  assertionRingBuffer.close();
  assertionBuffer.close();
  assertionInfoBuffer.close();
}

void AssertionContext::reset(ScriptedShadersBinDumpOwner const &dump_owner)
{
  close();
  init(dump_owner);
}

void AssertionContext::readback(ScriptedShadersBinDumpOwner const &dump_owner)
{
  if (!assertionBuffer)
    return;

  G_ASSERT(dump_owner.getDumpV2());

  int stride;
  uint32_t frame;
  if (const AssertEntry *data = (const AssertEntry *)assertionRingBuffer.lock(stride, frame, false))
  {
    eastl::vector<AssertEntry> assert_entries;
    assert_entries.assign(data, data + dump_owner.getDump()->classes.size());
    assertionRingBuffer.unlock();

    frame = frame % RING_BUFFER_SIZE;
    for (auto it = assert_entries.begin(); it != assert_entries.end(); ++it)
    {
      it = eastl::find_if(it, assert_entries.end(), [](const AssertEntry &entry) { return entry.asserted != 0; });
      if (it == assert_entries.end())
        break;

      int failed_shader_id = (int)eastl::distance(assert_entries.begin(), it);

      bool inserted = failedByClass.emplace(failed_shader_id).second;
      if (inserted)
      {
        auto &messages = dump_owner.getDumpV2()->messagesByShclass[failed_shader_id];
        if (it->assert_message_id >= messages.size())
        {
          logerr("shader assert: invalid message id %d for shader class %d (max %d)", it->assert_message_id, failed_shader_id,
            (int)messages.size());
          continue;
        }
        String message(messages[it->assert_message_id].c_str());
        String output;
        char *message_str = message;
        char *first_format = strchr(message_str, '%');
        if (!first_format)
          output = message;
        else
        {
          char *next_format = first_format;
          for (int id = 0; next_format; id++)
          {
            G_ASSERT(id < 16);
            enum
            {
              FMT_STRING,
              FMT_FLOAT,
              FMT_INT,
              FMT_UINT,
              FMT_HEX,

              FMT_UNKNOWN
            } format = FMT_UNKNOWN;
            char specifier = next_format[1];
            // Allowing %.X to not break older code
            if (next_format - message.c_str() < message.length() - 2 && next_format[1] == '.' && next_format[2] != 's')
              specifier = next_format[2];
            switch (specifier)
            {
              case 's': format = FMT_STRING; break;
              case 'f': format = FMT_FLOAT; break;
              case 'i':
              case 'd': format = FMT_INT; break;
              case 'u': format = FMT_UINT; break;
              case 'x': format = FMT_HEX; break;

              default: logerr("Invalid format specifier '%%%c' in a shader assert message", next_format[1]); break;
            }
            next_format = strchr(next_format + 1, '%');
            char symbol = next_format ? *next_format : 0;
            if (next_format)
              *next_format = 0;
            switch (format)
            {
              case FMT_STRING:
              {
                int string_id = (int)it->variables[id];
                const bool oob = string_id >= 0 && dump_owner.getDumpV2()->messagesByShclass[failed_shader_id].size() <= string_id;
                if (oob)
                  for (uint32_t i = 0; i < dump_owner.getDumpV2()->messagesByShclass[failed_shader_id].size(); ++i)
                    debug("String %d: %s", i, dump_owner.getDumpV2()->messagesByShclass[failed_shader_id][i].c_str());
                G_ASSERTF(!oob,
                  "In correct string-id %d"
                  "(raw: %f, arg-idx: %d).\n String collection size: %d\n Current message: %s\n Current output: %s",
                  string_id, it->variables[id], id, dump_owner.getDumpV2()->messagesByShclass[failed_shader_id].size(),
                  message.c_str(), output.c_str());
                if (oob)
                  break;
                const char *str =
                  string_id >= 0 ? dump_owner.getDumpV2()->messagesByShclass[failed_shader_id][string_id].c_str() : "<null>";
                output.aprintf(0, message_str, str);
                break;
              }

              case FMT_FLOAT: output.aprintf(0, message_str, it->variables[id]); break;

              case FMT_INT: output.aprintf(0, message_str, int(it->variables[id])); break;

              case FMT_UINT:
              case FMT_HEX: output.aprintf(0, message_str, unsigned(it->variables[id])); break;

              default: output.append(message_str); break;
            }
            if (symbol)
              *next_format = symbol;
            message_str = next_format;
          }
        }

        output += '\n';
        if (it->stack_id < stacksOnFrames[frame].size())
          output += stackhlp_get_call_stack_str(stacksOnFrames[frame][it->stack_id].data(), MAX_STACK_SIZE);
        logerr("%s", output.c_str());
      }
    }
    stacksOnFrames[frame].clear();
  }

  Sbuffer *newTarget = (Sbuffer *)assertionRingBuffer.getNewTarget(frame);
  if (!newTarget)
    return;
  assertionBuffer->copyTo(newTarget);
  assertionRingBuffer.startCPUCopy();
  currentFrame = (frame + 1) % RING_BUFFER_SIZE;
}

void AssertionContext::bind(int shader_class_id, ScriptedShadersBinDumpOwner const &dump_owner)
{
  if (!assertionBuffer)
    return;

  G_ASSERT(dump_owner.getDumpV2());

  bool empty = dump_owner.getDumpV2()->messagesByShclass[shader_class_id].empty();
  if (auto data = lock_sbuffer<int32_t>(assertionInfoBuffer.getBuf(), 0, 2, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
  {
    data[0] = empty ? -1 : shader_class_id;
    data[1] = empty ? -1 : (int)stacksOnFrames[currentFrame].size();
  }
  else
  {
    logerr("failed to lock assertion info buffer");
    return;
  }

  int slot = ShaderGlobal::get_int(assertion_buffer_slot.shadervarId);
  int info_slot = ShaderGlobal::get_int(assertion_buffer_info_slot.shadervarId);
  d3d::set_rwbuffer(ShaderStage::STAGE_PS, slot, assertionBuffer.getBuf());
  d3d::set_rwbuffer(ShaderStage::STAGE_VS, slot, assertionBuffer.getBuf());
  d3d::set_rwbuffer(ShaderStage::STAGE_CS, slot, assertionBuffer.getBuf());
  d3d::set_const_buffer(ShaderStage::STAGE_PS, info_slot, assertionInfoBuffer.getBuf());
  d3d::set_const_buffer(ShaderStage::STAGE_VS, info_slot, assertionInfoBuffer.getBuf());
  d3d::set_const_buffer(ShaderStage::STAGE_CS, info_slot, assertionInfoBuffer.getBuf());

  if (!empty)
  {
    stacksOnFrames[currentFrame].emplace_back();
    stackhlp_fill_stack(stacksOnFrames[currentFrame].back().data(), MAX_STACK_SIZE, 6);
  }
}

void readback_all()
{
  iterate_all_shader_dumps([](ScriptedShadersBinDumpOwner &owner) { owner.assertionCtx.readback(owner); }, false);
}

void reset_all()
{
  iterate_all_shader_dumps([](ScriptedShadersBinDumpOwner &owner) { owner.assertionCtx.reset(owner); }, false);
}

#endif

} // namespace shader_assert

static bool shader_assert_console_handler(const char *argv[], int argc) // move it somewhere else
{
  if (argc < 1)
    return false;
  int found = 0;
  // clang-format off
  CONSOLE_CHECK_NAME("shader_assert", "reset", 1, 1)
  {
    shader_assert::reset_all();
  }
  // clang-format on
  return found;
}
REGISTER_CONSOLE_HANDLER(shader_assert_console_handler);
