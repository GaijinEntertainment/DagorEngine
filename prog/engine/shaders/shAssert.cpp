// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shAssert.h"
#include <math/dag_hlsl_floatx.h>
#include <shaders/assert.hlsli>
#include <shaders/dag_shAssert.h>
#include <shaders/dag_linkedListOfShadervars.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <EASTL/vector.h>
#include <EASTL/vector_set.h>
#include <EASTL/array.h>
#include <util/dag_console.h>

namespace shader_assert
{

#if DAGOR_DBGLEVEL < 1
void init() {}
void close() {}
void readback() {}

static void reset() {}

ScopedShaderAssert::ScopedShaderAssert(const shaderbindump::ShaderClass &) : mShaderClassId(0) {}
ScopedShaderAssert::~ScopedShaderAssert() = default;
#else

#define SHADER_VAR(var) static shadervars::Node var(#var);

SHADER_VAR(shader_class_id);
SHADER_VAR(stack_id);
SHADER_VAR(assertion_buffer_slot);

static UniqueBuf g_assertion_buffer;
static RingCPUBufferLock g_assertion_ring_buffer;

static eastl::vector_set<int> g_failed_by_class;

static constexpr uint32_t RING_BUFFER_SIZE = 4;
static constexpr uint32_t MAX_STACK_SIZE = 64;

using stack_t = eastl::array<void *, MAX_STACK_SIZE>;
using stacks_on_frame_t = eastl::vector<stack_t>;

eastl::array<stacks_on_frame_t, RING_BUFFER_SIZE> g_stacks_on_frames;
uint32_t g_current_frame = 0;
uint32_t g_last_observed_bindump_gen = uint32_t(-1);

static ScriptedShadersBinDumpV2 *get_dump_v2() { return shBinDumpOwner().getDumpV2(); }

void init()
{
  if (!get_dump_v2() || !VariableMap::isVariablePresent(assertion_buffer_slot.shadervarId))
    return;

  G_ASSERT(VariableMap::isVariablePresent(shader_class_id.shadervarId));
  G_ASSERT(VariableMap::isVariablePresent(stack_id.shadervarId));

  g_failed_by_class.clear();
  g_stacks_on_frames.fill({});
  g_current_frame = 0;
  g_assertion_ring_buffer.init(sizeof(AssertEntry), shBinDump().classes.size(), RING_BUFFER_SIZE, "assertion_buffer",
    SBCF_UA_STRUCTURED_READBACK, 0, false);
  g_assertion_buffer = dag::buffers::create_ua_sr_structured(sizeof(AssertEntry), shBinDump().classes.size(), "assertion_buffer",
    dag::buffers::Init::Zero);
  g_last_observed_bindump_gen = shaderbindump::get_generation();
  debug("shaders::assert is inited");
}

void close()
{
  g_assertion_ring_buffer.reset();
  g_assertion_ring_buffer.close();
  g_assertion_buffer.close();
}

static void reset()
{
  close();
  init();
}

void readback()
{
  if (!g_assertion_buffer)
    return;

  uint32_t bindump_gen = shaderbindump::get_generation();
  if (g_last_observed_bindump_gen != bindump_gen)
  {
    reset();
    return;
  }
  g_last_observed_bindump_gen = bindump_gen;

  int stride;
  uint32_t frame;
  if (const AssertEntry *data = (const AssertEntry *)g_assertion_ring_buffer.lock(stride, frame, false))
  {
    eastl::vector<AssertEntry> assert_entries;
    assert_entries.assign(data, data + shBinDump().classes.size());
    g_assertion_ring_buffer.unlock();

    frame = frame % RING_BUFFER_SIZE;
    for (auto it = assert_entries.begin(); it != assert_entries.end(); ++it)
    {
      it = eastl::find_if(it, assert_entries.end(), [](const AssertEntry &entry) { return entry.asserted != 0; });
      if (it == assert_entries.end())
        break;

      int failed_shader_id = (int)eastl::distance(assert_entries.begin(), it);

      bool inserted = g_failed_by_class.emplace(failed_shader_id).second;
      if (inserted)
      {
        String message(get_dump_v2()->messagesByShclass[failed_shader_id][it->assert_message_id].c_str());
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
            bool is_string = next_format[1] == 's';
            next_format = strchr(next_format + 1, '%');
            char symbol = next_format ? *next_format : 0;
            if (next_format)
              *next_format = 0;
            if (is_string)
            {
              int string_id = (int)it->variables[id];
              const bool oob = string_id >= 0 && get_dump_v2()->messagesByShclass[failed_shader_id].size() <= string_id;
              if (oob)
                for (uint32_t i = 0; i < get_dump_v2()->messagesByShclass[failed_shader_id].size(); ++i)
                  debug("String %d: %s", i, get_dump_v2()->messagesByShclass[failed_shader_id][i].c_str());
              G_ASSERTF(!oob,
                "In correct string-id %d"
                "(raw: %f, arg-idx: %d).\n String collection size: %d\n Current message: %s\n Current output: %s",
                string_id, it->variables[id], id, get_dump_v2()->messagesByShclass[failed_shader_id].size(), message.c_str(),
                output.c_str());
              if (oob)
                break;
              const char *str = string_id >= 0 ? get_dump_v2()->messagesByShclass[failed_shader_id][string_id].c_str() : "<null>";
              output.aprintf(0, message_str, str);
            }
            else
              output.aprintf(0, message_str, it->variables[id]);
            if (symbol)
              *next_format = symbol;
            message_str = next_format;
          }
        }

        output += '\n';
        output += stackhlp_get_call_stack_str(g_stacks_on_frames[frame][it->stack_id].data(), MAX_STACK_SIZE);
        logerr(output);
      }
    }
    g_stacks_on_frames[frame].clear();
  }

  Sbuffer *newTarget = (Sbuffer *)g_assertion_ring_buffer.getNewTarget(frame);
  if (!newTarget)
    return;
  g_assertion_buffer->copyTo(newTarget);
  g_assertion_ring_buffer.startCPUCopy();
  g_current_frame = (frame + 1) % RING_BUFFER_SIZE;
}

