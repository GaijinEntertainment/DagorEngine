#import <UIKit/UIKit.h>
#import <GameKit/GameKit.h>
#import <CloudKit/CloudKit.h>
#include <osApiWrappers/dag_files.h>
#include <perfMon/dag_cpuFreq.h>

#include <math/random/dag_random.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_appstorekit.h>

@interface SaveGame : NSObject
-(id) init;
-(bool) saveData: (NSData *) data withName: (const char *) name;
-(NSData *) loadDataByName: (const char *) fname;
-(void)storeDidChange: (NSNotification *) notification;

@property (nonatomic, assign) bool cloudEnabled;

@end

@implementation SaveGame

-(id) init
{
  if (self=[super init])
  {
    self.cloudEnabled = false;
    // register to observe notifications from the store
    [[NSNotificationCenter defaultCenter]
        addObserver: self
           selector: @selector (storeDidChange:)
               name: NSUbiquitousKeyValueStoreDidChangeExternallyNotification
             object: [NSUbiquitousKeyValueStore defaultStore]];

    // get changes that might have happened while this
    // instance of your app wasn't running
    [[NSUbiquitousKeyValueStore defaultStore] synchronize];
  }

  return self;
}

-(void)fetchCloudToken
{
   [[CKContainer defaultContainer] accountStatusWithCompletionHandler:^(CKAccountStatus accountStatus, NSError *error)
   {
    if (error)
    {
      NSLog(@"An error occured in %@: %@", NSStringFromSelector(_cmd), error);
      /*dispatch_async(dispatch_get_main_queue(), ^{
        completionHandler(NO, error);
      });*/
    }
    else if (accountStatus == CKAccountStatusAvailable)
    {
      self.cloudEnabled = true;
      [[CKContainer defaultContainer] requestApplicationPermission:CKApplicationPermissionUserDiscoverability completionHandler:^(CKApplicationPermissionStatus applicationPermissionStatus, NSError *error) {
        if (error)
          NSLog(@"An error occured in %@: %@", NSStringFromSelector(_cmd), error);
        /*dispatch_async(dispatch_get_main_queue(), ^(void) {
          completionHandler(applicationPermissionStatus == CKApplicationPermissionStatusGranted, error);
        });*/
      }];
    }
    else if (accountStatus == CKAccountStatusNoAccount)
    {
      self.cloudEnabled = false;
      /*dispatch_async(dispatch_get_main_queue(), ^{
        completionHandler(NO, [NSError errorWithDomain:@"discoverability" code:404 userInfo:@{NSLocalizedDescriptionKey : @"Sign in to your iCloud account to write records. On the Home screen, launch Settings, tap iCloud, and enter your Apple ID. Turn iCloud Drive on. If you don't have an iCloud account, tap Create a new Apple ID."}]);
      });*/
    }
    else  if (accountStatus == CKAccountStatusRestricted)
    {
      self.cloudEnabled = false;
      /*dispatch_async(dispatch_get_main_queue(), ^{
        completionHandler(NO, [NSError errorWithDomain:@"discoverability" code:404 userInfo:@{NSLocalizedDescriptionKey : @"Sorry your access is restricted by parental controls."}]);
      });*/
    }
    else
    {
      self.cloudEnabled = false;
      /*dispatch_async(dispatch_get_main_queue(), ^{
        completionHandler(NO, [NSError errorWithDomain:@"discoverability" code:404 userInfo:@{NSLocalizedDescriptionKey : @"Unknown error occurred."}]);
      });*/
    }
  }];
}

-(void)storeDidChange: (NSNotification *) notification{
  // Get the list of keys that changed.
   NSDictionary *userInfo = [notification userInfo];
   NSNumber *reasonForChange = [userInfo objectForKey: NSUbiquitousKeyValueStoreChangeReasonKey];
   NSInteger reason = -1;

   // If a reason could not be determined, do not update anything.
   if (!reasonForChange)
      return;

   // Update only for changes from the server.
   reason = [reasonForChange integerValue];
   if ((reason == NSUbiquitousKeyValueStoreServerChange) ||
         (reason == NSUbiquitousKeyValueStoreInitialSyncChange))
   {
      // If something is changing externally, get the changes
      // and update the corresponding keys locally.
      NSArray *changedKeys = [userInfo objectForKey: NSUbiquitousKeyValueStoreChangedKeysKey];
      NSUbiquitousKeyValueStore *store = [NSUbiquitousKeyValueStore defaultStore];
      NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];

      // This loop assumes you are using the same key names in both
      // the user defaults database and the iCloud key-value store
      for (NSString *key in changedKeys)
      {
         id value = [store objectForKey: key];
         [userDefaults setObject: value forKey: key];
      }
   }
}

