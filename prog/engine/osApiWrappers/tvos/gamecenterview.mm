// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_tvosMainUi.h>
#include <osApiWrappers/appstorekit.h>
#include <debug/dag_debug.h>

namespace apple
{
  static DagorGamecenterView* gamecenterview;
  bool leaderboard_available = false;
  bool gamecenter_online = false;
}

using namespace apple;

@implementation DagorGamecenterView

- (id)initWithFrame:(CGRect)frame
{
  if (self = [super initWithFrame: frame])
  {
    apple::gamecenterview = self;
  }

  return self;
}

-(UIViewController *)getAuthenticationViewController { return _authenticationViewController; }

-(BOOL)isGamecenterAvailable
{
  return (NSClassFromString(@"GKLocalPlayer")) != nil;
}

-(BOOL)isMatched { return self.matchController.match != nil; }

-(BOOL)isPlayerLogged
{
  return [GKLocalPlayer localPlayer].isAuthenticated;
}

-(void)resetAchievements
{
  [GKAchievement resetAchievementsWithCompletionHandler:^(NSError *e)
   {
     if (e != nil)
     {
       debug("[GAMECENTER] could not reset achievements");
       kit.achievments.getCallback().onResetAll(false);
     }
     else
     {
       debug("[GAMECENTER] all achivements resets");
       kit.achievments.getCallback().onResetAll(true);
     }
   }];
}

-(BOOL)reportAchievment:(const char*)achievName withPercent:(int)percentComplete
{
  NSString* identifier = [[NSString alloc] initWithUTF8String:achievName];
  GKAchievement *achievement = [[GKAchievement alloc] initWithIdentifier:identifier];
  achievement.percentComplete = percentComplete;

  [GKAchievement reportAchievements:@[achievement]
              withCompletionHandler:^(NSError *e)
    {
      const char* achievName = [achievement.identifier cStringUsingEncoding:NSASCIIStringEncoding];
      if (e == nil)
      {
        debug("[GAMECENTER] saved achievement success");
        kit.achievments.getCallback().onPartlySaved(achievName, true);
      }
      else
      {
        debug("[GAMECENTER] saved achievement failed");
        kit.achievments.getCallback().onPartlySaved(achievName, false);
      }
    }
  ];

  return true;
}

-(BOOL)loadAchievementStatus:(const char*)achievName
{
   [GKAchievement loadAchievementsWithCompletionHandler:^(NSArray *achievements, NSError *e)
    {
      if (e != nil)
      {
        debug("[GAMECENTER] error in loading achievements");
      }

      if (achievements != nil)
      {
        // Process the array of achievements.
        for (GKAchievement* ach in achievements)
        {
          const char* achievName = [ach.identifier cStringUsingEncoding:NSASCIIStringEncoding];
          debug( "[GAMECENTER] loaded data for achievement %s", achievName);
          kit.achievments.getCallback().onReceivedInfo(achievName, ach.percentComplete);
        }
      }
    }
  ];

  return true;
}

-(BOOL)setupHostConnection:(const char*)data withLength:(size_t)length
{
  GKLocalPlayer* localPlayer = [GKLocalPlayer localPlayer];
  size_t localPlayerPid = [self.matchController getPlayerAddr:localPlayer];

  size_t hostId = localPlayerPid;
  NSArray<GKPlayer*> *players = self.matchController.match.players;

  for (GKPlayer* player in players)
  {
    size_t playerId = [self.matchController getPlayerAddr:player];
    if (playerId < hostId)
      hostId = playerId;
  }

  [self sendMatchData:data withLength:length withID:hostId isBroadcast:false];
  return true;
}

-(BOOL)isGameHost
{
  if (self.matchController.match == nil)
    return false;

  GKLocalPlayer* localPlayer = [GKLocalPlayer localPlayer];
  size_t localPlayerPid = [self.matchController getPlayerAddr:localPlayer];

  size_t minIdValue = localPlayerPid;
  NSArray<GKPlayer*> *players = self.matchController.match.players;
  for (GKPlayer* player in players)
  {
    size_t playerId = [self.matchController getPlayerAddr:player];
    if (playerId < minIdValue)
      minIdValue = playerId;
  }


  return (minIdValue == localPlayerPid);
}

