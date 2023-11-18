#pragma once


namespace dabfg
{

enum class CompilationStage
{
  REQUIRES_NODES_DECLARATION,
  REQUIRES_NAME_RESOLUTION,
  REQUIRES_NODE_DATA_GATHERING,
  REQUIRES_IR_GENERATION,
  REQUIRES_NODE_SCHEDULING,
  REQUIRES_STATE_DELTA_RECALCULATION,
  REQUIRES_RESOURCE_SCHEDULING,
  REQUIRES_FIRST_FRAME_HISTORY_HANDLING,
  UP_TO_DATE,

  REQUIRES_FULL_RECOMPILATION = REQUIRES_NODES_DECLARATION
};

inline bool operator<(CompilationStage first, CompilationStage second) { return static_cast<int>(first) < static_cast<int>(second); }

} // namespace dabfg
