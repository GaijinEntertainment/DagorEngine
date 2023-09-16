#pragma once

namespace ipc
{

class Endpoint;
class EnableEndpoint
{
  public:
    EnableEndpoint() : endpoint(NULL) { /* VOID */ }
    virtual ~EnableEndpoint() { /* VOID */ }
    virtual void setEndpoint(Endpoint *ep) { this->endpoint = ep; }

  protected:
    Endpoint *endpoint;
}; // class EnableEncpoint

} // namespace ipc
