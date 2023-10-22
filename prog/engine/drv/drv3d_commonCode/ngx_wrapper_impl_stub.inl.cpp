#if HAS_NVSDK_NGX
NVSDK_NGX_Result NgxWrapper::ngxInit(int, const wchar_t *, void *, const NVSDK_NGX_FeatureCommonInfo *)
{
  return NVSDK_NGX_Result_FAIL_FeatureNotSupported;
}

NVSDK_NGX_Result NgxWrapper::ngxShutdown(void *) { return NVSDK_NGX_Result_FAIL_FeatureNotSupported; }

#ifdef NGX_ENABLE_DEPRECATED_GET_PARAMETERS
NgxWrapper::NVSDK_NGX_Parameter_Result NgxWrapper::ngxGetParameters() { return {nullptr, NVSDK_NGX_Result_FAIL_FeatureNotSupported}; }
#endif

NgxWrapper::NVSDK_NGX_Parameter_Result NgxWrapper::ngxGetCapabilityParameters()
{
  return {nullptr, NVSDK_NGX_Result_FAIL_FeatureNotSupported};
}

NgxWrapper::NVSDK_NGX_Parameter_Result NgxWrapper::ngxAllocateParameters()
{
  return {nullptr, NVSDK_NGX_Result_FAIL_FeatureNotSupported};
}

NVSDK_NGX_Result NgxWrapper::ngxDestroyParameters(NVSDK_NGX_Parameter *) { return NVSDK_NGX_Result_FAIL_FeatureNotSupported; }

NVSDK_NGX_Result NgxWrapper::ngxCreateDlssFeature(void *, NVSDK_NGX_Handle **, NVSDK_NGX_Parameter *, NVSDK_NGX_DLSS_Create_Params *,
  uint32_t, uint32_t)
{
  return NVSDK_NGX_Result_FAIL_FeatureNotSupported;
}

NVSDK_NGX_Result NgxWrapper::ngxReleaseDlssFeature(NVSDK_NGX_Handle *) { return NVSDK_NGX_Result_FAIL_FeatureNotSupported; }

NVSDK_NGX_Result NgxWrapper::ngxEvaluateDlss(void *, NVSDK_NGX_Handle *, NVSDK_NGX_Parameter *, const void *,
  const NVSDK_NGX_Dimensions &)
{
  return NVSDK_NGX_Result_FAIL_FeatureNotSupported;
}

NVSDK_NGX_Result NgxWrapper::ngxDlssGetStats(unsigned long long *) { return NVSDK_NGX_Result_FAIL_FeatureNotSupported; }
#endif
