// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sepClientVerify.h"

#include <catch2/catch_test_macros.hpp>


namespace tests
{


void verify_client_state(const sepclient::SepClient &client, SepClientState expected_state)
{
  CAPTURE(expected_state);
  REQUIRE(client.isCallable() == (expected_state == SepClientState::WORKING));
  REQUIRE(client.isCloseCompleted() == (expected_state == SepClientState::CLOSED));
}


} // namespace tests
