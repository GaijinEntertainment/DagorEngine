//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>

class DataBlock;
class BsonStream;

namespace dm
{
struct DataBlockRWHelper;
struct ObjectDescriptor;
struct CollisionDmObject;
struct MetaPartProp;
struct PartId;
enum class ShownHitEvents;
} // namespace dm

namespace webui
{
struct IDmViewerSource
{
  virtual const dm::DataBlockRWHelper &getBlkHelper() const = 0;
  virtual dm::CollisionDmObject getCollisionDmObject(const dm::ObjectDescriptor &object) const = 0;
  virtual dag::ConstSpan<dm::MetaPartProp> getMetaPartProps(const dm::ObjectDescriptor &object) const = 0;
  virtual dag::ConstSpan<int16_t> getPartEffectPresetIds(const dm::ObjectDescriptor &object) const = 0;
  // Appends every object that has a collision DM; first goes the watched hero's vehicle if it has one, else the hero if it has one.
  virtual void collectDmObjects(dag::Vector<dm::ObjectDescriptor> &out_objects) const = 0;
  virtual void doPartDamage(const dm::ObjectDescriptor &object, const dm::PartId &part_id, float damage) const = 0;
  virtual bool isPartDrawn(const dm::ObjectDescriptor &object, const dm::PartId &part_id) const = 0;
  virtual void setPartDrawn(const dm::ObjectDescriptor &object, const dm::PartId &part_id, bool draw) const = 0;
  // The page format is that of the event renderer mask functions of damageModelExtras/webTools.h; set also redraws the events.
  virtual void setEventRendererElementsMask(const DataBlock &blk) const = 0;
  // False while the game has no mask to report; the plugin asks again on its next update.
  virtual bool getEventRendererElementsMask(BsonStream &out_bson) const = 0;
  virtual dm::ShownHitEvents getShownHitEvents() const = 0;
  // The game redraws the events with the new mode.
  virtual void setShownHitEvents(dm::ShownHitEvents shown) const = 0;
  // The events of the last shot as EventArray::write gives them; false when the game has none.
  virtual bool getLastHitEvents(DataBlock &out_blk) const = 0;
};
} // namespace webui
