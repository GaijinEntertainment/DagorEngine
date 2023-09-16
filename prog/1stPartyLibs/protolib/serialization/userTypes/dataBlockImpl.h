#pragma once

#include <protoConfig.h>
#include "dataBlock.h"

#include <ioSys/dag_memIo.h>
#include <compressionUtils/memSilentInPlaceLoadCB.h>

#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>


namespace proto
{
  enum DataBlockTags
  {
    DBT_BEGIN_DATABLOCK = 1,
    DBT_PARAM,
    DBT_END_DATABLOCK,
    DBT_PARAM_TYPE_STRING,
    DBT_PARAM_TYPE_INT,
    DBT_PARAM_TYPE_REAL,
    DBT_PARAM_TYPE_POINT2,
    DBT_PARAM_TYPE_POINT3,
    DBT_PARAM_TYPE_POINT4,
    DBT_PARAM_TYPE_IPOINT2,
    DBT_PARAM_TYPE_IPOINT3,
    DBT_PARAM_TYPE_BOOL,
    DBT_PARAM_TYPE_E3DCOLOR,
    DBT_PARAM_TYPE_MATRIX,
    DBT_PARAM_TYPE_INT64,
    DBT_PARAM_VALUE,
  };

  enum DagorDataSize
  {
    DDS_Point2    = 8,
    DDS_Point3    = 12,
    DDS_Point4    = 16,
    DDS_TM        = 48,
    DDS_E3DCOLOR  = 4,
  };

  static void test_sizeof_types()
  {
    G_STATIC_ASSERT(sizeof(Point2) == DDS_Point2);
    G_STATIC_ASSERT(sizeof(Point3) >= DDS_Point3);
    G_STATIC_ASSERT(sizeof(Point4) == DDS_Point4);
    G_STATIC_ASSERT(sizeof(TMatrix) == DDS_TM);
    G_STATIC_ASSERT(sizeof(E3DCOLOR) == DDS_E3DCOLOR);
  }

  static void writeParam(proto::io::OutputCodeStream & stream, const DataBlock & blk, int index)
  {
    const char * name = blk.getParamName(index);
    G_ASSERT(name != NULL);
    int paramType = blk.getParamType(index);
    stream.writeString(DBT_PARAM, name, strlen(name));

    switch (paramType)
    {
      case DataBlock::TYPE_STRING:
        {
          const char * value = blk.getStr(index);
          G_ASSERT(value != NULL);
          stream.writeString(DBT_PARAM_TYPE_STRING, value, strlen(value));
        }
        break;
      case DataBlock::TYPE_INT:
        stream.writeVarInt(DBT_PARAM_TYPE_INT, blk.getInt(index));
        break;
      case DataBlock::TYPE_REAL:
        stream.writeFloat(DBT_PARAM_TYPE_REAL, blk.getReal(index));
        break;
      case DataBlock::TYPE_POINT2:
        {
          const Point2 value = blk.getPoint2(index);
          stream.writeRAW(DBT_PARAM_TYPE_POINT2, &value, DDS_Point2);
        }
        break;
      case DataBlock::TYPE_POINT3:
        {
          const Point3 value = blk.getPoint3(index);
          stream.writeRAW(DBT_PARAM_TYPE_POINT3, &value, DDS_Point3);
        }
        break;
      case DataBlock::TYPE_POINT4:
        {
          const Point4 value = blk.getPoint4(index);
          stream.writeRAW(DBT_PARAM_TYPE_POINT4, &value, DDS_Point4);
        }
        break;
      case DataBlock::TYPE_IPOINT2:
        {
          const IPoint2 value = blk.getIPoint2(index);
          stream.writeVarInt(DBT_PARAM_TYPE_IPOINT2, value.x);
          stream.writeVarInt(DBT_PARAM_TYPE_IPOINT2, value.y);
        }
        break;
      case DataBlock::TYPE_IPOINT3:
        {
          const IPoint3 value = blk.getIPoint3(index);
          stream.writeVarInt(DBT_PARAM_TYPE_IPOINT3, value.x);
          stream.writeVarInt(DBT_PARAM_TYPE_IPOINT3, value.y);
          stream.writeVarInt(DBT_PARAM_TYPE_IPOINT3, value.z);
        }
        break;
      case DataBlock::TYPE_BOOL:
        stream.writeBool(DBT_PARAM_TYPE_BOOL, blk.getBool(index));
        break;
      case DataBlock::TYPE_E3DCOLOR:
        {
          const E3DCOLOR value = blk.getE3dcolor(index);
          stream.writeRAW(DBT_PARAM_TYPE_E3DCOLOR, &value, DDS_E3DCOLOR);
        }
        break;
      case DataBlock::TYPE_MATRIX:
        {
          const TMatrix value = blk.getTm(index);
          stream.writeRAW(DBT_PARAM_TYPE_MATRIX, &value, DDS_TM);
        }
        break;
      case DataBlock::TYPE_INT64:
        stream.writeVarInt(DBT_PARAM_TYPE_INT64, blk.getInt64(index));
        break;
      default:
        G_ASSERT(false);
    }
  }

