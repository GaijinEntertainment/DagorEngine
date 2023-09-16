#include <daGame/netUnit.h>

static const float resyncVersionDelay = 2.0f;
static const float resyncVersionResponseTimeout = 2.0f;

void NetUnit::onAuthorityUnitVersion(uint8_t remote_version_unsafe)
{
  // Only clients initiate version resync.
  if (isShadow())
  {
    if (isUnitVersionMatching(remote_version_unsafe))
    {
      // Versions are in sync.
      if (timeToResyncVersion > resyncVersionResponseTimeout)
      {
        sendUnitVersionMatch(getAuthorityUnitVersion());
        timeToResyncVersion = -1.f;
        resyncVersionRequests.clear();
        resyncVersionRequestsNumber = 0;
      }
    }
    else
    {
      // Desync detected
      if (timeToResyncVersion < 0.f)
      {
        // If we didn't request it already, request it now
        sendUnitVersionMismatch(remote_version_unsafe, getAuthorityUnitVersion());
        timeToResyncVersion = resyncVersionDelay + resyncVersionResponseTimeout;
      }
    }
  }
}

bool NetUnit::isUnitVersionMatching(uint8_t version) const { return getAuthorityUnitVersion() == version; }

void NetUnit::resetUnitVersion()
{
  timeToResyncVersion = -1.f;
  resyncVersionRequestsNumber = 0;
  resyncVersionRequests.clear();
}

void NetUnit::onUnitVersionMatch(uint8_t version, uint32_t sender_player_id)
{
  if (isUnitVersionMatching(version))
  {
    // Validate versions per client to prevent interference.
    if (sender_player_id < resyncVersionRequests.size() && resyncVersionRequests.get(sender_player_id))
    {
      resyncVersionRequestsNumber--;
      resyncVersionRequests.reset(sender_player_id);

      if (resyncVersionRequestsNumber <= 0)
      {
        resyncVersionRequestsNumber = 0;
        timeToResyncVersion = -1.f;
        resyncVersionRequests.clear();
      }
    }
  }
}

void NetUnit::onUnitVersionMismatch(uint8_t client_version, uint8_t server_version, uint32_t sender_player_id)
{
  if (isAuthority())
  {
    if (!isUnitVersionMatching(client_version) && isUnitVersionMatching(server_version))
    {
      if (sender_player_id >= resyncVersionRequests.size())
        if (!resyncVersionRequests.resize(sender_player_id + 1))
          return;
      if (!resyncVersionRequests.get(sender_player_id))
      {
        resyncVersionRequests.set(sender_player_id);
        resyncVersionRequestsNumber++;
      }
      if (timeToResyncVersion < 0.f)
        timeToResyncVersion = resyncVersionDelay;
    }
  }
  else
  {
    resyncVersionRequestsNumber++;
    timeToResyncVersion = resyncVersionDelay + resyncVersionResponseTimeout;
  }
}