-(BOOL)sendMatchData:(const char*)data withLength:(size_t)l withID:(size_t)playerID isBroadcast:(BOOL)broadcast
{
  if (self.matchController.match == nil)
    return false;

  NSError *e = nil;
  NSData *packet = [NSData dataWithBytes:data length:l];

#undef error //incomatible with func params
  if (broadcast)
  {
    [self.matchController.match sendDataToAllPlayers:packet withDataMode:GKMatchSendDataUnreliable error:&e];
  }
  else
  {
    NSArray<GKPlayer*> *players = self.matchController.match.players;
    NSArray<GKPlayer*> *receivers = nil;
    for (GKPlayer* player in players)
    {
      size_t curId = [self.matchController getPlayerAddr:player];
      if (curId == playerID)
      {
        receivers = @[player];
        break;
      }
    }

    [self.matchController.match sendData:packet toPlayers:receivers dataMode:GKMatchSendDataUnreliable error:&e];
  }

  if (e != nil)
  {
    debug("[GAMECENTER] sendMatchData error: %s", [[[e localizedDescription] description] cStringUsingEncoding:NSUTF8StringEncoding]);
    return false;  // Handle the error.
  }

  return true;
}

-(BOOL)getMatchPlayer:(int)index playerInfo:(Kit::Match::Player_t*)info
{
  if (self.matchController.match == nil)
    return false;

  NSArray<GKPlayer*> *players = self.matchController.match.players;
  if ((uint32_t)index >= [players count])
    return false;

  GKPlayer* player = players[index];

  info->setId([player.playerID cStringUsingEncoding:NSUTF8StringEncoding]);
  info->setAlias([player.alias cStringUsingEncoding:NSUTF8StringEncoding]);
  info->setDisplayName([player.displayName cStringUsingEncoding:NSUTF8StringEncoding]);
  return true;
}

-(BOOL)isPlayerConnected:(size_t)pid
{
  if (self.matchController.match == nil)
    return false;

  NSArray<GKPlayer*> *players = self.matchController.match.players;
  for (GKPlayer* player in players)
  {
      size_t playerId = [self.matchController getPlayerAddr: player];
      if (playerId == pid)
        return true;
  }

  return false;
}

-(const char*)getPlayerName
{
  GKLocalPlayer* localPlayer = [GKLocalPlayer localPlayer];

  static const char* data = nullptr;
  if (localPlayer.isAuthenticated)
    data = [localPlayer.alias cStringUsingEncoding:NSUTF8StringEncoding];

  if (!data || strlen(data) == 0)
    return "UnknownAlias";

  return data;
}


-(const char*)getPlayerDisplayName
{
  GKLocalPlayer* localPlayer = [GKLocalPlayer localPlayer];

  if (localPlayer.isAuthenticated)
    return [localPlayer.displayName cStringUsingEncoding:NSUTF8StringEncoding];
  else
    return "";
}

-(size_t)getSystemAddress
{
  GKLocalPlayer* localPlayer = [GKLocalPlayer localPlayer];
  size_t localPlayerPid = [self.matchController getPlayerAddr:localPlayer];

  return localPlayerPid;
}

- (void)setAuthenticationViewController:(UIViewController *)authenticationViewController
{
  if (authenticationViewController != nil)
  {
    _authenticationViewController = authenticationViewController;
    [ [NSNotificationCenter defaultCenter] postNotificationName:PresentAuthenticationViewController
                                                         object:self];
  }
}

-(void)setLastError:(NSError *)e
{
  _lastError = nil;
  if (e != nil)
    _lastError = [e copy];

  if (_lastError)
  {
    const char* errStr = [[[_lastError userInfo] description] cStringUsingEncoding:NSUTF8StringEncoding];
    debug("[GAMECENTER] ERROR: %s", errStr);
  }
}