-(bool) saveString: (NSString *) value withName: (const char*) name
{
  NSUbiquitousKeyValueStore *store = [NSUbiquitousKeyValueStore defaultStore];

  [store setString: value forKey: [NSString stringWithUTF8String: name]];
  return [store synchronize];
}

-(NSString *) loadStringByName: (const char *) name
{
  NSUbiquitousKeyValueStore *store = [NSUbiquitousKeyValueStore defaultStore];
  return [store stringForKey: [NSString stringWithUTF8String: name]];
}

-(bool) saveData: (NSData *) data withName: (const char *) name
{
  NSUbiquitousKeyValueStore *store = [NSUbiquitousKeyValueStore defaultStore];
  [store setData: data forKey: [NSString stringWithUTF8String: name]];
  return [store synchronize];
}

-(NSData *) loadDataByName: (const char *) name
{
  NSUbiquitousKeyValueStore *store = [NSUbiquitousKeyValueStore defaultStore];
  return [store dataForKey: [NSString stringWithUTF8String: name]];
}
@end

namespace apple
{

namespace internal
{
  static SaveGame *save_game = nil;
  const uint8_t CLOUD_SEED_LEN = 32;
  static char cloud_seed[CLOUD_SEED_LEN];

  void initIfNeed()
  {
    if (!save_game)
    {
      save_game = [[SaveGame alloc] init];
      SNPRINTF(cloud_seed, CLOUD_SEED_LEN, "%d", get_time_msec());

      [save_game fetchCloudToken];

      NSString *str = [[NSString alloc] initWithUTF8String: cloud_seed];
      [save_game saveString: str withName: cloud_seed];
    }
  }
} //end namespace internal

bool Kit::Storage::saveData(void *bytes, size_t length, const char *name)
{
  internal::initIfNeed();

  NSData *data = [NSData dataWithBytes: bytes length: length];
  return [internal::save_game saveData: data withName: name];
}

bool Kit::Storage::loadData(void *bytes, size_t *length, const char *name)
{
  internal::initIfNeed();

  NSData *data = [internal::save_game loadDataByName: name];
  if (*length >= data.length)
  {
    memcpy(bytes, data.bytes, data.length);
    *length = data.length;
    return true;
  }
  return false;
}

bool Kit::Storage::saveString(const char *value, const char *name)
{
  internal::initIfNeed();

  NSString *str = [[NSString alloc] initWithUTF8String: value];
  return [internal::save_game saveString: str withName: name];
}

const char *Kit::Storage::loadString(const char *name)
{
  internal::initIfNeed();

  NSString *str = [internal::save_game loadStringByName: name];
  return [str cStringUsingEncoding: NSUTF8StringEncoding];
}

const char *Kit::Cloud::getToken()
{
  const char* result = kit.storage.loadString(internal::cloud_seed);

  if (result == nullptr || strlen(result) == 0)
     result = "unknown_token";

  return result;
}

bool Kit::Cloud::isTokenValid()
{
  const char *token = getToken();
  return strcmp(token, internal::cloud_seed) == 0;
}

bool Kit::Cloud::isEnabled()
{
  internal::initIfNeed();

  [internal::save_game fetchCloudToken];

  return internal::save_game.cloudEnabled;
}

bool Kit::Cache::saveData(const void *data, size_t size, const char *name)
{
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
  NSString *cacheDirectory = [paths objectAtIndex: 0];
  NSString *filepath = [cacheDirectory stringByAppendingPathComponent: [[NSString alloc] initWithUTF8String: name]];

  file_ptr_t f = df_open([filepath cStringUsingEncoding: NSUTF8StringEncoding], DF_WRITE | DF_CREATE);
  if (f)
  {
    int saved_size = df_write(f, data, size);
    bool success = 0 == df_close(f);
    return success && saved_size == size;
  }
  return false;
}

const char *Kit::Cache::getPath()
{
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
  NSString *cacheDirectory = [paths objectAtIndex: 0];
  return [cacheDirectory cStringUsingEncoding: NSUTF8StringEncoding];
}

}//end namespace apple
