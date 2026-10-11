//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#import <UIKit/UIKit.h>
#include <supp/dag_define_KRNLIMP.h>

//! returns the window scene of the app, or nil before UIKit connects one. An app without a scene manifest also
//! gets one window scene from UIKit. Always nil on tvOS. Main thread only
KRNLIMP UIWindowScene *ios_get_window_scene();

//! returns the screen of ios_get_window_scene(), or the main screen when there is no scene. Any thread: off the
//! main thread returns the screen of the last main thread call, or the main screen before the first such call
KRNLIMP UIScreen *ios_get_screen();

//! runs the main run loop until UIKit connects a window scene. Returns at once on tvOS, or when Info.plist has no
//! scene manifest. Main thread only
KRNLIMP void ios_wait_for_window_scene();

#include <supp/dag_undef_KRNLIMP.h>
