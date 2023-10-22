#include "ngx_wrapper_base.h"

bool NgxWrapperBase::init(void *, const char *)
{
  debug("NGX: Won't init NGX, it is only supported on 64bit.");
  return false;
}
bool NgxWrapperBase::shutdown(void *) { return false; }
bool NgxWrapperBase::checkDlssSupport() { return false; }
bool NgxWrapperBase::isDlssQualityAvailableAtResolution(uint32_t, uint32_t, int) const { return false; }
bool NgxWrapperBase::createOptimalDlssFeature(void *, uint32_t, uint32_t, int, bool, uint32_t, uint32_t) { return false; }
bool NgxWrapperBase::releaseDlssFeature() { return false; }
DlssState NgxWrapperBase::getDlssState() const
{
#if _TARGET_PC_WIN
#if _TARGET_64BIT
  return DlssState::DISABLED; // DLSS is supported on win64, but it can be disabled in jam
#else
  return DlssState::NOT_SUPPORTED_32BIT;
#endif
#else
  return DlssState::NOT_SUPPORTED_INCOMPATIBLE_HARDWARE;
#endif
};
void NgxWrapperBase::getDlssRenderResolution(int &, int &) const {}
bool NgxWrapperBase::evaluateDlss(void *, const void *, int) { return false; }
bool NgxWrapperBase::dlssGetStats(unsigned long long *pVRAMAllocatedBytes) { return false; }
#if HAS_NVSDK_NGX
NgxWrapperBase::NVSDK_NGX_Parameter_Result NgxWrapperBase::ngxGetParameters()
{
  return {nullptr, NVSDK_NGX_Result_FAIL_FeatureNotSupported};
}
#endif
