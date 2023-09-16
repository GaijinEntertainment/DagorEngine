//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#import <UIKit/UIKit.h>

typedef void (*apple_events_callback)(void);
typedef void (*apple_events_callback_with_dictionary)(UIApplication *application, NSDictionary *launchOptions);
typedef bool (*apple_openurl_with_annotation_callback)(UIApplication *application, NSURL *url, NSString *sourceApp, id annotations);

namespace apple
{
void setOnDidFinishLaunchingWithOptionsFunc(apple_events_callback_with_dictionary func);
void setOnApplicationDidBecomeActiveFunc(apple_events_callback func);
void setOnOpenUrlApplication(apple_openurl_with_annotation_callback func);
} // namespace apple