-(void)getLeaderboardScores:(const char*)leaderboardName
                  timeRange:(int)timeScope                    //today 0, week 1, allTime 2
{
  GKLocalPlayer *localPlayer = [GKLocalPlayer localPlayer];

  GKLeaderboard *leaderboard = [[[GKLeaderboard alloc] init] autorelease];

  if (leaderboard != nil)
  {
    leaderboard.identifier = [[NSString alloc] initWithUTF8String:leaderboardName];
    leaderboard.playerScope = GKLeaderboardPlayerScopeGlobal;
    switch (timeScope)
    {
      case 1:
        leaderboard.timeScope = GKLeaderboardTimeScopeWeek;
      break;

      case 2:
        leaderboard.timeScope = GKLeaderboardTimeScopeAllTime;
      break;

      default:
        leaderboard.timeScope = GKLeaderboardTimeScopeToday;
    }
    leaderboard.range = NSMakeRange(1,10);

    debug("[GAMECENTER] Loading the scores in leaderboard...");

    kit.leaderboard.getCallback().onStartRead(leaderboardName);
    //kit.leaderboard.getCallback().onReceivedLocalRecord(leaderboard.localPlayerScore.value, leaderboard.localPlayerScore.rank);

    [leaderboard loadScoresWithCompletionHandler:
       ^(NSArray *scores, NSError *e)
       {
         if (scores != nil)
         {
           apple::leaderboard_available = true;
           GKLocalPlayer *lp = [GKLocalPlayer localPlayer];
           auto& cb = kit.leaderboard.getCallback();

           for (GKScore *score in scores)
           {
             GKPlayer* player = score.player;

             const char* displayName = [player.alias cStringUsingEncoding:NSUTF8StringEncoding];

             cb.onReceivedRecord(displayName, strlen(displayName), score.value, score.rank);
             if (lp == player)
               cb.onReceivedLocalRecord(score.value, score.rank);

             debug("[GAMECENTER] %d score %s %d", score.rank, displayName, score.value);
           }

           cb.onReceivedLocalRecord(leaderboard.localPlayerScore.value, leaderboard.localPlayerScore.rank);
           cb.onEndRead("");
         }

         if (e != nil)
         {
           apple::leaderboard_available = false;
           const char *message = [[e localizedDescription] UTF8String];
           debug("[GAMECENTER] Leaderboard scores error = %s", message);
           kit.leaderboard.getCallback().onError([e code], message);
         }
      }];
  }
}

-(void)checkLeaderboard:(const char*)leaderboardName
{
  GKLocalPlayer *localPlayer = [GKLocalPlayer localPlayer];
  GKLeaderboard *leaderboard = [[[GKLeaderboard alloc] init] autorelease];

  if (leaderboard != nil)
  {
    leaderboard.identifier = [[NSString alloc] initWithUTF8String:leaderboardName];
    leaderboard.playerScope = GKLeaderboardPlayerScopeGlobal;
    leaderboard.timeScope = GKLeaderboardTimeScopeToday;
    leaderboard.range = NSMakeRange(1,1);
    apple::leaderboard_available = false;

    debug("[GAMECENTER] Checking the leaderboard...");

    [leaderboard loadScoresWithCompletionHandler:
       ^(NSArray *scores, NSError *e)
       {
         if (scores != nil)
         {
           apple::leaderboard_available = true;
           kit.leaderboard.getCallback().onChangeAvailableState(true);
         }

         if (e != nil)
         {
           apple::leaderboard_available = false;
           const char *message = [[e localizedDescription] UTF8String];
           debug("[GAMECENTER] Leaderboard checking error = %s", message);
           kit.leaderboard.getCallback().onError([e code], message);
           kit.leaderboard.getCallback().onChangeAvailableState(false);
         }
      }];
  }
}

- (BOOL) reportScore:(int)score
       toLeaderboard:(const char*)leaderboard
{

  __block BOOL result = NO;

  GKLocalPlayer *localPlayer = [GKLocalPlayer localPlayer];

  if ([localPlayer isAuthenticated] == NO)
  {
    debug("[GAMECENTER] You must authenticate the local player first.");
    return NO;
  }

  if (!leaderboard)
  {
    debug("[GAMECENTER] leaderboard identifier is empty.");
    return NO;
  }

  NSString* leaderboardName = [[NSString alloc] initWithUTF8String:leaderboard];
  GKScore *scoreReporter = [[[GKScore alloc] initWithLeaderboardIdentifier:leaderboardName] autorelease];

  scoreReporter.value = (int64_t)score;
  scoreReporter.context = 0;

  debug("[GAMECENTER] attempting to report the score...");
  NSArray* scores = @[scoreReporter];
  [GKScore   reportScores:scores
    withCompletionHandler:^(NSError *e)
    {
      if (e == nil)
      {
        debug("[GAMECENTER] Report Score %d to Leaderboard %s", score, leaderboard);
        result = YES;
      }
      else
      {
        debug("[GAMECENTER] Failed to report the error. Error = %s", [[[e localizedDescription] description] cStringUsingEncoding:NSUTF8StringEncoding]);
      }
    }
  ];

  return result;
}

