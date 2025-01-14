// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  shader compiler interface
/************************************************************************/

#include <util/dag_string.h>
#include "const3d.h"
#include <shaders/dag_shaders.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include "shHardwareOpt.h"
#include "shaderVariantSrc.h"
#include "loadShaders.h"
#include "linkShaders.h"
#include <osApiWrappers/dag_cpuJobs.h>

namespace shc
{
// init compiler
void startup();

// close compiler
void shutdown();

// reset shader compiler internal structures (before next compilation)
void resetCompiler();

// Reset between source files of one target.
void reset_source_file();

// check shader file cache & return true, if cache needs recompilation
CompilerAction should_recompile(const ShVariantName &variant_name);
bool should_recompile_sh(const ShVariantName &variant_name, const String &sourceFileName);

// compile shader files & generate variants to disk. return false, if error occurs
void compileShader(const ShVariantName &variant_name, eastl::optional<StcodeInterface> &stcode_interface, bool no_save,
  bool should_rebuild, CompilerAction compiler_action);

// build shader binary dump from compiled shaders (*.cached files)
bool buildShaderBinDump(const char *bindump_fn, const char *sh_fn, bool forceRebuild, bool minidump, BindumpPackingFlags pack_flags,
  StcodeInterface *stcode_interface);

bool try_enter_shutdown();
bool try_lock_shutdown();
void unlock_shutdown();

String get_obj_file_name_from_source(const String &source_file_name, const ShVariantName &variant_name);

void setAssumedVarsBlock(DataBlock *block);
void setRequiredShadersBlock(DataBlock *block);
void setRequiredShadersDef(bool on);
void clearFlobVarRefList();
void addExplicitGlobVarRef(const char *var_name);

DataBlock *getAssumedVarsBlock();
bool getAssumedValue(const char *varname, const char *shader, bool is_global, float &out_val);
void registerAssumedVar(const char *name, bool known);
void reportUnusedAssumes();
bool isShaderRequired(const char *shader_name);
bool isGlobVarRequired(const char *var_name);


void setValidVariantsBlock(DataBlock *block);
void setInvalidAsNullDef(bool on);

void prepareTestVariantShader(const char *name);
void prepareTestVariant(const ShaderVariant::TypeTable *sv, const ShaderVariant::TypeTable *dv);
bool isValidVariant(const ShaderVariant::VariantSrc *sv, const ShaderVariant::VariantSrc *dv);
bool shouldMarkInvalidAsNull();

String search_include_with_pathes(const char *fn);

void setOutputUpdbPath(const char *path);
const char *getOutputUpdbPath();


struct Job : cpujobs::IJob
{
public:
  Job();
  void doJob() final override;
  void releaseJob() final override;

protected:
  virtual void doJobBody() = 0;
  virtual void releaseJobBody() = 0;
};

enum class JobMgrChoiceStrategy
{
  ROUND_ROBIN,
  LEAST_BUSY_COOPERATIVE
};

void init_jobs(unsigned num_workers);
void deinit_jobs();
unsigned worker_count();
bool is_multithreaded();
bool is_in_worker();
void await_all_jobs(void (*on_released_cb)() = nullptr);
void add_job(Job *job, JobMgrChoiceStrategy mgr_choice_strat);
} // namespace shc
