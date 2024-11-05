// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "endpoint.h"

#include <stdio.h>
#include <fstream>
#include <vector>
#include <string>

#include <windows.h>
#include <shellapi.h>

#include <ipc/msg.h>
#include <unicodeString.h>

#include "cef/app.h"
#include "cef/client.h"
#include "cef/cookies.h"


namespace ipc
{
using UnicodeString = webbrowser::UnicodeString;

static const std::string log_path_arg = "log-file";

struct Logger : public webbrowser::ILogger
{
  virtual ~Logger() { this->file.close(); }

  virtual void err(const char *msg) { this->write(ipc::msg::LogLevel::ERR, msg); }
  virtual void warn(const char *msg) { this->write(ipc::msg::LogLevel::WARN, msg); }
  virtual void info(const char *msg) { this->write(ipc::msg::LogLevel::INFO, msg); }
  virtual void dbg(const char *msg) { this->write(ipc::msg::LogLevel::DBG, msg); }
  virtual void flush() { this->file.flush(); }

  void setFile(const char *path) { file.open(path, std::ios::app); }
  void setRemote(ipc::Channel *c) { this->channel = c; }
  void write(ipc::msg::LogLevel l, const char *msg)
  {
    using namespace ipc::msg;
    if (this->channel && this->channel->isConnected())
    {
      flatbuffers::FlatBufferBuilder fbb;
      auto p = CreateLogDirect(fbb, l, msg);
      auto m = CreateAnyMessage(fbb, 0, Payload::Log, p.Union());
      fbb.Finish(m);
      if (!this->channel->send(fbb.GetBufferPointer(), fbb.GetSize()))
      {
        this->file << time(NULL) << " [E] Could not remote log : " << msg << std::endl;
        this->file << time(NULL) << " [E] Socket err: " << this->channel->error() << std::endl;
      }
    }
    char tag = 'D';
    switch (l)
    {
      case LogLevel::ERR: tag = 'E'; break;
      case LogLevel::WARN: tag = 'W'; break;
      case LogLevel::INFO: tag = 'I'; break;
      case LogLevel::DBG: tag = 'D'; break;
    }
    std::string ts = std::to_string(time(NULL));

    std::string formattedMessage = ts + " (" + pid + ")" + " [" + tag + "] " + msg;
    if (this->file)
    {
      for (auto logLine : logCache)
        this->file << logLine << std::endl;
      logCache.clear();

      this->file << formattedMessage << std::endl;
    }
    else
      logCache.emplace_back(formattedMessage);
  }

  std::ofstream file;
  ipc::Channel *channel = nullptr;
  std::string pid = std::to_string(GetCurrentProcessId());
  std::vector<std::string> logCache;
}; // struct Logger
Logger global_log;


Endpoint::Endpoint(CefRefPtr<cef::App> cef_app)
  : webbrowser::EnableLogging(global_log), app(cef_app), channel(*this, global_log), shm(global_log)
{
  int argc = 0;
  wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);

  DBG("%s: loading %d arguments", __FUNCTION__, argc);
  for (int i = 1; i < argc; i++)
  {
    wchar_t *k = argv[i];
    while (*k == L'-') k++;

    wchar_t *v = wcschr(k, L'=');
    if (v)
    {
      *v = L'\0';
      v++;
    }
    else if (i+1 < argc)
      v = argv[++i];
    else
      return;
    const std::string key = UnicodeString(k).utf8();
    this->params[key] = UnicodeString(v).utf8();
    DBG("%s: added [%s: %s]", __FUNCTION__, key.c_str(), this->params[key].c_str());
  }

  if (this->params.count(log_path_arg))
    global_log.setFile(this->params[log_path_arg].c_str());

  extraInfo = CefDictionaryValue::Create();
}


Endpoint::~Endpoint()
{
  CefShutdown();
}


