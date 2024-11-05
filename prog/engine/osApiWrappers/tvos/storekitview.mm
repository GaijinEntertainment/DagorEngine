// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_tvosMainUi.h>
#include <osApiWrappers/appstorekit.h>

namespace apple
{
  void setStoreKitRootView(DagorStoreKitView* view);
}

@implementation DagorStoreKitView
- (id)initWithFrame:(CGRect)frame
{
  if (self = [super initWithFrame: frame])
  {
    apple::setStoreKitRootView(self);
  }

  return self;
}
-(void)initApStorekit
{
  [ApStoreKit sharedKit];
}

-(void)addConsumbleItemId:(const char*)itemId
{
  NSString* itemIdStr = [[NSString alloc] initWithUTF8String:itemId];
  [[ApStoreKit sharedKit] addConsumableItem:itemIdStr];
}

-(void)restorePurchasedRecords
{
  //[[ApStoreKit sharedKit] restorePurchaseRecord];
  //[[ApStoreKit sharedKit] startValidatingReceiptsAndUpdateLocalStore];
  [[ApStoreKit sharedKit] startProductRequest];
}

-(void)buyApstoreItem:(const char*)itemName
{
  NSString* identifier =  [[NSString alloc] initWithUTF8String:itemName];
  [[ApStoreKit sharedKit] initiatePaymentRequestForProductWithIdentifier:identifier];
}

-(const char*)getItemPrice:(const char*)itemName
{
  NSString* identifier =  [[NSString alloc] initWithUTF8String:itemName];
  NSString* price = [[ApStoreKit sharedKit] getProductPrice:identifier];
  return [price cStringUsingEncoding:NSUTF8StringEncoding];
}

-(const char*)getVersion
{
  NSString* version = [[NSBundle mainBundle] objectForInfoDictionaryKey: @"CFBundleShortVersionString"];
  NSString* build = [[NSBundle mainBundle] objectForInfoDictionaryKey: (NSString *)kCFBundleVersionKey];
  NSString* versionBuild = versionBuild = [NSString stringWithFormat: @"%@(%@)", version, build];

  return [versionBuild cStringUsingEncoding:NSUTF8StringEncoding];
}

@end

namespace apple
{

static DagorStoreKitView* storekitView = nullptr;
void setStoreKitRootView(DagorStoreKitView* view)
{
  storekitView = view;
}

void Kit::initialize()
{
  if (storekitView)
    [storekitView initApStorekit];
}

void Kit::Store::addConsumbleItemId(const char* itemId)
{
  if (storekitView)
    [storekitView addConsumbleItemId:itemId];
}

void Kit::Store::restorePurchasedRecords()
{
  if (storekitView)
    [storekitView restorePurchasedRecords];
}

void Kit::Store::buyItem(const char* itemName)
{
  if (storekitView)
   [storekitView buyApstoreItem:itemName];
}

void Kit::Store::setChecker(Kit::Store::IChecker* checker)
{
  [[ApStoreKit sharedKit] setChecker:checker];
}

void Kit::Store::setCallback(Kit::Store::ICallback* callback)
{
  [[ApStoreKit sharedKit] setCallback:callback];
}

const char* Kit::Store::getItemPrice(const char* itemName)
{
  if (storekitView)
    return [storekitView getItemPrice:itemName];

  return "";
}

const char* Kit::Store::getVersion()
{
  if (storekitView)
    return [storekitView getVersion];

  return "unknown";
}

}//end namespace apple
