// Copyright (C) Gaijin Games KFT.  All rights reserved.

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>

#include <startup/dag_tvosMainUi.h>
#include <osApiWrappers/appstorekit.h>
#include <debug/dag_debug.h>

NSString *const ITunesSandboxServer = @"https://sandbox.itunes.apple.com/verifyReceipt";
NSString *const ITunesLiveServer = @"https://buy.itunes.apple.com/verifyReceipt";

NSString *const ApOriginalAppVersionKey = @"ApSKOrigBundleRef"; // Obfuscating record key name
NSString *const ApPurchaseRecordFilePath = @"ApSKOrigBundleFPath";

static NSDictionary* errorDictionary;
static apple::Kit::Store::IChecker* checker = nullptr;
static apple::Kit::Store::ICallback* callbacks = nullptr;

@implementation ApStoreKit

#pragma mark -
#pragma mark Singleton Methods

+ (ApStoreKit *)sharedKit
{
  static ApStoreKit *_sharedKit;
  if (!_sharedKit)
  {
    static dispatch_once_t oncePredicate;
    dispatch_once(&oncePredicate, ^{
      _sharedKit = [[super allocWithZone:nil] init];
      _sharedKit.consumbleItems = [[NSMutableArray alloc]init];
      _sharedKit.storableItems = [[NSMutableArray alloc]init];
      _sharedKit.availableProducts = [[NSMutableArray alloc]init];

      errorDictionary = @{@(21000) : @"The App Store could not read the JSON object you provided.",
                          @(21002) : @"The data in the receipt-data property was malformed or missing.",
                          @(21003) : @"The receipt could not be authenticated.",
                          @(21004) : @"The shared secret you provided does not match the shared secret on file for your account.",
                          @(21005) : @"The receipt server is not currently available.",
                          @(21006) : @"This receipt is valid but the subscription has expired.",
                          @(21007) : @"This receipt is from the test environment.",
                          @(21008) : @"This receipt is from the production environment."};

      [[SKPaymentQueue defaultQueue] addTransactionObserver:_sharedKit];

#if _TARGET_IOS || _TARGET_TVOS
      [[NSNotificationCenter defaultCenter] addObserver:_sharedKit
                                               selector:@selector(savePurchaseRecord)
                                                   name:UIApplicationDidEnterBackgroundNotification object:nil];
#elif _TARGET_PC_MACOSX
      [[NSNotificationCenter defaultCenter] addObserver:_sharedKit
                                               selector:@selector(savePurchaseRecord)
                                                   name:NSApplicationDidResignActiveNotification object:nil];
#endif
    });
  }

  return _sharedKit;
}

- (void)setChecker:(apple::Kit::Store::IChecker*) ck
{
  checker = ck;
}

- (void)setCallback:(apple::Kit::Store::ICallback*) cb
{
  callbacks = cb;
}

- (const char*)getRealId:(NSString*)itemId
{
  const char* productId = [itemId cStringUsingEncoding:NSASCIIStringEncoding];
  const char* realId = productId;
  if (checker != nullptr)
    realId = checker->getConsumableId(productId);

  return realId;
}

- (void)addConsumableItem:(NSString*) itemId
{
  for (NSString* str in self.consumbleItems)
  {
    if ([str isEqualToString: itemId])
    {
       debug("[ApStoreKit] also contain with name %s", [itemId cStringUsingEncoding:NSASCIIStringEncoding]);
       return;
    }
  }

  [self.consumbleItems addObject:itemId];

#if DAGOR_DBGLEVEL > 0
  for (NSString* currentString in self.consumbleItems)
  {
    debug("[ApStoreKit] saved item = %s", [currentString cStringUsingEncoding:NSASCIIStringEncoding]);
  }
#endif
}

- (void)initiatePaymentRequestForProductWithIdentifier:(NSString *)productId
{
  if (!self.availableProducts)
  {
    // TODO: FIX ME
    // Initializer might be running or internet might not be available
    debug("No products are available. Did you initialize ApStoreKit by calling [[ApStoreKit sharedKit] startProductRequest]?");
  }

  if (![SKPaymentQueue canMakePayments])
  {
#if _TARGET_IOS || _TARGET_TVOS
    UIAlertController *controller = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"In App Purchasing Disabled", @"")
                                                                        message:NSLocalizedString(@"Check your parental control settings and try again later", @"") preferredStyle:UIAlertControllerStyleAlert];

    [[UIApplication sharedApplication].keyWindow.rootViewController
     presentViewController:controller animated:YES completion:nil];
