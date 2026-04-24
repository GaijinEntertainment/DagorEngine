// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shader.h"
#include "device_context.h"
#include "backend/cmd/resources.h"

using namespace drv3d_vulkan;

void BaseProgram::removeFromContext(DeviceContext &ctx, ProgramID prog)
{
  inRemoval = true;
  ctx.dispatchCmd<CmdRemoveProgram>({prog});
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

  CmdAddGraphicsProgram cmd{prog};
  OSSpinlockScopedLock frontLock(ctx.getFrontLock());
  cmd.vs = Frontend::replay->shaderModuleUses.size();
  Frontend::replay->shaderModuleUses.push_back(vsmu);

  cmd.fs = Frontend::replay->shaderModuleUses.size();
  Frontend::replay->shaderModuleUses.push_back(fsmu);

  if (gsmup)
  {
    cmd.gs = Frontend::replay->shaderModuleUses.size();
    Frontend::replay->shaderModuleUses.push_back(*gsmup);
  }
  else
    cmd.gs = ~uint32_t(0);

  if (tcmup)
  {
    cmd.tc = Frontend::replay->shaderModuleUses.size();
    Frontend::replay->shaderModuleUses.push_back(*tcmup);
  }
  else
    cmd.tc = ~uint32_t(0);

  if (temup)
  {
    cmd.te = Frontend::replay->shaderModuleUses.size();
    Frontend::replay->shaderModuleUses.push_back(*temup);
  }
  else
    cmd.te = ~uint32_t(0);

  ctx.dispatchCmdNoLock(cmd);
}

void ComputeProgram::addToContext(DeviceContext &ctx, ProgramID prog, const CreationInfo &info)
{
  auto smb = eastl::make_unique<ShaderModuleBlob>(info.smb);
  if (smb->source.compressedData.empty())
    DAG_FATAL("Shader has no byte code blob");

  ctx.dispatchCmd<CmdAddComputeProgram>({prog, smb.release(), info.smh});
}
