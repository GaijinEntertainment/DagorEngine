#include "stdafx.h"
#include "yu_session.h"
#include "yu_http.h"

#include <yuplay2_session.h>


//==================================================================================================
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_session(const char* client_name)
{
  return new YuSession(client_name, NULL, 0, NULL);
}


//==================================================================================================
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_app_session(const char* client_name,
                                                           unsigned app_id)
{
  return new YuSession(client_name, NULL, app_id, NULL);
}


//==================================================================================================
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_session_distr(const char* client_name,
                                                             const char* distr)
{
  return new YuSession(client_name, NULL, 0, distr);
}


//==================================================================================================
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_app_session_distr(const char* client_name,
                                                                 unsigned app_id,
                                                                 const char* distr)
{
  return new YuSession(client_name, NULL, app_id, distr);
}


//==================================================================================================
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_app_game_session_distr(const char* client_name,
                                                                      const char* game,
                                                                      unsigned app_id,
                                                                      const char* distr)
{
  return new YuSession(client_name, game, app_id, distr);
}


//==================================================================================================
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_session_from_session(IYuplay2Session* from)
{
  if (from)
  {
    YuSession* sess = (YuSession*)from;
    return sess->createFromMe();
  }

  return new YuSession(NULL, NULL, 0, NULL);
}


//==================================================================================================
YU2DLL IYuplay2Session* YU2CALL yuplay2_create_session_from_session_app(IYuplay2Session* from,
                                                                        unsigned new_app_id)
{
  if (from)
  {
    YuSession* sess = (YuSession*)from;
    return sess->createFromMe(new_app_id);
  }

  return new YuSession(NULL, NULL, new_app_id, NULL);
}

//==================================================================================================
void YU2CALL yuplay2_shutdown_http()
{
  YuHttp::destroySelf();
}
