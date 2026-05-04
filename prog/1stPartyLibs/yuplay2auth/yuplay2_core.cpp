#include "stdafx.h"

#include <yuplay2_callback.h>


//==================================================================================================
YU2DLL const char* YU2CALL yuplay2_status_string(int status, bool brief)
{
  struct Status
  {
    Yuplay2Status status;
    const char* message;
    const char* briefMessage;
  };

  static Status codes[] =
  {
#define DECL_CODE(code, msg) { YU2_##code, msg, #code }
    DECL_CODE(OK, "OK"),
    DECL_CODE(FAIL, "Fail"),
    DECL_CODE(BUSY, "Busy"),
    DECL_CODE(WRONG_PARAMETER,  "Wrong parameter"),
    DECL_CODE(WRONG_LOGIN, "Authentication failed"),
    DECL_CODE(NOT_LOGGED_IN, "Not logged in"),
    DECL_CODE(EMPTY, "Empty"),
    DECL_CODE(ALREADY, "Operation is already done"),
    DECL_CODE(NOT_FOUND, "Not found"),
    DECL_CODE(FORBIDDEN, "Forbidden"),
    DECL_CODE(EXPIRED, "Token expired"),
    DECL_CODE(NO_MONEY, "Not enough money"),
    DECL_CODE(FROZEN, "Account is frozen"),
    DECL_CODE(FROZEN_BRUTEFORCE, "Account is frozen due to bruteforce"),
    DECL_CODE(NOT_OWNER, "Not owner"),
    DECL_CODE(PROFILE_DELETED, "Profile erased"),
    DECL_CODE(PSN_UNKNOWN, "Unknown PSN user"),
    DECL_CODE(PSN_RESTRICTED, "Disabled for PSN users"),
    DECL_CODE(NO_EULA_ACCEPTED, "Must accept EULA first"),
    DECL_CODE(2STEP_AUTH, "Two-step authentication code needed"),
    DECL_CODE(WRONG_PAYMENT, "Wrong payment method"),
    DECL_CODE(UNKNOWN, "Unknown error"),
    DECL_CODE(TENCENT_CLIENT_DLL_LOST, "Tencent client DLL lost"),
    DECL_CODE(TENCENT_CLIENT_NOT_RUNNING, "Tencent client isn't running"),
    DECL_CODE(WRONG_2STEP_CODE, "Wrong 2-step verification code"),
    DECL_CODE(SSL_CACERT, "Server CA cert check failed"),
    DECL_CODE(TIMEOUT, "Operation timed out"),
    DECL_CODE(SSL_ERROR, "SSL error"),
    DECL_CODE(SSL_CACERT_FILE, "CA cert file error"),
    DECL_CODE(HOST_RESOLVE, "Couldn't resolve host"),
    DECL_CODE(PAYMENT_LIMIT, "Purchase limit reached"),
    DECL_CODE(BAD_DOMAIN, "Prohibited domain"),
    DECL_CODE(WRONG_EMAIL, "Wrong e-mail"),
    DECL_CODE(DOI_INCOMPLETE, "Registration not complete, verify e-mail first"),
    DECL_CODE(FORBIDDEN_NEED_2STEP, "Requires account with two-step authentication"),
    DECL_CODE(UPDATE, "Update required"),
#undef DECL_CODE
  };

  for (int i = 0; i < sizeof(codes) / sizeof(codes[0]); ++i)
    if (codes[i].status == status)
      return brief ? codes[i].briefMessage : codes[i].message;

  return brief ? "CODE_UNKNOWN" : "Unknown error code";
}
