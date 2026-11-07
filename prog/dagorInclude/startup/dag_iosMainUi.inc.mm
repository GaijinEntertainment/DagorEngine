// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#pragma once

#import <UIKit/UIKit.h>
#import <CoreHaptics/CoreHaptics.h>
#import <GameController/GameController.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>
#include <memory/dag_mem.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_iosWindowScene.h>
#include <osApiWrappers/dag_hinge.h>
#include <drv/hid/dag_hiCreate.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <util/dag_string.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <unistd.h>
#include <signal.h>
#include "dag_addBasePathDef.h"
#include "dag_loadSettings.h"
#include "dag_iosCallback.h"

#include <userSystemInfo/systemInfo.h>

#if _TARGET_TVOS
#include "dag_tvosMainUi.h"
#endif

#import <Foundation/Foundation.h>

#include <pthread.h>

static eastl::function<void()> g_frame_callback;

static CADisplayLink *g_displayLink = nullptr;
int g_ios_fps_limit = 120;
extern bool g_ios_pause_rendering;
static bool g_ios_was_paused_before_sleep = false;
static bool g_ios_main_view_is_active = true;
// if true, the main loop will wait for native UI events to show (purchase popups, etc), the caller should set it back to false when the UI is dismissed
static bool g_ios_wait_for_ui_events = false;
UIInterfaceOrientation g_app_orientation = UIInterfaceOrientationUnknown;
UIInterfaceOrientation preferredLandscapeOrientation = UIInterfaceOrientationLandscapeLeft;

UIViewController *storedMainViewCtrl;

const int EDIT_FIELD_HEIGHT = 44;
const float EDIT_FIELD_WIDTH_SCALE = 0.75f;
const float EDIT_FIELD_MARGIN = 8.0f;

void (*dagor_ios_active_status_handler)(bool isVisible) = nullptr;
static apple_events_callback_with_dictionary g_onDidFinishLaunchingWithOptionsFunc = NULL;
static apple_continue_user_activity_callback g_onContinueUserActivityFunc = NULL;
static apple_events_callback g_onApplicationDidBecomeActiveFunc = NULL;
static apple_openurl_with_annotation_callback g_onOpenUrlAppWithAnnotationFunc = NULL;
static apple_register_remote_token_callback g_onRegisterRemoteToken = NULL;

static void (*on_editfield_finish_cb)(const char *str, int cursor, int status) = NULL;

NSString *const kGCMMessageIDKey = @"gcm.message_id";

bool dagor_fast_shutdown = false;

namespace apple
{

void excludeFolderFromBackup(NSString * path)
{
  NSURL *url = [NSURL fileURLWithPath:path];
  NSError *error = nil;
  [url setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:&error];
  if (error)
    logwarn("Error excluding %s from backup: %s", [path UTF8String], [[error localizedDescription] UTF8String]);
}

void excludeSystemFoldersFromBackup()
{
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString *documentsDirectory = [paths objectAtIndex:0];
  excludeFolderFromBackup(documentsDirectory);

  paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  NSString *appSupportDirectory = [paths objectAtIndex:0];
  excludeFolderFromBackup(appSupportDirectory);

  NSString *tempDirectory = NSTemporaryDirectory();
  excludeFolderFromBackup(tempDirectory);

  paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
  NSString *cachesDirectory = [paths objectAtIndex:0];
  excludeFolderFromBackup(cachesDirectory);
}

void setOnDidFinishLaunchingWithOptionsFunc(apple_events_callback_with_dictionary func)
{
  g_onDidFinishLaunchingWithOptionsFunc = func;
}

void setOnContinueUserActivityFunc(apple_continue_user_activity_callback func) { g_onContinueUserActivityFunc = func; }

void setOnApplicationDidBecomeActiveFunc(apple_events_callback func) { g_onApplicationDidBecomeActiveFunc = func; }

void setOnOpenUrlApplication(apple_openurl_with_annotation_callback func) { g_onOpenUrlAppWithAnnotationFunc = func; }

void setOnRegisterRemoteToken(apple_register_remote_token_callback func) { g_onRegisterRemoteToken = func; }

} // namespace apple

UIInterfaceOrientation getAppOrientation() { return g_app_orientation; }

void setUIInterfaceOrientationFromActiveWindow()
{
  UIWindowScene *activeWindow = ios_get_window_scene();
  if (!activeWindow)
    activeWindow = (UIWindowScene *)[[[UIApplication sharedApplication] windows] firstObject];
  if (activeWindow)
  {
    g_app_orientation = [activeWindow interfaceOrientation];

    switch (g_app_orientation)
    {
      case UIInterfaceOrientationLandscapeRight:
        preferredLandscapeOrientation = UIInterfaceOrientationLandscapeRight;
        break;
      case UIInterfaceOrientationLandscapeLeft:
        preferredLandscapeOrientation = UIInterfaceOrientationLandscapeLeft;
        break;
      default:
        break;
    }
  }
}

// On ProMotion (120Hz) devices the display link callback saturates the main thread, starving UIKit of time to render
// system keyboard key press/release animations (keys look visually stuck), so the frame rate is capped while it is visible
static bool g_ios_screen_keyboard_fps_cap = false;

static void apply_display_link_fps()
{
  if (g_displayLink)
    g_displayLink.preferredFramesPerSecond = (g_ios_screen_keyboard_fps_cap && g_ios_fps_limit > 30) ? 30 : g_ios_fps_limit;
}

void setFPSLimit(int lim)
{
  g_ios_fps_limit = lim ? lim : 120;
  apply_display_link_fps();
}

void setWaitForUIEvents(bool wait)
{
  g_ios_wait_for_ui_events = wait;
}

int ios_get_maximum_frames_per_second() { return ios_get_screen().maximumFramesPerSecond; }

void runIOSRunloop(const eastl::function<void()> &callback)
{
  g_frame_callback = callback;
  [[NSRunLoop currentRunLoop] run];
}

void runIOSRunloopUntil(const eastl::function<void()> &callback, const eastl::function<bool()> &keepRunning)
{
  g_frame_callback = callback;
  NSRunLoop *loop = [NSRunLoop currentRunLoop];
  for (; keepRunning() && [loop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];) {}
  g_frame_callback = {};
}


namespace workcycle_internal
{
intptr_t main_window_proc(void *, unsigned, uintptr_t, intptr_t);
}

static constexpr int MAX_SIMULTANEOUS_TOUCHES = 5;

