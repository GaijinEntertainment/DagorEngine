// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/messageQueue.h>
#include <dag/dag_vector.h>

namespace PropPanel
{

class ContainerPropertyControl;
class PropertyControlBase;

// This is needed to be able to create a modal dialog in response to an onChange/onClick event. Because the onChange/onClick
// event sent while drawing ImGui it is necessary the delay the notification to be able to render ImGui from the modal dialog's
// message loop.
// NOTE: ImGui porting: wherever this is used it works differently than the original.
class MessageQueue
{
public:
  ~MessageQueue() { release(); }

  void release();

  void sendDelayedOnWcChange(ContainerPropertyControl &container, int control_id);
  void sendDelayedOnWcChange(PropertyControlBase &control);
  void sendDelayedOnWcClick(ContainerPropertyControl &container, int control_id);
  void sendDelayedOnWcClick(PropertyControlBase &control);
  void sendDelayedPostEvent(ContainerPropertyControl &container, int data);

  void onContainerDelete(ContainerPropertyControl &container);

  void requestDelayedCallback(IDelayedCallbackHandler &callback_handler, void *user_data = nullptr);
  void removeDelayedCallback(IDelayedCallbackHandler &callback_handler);

  void processMessages();

private:
  enum class MessageType
  {
    Change,
    Click,
    PostEvent,
    Callback,
  };

  struct Message
  {
    MessageType messageType;
    ContainerPropertyControl *container;
    IDelayedCallbackHandler *callbackHandler;
    int controlId;
    void *userData;
  };

  void addMessage(MessageType message_type, ContainerPropertyControl &container, int control_id);

  dag::Vector<Message> messages;
};

extern MessageQueue message_queue;

} // namespace PropPanel