#ifndef __YUPLAY2_ITEM_IMPL__
#define __YUPLAY2_ITEM_IMPL__
#pragma once


#include <yuplay2_item.h>


class YuParserVal;
class YuJson;


class YuItemInfo : public IYuplay2ItemInfo
{
public:
  bool assign(const YuString& guid, const YuParserVal& json);

private:
  YuString guid;
  YuString title;
  YuString url;
  YuString shortDesc;
  float price = 0.0;
  YuString priceCurr;
  float priceUsd = 0.0;
  float discountMul = 0.0;
  int64_t discountTill = 0;
  unsigned flags = 0;
  int64_t itemId = 0;

  virtual const char* YU2VCALL getGuid() { return guid.c_str(); }
  virtual int64_t YU2VCALL getItemId() { return itemId; }
  virtual const char* YU2VCALL getTitle() { return title.c_str(); }
  virtual const char* YU2VCALL getUrl() { return url.c_str(); }
  virtual const char* YU2VCALL getShortDesc() { return shortDesc.c_str(); }

  virtual float YU2VCALL getPrice() { return price; }
  virtual float YU2VCALL getPriceUsd() { return priceUsd; }
  virtual const char* YU2VCALL getPriceCurr() { return priceCurr.c_str(); }

  virtual float YU2VCALL getDiscountMul() { return discountMul; }
  virtual int64_t YU2VCALL getDiscountTill() { return discountTill; }

  virtual unsigned YU2VCALL getFlags() { return flags; }

  virtual void YU2VCALL free() { delete this; }
};


class YuItemsInfo : public IYuplay2ItemsInfo
{
public:
  YuItemsInfo(const YuJson& json, const YuStringSet& guids);

private:
  typedef eastl::map<YuString, YuItemInfo>  Items;
  typedef Items::iterator                   ItemsI;
  typedef Items::const_iterator             ItemsCI;

  Items items;

  virtual IYuplay2ItemInfoBase* YU2VCALL itemInfo(const char* guid);
  virtual unsigned YU2VCALL getGuidsCount() { return (unsigned)items.size(); }
  virtual const char* YU2VCALL getGuid(unsigned idx);

  virtual void YU2VCALL free() { delete this; }
};


class YuItemPurchases : public IYuplay2ItemPurchases
{
public:
  YuItemPurchases(const YuJson& json, const YuStringSet& guids);

private:
  typedef eastl::map<YuString, int> Purchases;
  typedef Purchases::const_iterator PurchasesCI;

  Purchases purch;

  virtual int YU2VCALL purchaseCount(const char* guid);
  virtual unsigned YU2VCALL getGuidsCount() { return (unsigned)purch.size(); }
  virtual const char* YU2VCALL getGuid(unsigned idx);

  virtual void YU2VCALL free() { delete this; }
};


class GooglePurchaseInfo : public IYuplay2GooglePurchaseInfo
{
public:
  GooglePurchaseInfo(const YuString& purch_token, const YuString& item_id, int added_) :
    token(purch_token), itemId(item_id), added(added_) {}

private:
  YuString token;
  YuString itemId;
  int added;

  virtual const char* YU2VCALL getPurchaseToken() { return token.c_str(); }
  virtual const char* YU2VCALL getItemId() { return itemId.c_str(); }
  virtual int YU2VCALL getAdded() { return added; }

  virtual void YU2VCALL free() { delete this; }
};


#endif //__YUPLAY2_ITEM_IMPL__
