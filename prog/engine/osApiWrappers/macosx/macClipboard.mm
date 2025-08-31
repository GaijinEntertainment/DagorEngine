// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_clipboard.h>
#include <string.h>

#if _TARGET_PC
#include <AppKit/NSPasteboard.h>
#else
#include <UiKit/UIPasteboard.h>
#endif

bool clipboard::set_clipboard_ansi_text(const char *str)
{
#if _TARGET_PC
  [[NSPasteboard generalPasteboard] clearContents];
  [[NSPasteboard generalPasteboard]
    setString: [[NSString alloc] initWithCString:str encoding:NSASCIIStringEncoding]
  #if __MAC_OS_X_VERSION_MAX_ALLOWED > 101200
    forType:NSPasteboardTypeString
  #else
    forType:NSStringPboardType
  #endif
  ];
#elif _TARGET_TVOS
  //not implemented in tvOS
#else
  [UIPasteboard generalPasteboard].string = [[NSString alloc] initWithCString:str encoding:NSASCIIStringEncoding];
#endif
  return true;
}
bool clipboard::set_clipboard_utf8_text(const char *str)
{
#if _TARGET_PC
  [[NSPasteboard generalPasteboard] clearContents];
  [[NSPasteboard generalPasteboard]
    setString:[[NSString alloc] initWithUTF8String:str]
  #if __MAC_OS_X_VERSION_MAX_ALLOWED > 101200
    forType:NSPasteboardTypeString
  #else
    forType:NSStringPboardType
  #endif
  ];
#elif _TARGET_TVOS
  //not implemented in tvOS
#else
  [UIPasteboard generalPasteboard].string = [[NSString alloc] initWithUTF8String:str];
#endif
  return true;
}

bool clipboard::get_clipboard_ansi_text(char *buf, int buf_sz)
{
#if _TARGET_PC
  #if __MAC_OS_X_VERSION_MAX_ALLOWED > 101200
  NSString *str = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
  #else
  NSString *str = [[NSPasteboard generalPasteboard] stringForType:NSStringPboardType];
  #endif
#elif _TARGET_TVOS
  NSString *str = nullptr; //not implemented in tvOS
#else
  NSString *str = [UIPasteboard generalPasteboard].string;
#endif
  if (!str)
    return false;
  strncpy(buf, [str cStringUsingEncoding:NSASCIIStringEncoding], buf_sz-1); buf[buf_sz-1] = 0;
  return true;
}
bool clipboard::get_clipboard_utf8_text(char *buf, int buf_sz)
{
#if _TARGET_PC
  #if __MAC_OS_X_VERSION_MAX_ALLOWED > 101200
  NSString *str = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
  #else
  NSString *str = [[NSPasteboard generalPasteboard] stringForType:NSStringPboardType];
  #endif
#elif _TARGET_TVOS
  NSString *str = nullptr; //not implemented in tvOS
#else
  NSString *str = [UIPasteboard generalPasteboard].string;
#endif
  if (!str)
    return false;
  strncpy(buf, [str UTF8String], buf_sz-1); buf[buf_sz-1] = 0;
  return true;
}
bool clipboard::set_clipboard_bmp_image(struct TexPixel32* , int , int , int )
{
  return false;
}
bool clipboard::set_clipboard_file(const char *)
{
  return false;
}

#define EXPORT_PULL dll_pull_osapiwrappers_clipboard
#include <supp/exportPull.h>
