#pragma once
#include <vector>
#include <cef_browser.h>


namespace cef
{

class BrowserList
{
public:
  typedef CefRefPtr<CefBrowser> value_type;
  typedef int key_type;

  struct Entry
  {
    Entry(value_type b=nullptr) : browser(b) { }
    value_type browser;
    std::string title;

    key_type id() const { return this->browser ? this->browser->GetIdentifier() : -1; }
  }; // struct Entry


public:
  BrowserList(size_t n=16) { entries.reserve(n); }

  size_t size() const { return this->entries.size(); }

  value_type active() { return this->current.browser; }
  bool isActive(value_type b) const { return this->current.id() == b->GetIdentifier(); }
  const char *getActiveTitle() const { return (this->current.browser) ? this->current.title.c_str() : ""; }
  void setTitle(value_type b, const char *t);


  void add(value_type b, bool active=true);
  void remove(value_type b);
  bool select(key_type id);

  void closeAll();

private:
  std::vector<Entry> entries;
  Entry& current = stub;
  static Entry stub;
}; // struct BrowserList

} // namespace cef