  static void writeBlock(proto::io::OutputCodeStream & stream, const DataBlock & blk)
  {
    for (int i = 0; i < blk.blockCount(); ++i)
    {
      const DataBlock * child = blk.getBlock(i);
      G_ASSERT(child);
      const char * blockName = child->getBlockName();
      G_ASSERT(blockName != NULL);
      stream.writeString(DBT_BEGIN_DATABLOCK, blockName, strlen(blockName));
      writeBlock(stream, *child);
      stream.writeBool(DBT_END_DATABLOCK, true);
    }

    for (int i = 0; i < blk.paramCount(); ++i)
    {
      writeParam(stream, blk, i);
    }
  }

  void serialize(proto::io::OutputCodeStream & stream, proto::io::TNumber number, const DataBlock & blk)
  {
    if (blk.isEmpty())
    {
      stream.writeTag(proto::io::StreamTag::EMPTY, number);
    }
    else
    {
      stream.blockBegin(number);
      writeBlock(stream, blk);
      stream.blockEnd();
    }
  }

  bool readParam(proto::io::InputCodeStream & stream, proto::io::StreamTag tag, DataBlock & blk)
  {
    proto::string paramName;
    PROTO_VALIDATE(proto::io::readString(stream, tag, paramName));

    PROTO_VALIDATE(stream.readTag(tag));
    int paramType = tag.number;
    switch (paramType)
    {
      case DBT_PARAM_TYPE_STRING:
        {
          proto::string paramValue;
          PROTO_VALIDATE(proto::io::readString(stream, tag, paramValue));
          blk.addStr(paramName.c_str(), paramValue.c_str());
        }
        break;
      case DBT_PARAM_TYPE_INT:
        {
          int value = 0;
          PROTO_VALIDATE(stream.readVarInt(tag, value));
          blk.addInt(paramName.c_str(), value);
        }
        break;
      case DBT_PARAM_TYPE_REAL:
        {
          float value = 0;
          PROTO_VALIDATE(stream.readFloat(tag, value));
          blk.addReal(paramName.c_str(), value);
        }
        break;
      case DBT_PARAM_TYPE_POINT2:
        {
          Point2 value;
          PROTO_VALIDATE(stream.readRAW(tag, &value, DDS_Point2));
          blk.addPoint2(paramName.c_str(), value);
        }
        break;
      case DBT_PARAM_TYPE_POINT3:
        {
          Point3 value;
          PROTO_VALIDATE(stream.readRAW(tag, &value, DDS_Point3));
          blk.addPoint3(paramName.c_str(), value);
        }
        break;
      case DBT_PARAM_TYPE_POINT4:
        {
          Point4 value;
          PROTO_VALIDATE(stream.readRAW(tag, &value, DDS_Point4));
          blk.addPoint4(paramName.c_str(), value);
        }
        break;
      case DBT_PARAM_TYPE_IPOINT2:
        {
          IPoint2 value;
          PROTO_VALIDATE(stream.readVarInt(value.x));
          PROTO_VALIDATE(stream.readTag(tag));
          PROTO_VALIDATE(tag.number == DBT_PARAM_TYPE_IPOINT2);
          PROTO_VALIDATE(stream.readVarInt(value.y));
          blk.addIPoint2(paramName.c_str(), value);
        }
        break;
      case DBT_PARAM_TYPE_IPOINT3:
        {
          IPoint3 value;
          PROTO_VALIDATE(stream.readVarInt(value.x));
          PROTO_VALIDATE(stream.readTag(tag));
          PROTO_VALIDATE(tag.number == DBT_PARAM_TYPE_IPOINT3);
          PROTO_VALIDATE(stream.readVarInt(value.y));
          PROTO_VALIDATE(stream.readTag(tag));
          PROTO_VALIDATE(tag.number == DBT_PARAM_TYPE_IPOINT3);
          PROTO_VALIDATE(stream.readVarInt(value.z));
          blk.addIPoint3(paramName.c_str(), value);
        }
        break;
      case DBT_PARAM_TYPE_BOOL:
        {
          bool value = false;
          PROTO_VALIDATE(stream.readBool(tag, value));
          blk.addBool(paramName.c_str(), value);
        }
        break;
      case DBT_PARAM_TYPE_E3DCOLOR:
        {
          E3DCOLOR value;
          PROTO_VALIDATE(stream.readRAW(tag, &value, DDS_E3DCOLOR));
          blk.addE3dcolor(paramName.c_str(), value);
        }
        break;
      case DBT_PARAM_TYPE_MATRIX:
        {
          TMatrix value;
          PROTO_VALIDATE(stream.readRAW(tag, &value, DDS_TM));
          blk.addTm(paramName.c_str(), value);
        }
        break;
      case DBT_PARAM_TYPE_INT64:
        {
          int64_t value = 0;
          PROTO_VALIDATE(stream.readVarInt(tag, value));
          blk.addInt64(paramName.c_str(), value);
        }
        break;
      default:
        PROTO_ERROR("Unexpected param type");

    }
    return true;
  }

