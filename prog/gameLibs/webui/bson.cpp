// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "webui/bson.h"
#include <memory/dag_memBase.h>

BsonStream::BsonStream() : done(false), stream(midmem_ptr()), documentStack(midmem_ptr())
{
  mem_set_0(stream);
  mem_set_0(documentStack);

  begin();
}

BsonStream::BsonStream(size_t stream_reserve_bytes, size_t stack_reserve_bytes) : BsonStream()
{
  stream.reserve(stream_reserve_bytes);
  stream.reserve(stack_reserve_bytes);
}

dag::ConstSpan<uint8_t> BsonStream::closeAndGetData()
{
  G_ASSERT((done && documentStack.empty()) || (!done && documentStack.size() == 1));
  if (!done)
  {
    end();
    done = true;
  }
  return stream;
}

void BsonStream::begin()
{
  G_ASSERT(!done);

  documentStack.push_back(stream.size());

  uint32_t size = 0;
  append_items(stream, 4, (const uint8_t *)&size);
}

void BsonStream::begin(const char *name, uint8_t type)
{
  G_ASSERT(!done);

  stream.push_back(type);
  BsonType<const char *>::add(stream, name);

  begin();
}

void BsonStream::begin(const char *name) { begin(name, embedded_document_type); }

void BsonStream::beginArray(const char *name) { begin(name, array_type); }

void BsonStream::end()
{
  G_ASSERT(!done);

  stream.push_back(0);

  G_ASSERT_RETURN(!documentStack.empty(), );
  uint32_t document = documentStack.back();
  documentStack.pop_back();

  *(uint32_t *)&stream[document] = stream.size() - document;
}

void BsonStream::addName(const char *name, uint8_t type)
{
  stream.push_back(type);
  BsonType<const char *>::add(stream, name);
}

void BsonStream::add(const char *name, const char *value)
{
  addName(name, BsonType<const char *>::type);

  const int len = (int)::strlen(value) + 1;
  BsonType<int>::add(stream, len);
  BsonType<const char *>::add(stream, value);
}

void BsonStream::add(const DataBlock &value)
{
  for (int i = 0; i < value.paramCount(); ++i)
  {
    int type = value.getParamType(i);
    const char *paramName = value.getParamName(i);

    if (type == DataBlock::TYPE_INT)
      add(paramName, value.getInt(paramName));
    if (type == DataBlock::TYPE_INT64)
      add(paramName, value.getInt64(paramName));
    else if (type == DataBlock::TYPE_REAL)
      add(paramName, value.getReal(paramName));
    else if (type == DataBlock::TYPE_BOOL)
      add(paramName, value.getBool(paramName));
    else if (type == DataBlock::TYPE_STRING)
      add(paramName, value.getStr(paramName));
    else if (type == DataBlock::TYPE_POINT2)
      add(paramName, value.getPoint2(paramName));
    else if (type == DataBlock::TYPE_POINT3)
      add(paramName, value.getPoint3(paramName));
    else if (type == DataBlock::TYPE_POINT4)
      add(paramName, value.getPoint4(paramName));
  }

  for (int i = 0; i < value.blockCount(); ++i)
  {
    if (value.getBlock(i))
      add(value.getBlock(i)->getBlockName(), *value.getBlock(i));
  }
}

void BsonStream::add(const char *name, const Point2 &value)
{
  begin(name, embedded_document_type);
  add("x", value.x);
  add("y", value.y);
  end();
}

void BsonStream::add(const char *name, const Point3 &value)
{
  begin(name, embedded_document_type);
  add("x", value.x);
  add("y", value.y);
  add("z", value.z);
  end();
}

void BsonStream::add(const char *name, const DPoint3 &value)
{
  begin(name, embedded_document_type);
  add("x", float(value.x));
  add("y", float(value.y));
  add("z", float(value.z));
  end();
}

void BsonStream::add(const char *name, const Point4 &value)
{
  begin(name, embedded_document_type);
  add("x", value.x);
  add("y", value.y);
  add("z", value.z);
  add("w", value.w);
  end();
}

void BsonStream::add(const char *name, const IPoint2 &value)
{
  begin(name, embedded_document_type);
  add("x", value.x);
  add("y", value.y);
  end();
}

void BsonStream::add(const char *name, const IPoint3 &value)
{
  begin(name, embedded_document_type);
  add("x", value.x);
  add("y", value.y);
  add("z", value.z);
  end();
}

void BsonStream::add(const char *name, const IPoint4 &value)
{
  begin(name, embedded_document_type);
  add("x", value.x);
  add("y", value.y);
  add("z", value.z);
  add("w", value.w);
  end();
}

void BsonStream::add(const char *name, const E3DCOLOR &value)
{
  begin(name, embedded_document_type);
  add("r", value.r);
  add("g", value.g);
  add("b", value.b);
  add("a", value.a);
  end();
}

void BsonStream::add(const char *name, const TMatrix &value) { add(name, dag::ConstSpan<float>(value.array, 12)); }

void BsonStream::add(const char *name, const DataBlock &value)
{
  begin(name, embedded_document_type);
  add(value);
  end();
}

void BsonStream::addAsTreeNode(const DataBlock &blk, const char *name)
{
  beginArray(name);

  int paramCount = blk.paramCount();
  int blockCount = blk.blockCount();

  BsonStream::StringIndex i;

  for (; i.idx() < paramCount; i.increment())
  {
    begin(i.str());

    int paramNo = i.idx();

    add("name", blk.getParamName(paramNo));

    int type = blk.getParamType(paramNo);
    if (type == DataBlock::TYPE_STRING)
      add("value", blk.getStr(paramNo));
    else if (type == DataBlock::TYPE_INT)
      add("value", blk.getInt(paramNo));
    else if (type == DataBlock::TYPE_INT64)
      add("value", blk.getInt64(paramNo));
    else if (type == DataBlock::TYPE_REAL)
      add("value", blk.getReal(paramNo));
    else if (type == DataBlock::TYPE_POINT2)
      add("value", blk.getPoint2(paramNo));
    else if (type == DataBlock::TYPE_POINT3)
      add("value", blk.getPoint3(paramNo));
    else if (type == DataBlock::TYPE_POINT4)
      add("value", blk.getPoint4(paramNo));
    else if (type == DataBlock::TYPE_IPOINT2)
      add("value", blk.getIPoint2(paramNo));
    else if (type == DataBlock::TYPE_IPOINT3)
      add("value", blk.getIPoint3(paramNo));
    else if (type == DataBlock::TYPE_IPOINT4)
      add("value", blk.getIPoint4(paramNo));
    else if (type == DataBlock::TYPE_BOOL)
      add("value", blk.getBool(paramNo));
    else if (type == DataBlock::TYPE_E3DCOLOR)
      add("value", blk.getE3dcolor(paramNo));
    else if (type == DataBlock::TYPE_MATRIX)
      add("value", blk.getTm(paramNo));
    else
      G_ASSERTF(false, "%s", blk.getParamName(paramNo));

    end();
  }

  for (; i.idx() < paramCount + blockCount; i.increment())
  {
    begin(i.str());

    const DataBlock *childBlk = blk.getBlock(i.idx() - paramCount);
    const char *childName = childBlk->getBlockName();

    add("name", childName);
    addAsTreeNode(*childBlk, "children");

    end();
  }
  end();
}