#elif _TARGET_PC_MACOSX
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = NSLocalizedString(@"In App Purchasing Disabled", @"");
    alert.informativeText = NSLocalizedString(@"Check your parental control settings and try again later", @"");
    [alert runModal];
#endif
    return;
  }

  for (SKProduct* product in self.availableProducts)
  {
    if ([product.productIdentifier isEqualToString:productId])
    {
      SKPayment *payment = [SKPayment paymentWithProduct:product];
      [[SKPaymentQueue defaultQueue] addPayment:payment];
      return;
    }
  };
}


- (void)restorePurchaseRecord
{
  self.purchaseRecord = (NSMutableDictionary *)[[NSKeyedUnarchiver unarchiveObjectWithFile:ApPurchaseRecordFilePath] mutableCopy];
  if (self.purchaseRecord == nil)
  {
    self.purchaseRecord = [NSMutableDictionary dictionary];
  }
  debug("ApStoreKit: restored purchased records");
}

- (void)savePurchaseRecord
{
  NSError *e = nil;
  NSData *data = [NSKeyedArchiver archivedDataWithRootObject:self.purchaseRecord];
#if _TARGET_IOS || _TARGET_TVOS
  BOOL success = [data writeToFile:ApPurchaseRecordFilePath options:NSDataWritingAtomic | NSDataWritingFileProtectionComplete error:&e];
#elif _TARGET_PC_MACOSX
  BOOL success = [data writeToFile:ApPurchaseRecordFilePath options:NSDataWritingAtomic error:&error];
#endif

  if (!success)
  {
    debug("ApStoreKit: Failed to remember data record");
  }

  debug("ApStoreKit: save purchased data success");
}

- (BOOL)isProductPurchased:(NSString *)productId
{
  return [self.purchaseRecord.allKeys containsObject:productId];
}

