//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace PropPanel
{

// Do not forget to call remove_delayed_callback from the destructor of you inherit from this.
class IDelayedCallbackHandler
{
public:
  virtual void onImguiDelayedCallback(void *user_data) = 0;
};

void process_message_queue();
void request_delayed_callback(IDelayedCallbackHandler &callback_handler, void *user_data = nullptr);
void remove_delayed_callback(IDelayedCallbackHandler &callback_handler);

} // namespace PropPanel