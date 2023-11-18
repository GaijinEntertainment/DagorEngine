#pragma once

namespace drv3d_dx12
{
struct PipelineStageStateBase;
class BasePipeline;
class ComputePipeline;
} // namespace drv3d_dx12

namespace drv3d_dx12::debug
{
void report_resources(const PipelineStageStateBase &state, ComputePipeline *pipe);
void report_resources(const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline *base_pipe);
} // namespace drv3d_dx12::debug