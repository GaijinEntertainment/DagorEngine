#pragma once
#include "../webbrowser.h"

#include <string>

#include "log.h"

#include "ipc/channel.h"
#include "ipc/process.h"
#include "ipc/shmem.h"
#include "ipc/socket.h"


namespace webbrowser
{

class Host : public IBrowser,
             public ipc::Channel::MessageHandler,
             public EnableLogging
{
  public:
    Host(const IBrowser::Properties& p, IHandler& h, ILogger& l);
    virtual ~Host() { /* VOID */ }

  public: // webbrowser::IBrowser
    virtual bool open();
    virtual bool close();
    virtual bool tick();

    virtual void show();
    virtual void hide();

    virtual void go(const char *url);
    virtual void back();
    virtual void forward();
    virtual void reload();
    virtual void stop();

    virtual void consume(const io::MouseMove&);
    virtual void consume(const io::MouseWheel&);
    virtual void consume(const io::MouseButton&);
    virtual void consume(const io::Key&);

    virtual void resize(unsigned short, unsigned short);

    virtual void addWindowMethod(const char* name, unsigned params_cnt);

  public: // ipc::Channel::MessageHandler
    bool operator()(const ipc::Channel::Message&);
    void onChannelError();
    void onChannelClosed();

  protected:
    void shutdown();

  protected:
    IHandler& handler;

    ipc::Socket sock;
    ipc::Channel channel;
    ipc::SharedMemory shm;
    ipc::Process worker;

    bool isRunning = false;
    bool isTerminating = false;
    uint32_t started = 0;

    short width = 0, height = 0;

    const IBrowser::Properties& props;
}; // class Host

} // namespace webbrowser
