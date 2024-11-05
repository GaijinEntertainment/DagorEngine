// Copyright (C) Gaijin Games KFT.  All rights reserved.

#import <UIKit/UIApplication.h>
#import <Foundation/NSString.h>
#import <Foundation/NSURL.h>
#include <wchar.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_shellExecute.h>

void os_shell_execute(const char *op, const char *file, const char *params, const char *dir, bool force_sync, OpenConsoleMode)
{
#if _TARGET_IOS
  NSURL *url = [NSURL URLWithString:[NSString stringWithUTF8String:file]];
  if ([[UIApplication sharedApplication] canOpenURL:url])
  {
    [[UIApplication sharedApplication] openURL:url options:@{} completionHandler:^(BOOL success)
    {
      if (!success) debug("URL is not opened");
    }];
  }
  else
  {
    debug("Can't open URL");
  }
#else
  logerr("unsupported: shell_execute(%s, %s, %s, %s)", op, file, params, dir);
#endif
}
void os_shell_execute_w(const wchar_t *op, const wchar_t *file, const wchar_t *params, const wchar_t *dir,
  bool force_sync, OpenConsoleMode)
{
#if _TARGET_TVOS
  logerr("unsupported: shell_execute_w(...)");
#else
  if (!op || wcscmp(op, L"open") == 0)
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:[[NSString alloc]
      initWithBytes:file length:wcslen(file)*sizeof(wchar_t) encoding:NSUTF32LittleEndianStringEncoding]]];
#endif
}