-(BOOL)cancelFindMatch
{
  GKLocalPlayer *localPlayer = [GKLocalPlayer localPlayer];

  if ([localPlayer isAuthenticated] == NO)
  {
    debug("[GAMECENTER] You must authenticate the local player first.");
    return NO;
  }

  self.matchController.matchingInProgress = false;
  [self.eventViewController dismissViewControllerAnimated:YES completion:nil];

  [[GKMatchmaker sharedMatchmaker] cancel];
  return true;
}

-(BOOL)findMatch:(int)minPlayers maximumPlayers:(int)maxPlayers matchFlag:(int)flag
{
  GKLocalPlayer *localPlayer = [GKLocalPlayer localPlayer];

  if ([localPlayer isAuthenticated] == NO)
  {
    debug("[GAMECENTER] You must authenticate the local player first.");
    return NO;
  }

  GKMatchRequest *request = [[GKMatchRequest alloc] init];
  request.minPlayers = minPlayers;
  request.maxPlayers = maxPlayers;
  request.defaultNumberOfPlayers = 2;
  request.playerAttributes = 0; // NO SPECIAL ATTRIBS
  request.playerGroup = flag;

  self.matchController.matchingInProgress = true;
  self.matchController.match = nil;
  self.matchController.matchingCanceled = false;

  GKMatchmakerViewController *matchViewController = [[GKMatchmakerViewController alloc] initWithMatchRequest:request];
  matchViewController.matchmakerDelegate = self.matchController;

  [self.eventViewController presentViewController:matchViewController animated:YES completion:nil];
  return YES;
}

-(void)tryLoginToGameCenter
{
  GKLocalPlayer *localPlayer = [GKLocalPlayer localPlayer];

  localPlayer.authenticateHandler  = ^(UIViewController *viewController, NSError *e)
  {
    [self setLastError:e];

    if(viewController != nil)
    {
      [self setAuthenticationViewController:viewController];
    }
    else
    {
      bool authOk = [GKLocalPlayer localPlayer].isAuthenticated;
      if(authOk)
        [self.matchController beginListeningForMatches];

      kit.gamecenter.getCallback().onChangeLogged(authOk, "");
      apple::gamecenter_online = authOk;
    }

    if (e != nil)
    {
      apple::gamecenter_online = false;
    }
  };
}

-(const char*)getPlayerID
{
  GKLocalPlayer* localPlayer = [GKLocalPlayer localPlayer];

  if (localPlayer.isAuthenticated)
    return [localPlayer.playerID cStringUsingEncoding:NSUTF8StringEncoding];
  else
    return "";
}

@end


