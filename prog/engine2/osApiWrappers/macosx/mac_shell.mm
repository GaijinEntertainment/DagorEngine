#import <AppKit/NSRunningApplication.h>
#import <AppKit/NSWorkspace.h>
#import <Foundation/NSString.h>
#import <Foundation/NSURL.h>
#import <Foundation/NSTask.h>
#import <Foundation/NSArray.h>
#include <wchar.h>
#include <string.h>
#include <debug/dag_debug.h>

void os_shell_execute(const char *op, const char *file, const char *params, const char *dir, bool force_sync)
{
  if (!op || strcmp(op, "open") == 0)
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:file]]];
  else if (!op || strcmp(op, "explore") == 0)
    [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:dir] isDirectory:TRUE]];
  else if (strcmp(op, "start") == 0)
  {
    NSTask *task = [[NSTask alloc] init];
    task.launchPath = [NSString stringWithUTF8String:file];
    task.arguments  = [[NSArray alloc] initWithObjects: [NSString stringWithUTF8String:params], nil];
    [task launch];
  }
  else
    logerr("unsupported: shell_execute(%s, %s, %s, %s)", op, file, params, dir);
}
void os_shell_execute_w(const wchar_t *op, const wchar_t *file, const wchar_t *params, const wchar_t *dir,
  bool force_sync)
{
  if (!op || wcscmp(op, L"open") == 0)
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[[NSString alloc]
      initWithBytes:file length:wcslen(file)*sizeof(wchar_t) encoding:NSUTF32LittleEndianStringEncoding]]];
  else if (wcscmp(op, L"start") == 0)
  {
    NSTask *task = [[NSTask alloc] init];
    task.launchPath = [[NSString alloc] initWithBytes:file length:wcslen(file)*sizeof(wchar_t) encoding:NSUTF32LittleEndianStringEncoding];
    task.arguments  = [[NSArray alloc] initWithObjects:
      [[NSString alloc] initWithBytes:params length:wcslen(params)*sizeof(wchar_t) encoding:NSUTF32LittleEndianStringEncoding], nil];
    [task launch];
  }
  else if (wcscmp(op, L"explore") == 0)
    [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:[[NSString alloc]
      initWithBytes:dir length:wcslen(dir)*sizeof(wchar_t) encoding:NSUTF32LittleEndianStringEncoding]
      isDirectory:TRUE]];
  else
    logerr("unsupported: shell_execute_w()");
}
