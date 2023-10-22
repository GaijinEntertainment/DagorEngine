//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <render/daBfg/detail/bfg.h>


namespace dabfg
{

/**
 * \brief Holds ownership over a node's lifetime. As soon as a handle
 * to a node destroyed, the node is unregistered, but only if it was not
 * re-registered since the time that this handle was acquired.
 * \details Note that this is a move-only type.
 */
class NodeHandle
{
  friend class NameSpace;
  friend NodeHandle register_external_node(NodeNameId name_id, uint32_t generation);

  NodeHandle(detail::NodeUid id) : uid{id} {}

public:
  /// Constructs an invalid handle.
  NodeHandle() = default;

  NodeHandle(const NodeHandle &) = delete;
  NodeHandle &operator=(const NodeHandle &) = delete;

  /// Move constructor.
  NodeHandle(NodeHandle &&other) : uid{eastl::exchange(other.uid, detail::NodeUid{})} {}

  /// Move assignment operator.
  NodeHandle &operator=(NodeHandle &&other)
  {
    if (this == &other)
      return *this;

    if (uid.valid)
      detail::unregister_node(uid);

    uid = eastl::exchange(other.uid, detail::NodeUid{});

    return *this;
  }

  ~NodeHandle()
  {
    if (uid.valid)
      detail::unregister_node(uid);
  }

  /// \brief Returns true if the handle is valid.
  explicit operator bool() { return uid.valid; }

private:
  detail::NodeUid uid{};
};

} // namespace dabfg
