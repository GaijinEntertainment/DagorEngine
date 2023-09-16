#import <UIKit/UIApplication.h>
#import <Foundation/NSString.h>
#import <Foundation/NSURL.h>
#include <wchar.h>
#include <debug/dag_debug.h>

void os_shell_execute(const char *op, const char *file, const char *params, const char *dir, bool force_sync)
{
#if _TARGET_IOS
  if (!op || strcmp(op, "open") == 0)
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:file]]];
  else
#endif
    logerr("unsupported: shell_execute(%s, %s, %s, %s)", op, file, params, dir);
}
void os_shell_execute_w(const wchar_t *op, const wchar_t *file, const wchar_t *params, const wchar_t *dir,
  bool force_sync)
{
#if _TARGET_TVOS
  logerr("unsupported: shell_execute_w(...)");
#else
  if (!op || wcscmp(op, L"open") == 0)
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:[[NSString alloc]
      initWithBytes:file length:wcslen(file)*sizeof(wchar_t) encoding:NSUTF32LittleEndianStringEncoding]]];
#endif
}