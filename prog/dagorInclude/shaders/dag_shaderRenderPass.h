//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_renderPass.h>
#include <drv/shadersMetaData/renderPassLowering.h>
#include <generic/dag_span.h>

/// @brief Runtime of the DSHL render_pass objects.
///
/// A render_pass declared in DSHL is a shape: targets, subpasses, reads,
/// resolves and the framebuffer layouts it supports, stored in the shader
/// dump. This API turns a shape into a live d3d render pass and drives it.
/// Shapes and layouts are looked up in the main dump only.
namespace ShaderRenderPass
{
/// @brief Target capacity of a shape, of Targets and of Actions.
inline constexpr int MAX_TARGETS = RENDER_PASS_MAX_TARGETS;

/// @brief A render_pass shape. The value is the hash of its name, so it
/// survives a dump reload.
enum class Id : uint32_t
{
  Invalid = ~0u
};

/// @brief A framebuffer layout. The value is the hash of its name, so it
/// survives a dump reload.
enum class FramebufferId : uint32_t
{
  Invalid = ~0u
};

/// @return Id::Invalid for an unknown name. A subpass group name is not a
/// pass.
Id find_id(const char *name);

/// @return FramebufferId::Invalid for an unknown name. A layout group and an
/// anonymous layout have no id.
FramebufferId find_framebuffer_id(const char *name);

/// @name Low-level bindump queries
/// Should not be used by game code directly. A dump load or reload makes
/// every returned span and name invalid, so query again after it.
/// @{

enum class DepthUse : uint8_t
{
  Write,
  Read,   ///< read-only, and listed in reads
  Sampled ///< read-only, sampled through a register the caller binds
};

struct SubpassInfo
{
  dag::ConstSpan<int> colors;
  int depth = -1;
  DepthUse depthUse = DepthUse::Write;
  dag::ConstSpan<int> reads;
  int vrs = -1;
};

struct ResolvePair
{
  int src, dst;
};
int get_target_count(Id id);
const char *get_target_name(Id id, int i);
int get_target_slot(Id id, const char *name);
bool is_target_multisampled(Id id, int i);
dag::ConstSpan<ResolvePair> get_resolves(Id id);
int get_subpass_count(Id id);
SubpassInfo get_subpass_info(Id id, int subpass);

struct TargetFormatInfo
{
  uint32_t format = 0;
  uint8_t samples = 1;
};
dag::ConstSpan<FramebufferId> get_pass_framebuffers(Id id);
FramebufferId get_only_framebuffer(Id id);
const char *get_framebuffer_name(FramebufferId fb);
TargetFormatInfo get_framebuffer_target_format(Id id, FramebufferId fb, int target);
/// @}

enum class ExecutionConstraint : uint8_t
{
  NativeOnly, ///< only real pixel-local reads permitted
  SplitOnly,  ///< only T-register reads permitted
  Any,        ///< any of the above
};

/// @brief How this device is allowed to execute the render pass \p id
ExecutionConstraint get_execution_constraint(Id id);

/// @brief True if pixel-local reads are broken on this device and T-register reads must be used instead.
bool device_breaks_subpass_reads();

class Pass;

/// @brief The textures bound to the targets for one execution.
///
/// Every declared target is bound exactly once, except a vrs target
/// Pass::vrsEnabled() rejects, and a clear value is given exactly for the
/// targets the plan clears. Names are kept by pointer until begin(). A
/// sampled_depth target gets only its read-only depth bind here; the caller
/// binds the same texture to the register the shader samples.
class Targets
{
public:
  Targets &tex(const char *target, BaseTexture *t, uint8_t mip = 0, uint16_t layer = 0);
  Targets &clear(const char *target, ResourceClearValue v);
  /// @param slot The target index, from Pass::targetSlot() or get_target_slot().
  Targets &tex(int slot, BaseTexture *t, uint8_t mip = 0, uint16_t layer = 0);
  Targets &clear(int slot, ResourceClearValue v);

private:
  friend class Pass;
  struct Entry
  {
    const char *name = nullptr;
    RenderPassTarget target{};
    int slot = -1;
    bool hasClear = false;
  };
  Entry *entryFor(const char *target);
  Entry *entryFor(int slot);

