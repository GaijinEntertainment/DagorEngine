// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_appstorekit.h>
#include <startup/dag_tvosMainUi.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/map.h>

#define DEBUG_GAMECENTER_NETWORK 0

using namespace apple;

namespace internal
{
  eastl::map<void*, size_t> players;
  static constexpr int BAD_ID = 0;
}

@implementation MatchController
- (id) initWithView: (UIView*) view
{
  [super init];

  self.view = view;

  return self;
}

- (void) dealloc
{
  [super dealloc];
  [[GKLocalPlayer localPlayer] unregisterAllListeners];
}

- (void)beginListeningForMatches
{
  // Register for GKInvites
  [[GKLocalPlayer localPlayer] registerListener:self];
}

- (void)player:(GKPlayer *)player didAcceptInvite:(GKInvite *)invite
{
  //if ([self.multiplayerDelegate respondsToSelector:@selector(gameCenterManager:match:didAcceptMatchInvitation:player:)])
  //  [self.multiplayerDelegate gameCenterManager:self match:self.multiplayerMatch didAcceptMatchInvitation:invite player:player];
}

- (void)player:(GKPlayer *)player didRequestMatchWithRecipients:(NSArray<GKPlayer *> *)recipientPlayers;
//Deprecated: - (void)player:(GKPlayer *)player didRequestMatchWithPlayers:(NSArray *)playerIDsToInvite
{
  //if ([self.multiplayerDelegate respondsToSelector:@selector(gameCenterManager:match:didReceiveMatchInvitationForPlayer:playersToInvite:)])
  //  [self.multiplayerDelegate gameCenterManager:self match:self.multiplayerMatch didReceiveMatchInvitationForPlayer:player playersToInvite:playerIDsToInvite];
}

- (const char*)getPlayerIdStr:(GKPlayer *)player
{
  return [player.playerID cStringUsingEncoding:NSASCIIStringEncoding];
}

- (void)match:(GKMatch *)match didReceiveData:(NSData *)data
                                 forRecipient:(GKPlayer *)recipient
                             fromRemotePlayer:(GKPlayer *)player
{
  auto& p = *Kit::Match::Packet_t::create();
  p.systemAddress = [self getPlayerAddr:player];
  p.length = (uint32_t)[data length];
  p.bitSize = (uint32_t)(p.length * 8);
  p.data = new uint8_t[p.length];
  p.receiveTime = get_time_msec();
  memcpy(p.data, (uint8_t*)[data bytes], p.length);

  kit.match.pushPacket(&p);

#if DEBUG_GAMECENTER_NETWORK
  const char* playerId = [self getPlayerIdStr:player];
  const char* recepientId = [self getPlayerIdStr:recipient];
  size_t recepientAddress = [self getPlayerAddr:recepientId];
  debug("[GAMECENTER] match data from remote player %s (%zu) recepient %s (%zu)", playerId, p.systemAddress, recepientId, recepientAddress);
#endif
}

- (void)match:(GKMatch *)match player:(GKPlayer *)player
             didChangeConnectionState:(GKPlayerConnectionState)state
{
    switch (state)
    {
        case GKPlayerStateConnected:
          // Handle a new player connection.
        break;

        case GKPlayerStateDisconnected:
        {
          // A player just disconnected.
          size_t playerAddr = [self getPlayerAddr:player];
          kit.match.getCallback().onPlayerDisconnect(playerAddr);

#if DEBUG_GAMECENTER_NETWORK
          debug("[GAMECENTER] player %s disconnected", playerAddr);
#endif
        }
        break;
    }
}

- (void)match:(GKMatch *)theMatch connectionWithPlayerFailed:(NSString *)playerID withError:(NSError *)e
{
  // The match was unable to connect with the player due to an error
  const char *message = [[e localizedDescription] UTF8String];

  debug("[GAMECENTER] Failed to connect to player with error: %s", message);
}

- (void)match:(GKMatch *)match didReceiveData:(NSData *)data
                             fromRemotePlayer:(GKPlayer *)player
{
  auto& p = *Kit::Match::Packet_t::create();
  p.systemAddress = [self getPlayerAddr: player];
  p.length = (uint32_t)[data length];
  p.bitSize = (uint32_t)(p.length * 8);
  p.data = new uint8_t[p.length];
  memcpy(p.data, (uint8_t*)[data bytes], p.length);

  p.receiveTime = get_time_msec();

  kit.match.pushPacket(&p);

#if DEBUG_GAMECENTER_NETWORK
  const char* playerId = [self getPlayerIdStr:player];
  debug("[GAMECENTER] match data from player %s (%zu)", playerId, p.systemAddress);
#endif
}

- (void)match:(GKMatch *)match didFailWithError:(NSError *)error
{
  const char *message = [[error localizedDescription] UTF8String];
  debug("[GAMECENTER] Error match: %s", message);

  kit.match.getCallback().onFailedFind(message);
}

- (void)matchmakerViewControllerWasCancelled:(GKMatchmakerViewController *)viewController
{
  // Matchmaking was cancelled by the user
  self.matchingInProgress = false;
  self.matchingCanceled = true;
  self.match = nil;
  internal::players.clear();
  [((DagorGamecenterView*)self.view).eventViewController dismissViewControllerAnimated:YES completion:nil];

#if DEBUG_GAMECENTER_NETWORK
  debug("[GAMECENTER] Finding match was cancelled by player");
#endif
}

- (void)matchmakerViewController:(GKMatchmakerViewController *)viewController didFailWithError:(NSError *)e
{
  // Matchmaking failed due to an error
  [((DagorGamecenterView*)self.view).eventViewController dismissViewControllerAnimated:YES completion:nil];
  self.matchingInProgress = false;
  self.match = nil;
  internal::players.clear();

  const char *message = [[e localizedDescription] UTF8String];
  debug("[GAMECENTER] Error finding match: %s", message);

  kit.match.getCallback().onFailedFind(message);
}

- (void)matchmakerViewController:(GKMatchmakerViewController *)viewController didFindMatch:(GKMatch *)theMatch
{
  // A peer-to-peer match has been found, the game should start
  [((DagorGamecenterView*)self.view).eventViewController dismissViewControllerAnimated:YES completion:nil];

  if (self.match == nil && self.match.expectedPlayerCount == 0)
  {
    debug("[GAMECENTER] Ready to start match!");
    // Match was found and all players are connected

    self.match = theMatch;
    self.match.delegate = self;

    self.matchingInProgress = false;

    NSArray<GKPlayer*> *players = theMatch.players;
    int i = 0;
    for (GKPlayer* player in players)
    {
      const char* playerId = [self getPlayerIdStr:player];
      debug( "[GAMECENTER] player num = %d", i++);
      debug( "[GAMECENTER] player id = %s", playerId);
      debug( "[GAMECENTER] player alias = %s", [player.alias cStringUsingEncoding:NSUTF8StringEncoding]);
      debug( "[GAMECENTER] player display name = %s", [player.displayName cStringUsingEncoding:NSUTF8StringEncoding]);

      internal::players[(void*)player] = Kit::Match::Player_t::convStrToId(playerId);
    }

    kit.match.getCallback().onSuccessFind("");
  }
}

- (size_t)getPlayerAddr:(GKPlayer *)player
{
  auto it = internal::players.find((void*)player);
  if (it == internal::players.end())
  {
    size_t addr = Kit::Match::Player_t::convStrToId([player.playerID cStringUsingEncoding:NSASCIIStringEncoding]);
    debug( "[GAMECENTER] player id:[%zu] not found in match ", addr);
    return addr;
  }
  else
  {
    return it->second;
  }
}
@end