namespace apple
{

bool Kit::Gamecenter::exist4platform ()
{
  if (gamecenterview)
    return [gamecenterview isGamecenterAvailable];

  return false;
}

void Kit::Leaderboard::saveScoreAsync(int score, const char* leaderboard)
{
  if (gamecenterview)
    [gamecenterview reportScore:score toLeaderboard:leaderboard];
}

bool Kit::Match::setupHostConnection(const char* data, size_t length)
{
  if (gamecenterview)
    return [gamecenterview setupHostConnection:data withLength:length];

  return false;
}

const char* Kit::Gamecenter::getDisplayName()
{
  if (gamecenterview)
    return [gamecenterview getPlayerDisplayName];

  return "";
}

bool Kit::Match::isPlayerConnected(size_t id)
{
  if (gamecenterview)
    return [gamecenterview isPlayerConnected:id];

  return false;
}

bool Kit::Gamecenter::isLogged()
{
  if (gamecenterview)
    return [gamecenterview isPlayerLogged];

  return false;
}

bool Kit::Match::isHost()
{
  if (gamecenterview)
    return [gamecenterview isGameHost];

  return false;
}

const char* Kit::Gamecenter::getAlias ()
{
  if (gamecenterview)
    return [gamecenterview getPlayerName];

  return "UnkownAlias";//
}

bool Kit::Match::cancelFind()
{
  if (gamecenterview)
    return [gamecenterview cancelFindMatch];

  return false;
}

const char* Kit::Gamecenter::getID ()
{
  if (gamecenterview)
    return [gamecenterview getPlayerID];

  return "UnknownPlayerID";
}

bool Kit::Match::isFindCanceled()
{
  if (gamecenterview)
      return gamecenterview.matchController.matchingCanceled;

  return false;
}

bool Kit::Match::isActive()
{
  if (gamecenterview)
    return [gamecenterview isMatched];

  return false;
}

void Kit::Leaderboard::loadScoresAsync(const char* leaderboardName, int timeScope)
{
  if (gamecenterview)
   [gamecenterview getLeaderboardScores:leaderboardName timeRange:timeScope];
}


void Kit::Achievments::getAsync(const char* achievName)
{
  if (gamecenterview)
    [gamecenterview loadAchievementStatus:achievName];
}

bool Kit::Match::isFindInProgress()
{
  if (gamecenterview)
    return gamecenterview.matchController.matchingInProgress;

  return false;
}

size_t Kit::Match::getSystemAddress()
{
  if (gamecenterview)
    return [gamecenterview getSystemAddress];

  return -1;
}

bool Kit::Match::find(int minPlayers, int maxPlayers, int level)
{
  int levelGroup = level / LEVEL_GROUP_LENGTH;
  if (gamecenterview)
    return [gamecenterview findMatch:minPlayers maximumPlayers:maxPlayers matchFlag:levelGroup];

  return false;
}

bool Kit::Match::sendData(const char* data, size_t length, size_t playedID, bool broadcast)
{
  if (gamecenterview)
    return [gamecenterview sendMatchData:data withLength:length withID:playedID isBroadcast:broadcast];

  return false;
}

void Kit::Gamecenter::tryLogin()
{
  if (gamecenterview)
    [gamecenterview tryLoginToGameCenter];
}

void Kit::Gamecenter::generateIdentityVerificationSignature()
{
  GKLocalPlayer *localPlayer = [GKLocalPlayer localPlayer];

  [localPlayer generateIdentityVerificationSignatureWithCompletionHandler:
    ^(NSURL *publicKeyUrl, NSData *signature, NSData *salt, uint64_t timestamp, NSError *error)
    {

      if (error != nil)
      {
          NSLog(@"generateIdentityVerificationSignatureWithCompletionHandler Error:%@", error);
          Gamecenter::VerificationSignature vs = {0};
          kit.gamecenter.getCallback().onGenerateIdentityVerificationSignature(false, vs);
          return; //some sort of error, can't authenticate right now
      }
       Gamecenter::VerificationSignature vs =
       {publicKeyUrl.absoluteString.UTF8String,
        signature.bytes,
        signature.length,
        salt.bytes,
        salt.length,
        timestamp};
      kit.gamecenter.getCallback().onGenerateIdentityVerificationSignature(true, vs);
    }
  ];


}

bool Kit::Match::getPlayer(int index, Kit::Match::Player_t& player)
{
  Kit::Match::Player_t* info = &player;
  if (gamecenterview)
    return [gamecenterview getMatchPlayer:index playerInfo:info];

  return false;
}

void Kit::Leaderboard::checkAvailableAsync(const char *leaderboardName)
{
  if (gamecenterview)
   [gamecenterview checkLeaderboard:leaderboardName];
}

void Kit::Gamecenter::checkOnlineAsync()
{
  if (gamecenterview)
  {
    debug("[GAMECENTER] checkAvailableAsync not yet implemented");
  }
}

bool Kit::Gamecenter::isOnline()
{
  return apple::gamecenter_online;
}

bool Kit::Leaderboard::isAvailable()
{
  return apple::leaderboard_available;
}

void Kit::Achievments::setPartlyAsync(const char* achievement, size_t percentComplete)
{
  if (gamecenterview)
    [gamecenterview reportAchievment:achievement withPercent:percentComplete];
}

void Kit::Achievments::resetAllAsync()
{
  if (gamecenterview)
    [gamecenterview resetAchievements];
}

}//end namespace apple
