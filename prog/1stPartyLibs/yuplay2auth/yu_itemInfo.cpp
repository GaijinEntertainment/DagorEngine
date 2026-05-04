#include "stdafx.h"
#include "yu_itemInfo.h"
#include "yu_json.h"


//==================================================================================================
bool YuItemInfo::assign(const YuString& guid, const YuParserVal& json)
{
  this->guid = guid;

  flags = 0;

  itemId = json.at("item_id").i();

  price = json.at("shop_price").f();
  priceUsd = json.at("price_usd").f();

  discountMul = json.at("discount_mul").f();
  discountTill = json.at("discount_till").i();

  //Price 0.01 means free item
  if (price <= 0.01)
    price = 0.0;

  if (priceUsd <= 0.01)
    priceUsd = 0.0;

  title = json.at("title").s();
  url = json.at("url").s();
  shortDesc = json.at("short_desc").s();
  priceCurr = json.at("shop_price_curr").s();

  if (json.at("multi_purch").b())
    flags |= YU2_ITEM_MULTIPURCHASE;

  if (json.at("can_be_bought").b())
    flags |= YU2_ITEM_CAN_BE_BOUGHT;

  return true;
}





//==================================================================================================
YuItemPurchases::YuItemPurchases(const YuJson& json, const YuStringSet& guids)
{
  for (YuStringSetCI i = guids.begin(), end = guids.end(); i != end; ++i)
    purch[*i] = 0;

  const YuParserVal::Dict& items = json.at("items").o();

  for (YuParserVal::Dict::const_iterator i = items.begin(), end = items.end(); i != end; ++i)
  {
    for (YuStringSetCI gi = guids.begin(), gend = guids.end(); gi != gend; ++gi)
      if (!i->first.comparei(*gi))
        purch[*gi] = i->second.i();
  }
}


//==================================================================================================
int YU2VCALL YuItemPurchases::purchaseCount(const char* guid)
{
  if (guid)
  {
    PurchasesCI i = purch.find(guid);
    if (i != purch.end())
      return i->second;
  }

  return 0;
}


//==================================================================================================
const char* YU2VCALL YuItemPurchases::getGuid(unsigned idx)
{
  unsigned i = 0;

  for (PurchasesCI pi = purch.begin(), end = purch.end(); pi != end; ++pi, ++i)
    if (i == idx)
      return pi->first.c_str();

  return NULL;
}




//==================================================================================================
YuItemsInfo::YuItemsInfo(const YuJson& json, const YuStringSet& guids)
{
  const YuParserVal::Dict& it = json.at("items").o();

  for (YuParserVal::Dict::const_iterator i = it.begin(), end = it.end(); i != end; ++i)
    for (YuStringSetCI gi = guids.begin(), gend = guids.end(); gi != gend; ++gi)
      if (!i->first.comparei(*gi))
      {
        YuItemInfo info;
        if (info.assign(*gi, i->second))
        {
          items[*gi] = info;
          break;
        }
      }
}


//==================================================================================================
IYuplay2ItemInfoBase* YU2VCALL YuItemsInfo::itemInfo(const char* guid)
{
  if (guid)
  {
    ItemsI i = items.find(guid);
    if (i != items.end())
      return &i->second;
  }

  return NULL;
}


//==================================================================================================
const char* YU2VCALL YuItemsInfo::getGuid(unsigned idx)
{
  unsigned i = 0;

  for (ItemsCI pi = items.begin(), end = items.end(); pi != end; ++pi, ++i)
    if (i == idx)
      return pi->first.c_str();

  return NULL;
}

