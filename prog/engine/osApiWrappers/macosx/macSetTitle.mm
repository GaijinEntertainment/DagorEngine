// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_miscApi.h>

#if _TARGET_IOS|_TARGET_TVOS
#import <Foundation/Foundation.h>
#import <Foundation/NSProcessInfo.h>
#import <Foundation/NSString.h>
void win32_set_window_title(const char *) {}
void win32_set_window_title_utf8(const char *) {}

#else
#import <Cocoa/Cocoa.h>

void win32_set_window_title(const char *title)
{
  if (is_main_thread())
    [(NSWindow*)win32_get_main_wnd() setTitle:[[NSString alloc] initWithCString:title encoding:NSASCIIStringEncoding]];
}
void win32_set_window_title_utf8(const char *title)
{
  if (is_main_thread())
    [(NSWindow*)win32_get_main_wnd() setTitle:[[NSString alloc] initWithUTF8String:title]];
}
#endif

const char *os_get_process_name()
{
  return [[[NSProcessInfo processInfo] processName] UTF8String];
}

const char *os_get_default_lang()
{
  static char buf[32];
  if ([[NSLocale preferredLanguages] count] > 0)
  {
    NSString *preferredLang = [[NSLocale preferredLanguages] objectAtIndex:0];
#if _TARGET_TVOS
    //since tvOS 10.1 Locale.preferredLanguages() return [ "en-IN" ], instead of [ "en" ], so we need
    //to correct value to canonical format
    //https://developer.apple.com/library/content/technotes/tn2418
    preferredLang = [NSLocale canonicalLanguageIdentifierFromString:preferredLang];
#else
    //by some reason the above method does not work on ios
    if (preferredLang.length > 2)
      preferredLang = [preferredLang substringToIndex:2];
#endif
    strncpy(buf, [preferredLang UTF8String], sizeof(buf)-1); buf[sizeof(buf)-1] = 0;
  }
  else
    strcpy(buf, "en");
  return buf;
}

#define EXPORT_PULL dll_pull_osapiwrappers_setTitle
#include <supp/exportPull.h>
