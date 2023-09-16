#include "host.h"

#include <vector>
#include <assert.h>
#include <windows.h>
#include <mmsystem.h>

#include "ipc/msg.h"


namespace webbrowser
{
extern const wchar_t* get_cefprocess_exe_path();


Host::Host(const IBrowser::Properties& p, IHandler& h, ILogger& l) :
    EnableLogging(l),
    props(p), handler(h),
    channel(*this, l), shm(l),
    worker(get_cefprocess_exe_path(), l)
{
}

bool Host::operator()(const ipc::Channel::Message& input)
{
  using namespace ipc::msg;
  auto msg = GetAnyMessage(input.data); // TODO: Verify?
  flatbuffers::Verifier verifier(input.data, input.size);
  if (!VerifyAnyMessageBuffer(verifier))
  {
    ERR("[WBHP] Corrupt incoming message of size %zu at 0x%p", input.size, input.data);
    return false;
  }

  switch (msg->payload_type())
  {
    case Payload::Log:
      {
        auto m = msg->payload_as<ipc::msg::Log>();
        switch(m->level())
        {
          case LogLevel::ERR: ERR("[WBHP] %s", m->msg()->c_str()); break;
          case LogLevel::WARN: WRN("[WBHP] %s", m->msg()->c_str()); break;
          case LogLevel::INFO: INFO("[WBHP] %s", m->msg()->c_str()); break;
          case LogLevel::DBG: DBG("[WBHP] %s", m->msg()->c_str()); break;
        }
      }
      break;

    case Payload::BrowserInitialized:
      this->handler.onInitialized();
      break;

    case Payload::BrowserStarted:
      this->resize(this->width, this->height); // Force size (could have missed it), otherwise CEF won't render
      this->handler.onBrowserCreated();
      break;

    case Payload::BrowserExited:
      DBG("%s: BrowserExited received", __FUNCTION__);
      this->handler.onBrowserExited();
      this->shutdown();
      break;

    case Payload::BrowserError:
      {
        auto m = msg->payload_as<BrowserError>();
        this->handler.onBrowserError(webbrowser::WebBrowserError(m->error()));
        this->close();
      }
      break;

    case Payload::PageStateChanged:
      {
        auto m = msg->payload_as<PageStateChanged>();
        IHandler::PageInfo pi;
        pi.id = m->id();
        pi.title = m->title() ? m->title()->c_str() : "";
        pi.url = m->url() ? m->url()->c_str() : "";
        pi.error = m->error() ? m->error()->c_str() : "";
        pi.canGoBack = m->canBack();
        pi.canGoForward = m->canForward();

        switch (m->state())
        {
          case PageState::CREATED: this->handler.onPageCreated(pi); break;
          case PageState::LOADED: this->handler.onPageLoaded(pi); break;
          case PageState::ABORTED: this->handler.onPageAborted(pi); break;
          case PageState::FAILED: this->handler.onPageError(pi); break;
          default:
            DBG("[WBHP] ignoring PageStateChanged(%s)", EnumNamePageState(m->state()));
        }
      }
      break;

    case Payload::Paint:
      {
        auto m = msg->payload_as<Paint>();
        size_t size = m->width() * m->height() * 4;
        if (!size || size > this->shm.getSize())
          break; // Ignore late message after resize, don't read garbage

        std::vector<ipc::SharedMemory::data_type> buf(size);
        if (this->shm.read(buf.data(), buf.size()))
          this->handler.onPaint(buf.data(), m->width(), m->height());
        else
          DBG("%s: Could not read %zu bytes from %s, skipping frame", __FUNCTION__, buf.size(), shm.getKey());
      }
      break;

    case Payload::WindowMethodCall:
      {
        auto m = msg->payload_as<WindowMethodCall>();
        handler.onWindowMethodCall(m->name()->c_str(), m->p1()->c_str(), m->p2()->c_str());
      }
      break;

    case Payload::Popup:
      {
        auto m = msg->payload_as<Popup>();
        handler.onPopup(m->url()->c_str());
      }
      break;

    default:
      ERR("%s: Unexpected message from helper (%s)",
          __FUNCTION__, EnumNamePayload(msg->payload_type()));
      break;
  }

  return true;
}

void Host::onChannelError()
{
  this->handler.onBrowserError(WB_ERR_HELPER_LOST);
  this->shutdown();
}

bool Host::open()
{
  short port = 0;
  DBG("%s: got a list of %d ports", __FUNCTION__, this->props.availablePortsCount);
  for (short i = 0; i < this->props.availablePortsCount && !this->sock.isValid(); i++)
  {
    port = this->props.availablePorts[i];

    DBG("%s: trying port %d: %d", __FUNCTION__, i, port);
    this->sock.listen("127.0.0.1", port);
  }
  if (!this->sock.isValid())
  {
    WRN("%s: Could not find free port to listen on", __FUNCTION__);
    this->handler.onBrowserError(WB_ERR_HELPER_SPAWN);
    this->shutdown();
  }

  bool spawning = this->sock.isValid()
                  && this->worker.spawn(port,
                                       this->props.userAgent,
                                       this->props.resourcesDir,
                                       this->props.cachePath,
                                       this->props.logPath,
                                       this->props.backgroundColor,
                                       this->props.externalPopups,
                                       this->props.purgeCookiesOnStartup,
                                       this->props.parentWnd);
  if (!spawning)
  {
    WRN("%s: Could not create helper processs", __FUNCTION__);
    this->handler.onBrowserError(WB_ERR_HELPER_SPAWN);
    this->shutdown();
  }

  this->started = timeGetTime();
  return spawning;
}


bool Host::close()
{
  DBG("%s: closing browser (is running = %d)", __FUNCTION__, this->isRunning);
  if (this->isRunning) SEND_SIMPLE_MSG(BrowserClose)
  return this->isRunning;
}

void Host::shutdown()
{
  DBG("Host::shutdown");
  this->isTerminating = true;
  this->isRunning = false;
  this->shm.close();
  this->channel.shutdown();
}

bool Host::tick()
{
  if (this->isTerminating)
    return this->isRunning;

  if (this->channel.isConnected())
  {
    this->channel.process();
    return this->isRunning;
  }

  uint32_t curTime = timeGetTime();
  bool timedout = (curTime > this->started && (curTime - this->started) > 5000)  // No wrap around
               || (curTime < this->started && curTime > (this->started + 5000)); // Wrap around
  if (!this->sock.isValid() || timedout)
  {
    this->handler.onBrowserError(WB_ERR_HELPER_SPAWN);
    this->shutdown();
  }
  else if (this->sock.accept(this->channel.socket()) && this->channel.isConnected())
  {
    this->sock.close();
    DBG("Accepted helper connection");
    this->isRunning = true;
  }

  return this->isRunning;
}


void Host::show() { if (this->isRunning) SEND_SIMPLE_MSG(BrowserShow) }
void Host::hide() { if (this->isRunning) SEND_SIMPLE_MSG(BrowserHide) }

void Host::go(const char *url) { if (this->isRunning) SEND_DIRECT_MSG(Go, url) }
void Host::back() { if (this->isRunning) SEND_SIMPLE_MSG(GoBack) }
void Host::forward() { if (this->isRunning) SEND_SIMPLE_MSG(GoForward) }
void Host::reload() { if (this->isRunning) SEND_SIMPLE_MSG(BrowserRefresh) }
void Host::stop() { if (this->isRunning) SEND_SIMPLE_MSG(BrowserStop) }

void Host::consume(const io::MouseMove& e) { if (this->isRunning) SEND_DATA_MSG(MouseMove, e.x, e.y) }
void Host::consume(const io::MouseWheel& e) { if (this->isRunning) SEND_DATA_MSG(MouseWheel, e.x, e.y, e.dx, e.dy) }


void Host::consume(const io::MouseButton& e)
{
  if (this->isRunning)
    SEND_DATA_MSG(MouseButton, e.x, e.y, ipc::msg::MouseButtonId(e.btn), e.isUp)
}


void Host::consume(const io::Key& e)
{
  if (this->isRunning)
    SEND_DATA_MSG(Key, e.winKey, e.nativeKey, e.isSysKey, e.modifiers, ipc::msg::KeyEventType(e.event))
}


void Host::resize(unsigned short w, unsigned short h)
{
  unsigned int oldSize = this->shm.getSize();
  this->shm.resize(w * h * 4);
  if (oldSize < this->shm.getSize())
    DBG("%s: created texture buffer %s (%zu bytes)", __FUNCTION__, this->shm.getKey(), this->shm.getSize());

  this->width = w;
  this->height = h;
  if (this->isRunning)
    SEND_DIRECT_MSG(ViewResize, w, h, this->shm.getKey())
}


void Host::addWindowMethod(const char* name, unsigned params_cnt)
{
  if (this->isRunning)
  {
    DBG("Registered window.%s(%u parameters)", name, params_cnt);
    SEND_DIRECT_MSG(WindowMethod, name, params_cnt)
  }
}

} // namespace webbrowser