@interface Dagor2_App : UIApplication
{}
@end

namespace haptic
{
CHHapticEngine *g_hapticEngine = NULL;
}

@interface Dagor2_AppDelegate : NSObject <UIApplicationDelegate>
{
  bool isActive;
  bool isBkgModeSupported;
}
+ (Dagor2_AppDelegate *)sharedAppDelegate;
@end

#if _TARGET_IOS
@interface DagorViewCtrl : UIViewController
#else
@interface DagorViewCtrl : DagorGameCenterEventViewController
#endif
- (id)initWithView:(UIView *)view;
@end

UIViewController *getMainController()
{
  if (UIWindow *mainwnd = (UIWindow *)win32_get_main_wnd())
  {
    if (DagorViewCtrl *mainviewctrl = (DagorViewCtrl *)mainwnd.rootViewController)
      return mainviewctrl;
  }
  return nullptr;
}

UIViewController *getStoredMainController()
{
  return storedMainViewCtrl;
}

const char * getDeviceLocString(const char *key, const char *table, const char *fallback)
{
  NSString * loc;
  if (fallback)
    loc = NSLocalizedStringWithDefaultValue([NSString stringWithUTF8String:key],[NSString stringWithUTF8String:table],NSBundle.mainBundle,[NSString stringWithUTF8String:fallback],nil);
  else
    loc = NSLocalizedStringFromTable([NSString stringWithUTF8String:key],[NSString stringWithUTF8String:table],nil);
  if (loc)
    return [loc UTF8String];
  else
    return key;
}

@implementation DagorViewCtrl

NSString *appName;

- (id)initWithView:(UIView *)view
{
#if _TARGET_TVOS
  self = [super initWithView:view];
#else
  self = [super init];
#endif

  return self;
}

- (NSUInteger)supportedInterfaceOrientations
{
#if _TARGET_IOS
  return UIInterfaceOrientationMaskAll;
#else
  return UIInterfaceOrientationMaskLandscape;
#endif
}
- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation
{
  return preferredLandscapeOrientation;
}
- (BOOL)shouldAutorotate
{
  return YES;
}

- (BOOL)prefersHomeIndicatorAutoHidden
{
  return NO;
}

-(BOOL)prefersStatusBarHidden
{
  return YES;
}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures
{
  return UIRectEdgeAll;
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];

  [coordinator
    animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
    }
    completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
      setUIInterfaceOrientationFromActiveWindow();
    }];
}

- (void)viewWillAppear:(BOOL)animated
{
#if _TARGET_IOS
  [UIApplication sharedApplication].idleTimerDisabled = YES;
  g_ios_main_view_is_active = true;
  if (dagor_ios_active_status_handler)
    dagor_ios_active_status_handler(g_ios_main_view_is_active && !g_ios_pause_rendering);
#endif
}

- (void)viewWillDisappear:(BOOL)animated
{
#if _TARGET_IOS
  [UIApplication sharedApplication].idleTimerDisabled = NO;
  g_ios_main_view_is_active = false;
  if (dagor_ios_active_status_handler)
    dagor_ios_active_status_handler(g_ios_main_view_is_active && !g_ios_pause_rendering);
#endif
}
@end

#if _TARGET_IOS
@interface DagorView : UIView <UITextFieldDelegate>
{
  UITouch *pointers[MAX_SIMULTANEOUS_TOUCHES];
  UITextField *editField;
  UIToolbar *editToolbar;
  UIButton *editBG;
  UIBarButtonItem *editDoneButton;
  BOOL kbdVisible;
  NSInteger maxChars;
  int mousePtrIdx;
}
#else
@interface DagorView : DagorInetView
#endif

- (BOOL)acceptsFirstResponder;
- (BOOL)becomeFirstResponder;
- (BOOL)resignFirstResponder;

@property(nonatomic, assign) UIWindow *mainwnd;
@property(nonatomic, assign) float scale;

#if _TARGET_IOS
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event;
- (BOOL)showKeyboard;
- (void)showKeyboardRaw;
- (BOOL)showKeyboardIME:(const HumanInput::ScreenKeyboardParamsIOS &)params;
- (void)hideKeyboard;
- (void)initializeKeyboard;
- (void)clearPointers;
#endif

@property(readonly) BOOL kbdVisible;

@end

@implementation DagorView
{}

- (void)initCommon
{
  self.opaque = YES;
  self.backgroundColor = nil;
  self.contentScaleFactor = ios_get_screen().nativeScale;

#if _TARGET_IOS
  [self initializeKeyboard];

  for (int i = 0; i < MAX_SIMULTANEOUS_TOUCHES; i++)
  {
    pointers[i] = NULL;
  }

  self.multipleTouchEnabled = YES;
  self.exclusiveTouch = YES;
  mousePtrIdx = MAX_SIMULTANEOUS_TOUCHES + 1;
#endif
}

- (void)didMoveToWindow
{
  self.contentScaleFactor = self.window.screen.nativeScale;
}

- (void)setContentScaleFactor:(CGFloat)contentScaleFactor
{
  [super setContentScaleFactor:contentScaleFactor];
  self.scale = self.contentScaleFactor;
}

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  if (self)
    [self initCommon];

  return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
  self = [super initWithCoder:coder];

  if (self)
    [self initCommon];
  return self;
}

- (BOOL)acceptsFirstResponder
{
  return YES;
}

- (BOOL)becomeFirstResponder
{
  return YES;
}

- (BOOL)resignFirstResponder
{
  return YES;
}

#if _TARGET_IOS
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (!self.mainwnd)
    return;
  NSEnumerator *enumerator = [touches objectEnumerator];
  UITouch *touch = nil;

  while (touch = (UITouch *)[enumerator nextObject])
  {
    CGPoint locationInView = [touch locationInView:self];
    int mx = locationInView.x * self.scale;
    int my = locationInView.y * self.scale;
    workcycle_internal::main_window_proc(self.mainwnd, GPCM_TouchBegan, uintptr_t((void *)touch), mx | (my << 16));
  }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (!self.mainwnd)
    return;
  NSEnumerator *enumerator = [touches objectEnumerator];
  UITouch *touch = nil;

  while (touch = (UITouch *)[enumerator nextObject])
  {
    CGPoint locationInView = [touch locationInView:self];
    int mx = locationInView.x * self.scale;
    int my = locationInView.y * self.scale;
    workcycle_internal::main_window_proc(self.mainwnd, GPCM_TouchEnded, uintptr_t((void *)touch), mx | (my << 16));
  }
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
  [self touchesEnded:touches withEvent:event];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
  if (!self.mainwnd)
    return;
  NSEnumerator *enumerator = [touches objectEnumerator];
  UITouch *touch = nil;

  while (touch = (UITouch *)[enumerator nextObject])
  {
    CGPoint locationInView = [touch locationInView:self];
    int mx = locationInView.x * self.scale;
    int my = locationInView.y * self.scale;
    workcycle_internal::main_window_proc(self.mainwnd, GPCM_TouchMoved, uintptr_t((void *)touch), mx | (my << 16));
  }
}