ScopedShaderAssert::ScopedShaderAssert(const shaderbindump::ShaderClass &sh_class) :
  mShaderClassId((int)(&sh_class - shBinDump().classes.data()))
{
  if (!g_assertion_buffer || get_dump_v2()->messagesByShclass[mShaderClassId].empty())
    return;

  int slot = ShaderGlobal::get_int(assertion_buffer_slot.shadervarId);
  d3d::set_rwbuffer(ShaderStage::STAGE_PS, slot, g_assertion_buffer.getBuf());
  d3d::set_rwbuffer(ShaderStage::STAGE_VS, slot, g_assertion_buffer.getBuf());
  d3d::set_rwbuffer(ShaderStage::STAGE_CS, slot, g_assertion_buffer.getBuf());
  ShaderGlobal::set_int(shader_class_id.shadervarId, mShaderClassId);
  ShaderGlobal::set_int(stack_id.shadervarId, (int)g_stacks_on_frames[g_current_frame].size());

  g_stacks_on_frames[g_current_frame].emplace_back();
  stackhlp_fill_stack(g_stacks_on_frames[g_current_frame].back().data(), MAX_STACK_SIZE, 6);
}

ScopedShaderAssert::~ScopedShaderAssert()
{
  if (!g_assertion_buffer || get_dump_v2()->messagesByShclass[mShaderClassId].empty())
    return;

  ShaderGlobal::set_int(shader_class_id.shadervarId, -1);
  ShaderGlobal::set_int(stack_id.shadervarId, -1);
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
    shader_assert::reset();
  }
  // clang-format on
  return found;
}
REGISTER_CONSOLE_HANDLER(shader_assert_console_handler);
