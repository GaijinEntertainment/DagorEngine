#if _TARGET_PC_WIN|_TARGET_XBOX
  #include "../win32/pcre_stringpiece.h"
#elif _TARGET_ANDROID|_TARGET_C1|_TARGET_C2
  #include "../linux32/pcre_stringpiece.h"
#elif _TARGET_PC_LINUX
  #include "../linux64/pcre_stringpiece.h"
#elif _TARGET_C3

#elif _TARGET_APPLE
  #include "../macosx/pcre_stringpiece.h"
#endif
