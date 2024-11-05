// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_IOS
  #include <startup/dag_iosMainUi.inc.mm>
#elif _TARGET_TVOS
  #include <startup/dag_tvosMainUi.h>
#elif _TARGET_PC_MACOSX
  #include <startup/dag_macMainUi.inc.mm>
#endif
