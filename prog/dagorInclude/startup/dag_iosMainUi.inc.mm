// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)

#pragma once

#import <UIKit/UIKit.h>
#import <CoreHaptics/CoreHaptics.h>
#import <GameController/GameController.h>
#import <AuthenticationServices/AuthenticationServices.h>
#include <3d/dag_drv3dCmd.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>
#include <memory/dag_mem.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <humanInput/dag_hiCreate.h>
#include <humanInput/dag_hiKeybIds.h>
#include <util/dag_string.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <unistd.h>
#include "dag_addBasePathDef.h"
#include "dag_loadSettings.h"
#include "dag_iosCallback.h"

#if _TARGET_TVOS
#include "dag_tvosMainUi.h"
#endif

#include <pthread.h>

static eastl::function<void()> g_frame_callback;

static CADisplayLink *g_displayLink = nullptr;
int g_ios_fps_limit = 120;
extern bool g_ios_pause_rendering;
static bool g_ios_was_paused_before_sleep = false;
static bool g_ios_main_view_is_active = true;
UIInterfaceOrientation g_app_orientation = UIInterfaceOrientationUnknown;
UIInterfaceOrientation preferredLandscapeOrientation = UIInterfaceOrientationLandscapeLeft;

UIViewController *storedMainViewCtrl;

const int LOGIN_STATUS_INPROGRESS = -1;
const int LOGIN_STATUS_SUCCESS = 0;
const int LOGIN_STATUS_CANCEL = 1;
const int LOGIN_STATUS_FAIL = 2;

void (*dagor_ios_active_status_handler)(bool isVisible) = nullptr;
static apple_events_callback_with_dictionary g_onDidFinishLaunchingWithOptionsFunc = NULL;
static apple_events_callback g_onApplicationDidBecomeActiveFunc = NULL;
static apple_openurl_with_annotation_callback g_onOpenUrlAppWithAnnotationFunc = NULL;
static apple_register_remote_token_callback g_onRegisterRemoteToken = NULL;

static void (*on_editfield_finish_cb)(const char *str) = NULL;

NSString *const kGCMMessageIDKey = @"gcm.message_id";

bool dagor_fast_shutdown = false;

namespace apple
{

void setOnDidFinishLaunchingWithOptionsFunc(apple_events_callback_with_dictionary func)
{
  g_onDidFinishLaunchingWithOptionsFunc = func;
}

void setOnApplicationDidBecomeActiveFunc(apple_events_callback func) { g_onApplicationDidBecomeActiveFunc = func; }

void setOnOpenUrlApplication(apple_openurl_with_annotation_callback func) { g_onOpenUrlAppWithAnnotationFunc = func; }

void setOnRegisterRemoteToken(apple_register_remote_token_callback func) { g_onRegisterRemoteToken = func; }

} // namespace apple

UIInterfaceOrientation getAppOrientation() { return g_app_orientation; }

