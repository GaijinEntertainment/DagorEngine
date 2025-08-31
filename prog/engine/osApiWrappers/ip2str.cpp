// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_miscApi.h>
#include <supp/_platform.h>

#if _TARGET_PC_WIN | _TARGET_XBOX
#include <winsock2.h>
#elif _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | _TARGET_C3
#include <arpa/inet.h>
#elif _TARGET_C1 | _TARGET_C2



#endif


#if _TARGET_PC | _TARGET_IOS | _TARGET_TVOS | _TARGET_ANDROID | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX | _TARGET_C3
const char *ip_to_string(unsigned int ip)
{
#if _TARGET_C1 | _TARGET_C2 | _TARGET_C3


#else
  return inet_ntoa((struct in_addr &)ip);
#endif
}

unsigned int string_to_ip(const char *str)
{
#if _TARGET_C1 | _TARGET_C2 | _TARGET_C3


#else
  return inet_addr(str);
#endif
}

#else
//== to be implemented
const char *ip_to_string(unsigned int /*ip*/)
{
  G_ASSERT(0);
  return NULL;
}
unsigned int string_to_ip(const char * /*str*/)
{
  G_ASSERT(0);
  return 0;
}
#endif

#define EXPORT_PULL dll_pull_osapiwrappers_ip2str
#include <supp/exportPull.h>