- (BOOL)kbdVisible
{
  return kbdVisible;
}

- (void)dealloc
{
  editField.delegate = nil;
  [editField removeTarget:self action:NULL forControlEvents:UIControlEventAllEvents];
  [editBG removeTarget:self action:NULL forControlEvents:UIControlEventAllEvents];
  editDoneButton.target = nil;
  [editField release];
  [editToolbar release];
  [editBG release];
  [editDoneButton release];
  [super dealloc];
}

- (void)initializeKeyboard
{
  const CGSize screenSize = ios_get_screen().bounds.size;

  editField = [[UITextField alloc] initWithFrame:CGRectMake(0, 0, screenSize.width * EDIT_FIELD_WIDTH_SCALE, EDIT_FIELD_HEIGHT)];
  editField.delegate = self;
  editField.text = @" ";
  editField.font = [UIFont systemFontOfSize:14.0];
  editField.autocapitalizationType = UITextAutocapitalizationTypeNone;
  editField.autocorrectionType = UITextAutocorrectionTypeNo;
  editField.enablesReturnKeyAutomatically = NO;
  editField.keyboardAppearance = UIKeyboardAppearanceDefault;
  editField.keyboardType = UIKeyboardTypeDefault;
  editField.returnKeyType = UIReturnKeyDefault;
  editField.secureTextEntry = NO;
  editField.borderStyle = UITextBorderStyleRoundedRect;
  [editField addTarget:self action:@selector(editFieldChanged) forControlEvents:UIControlEventEditingChanged];

  editToolbar = [[UIToolbar alloc] initWithFrame:CGRectMake(0, 0, screenSize.width, EDIT_FIELD_HEIGHT)];
  editDoneButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                                                                 target:self
                                                                 action:@selector(doneClicked)];
  UIBarButtonItem *flexibleSpace = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace
                                                                                  target:nil
                                                                                  action:nil] autorelease];
  editToolbar.items = @[ flexibleSpace, editDoneButton ];
  [editToolbar sizeToFit];
  editToolbar.translatesAutoresizingMaskIntoConstraints = NO;

  editBG = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, screenSize.width, screenSize.height)];
  [editBG addTarget:self action:@selector(hideKeyboard) forControlEvents:UIControlEventTouchUpInside];
  editBG.backgroundColor = [UIColor blackColor];
  editBG.alpha = .4;
  kbdVisible = NO;

  if (@available(iOS 17.0, *))
    self.keyboardLayoutGuide.followsUndockedKeyboard = YES;
}

- (void)doneClicked
{
  [self hideKeyboard];
}

static NSUInteger utf16_index_from_codepoints(NSString *s, NSInteger cp)
{
  const NSUInteger len = s.length;
  NSUInteger i = 0;
  for (; cp > 0 && i < len; cp--)
    i += (CFStringIsSurrogateHighCharacter([s characterAtIndex:i]) && i + 1 < len) ? 2 : 1;
  return i;
}

static NSInteger codepoints_from_utf16_index(NSString *s, NSUInteger idx)
{
  const NSUInteger len = s.length;
  NSInteger cp = 0;
  for (NSUInteger i = 0; i < idx && i < len; cp++)
    i += (CFStringIsSurrogateHighCharacter([s characterAtIndex:i]) && i + 1 < len) ? 2 : 1;
  return cp;
}

static NSInteger codepoints_count(NSString *s) { return codepoints_from_utf16_index(s, s.length); }

static NSString *truncate_to_codepoints(NSString *s, NSInteger max_cp)
{
  NSUInteger idx = utf16_index_from_codepoints(s, max_cp);
  if (idx >= s.length)
    return s;
  idx = [s rangeOfComposedCharacterSequenceAtIndex:idx].location;
  return [s substringToIndex:idx];
}

static UITextContentType content_type_to_uikit(HumanInput::ScreenKeyboardParamsIOS::ContentType type)
{
  switch (type)
  {
    case HumanInput::ScreenKeyboardParamsIOS::CONTENT_PASSWORD: return UITextContentTypePassword;
    case HumanInput::ScreenKeyboardParamsIOS::CONTENT_EMAIL: return UITextContentTypeEmailAddress;
    default: return nil;
  }
}

- (NSRange)editFieldSelectionUtf16
{
  const NSUInteger len = editField.text.length;
  UITextRange *sel = editField.selectedTextRange;
  if (!sel)
    return NSMakeRange(len, 0);
  const NSInteger from = [editField offsetFromPosition:editField.beginningOfDocument toPosition:sel.start];
  const NSInteger to = [editField offsetFromPosition:editField.beginningOfDocument toPosition:sel.end];
  const NSUInteger start = (NSUInteger)MIN(MAX(from, (NSInteger)0), (NSInteger)len);
  const NSUInteger end = (NSUInteger)MIN(MAX(to, (NSInteger)start), (NSInteger)len);
  return NSMakeRange(start, end - start);
}

- (void)setEditFieldCursorUtf16:(NSUInteger)pos
{
  pos = MIN(pos, editField.text.length);
  if (UITextPosition *p = [editField positionFromPosition:editField.beginningOfDocument offset:(NSInteger)pos])
    editField.selectedTextRange = [editField textRangeFromPosition:p toPosition:p];
}

- (void)finishEditWithStatus:(int)status
{
  if (!on_editfield_finish_cb)
    return;

  NSString *text = editField.text ?: @"";
  NSInteger cursor = codepoints_from_utf16_index(text, [self editFieldSelectionUtf16].location);
  if (maxChars > 0)
  {
    text = truncate_to_codepoints(text, maxChars);
    cursor = MIN(cursor, codepoints_count(text));
  }
  const char *utf8 = text.UTF8String;
  if (!utf8)
    logwarn("iOS: IME text can't be converted to UTF-8");
  on_editfield_finish_cb(utf8 ? utf8 : "", (int)cursor, status);
}

