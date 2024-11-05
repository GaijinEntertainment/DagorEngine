//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#import <UIKit/UIKit.h>
#import <GameKit/GameKit.h>
#import <GameController/GameController.h>
#include <osApiWrappers/dag_appstorekit.h>

@interface DagorGameCenterEventViewController : GCEventViewController
- (id)initWithView:(UIView *)view;
- (void)viewDidAppear:(BOOL)animated;
@end

@interface MatchController : NSObject <GKMatchmakerViewControllerDelegate, GKMatchDelegate, GKLocalPlayerListener>
- (id)initWithView:(UIView *)view;
- (void)dealloc;
- (void)beginListeningForMatches;
/*GKMatchDelegate*/
- (void)match:(GKMatch *)match didReceiveData:(NSData *)data forRecipient:(GKPlayer *)recipient fromRemotePlayer:(GKPlayer *)player;
- (void)match:(GKMatch *)match didReceiveData:(NSData *)data fromRemotePlayer:(GKPlayer *)player;
- (void)match:(GKMatch *)match didFailWithError:(NSError *)error;
//- (void)match:(GKMatch *)match player:(GKPlayer *)player didChangeConnectionState:(GKPlayerConnectionState)state;
//- (BOOL)match:(GKMatch *)match shouldReinviteDisconnectedPlayer:(GKPlayer *)player;

/*GKMatchmakerViewControllerDelegate*/
- (void)matchmakerViewController:(GKMatchmakerViewController *)viewController didFindMatch:(GKMatch *)theMatch;
- (void)matchmakerViewControllerWasCancelled:(GKMatchmakerViewController *)viewController;
- (void)matchmakerViewController:(GKMatchmakerViewController *)viewController didFailWithError:(NSError *)error;
//- (void)matchmakerViewController:(GKMatchmakerViewController *)viewController didFindHostedPlayers:(NSArray<GKPlayer *> *)players;

/*GKLocalPlayerListener : GKInviteEventListener*/
- (void)player:(GKPlayer *)player didAcceptInvite:(GKInvite *)invite;
// Deprecated: - (void)player:(GKPlayer *)player didRequestMatchWithPlayers:(NSArray *)playerIDsToInvite;
- (void)player:(GKPlayer *)player didRequestMatchWithRecipients:(NSArray<GKPlayer *> *)recipientPlayers;

- (size_t)getPlayerAddr:(GKPlayer *)player;
- (const char *)getPlayerIdStr:(GKPlayer *)player;

@property(nonatomic, assign) UIView *view;
@property(nonatomic, retain) GKMatch *match;
@property(nonatomic, assign) bool matchingInProgress;
@property(nonatomic, assign) bool matchingCanceled;
@end

@interface DagorStoreKitView : UIView <UITextFieldDelegate>
- (id)initWithFrame:(CGRect)frame;
- (void)initApStorekit;
- (void)addConsumbleItemId:(const char *)itemId;
- (void)restorePurchasedRecords;
- (void)buyApstoreItem:(const char *)itemName;
- (const char *)getItemPrice:(const char *)itemName;
- (const char *)getVersion;
@end

@interface DagorGamecenterView : DagorStoreKitView
- (id)initWithFrame:(CGRect)frame;
- (BOOL)isGamecenterAvailable;
- (BOOL)isPlayerConnected:(size_t)pid;
- (BOOL)isGameHost;
- (BOOL)reportAchievment:(const char *)achievName withPercent:(int)percentComplete;
- (size_t)getSystemAddress;
- (BOOL)setupHostConnection:(const char *)data withLength:(size_t)size;
- (BOOL)loadAchievementStatus:(const char *)achievName;
- (void)resetAchievements;
- (void)setLastError:(NSError *)e;
- (BOOL)reportScore:(int)score toLeaderboard:(const char *)leaderboard;
- (void)getLeaderboardScores:(const char *)leaderboardName timeRange:(int)timeScope;
- (void)checkLeaderboard:(const char *)leaderboardName;
- (BOOL)findMatch:(int)minPlayers maximumPlayers:(int)maxPlayers matchFlag:(int)flag;
- (BOOL)cancelFindMatch;
- (BOOL)isMatched;
- (BOOL)getMatchPlayer:(int)index playerInfo:(apple::Kit::Match::Player_t *)info;
- (BOOL)sendMatchData:(const char *)data withLength:(size_t)size withID:(size_t)playerID isBroadcast:(BOOL)broadcast;
- (BOOL)isPlayerLogged;
- (const char *)getPlayerName;
- (const char *)getPlayerID;
- (const char *)getPlayerDisplayName;
- (void)tryLoginToGameCenter;

- (UIViewController *)getAuthenticationViewController;

@property(nonatomic, retain) UIViewController *authenticationViewController;
@property(nonatomic, retain) NSError *lastError;
@property(nonatomic, retain) DagorGameCenterEventViewController *eventViewController;
@property(nonatomic, retain) MatchController *matchController;
@end

@interface DagorInetView : DagorGamecenterView
- (id)initWithFrame:(CGRect)frame;
- (void)downloadFileFromNet:(const char *)url callback:(apple::Kit::Network::IDownloadCallback *)cb;
@end
