#include "cookies.h"

#include "../enableEndpoint.h"
#include <cef_cookie.h>


namespace cef
{

class CookieWiper : public CefCookieVisitor,
                    public ipc::EnableEndpoint
{
public:
  bool Visit(const CefCookie& c, int, int, bool& should_delete) override
  {
    CefString name(c.name.str);
    this->endpoint->DBG("%s wiping %s", __FUNCTION__, name.ToString().c_str());
    should_delete = true;
    return true; // continue visiting
  }

  IMPLEMENT_REFCOUNTING(CookieWiper);
}; // class CookieWiper


void clear_all_cookies(ipc::Endpoint* ep)
{
  ep->DBG("%s wiping cookies", __FUNCTION__);
  CefRefPtr<CookieWiper> cookieWiper = new CookieWiper();
  cookieWiper->setEndpoint(ep);
  CefCookieManager::GetGlobalManager(nullptr)->VisitAllCookies(cookieWiper);
}

} // namespace cef
