//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_fixedVectorMap.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorDriver3d.h>
#include <dasModules/dasScriptsLoader.h>

struct NodeEcsRegistrationAnnotation final : das::FunctionAnnotation
{
  NodeEcsRegistrationAnnotation(das::mutex &mtx) : das::FunctionAnnotation("bfg_ecs_node"), dabfgRuntimeMutex{mtx} {}

  bool apply(const das::FunctionPtr &, das::ModuleGroup &, const das::AnnotationArgumentList &, das::string &) override
  {
    return true;
  }
  void complete(das::Context *, const das::FunctionPtr &) override;
  bool finalize(const das::FunctionPtr &, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    das::string &) override;
  bool apply(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, das::string &) override { return true; };
  bool finalize(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    das::string &) override
  {
    return true;
  };

private:
  das::mutex &dabfgRuntimeMutex;
  struct RegistrationArguments
  {
    eastl::string nodeName{};
    eastl::string entityName{};
    eastl::string handleName{};
    eastl::string counterHandleName{};
  };
  dag::FixedVectorMap<eastl::string, RegistrationArguments, 4> arguments;
  using StrRegistrationArgumentsPair = eastl::pair<eastl::string, RegistrationArguments>;
};
DAG_DECLARE_RELOCATABLE(NodeEcsRegistrationAnnotation::StrRegistrationArgumentsPair);
