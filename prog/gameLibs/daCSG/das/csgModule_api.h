// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/graph_api.h>
#include <daScript/simulate/simulate.h>
#include <EASTL/unique_ptr.h>

// Hosts pull it: NEED_MODULE(CsgModule)
namespace dacsg
{
struct ModelState
{
  virtual ~ModelState() = default;
};

// a base before das::Context: outlives ~Context's [finalize]
class ModelStore
{
protected:
  dag::Vector<eastl::unique_ptr<ModelState>> states;

public:
  Graph graph; // after states: the graph dies first, then the leaf sources it points to
};

// The das::Context a host runs model() in: it holds the csg Graph and the state of each das module the model uses,
// so the host reads the results from it after model(). A worker made from it with makeWorkerFor must be one too
class ModelContext : public ModelStore, public das::Context
{
  static uint32_t newSlot();

public:
  explicit ModelContext(uint32_t stack_size);
  // the context of a bound call; a das error when the host made a plain das::Context
  static ModelContext &of(das::Context *ctx);
  // one T per context, made on first use; T derives from ModelState
  template <class T>
  T &state()
  {
    static const uint32_t slot = newSlot();
    if (states.size() <= slot)
      states.resize(slot + 1);
    if (!states[slot])
      states[slot].reset(new T);
    return static_cast<T &>(*states[slot]);
  }
};

// the Graph of the model's context; node ids are its indices, valid while the context lives
inline Graph &csg_graph(das::Context *ctx) { return ModelContext::of(ctx).graph; }

// For a bound call of a module whose das file module_das wraps it (the name it compiled under, FileInfo::name by strcmp, not a path):
// the model's calling line: at when at lies outside that file, else the first such line up the stack, else the otherwise argument
const das::LineInfo *model_call_site(das::Context *ctx, das::LineInfoArg *at, const char *module_das,
  const das::LineInfo *otherwise = nullptr);
// the part of a path after its last slash or backslash
const char *file_name(const char *path);
} // namespace dacsg
