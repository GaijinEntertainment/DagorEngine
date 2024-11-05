// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "messageQueueInternal.h"
#include <propPanel/control/container.h>

namespace PropPanel
{

MessageQueue message_queue;

void MessageQueue::release() { messages.clear(); }

void MessageQueue::sendDelayedOnWcChange(ContainerPropertyControl &container, int control_id)
{
  addMessage(MessageType::Change, container, control_id);
}

void MessageQueue::sendDelayedOnWcChange(PropertyControlBase &control)
{
  ContainerPropertyControl *container = control.getParent();
  G_ASSERT(container);
  G_ASSERT(container->isImguiContainer());
  sendDelayedOnWcChange(*static_cast<ContainerPropertyControl *>(container), control.getID());
}

void MessageQueue::sendDelayedOnWcClick(ContainerPropertyControl &container, int control_id)
{
  addMessage(MessageType::Click, container, control_id);
}

void MessageQueue::sendDelayedOnWcClick(PropertyControlBase &control)
{
  ContainerPropertyControl *container = control.getParent();
  G_ASSERT(container);
  G_ASSERT(container->isImguiContainer());
  sendDelayedOnWcClick(*static_cast<ContainerPropertyControl *>(container), control.getID());
}

void MessageQueue::sendDelayedPostEvent(ContainerPropertyControl &container, int data)
{
  addMessage(MessageType::PostEvent, container, data);
}

void MessageQueue::requestDelayedCallback(IDelayedCallbackHandler &callback_handler, void *user_data)
{
  Message message;
  message.messageType = MessageType::Callback;
  message.container = nullptr;
  message.callbackHandler = &callback_handler;
  message.controlId = 0;
  message.userData = user_data;
  messages.push_back(eastl::move(message));
}

void MessageQueue::addMessage(MessageType message_type, ContainerPropertyControl &container, int control_id)
{
  ContainerPropertyControl *rootContainer = container.getRootParent();
  G_ASSERT(rootContainer);
  G_ASSERT(rootContainer->isImguiContainer());

  Message message;
  message.messageType = message_type;
  message.container = static_cast<ContainerPropertyControl *>(rootContainer);
  message.callbackHandler = nullptr;
  message.controlId = control_id;
  message.userData = nullptr;
  messages.push_back(eastl::move(message));
}

void MessageQueue::onContainerDelete(ContainerPropertyControl &container)
{
  for (int i = 0; i < messages.size(); ++i)
    if (messages[i].container == &container)
    {
      messages.erase(&messages[i]);
      --i;
    }
}

void MessageQueue::removeDelayedCallback(IDelayedCallbackHandler &callback_handler)
{
  for (int i = 0; i < messages.size(); ++i)
  {
    Message &message = messages[i];

    if (message.callbackHandler == &callback_handler && message.messageType == MessageType::Callback)
    {
      messages.erase(&message);
      --i;
    }
  }
}

void MessageQueue::processMessages()
{
  // This is written this way to handle reentrancy, which is needed because of the modal dialogs.
  while (!messages.empty())
  {
    Message message = messages[0];
    messages.erase(messages.begin());

    if (message.messageType == MessageType::Change)
    {
      // The asserts are there to ensure that onWcChange call below matches this:
      // container.mEventHandler->onChange(pcbId, container.getRootParent());
      PropertyControlBase *control = message.container->getById(message.controlId);
      G_ASSERT(control);
      G_ASSERT(control->getEventHandler() == message.container->getEventHandler());
      G_ASSERT(control->getRootParent() == message.container->getRootParent());
      control->onWcChange(nullptr);
    }
    else if (message.messageType == MessageType::Click)
    {
      // The asserts are there to ensure that onWcChange call below matches this:
      // container.mEventHandler->onClick(pcbId, container.getRootParent());
      PropertyControlBase *control = message.container->getById(message.controlId);
      G_ASSERT(control);
      G_ASSERT(control->getEventHandler() == message.container->getEventHandler());
      G_ASSERT(control->getRootParent() == message.container->getRootParent());
      control->onWcClick(nullptr);
    }
    else if (message.messageType == MessageType::PostEvent)
    {
      message.container->onWcPostEvent(message.controlId);
    }
    else if (message.messageType == MessageType::Callback)
    {
      message.callbackHandler->onImguiDelayedCallback(message.userData);
    }
    else
    {
      G_ASSERT(false);
    }
  }
}

void process_message_queue() { message_queue.processMessages(); }

void request_delayed_callback(IDelayedCallbackHandler &callback_handler, void *user_data)
{
  message_queue.requestDelayedCallback(callback_handler, user_data);
}

void remove_delayed_callback(IDelayedCallbackHandler &callback_handler) { message_queue.removeDelayedCallback(callback_handler); }

} // namespace PropPanel