void setUIInterfaceOrientationFromActiveWindow()
{
  UIWindowScene *activeWindow = (UIWindowScene *)[[[UIApplication sharedApplication] windows] firstObject];
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

void setFPSLimit(int lim)
{
  g_ios_fps_limit = lim ? lim : 120;
  if (g_displayLink)
    g_displayLink.preferredFramesPerSecond = g_ios_fps_limit;
}

int ios_get_maximum_frames_per_second() { return [UIScreen mainScreen].maximumFramesPerSecond; }

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

static String globalJwtToken;
static int globalLoginResult = LOGIN_STATUS_INPROGRESS; // -1 means we're still polling

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
@interface DagorViewCtrl : UIViewController <ASAuthorizationControllerDelegate, ASAuthorizationControllerPresentationContextProviding>
#else
@interface DagorViewCtrl : DagorGameCenterEventViewController
#endif
- (id)initWithView:(UIView *)view;
- (void)login;
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


bool doAppleLogin()
{
  if (DagorViewCtrl *mainviewctrl = (DagorViewCtrl *)getStoredMainController())
  {
    globalJwtToken = "";
    globalLoginResult = LOGIN_STATUS_INPROGRESS;
    [mainviewctrl login];
    return true;
  }
  return false;
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

int pollAppleLogin(String &jwt)
{
  jwt = globalLoginResult == LOGIN_STATUS_SUCCESS ? globalJwtToken : "";
  return globalLoginResult;
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

- (BOOL)checkOSStatus:(OSStatus)status
{
  return status == noErr;
}

- (NSMutableDictionary *)keychainQueryForKey:(NSString *)key
{
  return [@{
    (__bridge id)kSecClass : (__bridge id)kSecClassGenericPassword,
    (__bridge id)kSecAttrService : key,
    (__bridge id)kSecAttrAccount : key,
    (__bridge id)kSecAttrAccessible : (__bridge id)kSecAttrAccessibleAfterFirstUnlock
  } mutableCopy];
}

- (BOOL)saveObject:(id)object forKey:(NSString *)key
{
  NSMutableDictionary *keychainQuery = [self keychainQueryForKey:key];
  // Deleting previous object with this key, because SecItemUpdate is more complicated.
  [self deleteObjectForKey:key];

  [keychainQuery setObject:[NSKeyedArchiver archivedDataWithRootObject:object] forKey:(__bridge id)kSecValueData];
  return [self checkOSStatus:SecItemAdd((__bridge CFDictionaryRef)keychainQuery, NULL)];
}

- (id)loadObjectForKey:(NSString *)key
{
  id object = nil;

  NSMutableDictionary *keychainQuery = [self keychainQueryForKey:key];

  [keychainQuery setObject:(__bridge id)kCFBooleanTrue forKey:(__bridge id)kSecReturnData];
  [keychainQuery setObject:(__bridge id)kSecMatchLimitOne forKey:(__bridge id)kSecMatchLimit];

  CFDataRef keyData = NULL;

  if ([self checkOSStatus:SecItemCopyMatching((__bridge CFDictionaryRef)keychainQuery, (CFTypeRef *)&keyData)])
    object = [NSKeyedUnarchiver unarchiveObjectWithData:(__bridge NSData *)keyData];

  if (keyData)
    CFRelease(keyData);

  return object;
}

- (BOOL)deleteObjectForKey:(NSString *)key
{
  NSMutableDictionary *keychainQuery = [self keychainQueryForKey:key];
  return [self checkOSStatus:SecItemDelete((__bridge CFDictionaryRef)keychainQuery)];
}

- (void)login
{
  extern char const *get_game_name();
  appName = [NSString stringWithFormat:@"%s_loginData", get_game_name()];
  ASAuthorizationAppleIDProvider *appleIDProvider = [[ASAuthorizationAppleIDProvider alloc] init];

  NSDictionary *data = [self loadObjectForKey:appName];
  NSString *userID = data ? [data objectForKey:@"userID"] : @"null";

  [appleIDProvider
    getCredentialStateForUserID:userID
                     completion:^(ASAuthorizationAppleIDProviderCredentialState credentialState, NSError *_Nullable error) {
                       if (credentialState == ASAuthorizationAppleIDProviderCredentialAuthorized)
                       {
                         globalJwtToken = [[data objectForKey:@"userJwt"] UTF8String];
                         globalLoginResult = LOGIN_STATUS_SUCCESS;
                       }
                       else
                       {
                         ASAuthorizationAppleIDRequest *request = [appleIDProvider createRequest];
                         request.requestedScopes = @[ ASAuthorizationScopeFullName, ASAuthorizationScopeEmail ];

                         ASAuthorizationController *authorizationController =
                           [[ASAuthorizationController alloc] initWithAuthorizationRequests:@[ request ]];
                         authorizationController.delegate = self;
                         authorizationController.presentationContextProvider = self;
                         [authorizationController performRequests];
                       }
                     }];
}
- (void)authorizationController:(ASAuthorizationController *)controller didCompleteWithError:(NSError *)error
{
  NSString *errorMsg = nil;
  NSString *errorDesc = @"";
  globalLoginResult = LOGIN_STATUS_FAIL; //fail by default
  switch (error.code)
  {
    case ASAuthorizationErrorCanceled:
      errorMsg = @"ASAuthorizationErrorCanceled";
      globalLoginResult = LOGIN_STATUS_CANCEL;
      break;
    case ASAuthorizationErrorFailed:
      errorMsg = @"ASAuthorizationErrorFailed";
      break;
    case ASAuthorizationErrorInvalidResponse:
      errorMsg = @"ASAuthorizationErrorInvalidResponse";
      break;
    case ASAuthorizationErrorNotHandled:
      errorMsg = @"ASAuthorizationErrorNotHandled";
      break;
    case ASAuthorizationErrorUnknown:
      errorMsg = @"ASAuthorizationErrorUnknown";
      break;
  }
  if (error.localizedDescription)
  {
    errorDesc = error.localizedDescription;
  }
  debug("Login error: %s %s", [errorMsg UTF8String], [errorDesc UTF8String] );
}
- (void)authorizationController:(ASAuthorizationController *)controller
   didCompleteWithAuthorization:(ASAuthorization *)authorization API_AVAILABLE(ios(13.0))
{
  if ([authorization.credential isKindOfClass:[ASAuthorizationAppleIDCredential class]])
  {
    ASAuthorizationAppleIDCredential *appleIDCredential = authorization.credential;
    if (appleIDCredential.identityToken == nil)
    {
      globalLoginResult = LOGIN_STATUS_FAIL;
      return;
    }

    NSString *idToken = [[NSString alloc] initWithData:appleIDCredential.identityToken encoding:NSUTF8StringEncoding];
    if (idToken == nil)
    {
      globalLoginResult = LOGIN_STATUS_FAIL;
    }
    else
    {
      NSDictionary *dict = @{@"userID" : appleIDCredential.user, @"userJwt" : idToken};
      [self saveObject:dict forKey:appName];
      globalJwtToken = [idToken UTF8String];
      globalLoginResult = LOGIN_STATUS_SUCCESS;
    }
  }
  else
  {
    globalLoginResult = LOGIN_STATUS_FAIL;
  }
}
- (ASPresentationAnchor)presentationAnchorForAuthorizationController:(ASAuthorizationController *)controller
{
  return self.view.window;
}
- (NSUInteger)supportedInterfaceOrientations
{
  return UIInterfaceOrientationMaskLandscape;
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
      // Stuff you used to do in willRotateToInterfaceOrientation would go here.
      // If you don't need anything special, you can set this block to nil.
      // if (size.width > size.height) {} else {};
    }
    completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
      // Stuff you used to do in didRotateFromInterfaceOrientation would go here.
      // If not needed, set to nil.
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
  UITextField *dummyField;
  UITextField *editField;
  UIToolbar *toolbar;
  UIBarButtonItem *textFieldToolBarItem;
  UIBarButtonItem *doneButton;
  BOOL kbdVisible;
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
- (void)showKeyboard;
- (void)showKeyboard_IME:(NSString *)text withHint:(NSString *)hint withIMEFlag:(NSInteger)ime_flag withKBFlag:(NSInteger)edit_flag withSecureMask:(BOOL)isSecure;
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
  self.contentScaleFactor = [UIScreen mainScreen].nativeScale;

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

- (void)initializeKeyboard
{

  CGRect applicationFrame = [[UIScreen mainScreen] applicationFrame];

  dummyField = [[[UITextField alloc] initWithFrame:CGRectMake(0, 0, 0, 0)] autorelease];
  dummyField.delegate = self;
  dummyField.text = @" ";
  dummyField.autocapitalizationType = UITextAutocapitalizationTypeNone;
  dummyField.autocorrectionType = UITextAutocorrectionTypeNo;
  dummyField.enablesReturnKeyAutomatically = NO;
  dummyField.keyboardAppearance = UIKeyboardAppearanceDefault;
  dummyField.keyboardType = UIKeyboardTypeDefault;
  dummyField.returnKeyType = UIReturnKeyDefault;
  dummyField.secureTextEntry = NO;

  editField = [[[UITextField alloc] initWithFrame:CGRectMake(0, 0, applicationFrame.size.width*0.8, 30)] autorelease];
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

  toolbar = [[[UIToolbar alloc]initWithFrame:CGRectMake(0, 0, [[UIScreen mainScreen] bounds].size.width, 30)] autorelease];
  doneButton = [[[UIBarButtonItem alloc]initWithTitle:@"Done" style:UIBarButtonItemStyleDone target:self action:@selector(doneClicked:)] autorelease];
  UIBarButtonItem *flexibleSpace = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil] autorelease];
  textFieldToolBarItem = [[UIBarButtonItem alloc] initWithCustomView:editField];
  toolbar.items = [NSArray arrayWithObjects:
                              flexibleSpace,
                              textFieldToolBarItem,
                              doneButton,
                              flexibleSpace,
                              nil];
  [toolbar sizeToFit];


  [dummyField setInputAccessoryView:toolbar];

  kbdVisible = NO;

  [self addSubview:dummyField];
}

-(void)doneClicked:(id)sender
{
  if (on_editfield_finish_cb)
    on_editfield_finish_cb([editField.text UTF8String]);
  [self hideKeyboard];

}

- (void)showKeyboard_IME:(NSString *)text withHint:(NSString *)hint withIMEFlag:(NSInteger)ime_flag withKBFlag:(NSInteger)edit_flag withSecureMask:(BOOL)isSecure
{
  dummyField.text = text;
  dummyField.placeholder = hint;
  dummyField.keyboardType = (UIKeyboardType)edit_flag;
  dummyField.returnKeyType = (UIReturnKeyType)ime_flag;
  dummyField.secureTextEntry = isSecure;

  editField.text = text;
  editField.placeholder = hint;
  editField.keyboardType = (UIKeyboardType)edit_flag;
  editField.returnKeyType = (UIReturnKeyType)ime_flag;
  editField.secureTextEntry = isSecure;

  kbdVisible = YES;
  [dummyField becomeFirstResponder];
  [editField becomeFirstResponder];
 }

- (void)showKeyboard
{
  kbdVisible = YES;
  dummyField.center = self.center;
  [dummyField becomeFirstResponder];
  [editField becomeFirstResponder];
}

- (void)hideKeyboard
{
  kbdVisible = NO;
  [editField resignFirstResponder];
  [dummyField resignFirstResponder];
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
  return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)_textField
{
  if (on_editfield_finish_cb && _textField == editField)
    on_editfield_finish_cb([editField.text UTF8String]);
  [self hideKeyboard];
  return YES;
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

void HumanInput::showScreenKeyboard(bool show)
{
  if (DagorViewCtrl *mainviewctrl = (DagorViewCtrl*)getMainController())
    if (DagorView *view = (DagorView *)mainviewctrl.view)
    {
      if (show)
        [view showKeyboard];
      else
        [view hideKeyboard];
    }
}
void HumanInput::showScreenKeyboardIME_iOS(const char *text, const char *hint, unsigned int ime_flag, unsigned int edit_flag, bool isSecure, void(on_finish_cb)(const char *str))
{
  if (DagorViewCtrl *mainviewctrl = (DagorViewCtrl*)getMainController())
    if (DagorView *view = (DagorView *)mainviewctrl.view)
    {
      on_editfield_finish_cb = on_finish_cb;
      [view showKeyboard_IME:[NSString stringWithUTF8String:text] withHint:[NSString stringWithUTF8String:hint] withIMEFlag:(NSInteger)ime_flag withKBFlag:(NSInteger)edit_flag withSecureMask:(BOOL)isSecure];
    }
}
#endif

UIWindow *createIOSDagorWindow(UIView *drawView, float scale)
{
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 90000
  CGRect rect = [[UIScreen mainScreen] bounds];
#else
  CGRect rect = [[UIScreen mainScreen] applicationFrame];
#endif

#if _TARGET_IOS
  [UIApplication sharedApplication].statusBarHidden = YES;
#endif
  UIWindow *mainwnd = [[UIWindow alloc] initWithFrame:rect];

  DagorView *mainview = [[DagorView alloc] initWithFrame:rect];
  [mainview addSubview:drawView];
  [mainwnd addSubview:mainview];
  mainview.mainwnd = mainwnd;
  mainview.scale = scale;

#if _TARGET_IOS
  DagorViewCtrl *mainviewctrl = [[DagorViewCtrl alloc] init];
  mainviewctrl.view = mainview;

  mainwnd.rootViewController = mainviewctrl;
  storedMainViewCtrl = mainviewctrl;

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

char ios_global_log_fname[1024] = "";

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

static void dagor_ios_before_main_init(int argc, char *argv[])
{
  static DagorSettingsBlkHolder stgBlkHolder;

  measure_cpu_freq();
#if !_TARGET_TVOS
#if DAGOR_DBGLEVEL == 0
  stderr = freopen("/dev/null", "wt", stderr);
#endif
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString *documentsDirectory = [paths objectAtIndex:0];
  {
    time_t rawtime;
    tm *t;
    char buf[64];

    time(&rawtime);
    t = localtime(&rawtime);
    _snprintf(buf, sizeof(buf), "%s-%04d.%02d.%02d-%02d.%02d.%02d.log", dd_get_fname(argv[0]), 1900 + t->tm_year, t->tm_mon + 1,
      t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    strcpy(ios_global_log_fname,
      [[documentsDirectory stringByAppendingPathComponent:[[NSString alloc] initWithUTF8String:buf]] UTF8String]);
  }

  pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif

  dgs_init_argv(argc, argv);
  dgs_report_fatal_error = messagebox_report_fatal_error;
  apply_hinstance(NULL, NULL);

#if DAGOR_DBGLEVEL != 0
  out_debug_str_fmt("BUILD TIMESTAMP:   %s %s\n\n", dagor_exe_build_date, dagor_exe_build_time);
#endif
  out_debug_str_fmt("app: <%s>", ios_global_log_fname);

  DagorHwException::setHandler("main");
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
  debug_cp();
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
  debug_cp();
  [delegate performSelector:@selector(createDisplayLink)];
  int res = DagorWinMain(0, 0);
  debug_cp();
  quit_game(res);
  debug_cp();
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
    if (!g_ios_pause_rendering)
      g_frame_callback();
  }
  else if (!g_ios_pause_rendering)
    logerr("error! frame callback isn't set for iOS");
}

extern "C" void dagor_ios_delegate_applicationWillFinishLaunching(UIApplication*)
{
  debug_cp();
}

extern "C" void dagor_ios_delegate_applicationDidFinishLaunching(UIApplication*, id target, SEL selector)
{
  debug_cp();
  flush_debug_file();
  [NSTimer scheduledTimerWithTimeInterval:(0) target:target selector:selector userInfo:nil repeats:NO];
}

extern "C" void dagor_ios_delegate_applicationWillTerminate(UIApplication*)
{
  debug_cp();
  debug("applicationWillTerminate, do _Exit(0)");
  flush_debug_file();
  _Exit(0);
}

extern "C" void dagor_ios_delegate_applicationWillResignActive(UIApplication*)
{
  debug_cp();
  flush_debug_file();
  workcycle_internal::main_window_proc(NULL, GPCM_Activate, GPCMP1_Inactivate, 0);

  g_ios_was_paused_before_sleep = g_ios_pause_rendering;
  g_ios_pause_rendering = true;

  if (dagor_ios_active_status_handler)
    dagor_ios_active_status_handler(g_ios_main_view_is_active && !g_ios_pause_rendering);
}

extern "C" void dagor_ios_delegate_applicationDidBecomeActive(UIApplication*)
{
  debug_cp();
  workcycle_internal::main_window_proc(NULL, GPCM_Activate, GPCMP1_Activate, 0);
  if (g_onApplicationDidBecomeActiveFunc)
    g_onApplicationDidBecomeActiveFunc();

  setUIInterfaceOrientationFromActiveWindow();

  g_ios_pause_rendering = g_ios_was_paused_before_sleep;

  if (dagor_ios_active_status_handler)
    dagor_ios_active_status_handler(g_ios_main_view_is_active && !g_ios_pause_rendering);
}

extern "C" void dagor_ios_delegate_applicationDidReceiveMemoryWarning(UIApplication*)
{
  logwarn("running out of memory");
}

extern "C" BOOL dagor_ios_delegate_openURL(UIApplication *application, NSURL *url, NSString *sourceApplication, id annotation)
{
  if (g_onOpenUrlAppWithAnnotationFunc)
    return g_onOpenUrlAppWithAnnotationFunc(application, url, sourceApplication, annotation);

  return NO;
}

extern "C" void dagor_ios_delegate_didRegisterForRemoteNotificationsWithDeviceToken(UIApplication *application, NSData *deviceToken)
{
  debug_cp();
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
  debug_cp();
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
  debug_cp();
  [delegate performSelector:@selector(applicationDidFinishLaunching:) withObject: application];
  if (g_onDidFinishLaunchingWithOptionsFunc)
    g_onDidFinishLaunchingWithOptionsFunc(application, launchOptions);

  return YES;
}

extern "C" void dagor_ios_delegate_dealloc()
{
  debug_cp();
  flush_debug_file();
}

extern "C" int main(int argc, char *argv[])
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
    debug_ctx("\nUnhandled exception:\n");
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

@implementation Dagor2_AppDelegate

+ (Dagor2_AppDelegate *)sharedAppDelegate
{
  return (Dagor2_AppDelegate *)[[UIApplication sharedApplication] delegate];
}

- (id)init
{
  self = [super init];

  debug_cp();

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

- (void)dealloc
{
  dagor_ios_delegate_dealloc();
  [super dealloc];
}
@end
