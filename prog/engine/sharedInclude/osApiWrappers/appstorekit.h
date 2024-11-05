// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#import <StoreKit/StoreKit.h>
#include <osApiWrappers/dag_appstorekit.h>

/*!
 *  @abstract The singleton class that takes care of In App Purchasing
 *  @discussion
 *  ApStoreKit provides three basic functionalities, namely, managing In App Purchases,
 *  remembering purchases for you and also provides a basic virtual currency manager
 *  based on example from developer.apple.com/documentation/storekit
 */

NSString *const PresentAuthenticationViewController = @"present_authentication_view_controller";

@interface ApStoreKit : NSObject <SKProductsRequestDelegate, SKPaymentTransactionObserver>
{}

@property(nonatomic, assign) NSMutableDictionary *purchaseRecord;

/*!
 *  @abstract Property that stores the list of available In App purchaseable products
 *
 *  @discussion
 *	This property is initialized after the call to startProductRequest completes
 *  If the startProductRequest fails, this property will be nil
 *  @seealso
 *  -startProductRequest
 */
@property(nonatomic, assign) NSMutableArray *availableProducts;

/*!
 *  @abstract Property that stores the user list of consumable products, that will be check from store
 *
 *  @discussion
 *	This property need to be initialize before startProductRequest calls
 */
@property(nonatomic, assign) NSMutableArray *consumbleItems;
@property(nonatomic, assign) NSMutableArray *storableItems;
@property(nonatomic, assign) NSString *sharedSecret;

/*!
 *  @abstract Property that stores item checker
 *  @seealso
 *  -apstorekit::ItemChecker
 */
- (void)setChecker:(apple::Kit::Store::IChecker *)checker;

/*!
 *  @abstract Property that stores pointer to callbacks
 *  @seealso
 *  -apstorekit::Callbacks
 */
- (void)setCallback:(apple::Kit::Store::ICallback *)callbacks;

/*!
 *  @abstract Accessor for the shared singleton object
 *
 *  @discussion
 *	Use this to access the only object of ApStoreKit
 */
+ (ApStoreKit *)sharedKit;

/*!
 *  @abstract Making the product request using StoreKit's SKProductRequest
 *
 *  @discussion
 *	This method should be called in your application:didFinishLaunchingWithOptions: method
 *  If this method fails, ApStoreKit will not work
 *
 *  @seealso
 *  -availableProducts
 */
- (void)startProductRequest;

/*!
 *  @abstract Making the product request using StoreKit's SKProductRequest
 *
 *  @discussion
 *	This method is normally called after fetching a list of products from your server.
 *  All your known products necessary to add with addConsumableItem
 *
 *  @seealso
 *  -availableProducts
 *  -startProductRequest
 *  -addConsumableItem
 */
- (void)startProductRequestWithProductIdentifiers:(NSArray *)items;

/*!
 *  @abstract Restores InApp Purchases made on other devices
 *
 *  @discussion
 *	This method restores your user's In App Purchases made on other devices.
 */
- (void)restorePurchases;

/*!
 *  @abstract Refreshes the App Store receipt and prompts the user to authenticate.
 *
 *  @discussion
 *	This method can generate a reciept while debugging your application. In a production
 *  environment this should only be used in an appropriate context because it will present
 *  an App Store login alert to the user (without explanation).
 */
- (void)refreshAppStoreReceipt;

/*!
 *  @abstract Initiates payment request for a In App Purchasable Product
 *
 *  @discussion
 *	This method starts a payment request to App Store.
 *  Purchased products are notified through callbacks if those available
 *  callbacks.onSuccessBuyStoreItem will be fired when the purchase completes
 *
 *  @seealso
 *  -isProductPurchased
 *  -expiryDateForProduct
 */
- (void)initiatePaymentRequestForProductWithIdentifier:(NSString *)productId;

/*!
 *  @abstract Checks whether the app version the user purchased is older than the required version
 *
 *  @discussion
 *	This method checks against the local store maintained by ApStoreKit when the app was originally purchased
 *  This method can be used to determine if a user should receive a free upgrade. For example, apps transitioning
 *  from a paid system to a freemium system can determine if users are "grandfathered-in" and exempt from extra
 *  freemium purchases.
 *
 *  @seealso
 *  -isProductPurchased
 */
- (BOOL)purchasedAppBeforeVersion:(NSString *)requiredVersion;

/*!
 *  @abstract Checks whether the product identified by the given productId is purchased previously
 *
 *  @discussion
 *	This method checks against the local store maintained by ApStoreKit if the product was previously purchased
 *
 *  @seealso
 *  -expiryDateForProduct
 */
- (BOOL)isProductPurchased:(NSString *)productId;

/*!
 *  @abstract Checks the expiry date for the product identified by the given productId
 *
 *  @discussion
 *	This method checks against the local store maintained by ApStoreKit for expiry date of a given product
 *  @seealso
 *  -isProductPurchased
 */
- (NSDate *)expiryDateForProduct:(NSString *)productId;


/*!
 *  @abstract This method returns the available credits (managed by ApStoreKit) for a given consumable
 *
 *  @discussion
 *	ApStoreKit provides a basic virtual currency manager for your consumables
 *  This method returns the available credits for a consumable
 *  A consumable ID is different from its product id, and it is configured in MKStoreKitConfigs.plist file
 *  callbacks.onExpiryDateForProduct to get notified when the purchase of the consumable completes
 *
 *  @seealso
 *  -isProductPurchased
 */
- (NSNumber *)availableCreditsForConsumable:(NSString *)consumableID;

- (NSString *)getProductPrice:(NSString *)productId;

/*!
 *  @abstract This method updates the available credits (managed by ApStoreKit) for a given consumable
 *
 *  @discussion
 *	ApStoreKit provides a basic virtual currency manager for your consumables
 *  This method should be called if the user consumes a consumable credit
 *  A consumable ID is different from its product id, and it is configured in MKStoreKitConfigs.plist file
 *  callbacks.onProductsPurchased kMKStoreKitProductPurchasedNotification to get notified when the purchase of the consumable completes
 *
 *  @seealso
 *  -isProductPurchased
 */
- (NSNumber *)consumeCredits:(NSNumber *)creditCountToConsume identifiedByConsumableIdentifier:(NSString *)consumableId;

- (void)addConsumableItem:(NSString *)itemId;
- (const char *)getRealId:(NSString *)itemId;

@end
