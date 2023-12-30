#include "shader.h"
#include "device.h"

using namespace drv3d_vulkan;

void BaseProgram::removeFromContext(DeviceContext &ctx, ProgramID prog)
{
  inRemoval = true;
  ctx.removeProgram(prog);
}

GraphicsProgram::GraphicsProgram(const CreationInfo &info) :
  inputLayout(info.layout),
  vertexShader(info.vs),
  fragmentShader(info.fs),
  geometryShader(info.vs->geometryShader.get()),
  controlShader(info.vs->controlShader.get()),
  evaluationShader(info.vs->evaluationShader.get())
{}

void GraphicsProgram::addToContext(DeviceContext &ctx, ProgramID prog, const CreationInfo &)
{
  auto vsmu = vertexShader->getUseInfo();
  auto fsmu = fragmentShader->getUseInfo();

  ShaderModuleUse gsmu = {};
  ShaderModuleUse *gsmup = nullptr;
  if (geometryShader)
  {
    gsmu = geometryShader->getUseInfo();
    gsmup = &gsmu;
  }

  ShaderModuleUse tcmu = {};
  ShaderModuleUse *tcmup = nullptr;
  if (controlShader)
  {
    tcmu = controlShader->getUseInfo();
    tcmup = &tcmu;
  }

  ShaderModuleUse temu = {};
  ShaderModuleUse *temup = nullptr;
  if (evaluationShader)
  {
    temu = evaluationShader->getUseInfo();
    temup = &temu;
  }

  ctx.addGraphicsProgram(prog, vsmu, fsmu, gsmup, tcmup, temup);
}

void ComputeProgram::addToContext(DeviceContext &ctx, ProgramID prog, const CreationInfo &info)
{
  auto smb = eastl::make_unique<ShaderModuleBlob>(spirv_extractor::getBlob(info.chunks, info.chunk_data, 0));
  if (smb->blob.empty())
    DAG_FATAL("Shader has no byte code blob");

  auto header = spirv_extractor::getHeader(VK_SHADER_STAGE_COMPUTE_BIT, info.chunks, info.chunk_data, 0);
  if (!header)
    DAG_FATAL("Shader has no header");

  ctx.addComputeProgram(prog, eastl::move(smb), *header);
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  ctx.attachComputeProgramDebugInfo(prog,
    eastl::make_unique<ShaderDebugInfo>(spirv_extractor::getDebugInfo(info.chunks, info.chunk_data, 0)));
#endif
}

#if D3D_HAS_RAY_TRACING

void RaytraceProgram::addToContext(DeviceContext &ctx, ProgramID prog, const CreationInfo &info)
{
  eastl::unique_ptr<RaytraceShaderGroup[]> groupUses(new RaytraceShaderGroup[groupCount]);
  eastl::unique_ptr<ShaderModuleUse[]> uses(new ShaderModuleUse[shaderCount]);
  for (uint32_t i = 0; i < shaderCount; ++i)
  {
    shaders[i] = info.shaders.get(info.shader_ids[i]);
    uses[i] = shaders[i]->getUseInfo();
  }
  eastl::copy(info.shader_groups, info.shader_groups + groupCount, shaderGroups.get());
  eastl::copy(info.shader_groups, info.shader_groups + groupCount, groupUses.get());

  ctx.addRaytraceProgram(prog, maxRecursionDepth, shaderCount, uses.release(), groupCount, groupUses.release());
}

#endif