//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_fixedMoveOnlyFunction.h>
#include <render/daFrameGraph/registry.h>
#include <render/daFrameGraph/detail/nodeUid.h>
#include <render/daFrameGraph/detail/nameSpaceNameId.h>


// Detail header for non-template implementations of
// function templates inside ../fg.h
namespace dafg::detail
{

// 128 bytes should be enough for anyone. If more is required, use
// an explicit unique_ptr indirection.
inline constexpr size_t MAX_CALLBACK_SIZE = 128;

// Note that accepting the parameters listed here into the execution
// callback is *optional*. daFG figures out what context you need
// and calls the node with appropriate arguments.
// TODO: in the future, a command buffer object will be passed here
using ExecutionCallback = dag::FixedMoveOnlyFunction<MAX_CALLBACK_SIZE, void(multiplexing::Index)>;
using DeclarationCallback =
  dag::FixedMoveOnlyFunction<MAX_CALLBACK_SIZE, ExecutionCallback(NodeNameId nodeId, InternalRegistry *registry) const>;
NodeUid register_node(NameSpaceNameId nsId, const char *name, const char *node_source, DeclarationCallback &&declaration_callback);

void unregister_node(NodeUid uid);

} // namespace dafg::detail
