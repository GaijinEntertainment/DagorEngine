//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DataBlock;
namespace Json
{
class Value;
}

bool blk_to_json(const DataBlock &blk, Json::Value &json, const DataBlock &scheme);
bool json_to_blk(const Json::Value &json, DataBlock &blk, const DataBlock &scheme, int nest_level);

void json_to_blk(const Json::Value &json, DataBlock &blk);