- (NSString*)getProductPrice:(NSString*)productId
{
  for (SKProduct *product in self.availableProducts)
  {
    if ([product.productIdentifier isEqualToString:productId])
    {
        NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
        [formatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
        [formatter setNumberStyle:NSNumberFormatterCurrencyStyle];
        [formatter setLocale:product.priceLocale];

        NSString *str = [formatter stringFromNumber:product.price];
        [formatter release];
        return str;
    }
  }

  return @"ErrFmtPrc";
}

-(NSDate*) expiryDateForProduct:(NSString*) productId
{
  NSNumber *expiresDateMs = self.purchaseRecord[productId];
  if ([expiresDateMs isKindOfClass:NSNull.class])
  {
    return NSDate.date;
  }
  else
  {
    return [NSDate dateWithTimeIntervalSince1970:[expiresDateMs doubleValue] / 1000.0f];
  }
}

- (NSNumber *)availableCreditsForConsumable:(NSString *)consumableId
{
  return self.purchaseRecord[consumableId];
}

- (NSNumber *)consumeCredits:(NSNumber *)creditCountToConsume identifiedByConsumableIdentifier:(NSString *)consumableId
{
  NSNumber *currentConsumableCount = self.purchaseRecord[consumableId];
  currentConsumableCount = @([currentConsumableCount doubleValue] - [creditCountToConsume doubleValue]);
  self.purchaseRecord[consumableId] = currentConsumableCount;
  [self savePurchaseRecord];
  return currentConsumableCount;
}

- (void)setDefaultCredits:(NSNumber *)creditCount forConsumableIdentifier:(NSString *)consumableId
{
  if (self.purchaseRecord[consumableId] == nil)
  {
    self.purchaseRecord[consumableId] = creditCount;
    [self savePurchaseRecord];
  }
}

- (void)restorePurchases
{
  [[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

- (void)startProductRequestWithProductIdentifiers:(NSArray*) items
{
  SKProductsRequest *productsRequest = [[SKProductsRequest alloc]
                                        initWithProductIdentifiers:[NSSet setWithArray:items]];
  [productsRequest setDelegate:self];
  [productsRequest start];
}

- (void)startProductRequest
{
  NSMutableArray *productsArray = [NSMutableArray array];

  [productsArray addObjectsFromArray:self.consumbleItems];
  [productsArray addObjectsFromArray:self.storableItems];

  for (NSString* currentString in productsArray)
  {
    debug("ApStoreKit/startProductRequest: saved item = %s", [currentString cStringUsingEncoding:NSASCIIStringEncoding]);
  }

  [self startProductRequestWithProductIdentifiers:productsArray];
}

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
  if (response.invalidProductIdentifiers.count > 0)
  {
    NSArray<NSString *>* ids = response.invalidProductIdentifiers;
    for (NSString* currentId in ids)
    {
      const char* str = [currentId cStringUsingEncoding:NSASCIIStringEncoding];
      debug("ApStoreKit: invalid Product IDs: %s", str);
      if (callbacks)
        callbacks->onReceivedInvalidStoreItem(str);
    }
  }

  [self.availableProducts removeAllObjects];
  [self.availableProducts addObjectsFromArray:response.products];

  for (SKProduct *product in self.availableProducts)
  {
    const char* str = [product.productIdentifier cStringUsingEncoding:NSASCIIStringEncoding];
    debug("ApStoreKit: valid product IDs: %s", str);

    if (callbacks)
      callbacks->onReceivedValidStoreItem(str);
  }
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error
{
  debug("ApStoreKit: product request failed with error");
}

- (void)refreshAppStoreReceipt
{
  SKReceiptRefreshRequest *refreshReceiptRequest = [[SKReceiptRefreshRequest alloc] initWithReceiptProperties:nil];
  refreshReceiptRequest.delegate = self;
  [refreshReceiptRequest start];
}

- (void)requestDidFinish:(SKRequest *)request
{
  // SKReceiptRefreshRequest
  if([request isKindOfClass:[SKReceiptRefreshRequest class]])
  {
    NSURL *receiptUrl = [[NSBundle mainBundle] appStoreReceiptURL];
    if ([[NSFileManager defaultManager] fileExistsAtPath:[receiptUrl path]])
    {
      debug("App receipt exists. Preparing to validate and update local stores.");
      [self startValidatingReceiptsAndUpdateLocalStore];
    }
    else
    {
      debug("[AppKit] Receipt request completed but there is no receipt. The user may have refused to login, or the reciept is missing.");
      // Disable features of your app, but do not terminate the app
    }
  }
}

- (void)startValidatingAppStoreReceiptWithCompletionHandler:(void (^)(NSArray *receipts, NSError *error)) completionHandler
{
  NSURL *receiptURL = [[NSBundle mainBundle] appStoreReceiptURL];
  NSError *receiptError;
  BOOL isPresent = [receiptURL checkResourceIsReachableAndReturnError:&receiptError];
  if (!isPresent)
  {
    // No receipt - In App Purchase was never initiated
    completionHandler(nil, nil);
    return;
  }

  NSData *receiptData = [NSData dataWithContentsOfURL:receiptURL];
  if (!receiptData)
  {
    // Validation fails
    debug("[AppKit] Receipt exists but there is no data available. Try refreshing the reciept payload and then checking again.");
    completionHandler(nil, nil);
    return;
  }

  NSError *error;
  NSMutableDictionary *requestContents = [NSMutableDictionary dictionaryWithObject:
                                          [receiptData base64EncodedStringWithOptions:0] forKey:@"receipt-data"];
  NSString *sharedSecret = self.sharedSecret;
  if (sharedSecret)
    requestContents[@"password"] = sharedSecret;

  NSData *requestData = [NSJSONSerialization dataWithJSONObject:requestContents options:0 error:&error];

#ifdef DEBUG
  NSMutableURLRequest *storeRequest = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:ITunesSandboxServer]];
#else
  NSMutableURLRequest *storeRequest = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:ITunesLiveServer]];
#endif

  [storeRequest setHTTPMethod:@"POST"];
  [storeRequest setHTTPBody:requestData];

  NSURLSession *session = [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration]];

  [[session dataTaskWithRequest:storeRequest completionHandler:^(NSData *data, NSURLResponse *response, NSError *error)
  {
    if (!error)
    {
      NSDictionary *jsonResponse = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
      NSInteger status = [jsonResponse[@"status"] integerValue];

      if (jsonResponse[@"receipt"] != [NSNull null])
      {
        NSString *originalAppVersion = jsonResponse[@"receipt"][@"original_application_version"];
        if (nil != originalAppVersion)
        {
          [self.purchaseRecord setObject:originalAppVersion forKey:ApOriginalAppVersionKey];
          [self savePurchaseRecord];
        }
        else
        {
          completionHandler(nil, nil);
        }
      }
      else
      {
        completionHandler(nil, nil);
      }

      if (status != 0)
      {
        NSError *error = [NSError errorWithDomain:@"com.mugunthkumar.mkstorekit" code:status
                                         userInfo:@{NSLocalizedDescriptionKey : errorDictionary[@(status)]}];
        completionHandler(nil, error);
      }
      else
      {
        NSMutableArray *receipts = [jsonResponse[@"latest_receipt_info"] mutableCopy];
        if (jsonResponse[@"receipt"] != [NSNull null])
        {
          NSArray *inAppReceipts = jsonResponse[@"receipt"][@"in_app"];
          [receipts addObjectsFromArray:inAppReceipts];
          completionHandler(receipts, nil);
        }
        else
        {
          completionHandler(nil, nil);
        }
      }
    }
    else
    {
      completionHandler(nil, error);
    }
  }] resume];
}

