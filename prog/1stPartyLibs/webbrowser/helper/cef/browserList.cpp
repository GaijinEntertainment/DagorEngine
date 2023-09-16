#include "browserList.h"

#include <algorithm>

namespace cef
{

//static
BrowserList::Entry BrowserList::stub;

void BrowserList::add(value_type b, bool active)
{
  auto p = std::find_if(this->entries.begin(), this->entries.end(),
                        [&b](const Entry& v) { return b->GetIdentifier() == v.id(); });
  if (p == this->entries.end())
  {
    this->entries.emplace_back(b);
    if (active)
      this->current = this->entries.back();
  }
  else if (active)
    this->current = *p;
}


void BrowserList::remove(value_type b)
{
  auto p = std::find_if(this->entries.begin(), this->entries.end(),
                        [&b](const Entry& v) { return b->GetIdentifier() == v.id(); });
  if (p == this->entries.end())
    return;

  const bool shouldActivateLast = this->isActive(b);

  p->browser = nullptr;
  this->entries.erase(p);
  this->entries.shrink_to_fit();

  if (this->entries.empty())
    this->current = stub;
  else if (shouldActivateLast)
    this->current = this->entries.back();
}


bool BrowserList::select(key_type id)
{
  auto p = std::find_if(this->entries.begin(), this->entries.end(),
                        [id](const Entry& v) { return v.id() == id; });
  if (p != this->entries.end())
    this->current = *p;

  return p != this->entries.end();
}


void BrowserList::setTitle(value_type b, const char *t)
{
  auto p = std::find_if(this->entries.begin(), this->entries.end(),
                        [&b](const Entry& v) { return b->GetIdentifier() == v.id(); });
  if (p != this->entries.end())
    p->title = t;
}

void BrowserList::closeAll()
{
  for (auto &e : entries)
    if (e.browser.get())
      e.browser->GetHost()->CloseBrowser(true);
}

} // namespace cef
