//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

class DataBlock;
class VoiceSession;
class VoiceManager;

class IVoiceSessionCb
{
public:
  // NOTE: str_uid is a temporary pointer! copy string if needed
  virtual void onVoiceSessionJoined(const char *str_uid, const char *name) = 0;
  virtual void onVoiceSessionParted(const char *str_uid, const char *name) = 0;
  virtual void onVoiceSessionUpdated(const char *str_uid, const char *name, bool is_speaking) = 0;
  virtual void onSessionConnect(bool connected) = 0;
  virtual void onSessionDisconnect() = 0;
};


class VoiceSession
{
public:
  virtual void destroy() = 0;
  virtual void setCb(IVoiceSessionCb *cb) = 0;

protected:
  VoiceSession() {}
  virtual ~VoiceSession() {}
};


class VoiceManager
{
public:
  virtual ~VoiceManager() {}

  virtual void init(const DataBlock &cfg) = 0;
  virtual void shutdown() = 0;

  virtual void setLogin(const char *login, const char *pass) = 0;
  virtual void stop() = 0;

  virtual VoiceSession *createSession(const char *uri, const char *login = nullptr, const char *pass = nullptr) = 0;

  virtual void update() = 0;
  virtual void updateAlways() = 0;
  virtual void updateSettings() = 0;

  virtual void setVolumeOut(float val) = 0;
  virtual void setVolumeIn(float val) = 0;
  virtual void muteLocalMic(bool mute) = 0;
  virtual void muteRemoteSound(bool mute) = 0;
  virtual void muteRemotePeer(bool mute, const char *name) = 0;
  virtual void muteRemotePeer(bool mute, uint64_t user_id) = 0;
  virtual bool isPlayerMuted(const char *name) = 0;

  virtual void setRecordDevice(int id) = 0;

  virtual bool isOffline() const = 0;
};

VoiceManager *create_voice_manager_tamvoice();
