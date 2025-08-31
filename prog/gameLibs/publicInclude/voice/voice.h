//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <daECS/core/entityId.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>

class DataBlock;
class VoiceSession;
class VoiceManager;

class IVoiceSessionCb
{
public:
  // NOTE: str_uid is a temporary pointer! copy string if needed
  virtual void onVoiceSessionJoined(const char *str_uid, const char *name, ecs::EntityId speaker_eid) = 0;
  virtual void onVoiceSessionParted(const char *str_uid, const char *name) = 0;
  virtual void onVoiceSessionUpdated(const char *str_uid, const char *name, bool is_speaking) = 0;
  virtual void onVoiceSessionSpeakerEidChanged(const char *str_uid, ecs::EntityId speaker_eid) = 0;
  virtual void onSessionConnect(bool connected) = 0;
  virtual void onSessionDisconnect() = 0;
  virtual ~IVoiceSessionCb() = default;
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
  using SessionsUpdater = eastl::function<void(VoiceManager &, const eastl::string &, const eastl::string &, const eastl::string &,
    eastl::unique_ptr<IVoiceSessionCb>)>;

  virtual ~VoiceManager() {}

  virtual void init(const DataBlock &cfg) = 0;
  virtual void shutdown() = 0;

  virtual void setLogin(const char *login, const char *pass) = 0;
  virtual void stop() = 0;

  virtual VoiceSession *createSession(const char *uri, const char *login = nullptr, const char *pass = nullptr) = 0;

  virtual void joinRoom(const char *uri, const char *login = nullptr, const char *pass = nullptr,
    eastl::unique_ptr<IVoiceSessionCb> cb = nullptr) = 0;
  virtual void setSessionsUpdater(SessionsUpdater &&) {}

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

class CompositeVoiceManager : public VoiceManager
{
public:
  virtual void registerVoiceManager(const char *prefix, eastl::unique_ptr<VoiceManager> &&manager) = 0;

  virtual void muteLocalMicFor(const char *uri, bool mute) = 0;
  virtual void muteRemoteSoundFor(const char *uri, bool mute) = 0;
  virtual void muteRemotePeerFor(const char *uri, bool mute, const char *name) = 0;
  virtual void muteRemotePeerFor(const char *uri, bool mute, uint64_t user_id) = 0;
};

VoiceManager *create_voice_manager_tamvoice();
VoiceManager *create_voice_manager_proximity_chat();
CompositeVoiceManager *create_voice_manager_composite();

constexpr const char *TAMVOICE_URL_PREFIX = "tamvchn://";
constexpr const char *PROXIMITY_CHAT_URL_PREFIX = "danetvoice://";
constexpr const char *PROXIMITY_CHAT_DEFAULT_ROOM = "danetvoice://proximity_chat";
