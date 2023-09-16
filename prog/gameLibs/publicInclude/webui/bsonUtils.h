//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <webui/bson.h>

template <typename Callback>
static void bson_document(BsonStream &bson, const char *name, const Callback &callback)
{
  bson.begin(name);
  callback();
  bson.end();
}

template <typename Array, typename Callback>
static void bson_array_of_documents(BsonStream &bson, const char *name, const Array &arr, const Callback &callback)
{
  bson.beginArray(name);
  for (BsonStream::StringIndex i; i.idx() < arr.size(); i.increment())
  {
    bson.begin(i.str());
    callback(i.idx(), arr[i.idx()]);
    bson.end();
  }
  bson.end();
}

template <typename Array, typename Callback>
static void bson_array_of_documents(BsonStream &bson, const char *name, Array &arr, const Callback &callback)
{
  bson.beginArray(name);
  for (BsonStream::StringIndex i; i.idx() < arr.size(); i.increment())
  {
    bson.begin(i.str());
    callback(i.idx(), arr[i.idx()]);
    bson.end();
  }
  bson.end();
}

template <typename Array, typename Callback, typename FilterCallback>
static void bson_array_of_documents(BsonStream &bson, const char *name, const Array &arr, const FilterCallback &filter_callback,
  const Callback &callback)
{
  bson.beginArray(name);
  for (BsonStream::StringIndex i, j; i.idx() < arr.size(); i.increment())
  {
    if (!filter_callback(i.idx(), arr[i.idx()]))
      continue;

    bson.begin(j.str());
    callback(i.idx(), arr[i.idx()]);
    bson.end();

    j.increment();
  }
  bson.end();
}

template <typename Array, typename Callback>
static void bson_array(BsonStream &bson, const char *name, const Array &arr, const Callback &callback)
{
  bson.beginArray(name);
  for (BsonStream::StringIndex i; i.idx() < arr.size(); i.increment())
  {
    bson.add(i.str(), callback(i.idx(), arr[i.idx()]));
  }
  bson.end();
}
