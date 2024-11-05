//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>

class IPoint2;
class IPoint4;
class IGenLoad;

#include <supp/dag_define_KRNLIMP.h>

struct TreeBitmapNode
{
  void *subnodesOrMapdata;
  uint8_t fillColor;


  static constexpr int HAS_SUBNODES = 1 << 7;
  enum
  {
    MAPDATA_1BIT = 1 << 2, // 4
    MAPDATA_2BIT = 1 << 3, // 8

    MAPDATA_PIX1X1 = 1 << 4,
    MAPDATA_PIX2X2 = 2 << 4,
    MAPDATA_PIX4X4 = 3 << 4,
    MAPDATA_PIX8X8 = 4 << 4,
  };


  KRNLIMP TreeBitmapNode();
  KRNLIMP ~TreeBitmapNode();

  void *generateSubnodes(dag::ConstSpan<uint8_t> data, const IPoint2 &data_dimensions, const IPoint4 &rect, int size, int dim,
    int subnode_shift);

  KRNLIMP uint8_t get(uint16_t x, uint16_t y, int pix_size) const;
  KRNLIMP void gatherPixels(const IPoint4 &rect, dag::Span<uint8_t> out, const int width, const int pix_size) const;

  KRNLIMP void create(dag::ConstSpan<uint8_t> data, const IPoint2 &data_dimensions, const IPoint4 &rect);
  KRNLIMP void create(dag::ConstSpan<uint8_t> data, const IPoint2 &data_dimensions);

  KRNLIMP void load(IGenLoad &cb);
};

#include <supp/dag_undef_KRNLIMP.h>
