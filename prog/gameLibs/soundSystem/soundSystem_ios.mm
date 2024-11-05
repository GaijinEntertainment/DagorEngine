// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "soundSystem_ios.h"
#import <UIKit/UIKit.h>
#include <AVFoundation/AVFoundation.h>

namespace sndsys
{

static SoundSuspendHandler sound_handler_cb = nullptr;
static bool is_suspended = false;

void registerIOSNotifications(SoundSuspendHandler handler)
{
  sound_handler_cb = handler;
  [[NSNotificationCenter defaultCenter] addObserverForName:AVAudioSessionInterruptionNotification object:nil queue:nil usingBlock:^(NSNotification *notification)
  {
    bool began = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue] == AVAudioSessionInterruptionTypeBegan;

    if (began == is_suspended)
      return;
    if (began && [[notification.userInfo valueForKey:AVAudioSessionInterruptionWasSuspendedKey] boolValue])
      return;

    is_suspended = began;
    if (!began)
    {
      //re-activate sound session need with delay, otherwise it will be the error
      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        [[AVAudioSession sharedInstance] setActive:TRUE error:nil];
        if (sound_handler_cb)
          sound_handler_cb(began);
      });
      return;
    }

    if (sound_handler_cb)
      sound_handler_cb(began);
  }];

  [[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationDidBecomeActiveNotification object:nil queue:nil usingBlock:^(NSNotification *notification)
  {
    if (!is_suspended)
      return;

    NSError *errorMessage;
    if(![[AVAudioSession sharedInstance] setActive:TRUE error:&errorMessage])
    {
      return;
    }
    if (sound_handler_cb)
      sound_handler_cb(false);
    is_suspended = false;
  }];
}
}