- (BOOL)purchasedAppBeforeVersion:(NSString *)requiredVersion
{
  NSString *actualVersion = [self.purchaseRecord objectForKey:ApOriginalAppVersionKey];

  if ([requiredVersion compare:actualVersion options:NSNumericSearch] == NSOrderedDescending)
  {
    // actualVersion is lower than the requiredVersion
    return YES;
  }
  else
    return NO;
}

- (void)startValidatingReceiptsAndUpdateLocalStore
{
  [self startValidatingAppStoreReceiptWithCompletionHandler:^(NSArray *receipts, NSError *error)
  {
    if (error)
    {
      NSLog(@"Receipt validation failed with error: %@", error);
      //[[NSNotificationCenter defaultCenter] postNotificationName:ApStoreKitReceiptValidationFailedNotification object:error];
    }
    else
    {
      __block BOOL purchaseRecordDirty = NO;
      [receipts enumerateObjectsUsingBlock:^(NSDictionary *receiptDictionary, NSUInteger idx, BOOL *stop)
      {
        NSString *productIdentifier = receiptDictionary[@"product_id"];
        NSNumber *expiresDateMs = receiptDictionary[@"expires_date_ms"];
        if (expiresDateMs)
        { // renewable subscription
          NSNumber *previouslyStoredExpiresDateMs = self.purchaseRecord[productIdentifier];
          if (!previouslyStoredExpiresDateMs
              || [previouslyStoredExpiresDateMs isKindOfClass:NSNull.class])
          {
            self.purchaseRecord[productIdentifier] = expiresDateMs;
            purchaseRecordDirty = YES;
          }
          else
          {
            if ([expiresDateMs doubleValue] > [previouslyStoredExpiresDateMs doubleValue])
            {
              self.purchaseRecord[productIdentifier] = expiresDateMs;
              purchaseRecordDirty = YES;
            }
          }
        }
      }];

      if (purchaseRecordDirty)
      {
        [self savePurchaseRecord];
      }

      [self.purchaseRecord enumerateKeysAndObjectsUsingBlock:^(NSString *productIdentifier, NSNumber *expiresDateMs, BOOL *stop)
      {
        if (![expiresDateMs isKindOfClass: [NSNull class]])
        {
          if ([[NSDate date] timeIntervalSince1970] > [expiresDateMs doubleValue])
          {
            //[[NSNotificationCenter defaultCenter] postNotificationName:ApStoreKitSubscriptionExpiredNotification object:productIdentifier];
          }
        }
      }];
    }
  }];
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedDownloads:(NSArray *)downloads
{
  [downloads enumerateObjectsUsingBlock:^(SKDownload *thisDownload, NSUInteger idx, BOOL *stop)
  {
    SKDownloadState state;
#if _TARGET_IOS|_TARGET_TVOS
    state = thisDownload.downloadState;
#else
    state = thisDownload.state;
#endif
    switch (state)
    {
      case SKDownloadStateActive:
        if (callbacks)
        {
          const char* realId = [self getRealId:thisDownload.transaction.payment.productIdentifier];
          callbacks->onDownloadInProgess(realId, thisDownload.progress);
        }
      break;

      case SKDownloadStateFinished:
      {
        NSString *documentDirectory = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
        NSString *contentDirectoryForThisProduct =
        [[documentDirectory stringByAppendingPathComponent:@"Contents"]
         stringByAppendingPathComponent:thisDownload.transaction.payment.productIdentifier];
        [NSFileManager.defaultManager createDirectoryAtPath:contentDirectoryForThisProduct withIntermediateDirectories:YES attributes:nil error:nil];
        NSError *error = nil;
        [NSFileManager.defaultManager moveItemAtURL:thisDownload.contentURL
                                              toURL:[NSURL URLWithString:contentDirectoryForThisProduct]
                                              error:&error];
        if (callbacks)
        {
          const char* realId = [self getRealId:thisDownload.transaction.payment.productIdentifier];
          callbacks->onDonwloadFinished(realId);
        }

        [queue finishTransaction:thisDownload.transaction];
      }

      break;

      default:
      break;
    }
  }];
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions
{
  for (SKPaymentTransaction *transaction in transactions)
  {
    switch (transaction.transactionState)
    {
      case SKPaymentTransactionStatePurchasing:
        break;

      case SKPaymentTransactionStateDeferred:
        [self deferredTransaction:transaction inQueue:queue];
        break;

      case SKPaymentTransactionStateFailed:
        [self failedTransaction:transaction inQueue:queue];
        break;

      case SKPaymentTransactionStatePurchased:
      case SKPaymentTransactionStateRestored:
      {

        if (transaction.downloads.count > 0)
        {
          [SKPaymentQueue.defaultQueue startDownloads:transaction.downloads];
        }
        else
        {
          [queue finishTransaction:transaction];
        }

        NSArray *consumables = self.consumbleItems;
        if ([consumables containsObject:transaction.payment.productIdentifier])
        {
          const char* realId = [self getRealId:transaction.payment.productIdentifier];
          NSString *consumableId = @"";
          size_t consumableCount = 0;
          size_t currentConsumableCount = 0;
          if (checker != nullptr)
          {
            consumableId =  [[NSString alloc] initWithUTF8String:realId];
            consumableCount = checker->getConsumableCount(realId);
            currentConsumableCount = [self.purchaseRecord[consumableId] doubleValue];
          }
          consumableCount += currentConsumableCount;
          self.purchaseRecord[consumableId] = @(consumableCount);

          debug("self.callbacks %d", callbacks);
          if (callbacks != nil)
            callbacks->onSuccessBuyStoreItem(realId);
        }
        else
        {
          // non-consumable or subscriptions
          // subscriptions will eventually contain the expiry date after the receipt is validated during the next run
          self.purchaseRecord[transaction.payment.productIdentifier] = [NSNull null];
        }

        [self savePurchaseRecord];
      }
      break;
    }
  }
}

- (void)failedTransaction:(SKPaymentTransaction *)transaction inQueue:(SKPaymentQueue *)queue
{
  NSLog(@"Transaction Failed with error: %@", transaction.error);
  [queue finishTransaction:transaction];

  const char* realId = [self getRealId:transaction.payment.productIdentifier];
  const char* message = [transaction.error.localizedDescription cStringUsingEncoding: NSUTF8StringEncoding];
  if (callbacks)
    callbacks->onFailedBuyStoreItem(realId, message);

  debug("[AppKit] Transaction Failed with id = %s", realId);
}

- (void)deferredTransaction:(SKPaymentTransaction *)transaction inQueue:(SKPaymentQueue *)queue
{
  NSLog(@"Transaction Deferred: %@", transaction);

  const char* realId = [self getRealId:transaction.payment.productIdentifier];
  if (callbacks)
    callbacks->onDefferedTransaction(realId);

  debug("[AppKit] Transaction deffered with id = %s", realId);
}

@end
