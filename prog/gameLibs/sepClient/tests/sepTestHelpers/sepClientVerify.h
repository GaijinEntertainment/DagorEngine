// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sepClient/sepClient.h>

#include <catch2/catch_tostring.hpp>


namespace tests
{

enum class SepClientState
{
  WORKING,
  CLOSING,
  CLOSED,
};

void verify_client_state(const sepclient::SepClient &client, SepClientState expected_state);

} // namespace tests


CATCH_REGISTER_ENUM(tests::SepClientState, tests::SepClientState::WORKING, tests::SepClientState::CLOSING,
  tests::SepClientState::CLOSED);
