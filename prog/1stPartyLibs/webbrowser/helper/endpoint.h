// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <map>
#include <string>

#include "../webbrowser.h"
#include <ipc/channel.h>
#include <ipc/shmem.h>

#include "cef/app.h"
#include "cef/client.h"


namespace ipc
{

class Endpoint : public ipc::Channel::MessageHandler,
                 public webbrowser::EnableLogging
{
  public:
    Endpoint(CefRefPtr<cef::App> app);
    ~Endpoint();

  public:
    bool init(const CefMainArgs& args);
    void run();
    void stop();

    const CefRefPtr<CefDictionaryValue>& getExtraInfo() const { return extraInfo; }

  public:
    void onInitialized();
    void onShutdown();

    void onBrowserCreated();
    void onBrowserError(webbrowser::WebBrowserError);

    void onPageCreated(const webbrowser::IHandler::PageInfo& pi);
    void onPageAborted(const webbrowser::IHandler::PageInfo& pi);
    void onPageLoaded(const webbrowser::IHandler::PageInfo& pi);
    void onPageError(const webbrowser::IHandler::PageInfo& pi);

    void onPopup(const char* url);

    void onPaint(const uint8_t *buf, unsigned short w, unsigned short h);

    void onWindowMethodCall(const char* name, const char* p1, const char* p2);

  public: // ipc::Channel::MessageHandler
    virtual bool operator()(const ipc::Channel::Message& msg);
    virtual void onChannelError();

  private:
    Channel channel;
    SharedMemory shm;
    bool shouldStop = false;

    CefRefPtr<cef::App> app;
    CefRefPtr<cef::Client> client;
    CefRefPtr<CefDictionaryValue> extraInfo;

    std::map<std::string, std::string> params;

    unsigned getBgColor() const;
}; // class Endpoint

} // namespace ipc
