// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_appstorekit.h>

namespace apple
{

namespace internal
{
  struct GamecenterDefaultCb : public Kit::Gamecenter::ICallback
  {
    void onChangeLogged(bool success, const char *message) override {}
    void onGenerateIdentityVerificationSignature(bool success, const Kit::Gamecenter::VerificationSignature& signature) override {}
    void onChangeAvailable(bool status) override {}
  } gamecenterDefaultCb;

  struct MatchDefaultCb : public Kit::Match::ICallback
  {
    void onSuccessFind(const char *message) override {}
    void onFailedFind(const char *message) override {}
    void onPlayerDisconnect(size_t playedID) override {}
  } matchDefaultCb;

  struct LeaderboardDefaultCb : public Kit::Leaderboard::ICallback
  {
    void onStartRead(const char *leaderboardName) override {}
    void onReceivedRecord(const char *name, size_t namelen, size_t score, size_t rank) override {}
    void onReceivedLocalRecord(size_t score, size_t rank) override {}
    void onEndRead(const char *leaderboardName) override {}
    void onChangeAvailableState(bool available) override {}
    void onError(int code, const char* message) override {}
    void onScoreSaved(bool status) override {}
  } leaderboardDefaultCb;

  struct AchievmentsDefaultCb : public Kit::Achievments::ICallback
  {
    void onReceivedInfo(const char *name, int complete) override {}
    void onPartlySaved(const char *name, bool saved) override {}
    void onResetAll(bool reset) override {}
  } achievmentsDefaultCb;

  Kit::Gamecenter::ICallback* authCb = &gamecenterDefaultCb;
  Kit::Match::ICallback* matchingCb = &matchDefaultCb;
  Kit::Leaderboard::ICallback* leaderboardCb = &leaderboardDefaultCb;
  Kit::Achievments::ICallback* achievloadCb = &achievmentsDefaultCb;

}//end namespace callbacks

void Kit::Gamecenter::setCallback(Kit::Gamecenter::ICallback* callback)
{
  internal::authCb = callback ? callback : &internal::gamecenterDefaultCb;
}


void Kit::Achievments::setCallback(Kit::Achievments::ICallback* callback)
{
  internal::achievloadCb = callback ? callback : &internal::achievmentsDefaultCb;
}

void Kit::Match::setCallback(Kit::Match::ICallback* callback)
{
  internal::matchingCb = callback ? callback : &internal::matchDefaultCb;
}

void Kit::Leaderboard::setCallback(Kit::Leaderboard::ICallback* callback)
{
  internal::leaderboardCb = callback ? callback : &internal::leaderboardDefaultCb;
}

Kit::Leaderboard::ICallback& Kit::Leaderboard::getCallback() { return *internal::leaderboardCb; }
Kit::Achievments::ICallback& Kit::Achievments::getCallback() { return *internal::achievloadCb; }
Kit::Match::ICallback& Kit::Match::getCallback() { return *internal::matchingCb; }
Kit::Gamecenter::ICallback& Kit::Gamecenter::getCallback() { return *internal::authCb; }

}//end namespace appl
