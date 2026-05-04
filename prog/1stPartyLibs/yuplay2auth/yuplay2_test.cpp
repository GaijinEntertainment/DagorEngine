#include "stdafx.h"

#include "yu_http.h"
#include "yu_debug.h"

#include <yuplay2_test.h>


//==================================================================================================
bool YuHttpTest::testSimpleGet()
{
//  YuHttpSyncRequest get("http://yuplay.com/hello.html");
//
//  if (get.ok())
//  {
//    YuDebug::out("Simple GET returned: %s", get.dataStr().c_str());
//    return true;
//  }
//
//  YuDebug::out("Simple GET failed");
  return false;
}


//==================================================================================================
bool YuHttpTest::testGetParams()
{
//  YuStrMap params;
//  params["foo"] = "bar";
//
//  YuHttpSyncRequest get("http://yuplay.com/hello.php", params, false);
//
//  if (get.ok())
//  {
//    YuDebug::out("GET with params returned: %s", get.dataStr().c_str());
//    return true;
//  }
//
//  YuDebug::out("GET with prams failed");
  return false;
}


//==================================================================================================
bool YuHttpTest::testHttpsGet()
{
//  YuHttpSyncRequest get("https://yuplay.yuplay.com/test.html");
//
//  if (get.ok())
//  {
//    YuDebug::out("HTTPS GET returned: %s", get.dataStr().c_str());
//    return true;
//  }
//
//  YuDebug::out("HTTPS GET failed");
  return false;
}


//==================================================================================================
bool YuHttpTest::testSimplePost()
{
//  YuHttpSyncRequest post("http://yuplay.com/hello.php", true);
//
//  if (post.ok())
//  {
//    YuDebug::out("Simple POST returned: %s", post.dataStr().c_str());
//    return true;
//  }
//
//  YuDebug::out("Simple POST failed");
  return false;
}


//==================================================================================================
bool YuHttpTest::testPostParams()
{
//  YuStrMap params;
//  params["foo"] = "bar";
//
//  YuHttpSyncRequest post("http://yuplay.com/hello.php", params);
//
//  if (post.ok())
//  {
//    YuDebug::out("POST with params returned: %s", post.dataStr().c_str());
//    return true;
//  }
//
//  YuDebug::out("POST with prams failed");
  return false;
}


//==================================================================================================
bool YuHttpTest::testHttpsPost()
{
//  YuStrMap params;
//  params["foo"] = "bar";
//
//  YuHttpSyncRequest post("https://yuplay.yuplay.com/check_login.php", params);
//
//  if (post.ok())
//  {
//    YuDebug::out("HTTPS POST returned: %s", post.dataStr().c_str());
//    return true;
//  }
//
//  YuDebug::out("HTTPS POST failed");
  return false;
}