- (BOOL)showKeyboardIME:(const HumanInput::ScreenKeyboardParamsIOS &)params
{
  NSString *text = (params.text ? [NSString stringWithUTF8String:params.text] : nil) ?: @"";
  NSString *hint = (params.hint && *params.hint) ? [NSString stringWithUTF8String:params.hint] : nil;
  maxChars = params.maxChars;
  if (maxChars > 0)
    text = truncate_to_codepoints(text, maxChars);

  editField.secureTextEntry = params.secure;
  editField.keyboardType = (UIKeyboardType)params.keyboardType;
  editField.returnKeyType = (UIReturnKeyType)params.returnKeyType;
  editField.autocapitalizationType = (UITextAutocapitalizationType)params.autocapitalizationType;
  editField.textContentType = content_type_to_uikit(params.contentType);
  editField.text = text;
  editField.placeholder = hint;
  if (editField.isFirstResponder)
    [editField reloadInputViews];

  if (![self showKeyboard])
    return NO;

  const NSUInteger cursor = params.cursorPos < 0 ? text.length : utf16_index_from_codepoints(text, params.cursorPos);
  [self setEditFieldCursorUtf16:cursor];
  NSString *expectedText = text;
  dispatch_async(dispatch_get_main_queue(), ^{
    if (kbdVisible && editField.isFirstResponder && [editField.text isEqualToString:expectedText])
      [self setEditFieldCursorUtf16:cursor];
  });
  return YES;
}

- (void)showKeyboardRaw
{
  if (!kbdVisible)
  {
    on_editfield_finish_cb = NULL;
    editField.secureTextEntry = NO;
    editField.keyboardType = UIKeyboardTypeDefault;
    editField.returnKeyType = UIReturnKeyDefault;
    editField.autocapitalizationType = UITextAutocapitalizationTypeNone;
    editField.textContentType = nil;
    editField.placeholder = nil;
    editField.text = @" ";
  }
  [self showKeyboard];
}

