//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <EASTL/shared_ptr.h>
#include <generic/dag_fixedMoveOnlyFunction.h>
#include <render/daFrameGraph/daFG.h>
#include <util/dag_generationRefId.h>


namespace dafg::metronome
{

struct SubgraphIdDummy
{};
using SubgraphId = GenerationRefId<8, SubgraphIdDummy>;

/// \brief State of an update request, reported by UpdateToken::status().
enum class UpdateStatus : uint8_t
{
  /// Default value of token.
  NotScheduled,
  /// Request queued, waiting for the frame Metronome selects.
  Pending,
  /// The subgraph's nodes are registered in daFG in the current frame.
  Running,
  /// The subgraph ran and its nodes were unregistered again; transient
  /// resources used by that update may now be released.
  Complete,
  /// The token was invalidated by a newer schedule() on the same handle,
  /// or the subgraph was destroyed.
  Superseded,
};

/**
 * \brief Identifies a single schedule() request.
 * \details A token stays live until the next schedule() on the same
 * SubgraphHandle. Default-constructed tokens report UpdateStatus::NotScheduled.
 */
class UpdateToken
{
public:
  UpdateToken() = default;

  /// \brief Status of this specific request.
  UpdateStatus status() const;

private:
  friend class SubgraphHandle;
  UpdateToken(SubgraphId subgraph_id, uint32_t schedule_generation) : subgraphId{subgraph_id}, scheduleGeneration{schedule_generation}
  {}
  SubgraphId subgraphId;
  uint32_t scheduleGeneration = 0;
};

/// \brief True if this request is in the given state.
inline bool operator==(const UpdateToken &token, UpdateStatus status) { return token.status() == status; }
/// \brief False if this request is in the given state.
inline bool operator!=(const UpdateToken &token, UpdateStatus status) { return !(token == status); }

namespace detail
{
/// Type-erased re-registration thunk: registers the stored node with daFG each
/// time the subgraph is activated. Implemented in the .cpp.
using NodeActivator = dag::FixedMoveOnlyFunction<64, NodeHandle(NameSpace, const char *, const char *)>;
void register_node_impl(SubgraphId subgraph_id, NameSpace ns, const char *name, const char *source_location, NodeActivator activator);
} // namespace detail

/**
 * \brief Move-only RAII handle to a registered subgraph.
 * \details Destroying the handle removes the subgraph from the scheduler; any
 * nodes currently registered in daFG are unregistered.
 */
class SubgraphHandle
{
public:
  SubgraphHandle() = default;
  SubgraphHandle(const SubgraphHandle &) = delete;
  SubgraphHandle &operator=(const SubgraphHandle &) = delete;
  SubgraphHandle(SubgraphHandle &&other) noexcept;
  SubgraphHandle &operator=(SubgraphHandle &&other) noexcept;
  ~SubgraphHandle();

  /// \brief Returns true if the handle refers to a subgraph.
  bool valid() const { return static_cast<bool>(id); }
  /// \brief Returns true if the handle refers to a subgraph.
  explicit operator bool() const { return valid(); }

  /**
   * \brief Stores a daFG node belonging to this subgraph. The node is NOT
   * registered with daFG now; Metronome registers it only on the frame it
   * selects to run this subgraph, and is unregistered if not selected again at the next tick.
   * \details Mirrors dafg::register_node: same declaration-callback contract
   * (Registry -> execution callback, optionally taking multiplexing::Index).
   * \p name and \p source_location are copied. Pass DAFG_PP_NODE_SRC as
   * \p source_location.
   */
  template <class F>
  void register_node(NameSpace ns, const char *name, const char *source_location, F &&declaration_callback)
  {
    auto boxed = eastl::make_shared<eastl::decay_t<F>>(eastl::forward<F>(declaration_callback));
    detail::register_node_impl(id, ns, name, source_location,
      [boxed = eastl::move(boxed)](NameSpace activation_ns, const char *activation_name, const char *activation_src) -> NodeHandle {
        return activation_ns.registerNode(activation_name, activation_src, [boxed](Registry registry) { return (*boxed)(registry); });
      });
  }

  /// \brief Same as register_node(NameSpace, ...), registering into dafg::root().
  template <class F>
  void register_node(const char *name, const char *source_location, F &&declaration_callback)
  {
    register_node(dafg::root(), name, source_location, eastl::forward<F>(declaration_callback));
  }

  /**
   * \brief Requests one update of this subgraph.
   * \details Guaranteed to run within update_max_delay_frames update() ticks.
   * \note Any prior Already running request is not canceled.
   * \note Any prior Pending request is superseded by this new request.
   * \return A token identifying this request.
   */
  UpdateToken schedule();

private:
  friend SubgraphHandle make_subgraph(const char *name, uint32_t update_max_delay_frames);
  SubgraphHandle(SubgraphId subgraph_id) : id{subgraph_id} {}
  SubgraphId id;
};

/**
 * \brief Creates a subgraph named \p name whose update is guaranteed to run
 * within \p update_max_delay_frames update() ticks after schedule().
 */
SubgraphHandle make_subgraph(const char *name, uint32_t update_max_delay_frames);

/**
 * \brief Per-frame scheduler tick. Call exactly once per frame.
 * \note Must be called in the same frame as dafg::run_nodes().
 * \note Must be called before dafg::run_nodes().
 * \details It first unregisters the previous frame's activated subgraphs
 * (completing their requests), then selects and registers this frame's
 * subgraph(s). Both steps happen before dafg::run_nodes(), so daFG sees a
 * single incremental recompilation window per frame.
 *
 * On a forced frame, only the forced subgraph(s) run. The least-slack
 * pending subgraph is deferred, to avoid a multi-subgraph spike. A deferred
 * subgraph keeps its slack and becomes forced once its age reaches
 * update_max_delay_frames. The deadline guarantee still holds.
 */
void update();

} // namespace dafg::metronome
