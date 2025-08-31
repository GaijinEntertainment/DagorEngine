// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shader.h"
#include "device_context.h"

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
  auto smb = eastl::make_unique<ShaderModuleBlob>(info.smb);
  if (smb->blob.empty())
    DAG_FATAL("Shader has no byte code blob");

  ctx.addComputeProgram(prog, eastl::move(smb), info.smh);
}