  static bool readBlock(proto::io::InputCodeStream & stream, DataBlock & blk)
  {
    proto::string tmpString;
    proto::io::StreamTag tag;
    PROTO_VALIDATE(stream.readTag(tag));

    while (!tag.isBlockEnded())
    {
      switch (tag.number)
      {
        case DBT_BEGIN_DATABLOCK:
          PROTO_VALIDATE(proto::io::readString(stream, tag, tmpString));
          PROTO_VALIDATE(readBlock(stream, *blk.addNewBlock(tmpString.c_str())));
          break;
        case DBT_PARAM:
          PROTO_VALIDATE(readParam(stream, tag, blk));
          break;
        case DBT_END_DATABLOCK:
          return true;
        default:
          PROTO_ERROR("Unexpected tag");
      }

      PROTO_VALIDATE(stream.readTag(tag));
    }
    return true;
  }

  bool serialize(proto::io::InputCodeStream & stream, proto::io::StreamTag tag, DataBlock & blk)
  {
    blk.reset();
    if (tag.type == proto::io::StreamTag::EMPTY)
    {
      return true;
    }
    else if (tag.type == proto::io::StreamTag::BLOCK_BEGIN)
    {
      PROTO_VALIDATE(readBlock(stream, blk));
      return true;
    }

    // backward compatibility remove it later
    proto::io::ConstBuffer buffer;
    PROTO_VALIDATE(stream.readString(tag, buffer));

    InPlaceMemLoadCB crd(buffer.begin(), buffer.size());
    PROTO_VALIDATE(dblk::load_from_stream(blk, crd, dblk::ReadFlag::ROBUST|dblk::ReadFlag::BINARY_ONLY));
    return true;
  }


  void load(const JsonReader & ser, const char * name, DataBlock & value)
  {
    value.reset();
    const Json::Value &jv = ser.getJson().get(name, Json::Value());
    if (!jv.isString())
      return;
    const char *txtStr = jv.asCString();
    int txtLen = (int)strlen(txtStr);
    InPlaceMemLoadCB crd(txtStr, txtLen);
    dblk::clr_flag(value, dblk::ReadFlag::BINARY_ONLY);
    dblk::load_from_stream(value, crd, dblk::ReadFlag::ROBUST);
  }

  void save(const JsonWriter & ser, const char * name, const DataBlock & value)
  {
    DynamicMemGeneralSaveCB cwr(tmpmem);
    value.saveToTextStream(cwr);
    cwr.write("", 1);
    ser.getJson()[name] = (const char *)cwr.data();
  }
}