- (BOOL)showKeyboard
{
  if (kbdVisible)
    return YES;
  if (self.window == nil)
    return NO;

  [self addSubview:editBG];
  [self addSubview:editToolbar];
  [self addSubview:editField];
  UILayoutGuide *kbdGuide = self.keyboardLayoutGuide;
  [NSLayoutConstraint activateConstraints:@[
    [editToolbar.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
    [editToolbar.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
    [editToolbar.bottomAnchor constraintEqualToAnchor:kbdGuide.topAnchor],
    [editToolbar.heightAnchor constraintEqualToConstant:editToolbar.frame.size.height],
  ]];
  kbdVisible = YES;
  [UIView performWithoutAnimation:^{
    [self setNeedsLayout];
    [self layoutIfNeeded];
  }];

  if (![editField becomeFirstResponder])
  {
    logwarn("iOS: edit field can't become first responder, screen keyboard is not shown");
    kbdVisible = NO;
    [self removeKeyboardControls];
    return NO;
  }

  g_ios_screen_keyboard_fps_cap = true;
  apply_display_link_fps();
  return YES;
}

- (void)removeKeyboardControls
{
  [editField removeFromSuperview];
  [editToolbar removeFromSuperview];
  [editBG removeFromSuperview];
}

- (void)hideKeyboard
{
  if (!kbdVisible)
    return;

  kbdVisible = NO;
  [self finishEditWithStatus:HumanInput::IME_STATUS_CLOSED];
  [editField resignFirstResponder];
  [self removeKeyboardControls];

  g_ios_screen_keyboard_fps_cap = false;
  apply_display_link_fps();
}

static CGFloat toolbar_items_min_x(UIToolbar *toolbar, UIView *space, CGFloat fallback)
{
  CGFloat minX = fallback;
  NSMutableArray<UIView *> *stack = [NSMutableArray arrayWithArray:toolbar.subviews];
  while (stack.count)
  {
    UIView *view = stack.lastObject;
    [stack removeLastObject];
    if (view.hidden)
      continue;
    if ([view isKindOfClass:[UIControl class]])
      minX = fmin(minX, [view convertRect:view.bounds toView:space].origin.x);
    else
      [stack addObjectsFromArray:view.subviews];
  }
  return minX;
}

- (void)layoutKeyboardControls
{
  const CGRect bounds = self.bounds;
  const UIEdgeInsets safeInsets = self.safeAreaInsets;

  editBG.frame = bounds;
  [editToolbar layoutIfNeeded];
  const CGRect editToolbarFrame = editToolbar.frame;

  const CGFloat fieldAreaLeft = safeInsets.left + EDIT_FIELD_MARGIN;
  const CGFloat fieldAreaRight = toolbar_items_min_x(editToolbar, self, bounds.size.width - safeInsets.right) - EDIT_FIELD_MARGIN;
  CGRect editFieldFrame;
  editFieldFrame.size.width = fmax(0.0,
    fmin(bounds.size.width * EDIT_FIELD_WIDTH_SCALE - safeInsets.left - safeInsets.right, fieldAreaRight - fieldAreaLeft));
  editFieldFrame.size.height = EDIT_FIELD_HEIGHT;
  editFieldFrame.origin.x = fieldAreaLeft + (fieldAreaRight - fieldAreaLeft - editFieldFrame.size.width) / 2;
  editFieldFrame.origin.y = CGRectGetMidY(editToolbarFrame) - editFieldFrame.size.height / 2;
  editField.frame = editFieldFrame;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  if (kbdVisible)
    [self layoutKeyboardControls];
}

- (BOOL)textField:(UITextField *)_textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
  if (!on_editfield_finish_cb) // means not IME mode
  {
    if ([string length] == 0)
    {
      workcycle_internal::main_window_proc(self.mainwnd, GPCM_KeyPress, HumanInput::DKEY_BACK, 0);
      workcycle_internal::main_window_proc(self.mainwnd, GPCM_KeyRelease, HumanInput::DKEY_BACK, 0);
    }
    else
    {
      for (int i = 0; i < [string length]; i++)
      {
        workcycle_internal::main_window_proc(self.mainwnd, GPCM_Char, [string characterAtIndex:i], 0);
      }
    }
    return NO;
  }

  if (_textField.markedTextRange)
    return YES;

  NSString *text = _textField.text ?: @"";
  BOOL applyManually = NO;
  if (_textField.secureTextEntry)
  {
    const NSRange sel = [self editFieldSelectionUtf16];
    if (range.location == 0 && range.length == text.length && !NSEqualRanges(sel, range))
    {
      range = sel;
      if (string.length == 0 && range.length == 0 && range.location > 0) // backspace
        range = [text rangeOfComposedCharacterSequenceAtIndex:range.location - 1];
    }
    applyManually = YES;
  }
  if (NSMaxRange(range) > text.length)
    return NO;

  if (maxChars > 0)
  {
    const NSInteger allowed = maxChars - (codepoints_count(text) - codepoints_count([text substringWithRange:range]));
    if (codepoints_count(string) > allowed)
    {
      if (allowed <= 0)
        return NO;
      string = truncate_to_codepoints(string, allowed);
      applyManually = YES;
    }
  }

  if (!applyManually)
    return YES;
  _textField.text = [text stringByReplacingCharactersInRange:range withString:string];
  [self setEditFieldCursorUtf16:range.location + string.length];
  return NO;
}

- (void)editFieldChanged
{
  if (!on_editfield_finish_cb || maxChars <= 0 || editField.markedTextRange)
    return;
  NSString *text = editField.text ?: @"";
  const NSInteger excess = codepoints_count(text) - maxChars;
  if (excess <= 0)
    return;

  const NSUInteger cursor = [self editFieldSelectionUtf16].location;
  NSUInteger start = cursor;
  NSInteger removed = 0;
  while (removed < excess && start > 0)
  {
    const NSRange r = [text rangeOfComposedCharacterSequenceAtIndex:start - 1];
    removed += codepoints_count([text substringWithRange:r]);
    start = r.location;
  }
  NSString *result = [text stringByReplacingCharactersInRange:NSMakeRange(start, cursor - start) withString:@""];
  if (removed < excess)
    result = truncate_to_codepoints(result, maxChars);
  editField.text = result;
  [self setEditFieldCursorUtf16:start];
}

- (BOOL)textFieldShouldReturn:(UITextField *)_textField
{
  [self hideKeyboard];
  return YES;
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
  [self hideKeyboard];
}

- (void)clearPointers
{
  for (int i = 0; i < MAX_SIMULTANEOUS_TOUCHES; i++)
  {
    if (pointers[i])
    {
      [(UITouch *)(pointers[i]) release];
      pointers[i] = NULL;
    }
  }
}
#endif

@end

static GCEventViewController *eventViewControler = NULL;
GCEventViewController *get_EventViewController() { return eventViewControler; }

#if _TARGET_IOS

static DagorView *get_main_dagor_view()
{
  if (UIViewController *mainviewctrl = getMainController())
    if ([mainviewctrl.view isKindOfClass:[DagorView class]])
      return (DagorView *)mainviewctrl.view;
  return nil;
}

void HumanInput::showScreenKeyboard(bool show)
{
  if (DagorView *view = get_main_dagor_view())
  {
    if (show)
      [view showKeyboardRaw];
    else
      [view hideKeyboard];
  }
}

bool HumanInput::showScreenKeyboardIME_iOS(const ScreenKeyboardParamsIOS &params,
  void (*on_finish_cb)(const char *str, int cursor, int status))
{
  DagorView *view = get_main_dagor_view();
  if (!view)
    return false;
  if (view.kbdVisible && on_editfield_finish_cb)
  {
    logwarn("iOS: IME is already shown, close it before showing a new one");
    return false;
  }
  on_editfield_finish_cb = on_finish_cb;
  if ([view showKeyboardIME:params])
    return true;
  on_editfield_finish_cb = NULL;
  return false;
}
#endif

#if _TARGET_IOS && __IPHONE_OS_VERSION_MAX_ALLOWED >= 270100
API_AVAILABLE(ios(27.1)) static HingeState to_hinge_state(UIHinge *hinge)
{
  HingeState state;
  if (!hinge) // the interaction left a hierarchy that gives hinge updates
    return state;
  state.angle = hinge.angle;
  switch (hinge.status)
  {
    case UIHingeStatusClosed: state.status = HingeStatus::CLOSED; break;
    case UIHingeStatusPartiallyOpen: state.status = HingeStatus::PARTIALLY_OPEN; break;
    case UIHingeStatusFullyOpen: state.status = HingeStatus::FULLY_OPEN; break;
    default: state.status = HingeStatus::UNKNOWN; break;
  }
  return state;
}
#endif

UIWindow *createIOSDagorWindow(UIView *drawView, float scale)
{
  UIWindowScene *scene = ios_get_window_scene();
  UIWindow *mainwnd = scene ? [[UIWindow alloc] initWithWindowScene:scene] : [[UIWindow alloc] initWithFrame:ios_get_screen().bounds];
  CGRect rect = mainwnd.bounds;
  if ([scene.delegate respondsToSelector:@selector(setWindow:)])
    [(id<UIWindowSceneDelegate>)scene.delegate setWindow:mainwnd];

  DagorView *mainview = [[DagorView alloc] initWithFrame:rect];
  mainview.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  drawView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [mainview addSubview:drawView];
  [mainwnd addSubview:mainview];
  mainview.mainwnd = mainwnd;

#if _TARGET_IOS
  DagorViewCtrl *mainviewctrl = [[DagorViewCtrl alloc] init];
  mainviewctrl.view = mainview;

  mainwnd.rootViewController = mainviewctrl;
  storedMainViewCtrl = mainviewctrl;

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 270100
  if (@available(iOS 27.1, *))
  {
    UIHingeInteraction *hinge =
      [[UIHingeInteraction alloc] initWithUpdateHandler:^(UIHingeInteraction *, UIHingeInteractionUpdate *update) {
        set_hinge_state(to_hinge_state(update.hinge));
      }];
    [mainview addInteraction:hinge];
    [hinge release];
  }
#endif

#elif _TARGET_TVOS
  mainview.eventViewController = [[DagorViewCtrl alloc] initWithView:mainview];
  eventViewControler = mainview.eventViewController;

  mainview.matchController = [[MatchController alloc] initWithView:mainview];
  apple::kit.initialize();

  [mainwnd setRootViewController:eventViewControler];
#endif

  [mainwnd layoutIfNeeded];

  [mainwnd makeKeyAndVisible];

  return mainwnd;
}

void DagorWinMainInit(int nCmdShow, bool debugmode);
int DagorWinMain(int nCmdShow, bool debugmode);

extern void default_crt_init_kernel_lib();
extern void default_crt_init_core_lib();
extern void apply_hinstance(void *hInstance, void *hPrevInstance);
extern void macosx_hide_windows_on_alert();

extern "C" const char *dagor_exe_build_date;
extern "C" const char *dagor_exe_build_time;

extern void messagebox_report_fatal_error(const char *title, const char *msg, const char *call_stack);

#if _TARGET_TVOS
// need to correct close log when catch crash
namespace debug_internal
{
void force_close_log();
}

struct LogGuardtvOS
{
  ~LogGuardtvOS() { debug_internal::force_close_log(); }
};
#endif

static int rotate_debug_files(const char *log_name, const char *debug_path, const char *debug_ext, const int count)
{
  char debug[DAGOR_MAX_PATH];
  memset(debug, '\0', DAGOR_MAX_PATH);
  SNPRINTF(debug, sizeof(debug), "%s/", debug_path);

  const int filenameOffset = (int)strlen(debug);
  const int debugBufferLenght = sizeof(debug) - filenameOffset;
  char *debugFilename = debug + filenameOffset;

  SNPRINTF(debugFilename, debugBufferLenght, "%s-0%s", log_name, debug_ext);

  // Step 0: Check 'debug[0..Count]' and return first not existing log index
  for (int logIndex = 0; logIndex <= count;)
  {
    if (!dd_file_exists(debug))
      return logIndex;

    SNPRINTF(debugFilename, debugBufferLenght, "%s-%d%s", log_name, ++logIndex, debug_ext);
  }

  // Step 1: Remove 'debug0' - the oldest log file
  SNPRINTF(debugFilename, debugBufferLenght, "%s-0%s", log_name, debug_ext);
  if (unlink(debug) != 0)
    return 0;

  char debug1[DAGOR_MAX_PATH];
  memcpy(debug1, debug, DAGOR_MAX_PATH); // For copy nullterminators after string end

  // Step 2: Shift log names like debug1 -> debug0, debug2 -> debug1, etc.
  for (int logIndex = 1; logIndex <= count; ++logIndex)
  {
    SNPRINTF(debugFilename, debugBufferLenght, "%s-%d%s", log_name, logIndex, debug_ext);
    SNPRINTF(debug1 + filenameOffset, sizeof(debug1) - filenameOffset, "%s-%d%s", log_name, logIndex - 1, debug_ext);

    if (!dd_rename(debug, debug1))
      return 0;
  }

  return count;
}

// Cleanup log folder from old logs (old name formatting)
static void remove_old_logs(const char *debug_path)
{
  @autoreleasepool
  {
    NSString *debugPath = [[NSString alloc] initWithUTF8String:debug_path];
    NSFileManager *fileManager = [NSFileManager defaultManager];

    NSError *error = nil;
    NSArray *files = [fileManager contentsOfDirectoryAtPath:debugPath error:&error];

    if (error)
      return;

    // Remove files like: aces-*.clog aces-*.log
    for (NSString *log : files)
      if (strstr([log UTF8String], "aces-"))
      {
        NSString *fullPath = [debugPath stringByAppendingPathComponent:log];
        BOOL isDir = NO;
        if ([fileManager fileExistsAtPath:fullPath isDirectory:&isDir] && !isDir)
          [fileManager removeItemAtPath:fullPath error:&error];
      }
  }
}

extern void setup_debug_system(const char *exe_fname, const char *prefix, const char *debug_fname, bool datetime_name,
  const size_t rotated_count);

static void dagor_ios_before_main_init(int argc, char *argv[])
{
  static DagorSettingsBlkHolder stgBlkHolder;

  measure_cpu_freq();

#if defined OPTIONAL_ADD_ARGS
  OPTIONAL_ADD_ARGS(argc, argv);
#endif

  dgs_init_argv(argc, argv);

#if !_TARGET_TVOS
#if DAGOR_DBGLEVEL == 0
  stderr = freopen("/dev/null", "wt", stderr);
#endif
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString *documentsDirectory = [paths objectAtIndex:0];
  {
    remove_old_logs([documentsDirectory UTF8String]);

    const char *logsDirName = ::dgs_get_argv("logsDirName");
    const bool useDateTimeName = logsDirName ? false : (DAGOR_DBGLEVEL > 0);
    const size_t useRotatedLogs = logsDirName ? DAGOR_DBGLEVEL > 0 ? 0 : 3 : 3;
    setup_debug_system(logsDirName ? logsDirName : argv[0], [documentsDirectory UTF8String], "debug", useDateTimeName, useRotatedLogs);
  }

  pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif

  dgs_report_fatal_error = messagebox_report_fatal_error;
  apply_hinstance(NULL, NULL);

  set_debug_console_ios_file_output(true);
  set_copy_debug_to_ios_console(DAGOR_DBGLEVEL > 0);

#if DAGOR_DBGLEVEL != 0
  debug("BUILD TIMESTAMP:   %s %s\n\n", dagor_exe_build_date, dagor_exe_build_time);
#endif
  debug("app: <%s>", get_log_filename());

  DagorHwException::setHandler("main");

  // writing to a socket closed by the peer leads to SIGPIPE, process will exit if it is not ignored
  signal(SIGPIPE, SIG_IGN);
}

static void dagor_ios_main_init_impl()
{
  default_crt_init_kernel_lib();

  dagor_init_base_path();
  dagor_change_root_directory(::dgs_get_argv("rootdir"));

  ::DagorWinMainInit(0, 0);

  default_crt_init_core_lib();
}

extern "C" int dagor_ios_main_init(int argc, char *argv[])
{
  DEBUG_CP();
  dagor_ios_before_main_init(argc, argv);
  dagor_ios_main_init_impl();
  return 0;
}

extern "C" void dagor_ios_delegate_init()
{
  id<CHHapticDeviceCapability> hapticCapability = [CHHapticEngine capabilitiesForHardware];
  bool isHapticSupported = [hapticCapability supportsHaptics];
  if (isHapticSupported)
  {
    NSError *error;
    haptic::g_hapticEngine = [[CHHapticEngine alloc] initAndReturnError:&error];
    [haptic::g_hapticEngine setResetHandler:^{
      // Try Restarting
      NSError *startupError;
      [haptic::g_hapticEngine startAndReturnError:&startupError];
    }];
  }
}

extern "C" void dagor_ios_delegate_main(id delegate)
{
  DEBUG_CP();
  ios_wait_for_window_scene();
  [delegate performSelector:@selector(createDisplayLink)];
  int res = DagorWinMain(0, 0);
  DEBUG_CP();
  quit_game(res);
  DEBUG_CP();
}

extern "C" void dagor_ios_delegate_createDisplayLink(id target, SEL selector)
{
#if _TARGET_IOS
  g_displayLink = [CADisplayLink displayLinkWithTarget:target selector:selector];
  g_displayLink.preferredFramesPerSecond = g_ios_fps_limit;
  [g_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
#endif
}

extern "C" void dagor_ios_delegate_step(CADisplayLink*)
{
  if (g_frame_callback)
  {
    if (!g_ios_pause_rendering && !g_ios_wait_for_ui_events)
      g_frame_callback();
  }
  else if (!g_ios_pause_rendering)
    logerr("error! frame callback isn't set for iOS");
}

extern "C" void dagor_ios_delegate_applicationWillFinishLaunching(UIApplication*)
{
  DEBUG_CP();
}

extern "C" void dagor_ios_delegate_applicationDidFinishLaunching(UIApplication*, id target, SEL selector)
{
  DEBUG_CP();
  flush_debug_file();
  [NSTimer scheduledTimerWithTimeInterval:(0) target:target selector:selector userInfo:nil repeats:NO];
}

extern "C" void dagor_ios_delegate_applicationWillTerminate(UIApplication*)
{
  DEBUG_CP();
  debug("applicationWillTerminate, do _Exit(0)");
  flush_debug_file();
  _Exit(0);
}

extern "C" void dagor_ios_delegate_applicationWillResignActive(UIApplication*)
{
  DEBUG_CP();
  flush_debug_file();
  workcycle_internal::main_window_proc(NULL, GPCM_Activate, GPCMP1_Inactivate, 0);

  g_ios_was_paused_before_sleep = g_ios_pause_rendering;
  g_ios_pause_rendering = true;

  if (dagor_ios_active_status_handler)
    dagor_ios_active_status_handler(g_ios_main_view_is_active && !g_ios_pause_rendering);
}

extern "C" void dagor_ios_delegate_applicationDidBecomeActive(UIApplication*)
{
  DEBUG_CP();
  workcycle_internal::main_window_proc(NULL, GPCM_Activate, GPCMP1_Activate, 0);
  if (g_onApplicationDidBecomeActiveFunc)
    g_onApplicationDidBecomeActiveFunc();

  setUIInterfaceOrientationFromActiveWindow();

  g_ios_pause_rendering = g_ios_was_paused_before_sleep;

  if (dagor_ios_active_status_handler)
    dagor_ios_active_status_handler(g_ios_main_view_is_active && !g_ios_pause_rendering);

  apple::excludeSystemFoldersFromBackup();
}

extern "C" void dagor_ios_delegate_applicationDidReceiveMemoryWarning(UIApplication*)
{
  int freeMemMb = 0, totalMemMb = 0;
  systeminfo::get_physical_memory_free(freeMemMb);
  systeminfo::get_physical_memory(totalMemMb);
  logwarn("running out of memory. free memory is %d out of %d", freeMemMb, totalMemMb);
}

extern "C" BOOL dagor_ios_delegate_openURL(UIApplication *application, NSURL *url, NSString *sourceApplication, id annotation)
{
  if (g_onOpenUrlAppWithAnnotationFunc)
    return g_onOpenUrlAppWithAnnotationFunc(application, url, sourceApplication, annotation);

  return NO;
}

extern "C" void dagor_ios_delegate_didRegisterForRemoteNotificationsWithDeviceToken(UIApplication *application, NSData *deviceToken)
{
  DEBUG_CP();
#if DAGOR_DBGLEVEL > 0
  // internal notification system register
  const int token_len = [deviceToken length];
  const unsigned char *token_bytes = (const unsigned char *)[deviceToken bytes];
  char out_str_buf[100];
  for (int i = 0; i < token_len; ++i)
    snprintf(out_str_buf + i * 2, sizeof(out_str_buf) - i * 2, "%.2X", token_bytes[i]);

  logwarn("PN: APNs device token retrieved: %s",out_str_buf);
#endif
  if (g_onRegisterRemoteToken)
    g_onRegisterRemoteToken(application,deviceToken);
  // TODO: With swizzling disabled you must set the APNs device token here.
  // [FIRMessaging messaging].APNSToken = deviceToken;
}

extern "C" void dagor_ios_delegate_didFailToRegisterForRemoteNotificationsWithError(UIApplication *application, NSError *error)
{
  DEBUG_CP();
  logwarn([[NSString stringWithFormat:@"PN: Fail to register for remote notifications with error %@", error] UTF8String]);
}

using FetchCompletionHandler = void (^)(UIBackgroundFetchResult);
extern "C" void dagor_ios_delegate_didReceiveRemoteNotification(UIApplication *application, NSDictionary *userInfo, FetchCompletionHandler completionHandler)
{
  // If you are receiving a notification message while your app is in the background,
  // this callback will not be fired till the user taps on the notification launching the application.
  // TODO: Handle data of notification

  // With swizzling disabled you must let Messaging know about the message, for Analytics
  // [[FIRMessaging messaging] appDidReceiveMessage:userInfo];
#if DAGOR_DBGLEVEL > 0
  // Print message ID.
  if (userInfo[kGCMMessageIDKey])
    logdbg([[NSString stringWithFormat:@"PN: Did receive notification Message ID: %@", userInfo[kGCMMessageIDKey]] UTF8String]);
#endif

  completionHandler(UIBackgroundFetchResultNewData);
}

extern "C" BOOL dagor_ios_delegate_didFinishLaunchingWithOptions(id delegate, UIApplication *application, NSDictionary *launchOptions)
{
  DEBUG_CP();
  [delegate performSelector:@selector(applicationDidFinishLaunching:) withObject: application];
  if (g_onDidFinishLaunchingWithOptionsFunc)
    g_onDidFinishLaunchingWithOptionsFunc(application, launchOptions);

  return YES;
}

extern "C" BOOL dagor_ios_delegate_continueUserActivity(UIApplication *application, NSUserActivity *userActivity, void (^restorationHandler)(NSArray *))
{
  DEBUG_CP();
  if (g_onContinueUserActivityFunc)
    return g_onContinueUserActivityFunc(application, userActivity);

  return NO;
}

extern "C" void dagor_ios_delegate_dealloc()
{
  DEBUG_CP();
  flush_debug_file();
}

int main(int argc, char *argv[])
{
  dagor_ios_before_main_init(argc, argv);

#if _TARGET_TVOS
  LogGuardtvOS logguard;
#endif

  DAGOR_TRY
  {
    dagor_ios_main_init_impl();

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    UIApplicationMain(argc, argv, NULL, @"Dagor2_AppDelegate");
    [pool release];
  }
  DAGOR_CATCH(DagorException e)
  {
#ifdef DAGOR_EXCEPTIONS_ENABLED
    DagorHwException::reportException(e, true);
#endif
#if _TARGET_TVOS
  }
  DAGOR_CATCH(NSException * exception)
  {
#if DAGOR_EXCEPTIONS_ENABLED
    debug("Exception name: %s", [[exception name] UTF8String]);
    debug("Exception reason: %s", [[exception reason] UTF8String]);
    debug("Exception description: %s", [[exception description] UTF8String]);
    NSLog(@"Exception - %@", [exception description]);
    exit(EXIT_FAILURE);
#endif
#endif
  }
  DAGOR_CATCH(...)
  {
    DEBUG_CTX("\nUnhandled exception:\n");
    _exit(2);
  }
  return 0;
}

namespace workcycle_internal
{
intptr_t main_window_proc(void *, unsigned, uintptr_t, intptr_t);
}

@implementation Dagor2_App
- (id)init
{
  if ((self = [super init]))
  {
    [self setDelegate:[[Dagor2_AppDelegate alloc] init]];
  }
  return self;
}

- (void)dealloc
{
  id delegate = [self delegate];
  if (delegate)
  {
    [self setDelegate:nil];
    [delegate release];
  }

  [super dealloc];
}
@end

#if _TARGET_IOS
// with a scene manifest in Info.plist UIKit sends the activation, URL and user activity events to the scene
// delegate and not to the app delegate
@interface DagorSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, retain) UIWindow *window;
@end

@implementation DagorSceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions
{
  DEBUG_CP();
  [self scene:scene openURLContexts:connectionOptions.URLContexts];
  for (NSUserActivity *activity in connectionOptions.userActivities)
    [self scene:scene continueUserActivity:activity];
}

- (void)sceneWillResignActive:(UIScene *)scene
{
  dagor_ios_delegate_applicationWillResignActive([UIApplication sharedApplication]);
}

- (void)sceneDidBecomeActive:(UIScene *)scene
{
  dagor_ios_delegate_applicationDidBecomeActive([UIApplication sharedApplication]);
}

- (void)scene:(UIScene *)scene openURLContexts:(NSSet<UIOpenURLContext *> *)URLContexts
{
  for (UIOpenURLContext *context in URLContexts)
    dagor_ios_delegate_openURL([UIApplication sharedApplication], context.URL, context.options.sourceApplication,
      context.options.annotation);
}

- (void)scene:(UIScene *)scene continueUserActivity:(NSUserActivity *)userActivity
{
  dagor_ios_delegate_continueUserActivity([UIApplication sharedApplication], userActivity, nil);
}

- (void)dealloc
{
  [_window release];
  [super dealloc];
}
@end
#endif

@implementation Dagor2_AppDelegate

+ (Dagor2_AppDelegate *)sharedAppDelegate
{
  return (Dagor2_AppDelegate *)[[UIApplication sharedApplication] delegate];
}

- (id)init
{
  self = [super init];

  DEBUG_CP();

  isActive = FALSE;
  isBkgModeSupported = NO;

  dagor_ios_delegate_init();

  return self;
}

- (void)main
{
  dagor_ios_delegate_main(self);
}

- (void)createDisplayLink
{
  dagor_ios_delegate_createDisplayLink(self, @selector(step:));
}

- (void)step:(CADisplayLink *)sender
{
  dagor_ios_delegate_step(sender);
}

- (void)applicationWillFinishLaunching:(UIApplication *)application
{
  dagor_ios_delegate_applicationWillFinishLaunching(application);
}

- (void)applicationDidFinishLaunching:(UIApplication *)application
{
  dagor_ios_delegate_applicationDidFinishLaunching(application, self, @selector(main));
}

- (void)applicationWillTerminate:(UIApplication *)application
{
  dagor_ios_delegate_applicationWillTerminate(application);
}

- (void)applicationWillResignActive:(UIApplication *)application
{
  dagor_ios_delegate_applicationWillResignActive(application);
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
  dagor_ios_delegate_applicationDidBecomeActive(application);
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
  dagor_ios_delegate_applicationDidReceiveMemoryWarning(application);
}

- (BOOL)application:(UIApplication *)app openURL:(NSURL *)url options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options
{
  return [self application:app openURL:url sourceApplication:nil annotation:options];
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
  return dagor_ios_delegate_openURL(application, url, sourceApplication, annotation);
}

// Requires 'aps-environment' entitlement
- (void)application:(UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken
{
  dagor_ios_delegate_didRegisterForRemoteNotificationsWithDeviceToken(application, deviceToken);
}

- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
  dagor_ios_delegate_didFailToRegisterForRemoteNotificationsWithError(application, error);
}

- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo
    fetchCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler
{
  dagor_ios_delegate_didReceiveRemoteNotification(application, userInfo, completionHandler);
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
  return dagor_ios_delegate_didFinishLaunchingWithOptions(self, application, launchOptions);
}

- (BOOL)application:(UIApplication *)application continueUserActivity:(NSUserActivity *)userActivity restorationHandler:(void (^)(NSArray<id<UIUserActivityRestoring>> *restorableObjects))restorationHandler
{
  return dagor_ios_delegate_continueUserActivity(application, userActivity, restorationHandler);
}

#if _TARGET_IOS
- (UISceneConfiguration *)application:(UIApplication *)application
  configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession
                                 options:(UISceneConnectionOptions *)options
{
  UISceneConfiguration *config = [UISceneConfiguration configurationWithName:nil sessionRole:connectingSceneSession.role];
  config.delegateClass = [DagorSceneDelegate class];
  return config;
}
#endif

- (void)dealloc
{
  dagor_ios_delegate_dealloc();
  [super dealloc];
}
@end
