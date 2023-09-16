return {
  setTestingMode = @(_) null
  addProviderInit = @(provider, id) false
  addProviderInitWithPriority = @(_, __, ___) null
  isAdsInited = @() false
  isAdsInitedForProvider = @(_) false
  setPriorityForProvider = @(_, __) null
  loadAds = function() {}
  showAds = function() {}
  isAdsLoaded = @() false
  getProvidersStatus = @() ""
  ADS_STATUS_REWARDED = 0
  ADS_STATUS_NOT_INITED = 1
  ADS_STATUS_NOT_READY = 2
  ADS_STATUS_FAIL = 3
  ADS_STATUS_DISMISS = 4
  ADS_STATUS_SHOWN = 5
  ADS_STATUS_LOADED = 6
  ADS_STATUS_NOT_FOUND = 7
  ADS_STATUS_OK = 8
}