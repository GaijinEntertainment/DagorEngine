#if _TARGET_PC_LINUX
  #include "curl_config_linux.h"
#elif _TARGET_APPLE
  #include "curl_config_macosx.h"
#elif _TARGET_ANDROID
  #include "curl_config_android.h"
#elif _TARGET_C1 | _TARGET_C2

#endif
