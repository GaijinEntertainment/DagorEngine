#include <ecs/gameres/commonLoadingJobMgr.h>
#include <osApiWrappers/dag_cpuJobs.h>

static int common_job_mgr_id = cpujobs::COREID_IMMEDIATE;

void ecs::set_common_loading_job_mgr(int mgr_id) { common_job_mgr_id = mgr_id >= 0 ? mgr_id : cpujobs::COREID_IMMEDIATE; }

int ecs::get_common_loading_job_mgr() { return common_job_mgr_id; }
