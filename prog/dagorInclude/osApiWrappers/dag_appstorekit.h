//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <cstring>
#include <cstdlib>
#include <util/dag_stdint.h>

namespace apple
{

// if you want to test api on non-tvOS,
// add in jam gameLibs/tvOS/developapi/developapi.h

struct Kit
{
  static const uint32_t DATA_LENGTH = 128;
  static const uint32_t LEVEL_GROUP_LENGTH = 5;

  void initialize();

  struct Network
  {
    struct IDownloadCallback
    {
      virtual void onRequestDone(bool success, int httpCode, const char *data, size_t size) = 0;
      virtual void onHttpProgress(size_t dltotal, size_t dlnow) = 0;
      virtual void onRelease() = 0;
    };

    void downloadFile(const char *url, IDownloadCallback *cb);
  } network;

  struct Storage
  {
    bool loadData(void *bytes, size_t *length, const char *name);
    bool saveData(void *bytes, size_t length, const char *name);

    bool saveString(const char *value, const char *name);
    const char *loadString(const char *name);
  } storage;


  struct Cache
  {
    const char *getPath();
    bool saveData(const void *bytes, size_t size, const char *name);
  } cache;

  struct Cloud
  {
    bool isEnabled();
    bool isTokenValid();
    const char *getToken();
  } cloud;

  struct Store
  {
    struct IChecker
    {
      virtual const char *getConsumableId(const char *cid) { return cid; }

      virtual size_t getConsumableCount(const char * /*cid*/) { return 0; }

      virtual ~IChecker() {}
    };

    struct ICallback
    {
      virtual void onReceivedValidStoreItem(const char * /*itemId*/) = 0;
      virtual void onReceivedInvalidStoreItem(const char * /*itemId*/) = 0;
      virtual void onSuccessBuyStoreItem(const char * /*itemId*/) = 0;
      virtual void onFailedBuyStoreItem(const char * /*itemId*/, const char * /*message*/) = 0;
      virtual void onDefferedTransaction(const char * /*itemId*/) = 0;
      virtual void onDownloadInProgess(const char * /*itemId*/, size_t /*percent*/) = 0;
      virtual void onDonwloadFinished(const char * /*itemId*/) = 0;
      virtual ~ICallback() {}
    };

    void buyItem(const char *itemName);
    const char *getItemPrice(const char *itemName);
    void addConsumbleItemId(const char *itemId);
    void restorePurchasedRecords();
    void requestConsumableItems();
    void setChecker(IChecker *checker);
    void setCallback(ICallback *callback);
    ICallback &getCallback();

    const char *getVersion();
  } store;

  struct Leaderboard
  {
    struct ICallback
    {
      virtual void onStartRead(const char *leaderboardName) = 0;
      virtual void onReceivedRecord(const char *name, size_t namelen, size_t score, size_t rank) = 0;
      virtual void onReceivedLocalRecord(size_t score, size_t rank) = 0;
      virtual void onEndRead(const char *leaderboardName) = 0;
      virtual void onChangeAvailableState(bool available) = 0;
      virtual void onScoreSaved(bool status) = 0;
      virtual void onError(int code, const char *message) = 0;
      virtual ~ICallback() {}
    };

    struct Score_t
    {
      char name[DATA_LENGTH];
      size_t value;
      size_t rank;
    };

    ICallback &getCallback();
    void setCallback(ICallback *callback);

    bool isAvailable();
    void checkAvailableAsync(const char *leaderboard);

    void loadScoresAsync(const char *leaderboard, int timeScope);
    void saveScoreAsync(int score, const char *leaderboard);
  } leaderboard;

  struct Achievments
  {
    struct ICallback
    {
      virtual void onReceivedInfo(const char *name, int complete) = 0;
      virtual void onPartlySaved(const char *name, bool saved) = 0;
      virtual void onResetAll(bool reset) = 0;
      virtual ~ICallback() {}
    };

    ICallback &getCallback();
    void setCallback(ICallback *callback);

    void setPartlyAsync(const char *achievement, size_t percenteComplete);
    void resetAllAsync();

    void getAsync(const char *achievName);

  } achievments;

  struct Gamecenter
  {
    struct VerificationSignature
    {
      const char *publicKeyUrl;
      const void *signature;
      size_t signature_length;
      const void *salt;
      size_t salt_length;
      uint64_t timestamp;
    };

    struct ICallback
    {
      virtual void onGenerateIdentityVerificationSignature(bool success, const VerificationSignature &signature) = 0;
      virtual void onChangeLogged(bool success, const char *message) = 0;
      virtual void onChangeAvailable(bool status) = 0;
      virtual ~ICallback() {}
    };

    ICallback &getCallback();
    void setCallback(ICallback *callback);

    void tryLogin();
    bool isLogged();
    void generateIdentityVerificationSignature();

    bool isOnline();
    void checkOnlineAsync();

    const char *getAlias();
    const char *getID();
    const char *getDisplayName();

    bool exist4platform();

  } gamecenter;

  struct Match
  {
    struct ICallback
    {
      virtual void onSuccessFind(const char *message) = 0;
      virtual void onFailedFind(const char *message) = 0;
      virtual void onPlayerDisconnect(size_t playedID) = 0;
      virtual ~ICallback() {}
    };

    struct Player_t
    {
      char str_id[DATA_LENGTH];
      char alias[DATA_LENGTH];
      char displayName[DATA_LENGTH];
      size_t id;

      Player_t()
      {
        memset(str_id, 0, DATA_LENGTH);
        memset(alias, 0, DATA_LENGTH);
        memset(displayName, 0, DATA_LENGTH);
      }

      void setId(const char *str)
      {
        strncpy(str_id, str, DATA_LENGTH - 1);
        id = convStrToId(str);
      }

      void setAlias(const char *str) { strncpy(alias, str, DATA_LENGTH - 1); }
      void setDisplayName(const char *str) { strncpy(displayName, str, DATA_LENGTH - 1); }

      static size_t convStrToId(const char *str)
      {
        size_t offset = 0;
        if (str[0] == 'G' && str[1] == ':')
          offset = 2;

        size_t result = atoll(str + offset);
        return result;
      }
    };

    struct Packet_t
    {
      size_t systemIndex;
      size_t systemAddress;
      uint32_t length;
      uint32_t bitSize;
      uint8_t *data;
      int receiveTime; // in ms

      static Packet_t *create() { return new Packet_t(); }
    };

    ICallback &getCallback();
    void setCallback(ICallback *callback);

    void pushPacket(Packet_t *p);
    Packet_t *popPacket();
    bool deallocatePacket(Packet_t *packet);
    bool sendData(const char *data, size_t length, size_t playedID, bool broadcast = false);

    bool find(int minPlayers, int maxPlayers, int level);
    bool cancelFind();
    bool isFindCanceled();
    bool isFindInProgress();

    bool isActive();
    size_t getSystemAddress();

    bool setupHostConnection(const char *data, size_t length);
    bool isHost();
    bool isClient();

    bool getPlayer(int index, Player_t &player);
    bool isPlayerConnected(size_t id);
  } match;
};

extern Kit kit;

} // end namespace apple