bool Endpoint::operator()(const ipc::Channel::Message& input)
{
  if (!this->client.get())
    return true;

  using namespace ipc::msg;
  auto msg = GetAnyMessage(input.data);
  switch(msg->payload_type())
  {
    case Payload::MouseMove:
      {
        auto m = msg->payload_as<MouseMove>();
        CefMouseEvent ce;
        ce.x = m->x();
        ce.y = m->y();
        this->client->sendMouseMove(ce);
      }
      break;

    case Payload::MouseWheel:
      {
        auto m = msg->payload_as<MouseWheel>();
        CefMouseEvent ce;
        ce.x = m->x();
        ce.y = m->y();
        this->client->sendMouseWheel(ce, m->dx(), m->dy());
      }
      break;

    case Payload::MouseButton:
      {
        auto m = msg->payload_as<MouseButton>();
        CefMouseEvent ce;
        ce.x = m->x();
        ce.y = m->y();
        this->client->sendMouseButton(ce, short(m->btn()), m->isUp());
      }
      break;

    case Payload::Key:
      {
        auto m = msg->payload_as<Key>();
        CefKeyEvent ce;
        ce.windows_key_code = m->winKey();
        ce.native_key_code = m->nativeKey();
        ce.is_system_key = m->isSysKey();
        ce.modifiers = m->modifiers();
        switch (m->eventType())
        {
          case KeyEventType::PRESS: ce.type = KEYEVENT_RAWKEYDOWN; break;
          case KeyEventType::RELEASE: ce.type = KEYEVENT_KEYUP; break;
          case KeyEventType::CHAR: ce.type = KEYEVENT_CHAR; break;
        }
        this->client->sendKeyEvent(ce);
      }
      break;

    case Payload::ViewResize:
      {
        auto m = msg->payload_as<ViewResize>();
        this->client->resize(m->width(), m->height());
        this->shm.close();
        this->shm.attach(m->key()->c_str(), m->width() * m->height() * 4);
      }
      break;

    case Payload::BrowserShow:
       this->client->show();
       break;

    case Payload::BrowserHide:
      this->client->hide();
      break;

    case Payload::BrowserStop:
      this->client->stop();
      break;

    case Payload::BrowserClose:
      DBG("%s: BrowserClose received", __FUNCTION__);
      this->client->shutdown();
      break;

    case Payload::BrowserRefresh:
      this->client->reload();
      break;

    case Payload::GoForward:
      this->client->forward();
      break;

    case Payload::GoBack:
      this->client->back();
      break;

    case Payload::Go:
      {
        auto m = msg->payload_as<Go>();
        if (!this->client->hasBrowser())
        {
          CefWindowInfo wi;
          char* endSym = NULL;
          HWND parent = (HWND)::strtoull(this->params["lwb-hndl"].c_str(), &endSym, 16);

          this->client->setWindowHandle(parent);
          this->client->setPopupsExternal(this->params.count("lwb-extpop"));

          bool offScreen = !this->params.count("lwb-gdi");

          if (offScreen)
            wi.SetAsWindowless(parent);
          else
          {
            RECT rect = { 0 };
            ::GetClientRect(parent, &rect);
            CefRect cr(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
            DBG("%s: settings as child to %x @{%d, %d} of size {%d, %d}", __FUNCTION__, parent,
                cr.x, cr.y, cr.width, cr.height);
            wi.SetAsChild(parent, cr);
          }

          CefBrowserSettings bs;

          bs.background_color = getBgColor();

          if (offScreen)
            bs.windowless_frame_rate = 60; // CEF's default is 30

          CefBrowserHost::CreateBrowserSync(wi,
                                            this->client.get(),
                                            m->url()->c_str(),
                                            bs,
                                            extraInfo,
                                            nullptr);
          DBG("%s: create browser for %s (%u external js methods)", __FUNCTION__,
              m->url()->c_str(), extraInfo->GetSize());
        }
        else
        {
          this->client->go(m->url()->c_str());
          DBG("%s: reuse browser for %s", __FUNCTION__, m->url()->c_str());
        }
      }
      break;

    case Payload::WindowMethod:
      {
        auto m = msg->payload_as<WindowMethod>();
        DBG("%s: add window.%s(%u arguments)", __FUNCTION__, m->name()->c_str(), m->parametersCnt());
        extraInfo->SetInt(m->name()->c_str(), m->parametersCnt());
      }
      break;

    default:
      break;
  }

  return true;
}


unsigned Endpoint::getBgColor() const
{
  unsigned ret = 0xFF555555;

  auto colori = this->params.find("lwb-bg");
  if (colori != this->params.end() && colori->second.length())
  {
    char* endSym = NULL;
    ret = ::strtoul(colori->second.c_str(), &endSym, 16);
  }

  return ret;
}


void Endpoint::onChannelError()
{
  ERR("%s: lost connection to host: %d", __FUNCTION__, this->channel.error());
  this->stop();
}


bool Endpoint::init(const CefMainArgs& args)
{
  unsigned short port = atoi(this->params["lwb-port"].c_str());
  if (!this->channel.connect(port))
  {
    ERR("%s: could not connect to %d: %d",
        __FUNCTION__,
        port,
        this->channel.error());
    return false;
  }

  global_log.setRemote(&this->channel);
  DBG("%s: set remote logging", __FUNCTION__);

  app->setEndpoint(this);

  CefSettings settings;
  settings.multi_threaded_message_loop = false;
  settings.no_sandbox = true;
  settings.background_color = getBgColor();
  settings.windowless_rendering_enabled = true;
  if (this->params.count("cache-path"))
    CefString(&settings.cache_path).FromString(this->params["cache-path"]);

  if (!CefInitialize(args, settings, app.get(), NULL))
  {
    ERR("%s: could not initialize CEF", __FUNCTION__);
    this->onBrowserError(webbrowser::WB_ERR_HELPER_INIT);
    return false;
  }

  return true;
}


void Endpoint::run()
{
  while (!this->shouldStop || this->client->hasBrowser())
  {
    if (this->shouldStop)
      this->client->shutdown();

    CefDoMessageLoopWork();
    if (this->channel.process() == 0)
      SleepEx(1, FALSE); // Don't burn CPU when nothing to do
  }
}


void Endpoint::stop()
{
  DBG("STOP!");
  this->shouldStop = true;
}


void Endpoint::onInitialized()
{
  this->client = new cef::Client();
  this->client->setEndpoint(this);
  if (!this->params.count("keep-cookies"))
    cef::clear_all_cookies(this);
  SEND_SIMPLE_MSG(BrowserInitialized)
}


void Endpoint::onShutdown()
{
  DBG("%s", __FUNCTION__);

  if (this->params.count("keep-cookies"))
    CefCookieManager::GetGlobalManager(nullptr)->FlushStore(nullptr);

  this->stop();
  SEND_SIMPLE_MSG(BrowserExited)
}


void Endpoint::onBrowserCreated()
{
  DBG("%s", __FUNCTION__);
  SEND_SIMPLE_MSG(BrowserStarted)
}


void Endpoint::onBrowserError(webbrowser::WebBrowserError err)
{
  SEND_DATA_MSG(BrowserError, err)
}


void Endpoint::onPageCreated(const webbrowser::IHandler::PageInfo& pi)
{
  SEND_DIRECT_MSG(PageStateChanged,
                  PageState::CREATED,
                  PageType::UNDEFINED,
                  0,
                  pi.url,
                  pi.title,
                  pi.error,
                  pi.canGoBack,
                  pi.canGoForward)
}


void Endpoint::onPageAborted(const webbrowser::IHandler::PageInfo& pi)
{
  SEND_DIRECT_MSG(PageStateChanged,
                  PageState::ABORTED,
                  PageType::UNDEFINED,
                  0,
                  pi.url,
                  pi.title,
                  pi.error,
                  pi.canGoBack,
                  pi.canGoForward)
}


void Endpoint::onPageLoaded(const webbrowser::IHandler::PageInfo& pi)
{
  SEND_DIRECT_MSG(PageStateChanged,
                  PageState::LOADED,
                  PageType::UNDEFINED,
                  0,
                  pi.url,
                  pi.title,
                  pi.error,
                  pi.canGoBack,
                  pi.canGoForward)
}


void Endpoint::onPageError(const webbrowser::IHandler::PageInfo& pi)
{
  SEND_DIRECT_MSG(PageStateChanged,
                  PageState::FAILED,
                  PageType::UNDEFINED,
                  0,
                  pi.url,
                  pi.title,
                  pi.error,
                  pi.canGoBack,
                  pi.canGoForward)
}


void Endpoint::onPopup(const char* url)
{
  SEND_DIRECT_MSG(Popup, url)
}


void Endpoint::onPaint(const uint8_t *buf, unsigned short w, unsigned short h)
{
  if (!this->shm.write(buf, w*h*4))
    return;

  SEND_DATA_MSG_SILENT(Paint, w, h)
}


void Endpoint::onWindowMethodCall(const char* name, const char* p1, const char* p2)
{
  SEND_DIRECT_MSG(WindowMethodCall, name, p1, p2)
}

} // namespace ipc