  Entry entries[MAX_TARGETS];
  int count = 0;
};

/// @brief How the contents of a target enter the pass.
enum class RpLoad : uint8_t
{
  Load,    ///< Target contents are loaded from the texture.
  Clear,   ///< Target is fast-cleared to the clear value.
  DontCare ///< Target contents are undefined/discarded.
};

/// @brief How the contents of a target leave the pass.
enum class RpStore : uint8_t
{
  Store,   ///< Results of the render pass are saved to the texture.
  DontCare ///< Results of the render pass are discarded and texture contents are **undefined**.
};

/// @brief Specifies what happens with the contents of targets at the start
/// and end of a render pass. On non-mobile platforms, the only choice that
/// must be made is whether to clear or load the contents at the start of the pass.
///
/// @details A target without an entry loads and stores its contents. A
/// resolve source loads its contents and discards them at the end. A resolve
/// destination, a vrs attachment and a target that no subpass writes take no
/// entry, because their actions are fixed. An entry for one of them is fatal.
///
/// On mobile devices, careful use of DontCare can avoid unnecessary
/// memory traffic and improve both performance and thermals. Use with caution,
/// as undefined contents are truly undefined and can lead to visual artifacts.
class Actions
{
public:
  Actions &load(const char *target, RpLoad a);
  Actions &clear(const char *target) { return load(target, RpLoad::Clear); }
  Actions &store(const char *target, RpStore a);

private:
  friend class Pass;
  struct Entry
  {
    const char *name = nullptr;
    RpLoad load = RpLoad::Load;
    RpStore store = RpStore::Store;
    bool hasLoad = false, hasStore = false;
  };
  Entry *entryFor(const char *target);

  Entry entries[MAX_TARGETS];
  int count = 0;
};

/// @brief A runtime render pass object that can begin a render pass.
///
/// @details fixes a render pass shape and a framebuffer layout as specified in DSHL,
/// as well as start/end actions, execution mode (true subpass reads or
/// emulated T-register reads) and a debug name.
///
/// @note Survives device resets and best-efforts bindump reloads. If a pass or framebuffer
/// changes or disappears after a reload, begin() is fatal until the instance is recreated.
class Pass
{
public:
  enum class Execution : uint8_t
  {
    Auto,      ///< Native preferred.
    ForceSplit ///< Force T-register reads. Requires allow_split on the render pass in DSHL.
  };

  Pass() = default;
  Pass(Id id, FramebufferId fb, const char *debug_name = nullptr, const Actions *plan = nullptr,
    Execution execution = Execution::Auto);
  Pass(Pass &&o) noexcept;
  Pass &operator=(Pass &&o) noexcept;
  ~Pass();
  Pass(const Pass &) = delete;
  Pass &operator=(const Pass &) = delete;

  bool valid() const { return impl != nullptr; }
  int targetSlot(const char *name) const;
  bool runsSplit() const;
  bool vrsEnabled() const;

  struct Impl;

private:
  friend void begin(const Pass &p, const Targets &t, const RenderPassArea *area);
  static bool resolveTargets(const Impl &p, const Targets &t, RenderPassTarget *out, bool *clear_set, const char **bad_name,
    int *bad_slot);

  Impl *impl = nullptr;
};

/// @brief Begins the specified render pass \p p, binding the targets \p t,
/// performing the necessary clears and setting the render area to \p area.
/// @note if \p area is nullptr, the render area is set to the full resolution of the render targets
void begin(const Pass &p, const Targets &t, const RenderPassArea *area = nullptr);
/// @brief Advances the current render-pass to the next subpass.
void next_subpass();
/// @brief Ends the current render pass.
void end();

/// @brief RAII begin()/end(). The destructor asserts on a skipped subpass.
class Scope
{
public:
  Scope(const Pass &p, const Targets &t, const RenderPassArea *area = nullptr);
  ~Scope();
  Scope(const Scope &) = delete;
  Scope &operator=(const Scope &) = delete;

  void nextSubpass();

private:
  int subpassCount = 0;
};

/// @brief Helper for migration. Publishes a (pass, subpass) for code that
/// draws a shape with plain set_render_target, so dev builds check the
/// drawn shaders opt-ins against it. A no-op in release.
///
/// @warning Binds no reads and sets no interval, so using with a
/// render pass w/ pixel-local reads is a fatal.
class Marker
{
public:
#if DAGOR_DBGLEVEL > 0
  explicit Marker(Id id, int subpass = 0);
  ~Marker();
#else
  explicit Marker(Id, int = 0) {}
#endif
  Marker(const Marker &) = delete;
  Marker &operator=(const Marker &) = delete;

#if DAGOR_DBGLEVEL > 0
private:
  uint32_t prevNameHash = 0;
  uint32_t prevShapeHash = 0;
  int prevSubpass = -1;
  const char *prevName = nullptr;
#endif
};

/// @brief Interface for the shader system to validate the shader render pass opt-in.
struct ActivePass
{
  Id passNameHash = Id::Invalid;
  uint32_t shapeHash = 0;
  int subpass = -1;
  const char *passName = nullptr;
};
ActivePass get_active_pass();
} // namespace ShaderRenderPass
