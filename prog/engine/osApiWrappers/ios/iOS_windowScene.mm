// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_iosWindowScene.h>
#include <osApiWrappers/dag_atomic.h>

static void *volatile last_screen = nullptr;

UIWindowScene *ios_get_window_scene()
{
#if _TARGET_IOS
  UIWindowScene *fallback = nil;
  for (UIScene *scene in [UIApplication sharedApplication].connectedScenes)
  {
    if (![scene isKindOfClass:[UIWindowScene class]])
      continue;
    if (scene.activationState == UISceneActivationStateForegroundActive)
      return (UIWindowScene *)scene;
    if (!fallback)
      fallback = (UIWindowScene *)scene;
  }
  return fallback;
#else
  return nil;
#endif
}

UIScreen *ios_get_screen()
{
  if (![NSThread isMainThread])
  {
    UIScreen *screen = (__bridge UIScreen *)interlocked_acquire_load_ptr(last_screen);
    return screen ? screen : [UIScreen mainScreen];
  }
  UIWindowScene *scene = ios_get_window_scene();
  UIScreen *screen = scene ? scene.screen : [UIScreen mainScreen];

  // a replaced screen is never released because other threads read last_screen without a lock
  // the cost is one retain each time the screen changes
  if ((__bridge void *)screen != interlocked_acquire_load_ptr(last_screen))
    interlocked_release_store_ptr(last_screen, (__bridge void *)[screen retain]);
  return screen;
}

void ios_wait_for_window_scene()
{
#if _TARGET_IOS
  // with a scene manifest UIKit connects the scene after didFinishLaunching, and a window made
  // before that has no scene and is not shown
  if (![[NSBundle mainBundle] objectForInfoDictionaryKey:@"UIApplicationSceneManifest"])
    return;
  NSRunLoop *loop = [NSRunLoop currentRunLoop];
  while (!ios_get_window_scene())
    [loop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
#endif
}
