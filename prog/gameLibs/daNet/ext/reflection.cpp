// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/reflection.h>
#include <daNet/idFieldSerializer.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Quat.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <debug/dag_debug.h>
#include <math/dag_adjpow2.h>
#include <util/dag_stlqsort.h>

#define debug(...) logmessage(_MAKE4C('DNET'), __VA_ARGS__)

#define DEBUG_BORDERS_REFLECTION  0
#define DEBUG_BORDERS_REPLICATION 0

namespace danet
{
struct DummyEventListener
{
  static void OnVarCreated(...);
  static void OnStateChanged(...);
};
G_STATIC_ASSERT((!eastl::is_trivially_copyable<ReflectionVarBase<int, DummyEventListener>>::value)); // to avoid write it in BitStream
                                                                                                     // accidently
WinCritSec reflection_lists_critical_section;
List<ReflectableObject, true> changed_reflectables;
List<ReflectableObject, true> all_reflectables;
ReplicatedObjectCreator registered_repl_obj_creators[DANET_REPLICATION_MAX_CLASSES];
int num_registered_obj_creators = 0;
uint32_t option_flags = 0;

struct SortCreator
{
  inline bool operator()(const ReplicatedObjectCreator &l, const ReplicatedObjectCreator &r) const
  {
    return strcmp(l.name, r.name) < 0;
  }
};

void normalizeReplication()
{
  stlsort::sort(registered_repl_obj_creators, registered_repl_obj_creators + num_registered_obj_creators, SortCreator());
  for (int i = 0; i < num_registered_obj_creators; ++i)
    *registered_repl_obj_creators[i].class_id_ptr = i;
}

void on_reflection_var_created(danet::ReflectionVarMeta *meta, danet::ReflectableObject *robj, uint8_t persistent_id,
  const char *var_name, uint16_t var_flags, uint16_t var_bits)
{
  robj->checkWatermark();
#if DAGOR_DBGLEVEL > 0
  if (!(var_flags & RVF_EXCLUDED))
  {
    const danet::ReflectionVarMeta *sameNameMeta = robj->searchVarByName(var_name);
    G_ASSERTF(!sameNameMeta, "duplicate reflection var %s", var_name);
    const danet::ReflectionVarMeta *sameIdMeta = robj->getVarByPersistentId(persistent_id);
    G_ASSERTF(!sameIdMeta, "duplicate reflection var id %d", persistent_id);
  }
#endif
  if (robj->varList.tail)
  {
    robj->varList.tail->next = meta;
    robj->varList.tail = meta;
  }
  else
    robj->varList.tail = robj->varList.head = meta;
  meta->next = NULL;
  meta->persistentId = persistent_id;
  meta->name = var_name;
  meta->flags = var_flags;
  meta->numBits = var_bits;
}

void on_reflection_var_changed(danet::ReflectionVarMeta *meta, danet::ReflectableObject *robj, void *new_val)
{
  robj->checkWatermark();
  if ((meta->flags & RVF_EXCLUDED) || (robj->isRelfectionFlagSet(danet::ReflectableObject::EXCLUDED)))
  {
    if (meta->flags & RVF_CALL_HANDLER_FORCE)
      robj->onReflectionVarChanged(meta, new_val);
    return;
  }
  if (!(meta->flags & RVF_CHANGED))
  {
    meta->flags |= RVF_CHANGED;
    DANET_DLIST_PUSH(danet::changed_reflectables, robj, prevChanged, nextChanged);
  }
  if (meta->flags & (RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE))
    robj->onReflectionVarChanged(meta, new_val);
}

static void writeIntBits(danet::BitStream &bs, uint32_t val, uint32_t numBits);
static bool readIntBits(danet::BitStream &bs, uint32_t &retVal, uint32_t numBits);

void ReflectableObject::markVarWithFlag(ReflectionVarMeta *var, uint16_t f, bool set)
{
  checkWatermark();

  G_ASSERT(f);         // what the hell?
#if DAGOR_DBGLEVEL > 0 // make sure this var is belong to us
  bool fnd = false;
  for (const ReflectionVarMeta *v = varList.head; v && !fnd; v = v->next)
    fnd = v == var;
  G_ASSERT(fnd);
#endif
  if (set)
    var->flags |= f;
  else
    var->flags &= ~f;
}

int ReflectableObject::serialize(BitStream &bs, uint16_t flags_to_have, bool fth_all, uint16_t flags_to_ignore, bool fti_all,
  bool do_reset_changed_flag)
{
  checkWatermark();

  if (reflectionFlags & EXCLUDED)
    return 0;

  G_ASSERT(flags_to_have || fth_all);

  ReflectionVarMeta *varsToWrite[DANET_REFLECTION_MAX_VARS_PER_OBJECT];
  G_STATIC_ASSERT(sizeof(varsToWrite) <= 4096); // to much stack?
  G_STATIC_ASSERT(DANET_REFLECTION_MAX_VARS_PER_OBJECT <= IdFieldSerializer255::MAX_FIELDS_NUM);

  BitSize_t data_size_pos = ~0u;
  // first pass - fill in indexes only
  BitSize_t numVarsPos = 0;
  uint32_t numSerializedVars = 0; // in current reflectable
  IdFieldSerializer255 idFieldSerializer;
  for (ReflectionVarMeta *v = varList.head; v; v = v->next)
  {
    if (v->flags & RVF_EXCLUDED)
      continue;

    const bool allIgnoreFlagsExists = flags_to_ignore && (v->flags & flags_to_ignore) == flags_to_ignore;
    const bool anyIgnoreFlagsExists = (v->flags & flags_to_ignore) != 0;
    if (fti_all ? allIgnoreFlagsExists : anyIgnoreFlagsExists)
      continue;

    const bool allFlagsExists = (v->flags & flags_to_have) == flags_to_have;
    const bool anyFlagsExists = (v->flags & flags_to_have) != 0;

    if (!flags_to_have || (fth_all ? allFlagsExists : anyFlagsExists))
    {
      if (numSerializedVars == 0) // write header on first var write
      {
        bs.Write((uint16_t)0); // reserve space for data size (for skip in case if deserialization fails)
        data_size_pos = bs.GetWriteOffset();
        mpi::write_object_ext_uid(bs, getUID(), mpiObjectExtUID);
        bs.AlignWriteToByteBoundary();
        numVarsPos = bs.GetWriteOffset();
        bs.Write((uint16_t)0);                                 // reserve space for offset to sizes
        bs.Write(static_cast<IdFieldSerializer255::Index>(0)); // reserve space for vars count
      }

      idFieldSerializer.setFieldId(v->persistentId);
      varsToWrite[numSerializedVars++] = v;
    }
  }

  if (numSerializedVars)
  {
    G_ASSERT(numSerializedVars <= DANET_REFLECTION_MAX_VARS_PER_OBJECT);
    G_ASSERT(data_size_pos != ~0u);

    // second pass - write var data
    for (int i = 0; i < numSerializedVars; ++i)
    {
      ReflectionVarMeta *v = varsToWrite[i];
      G_ASSERT(v->coder);
#if DEBUG_BORDERS_REFLECTION
      bs.Write((uint8_t)117);
#endif
      BitSize_t pos_before_write = bs.GetWriteOffset();
      (*v->coder)(DANET_REFLECTION_OP_ENCODE, v, this, &bs);
      BitSize_t pos_after_write = bs.GetWriteOffset();
      G_ASSERTF(pos_after_write > pos_before_write, "%s in object '%s' var coder 0x%p for var '%s' (0x%p) did not writed any data",
        __FUNCTION__, getClassName(), (void *)v->coder, v->getVarName(), v);
      idFieldSerializer.setFieldSize(pos_after_write - pos_before_write);

#if DAGOR_DBGLEVEL > 0
      if ((option_flags & DUMP_REFLECTION) || (v->flags & RVF_DEBUG) || (reflectionFlags & DEBUG_REFLECTION))
        debug("%s serialize var '%s'(0x%p == 0x%x) %d bytes in obj '%s'(0x%p)", __FUNCTION__, v->getVarName(), v, v->getValue<int>(),
          BITS_TO_BYTES(v->numBits), getClassName(), this);
#endif

#if DEBUG_BORDERS_REFLECTION
      bs.Write((uint8_t)118);
#endif
      if (do_reset_changed_flag)
        v->flags &= ~RVF_CHANGED;
    }

    // third pass - write var ids and var data sizes
    idFieldSerializer.writeFieldsIndex(bs, numVarsPos);
    idFieldSerializer.writeFieldsSize(bs, numVarsPos);
    BitSize_t curPos = bs.GetWriteOffset();

    // write total size of writed data
    uint32_t total_data_size = BITS_TO_BYTES(curPos - data_size_pos);
    G_ASSERT(total_data_size <= 65535); // max - 64K per reflectable
    bs.WriteAt((uint16_t)total_data_size, data_size_pos - BYTES_TO_BITS(sizeof(uint16_t)));
#if DAGOR_DBGLEVEL > 0
    if (option_flags & DUMP_REFLECTION)
      debug("%s serialized object 0x%p '%s' %u vars %u(+2) bytes", __FUNCTION__, this, getClassName(), numSerializedVars,
        total_data_size);
#endif
  }

  if (do_reset_changed_flag)
    checkAndRemoveFromChangedList();

  return numSerializedVars;
}

int serializeChangedReflectables(BitStream &bs, uint16_t flags_to_have, bool fth_all, uint16_t flags_to_ignore, bool fti_all,
  bool do_reset_changed_flag)
{
  WinAutoLock l(danet::reflection_lists_critical_section);
  if (!changed_reflectables.head)
    return 0;

  BitSize_t writeNumReflPos = bs.GetWriteOffset();
  bs.Write((uint16_t)0); // make room for num serialized reflectables

  int numSerializedReflectables = 0;
  for (ReflectableObject *r = changed_reflectables.head; r;)
  {
    ReflectableObject *nextChanged = r->nextChanged;
    r->checkWatermark();

    if ((r->isRelfectionFlagSet(ReflectableObject::EXCLUDED)) == 0 && r->isNeedSerialize())
    {
      BitSize_t pos2_before_write = bs.GetWriteOffset();
      (void)pos2_before_write;
      if (r->serialize(bs, flags_to_have, fth_all, flags_to_ignore, fti_all, do_reset_changed_flag))
      {
        G_ASSERT(bs.GetWriteOffset() - pos2_before_write); // nothing was written, but returned true?
        bs.AlignWriteToByteBoundary();
        uint32_t data_written = BITS_TO_BYTES(bs.GetWriteOffset() - pos2_before_write);
        (void)data_written;
        G_ASSERTF(data_written <= 65535, "%s 0x%p type='%s' writed more than 65535 byte data (%d)", __FUNCTION__, r, r->getClassName(),
          data_written);
        ++numSerializedReflectables;
      }
    }
    else if (do_reset_changed_flag)
      r->resetChangeFlag();

    r = nextChanged;
  }

  // write reflectable count
  G_ASSERT(numSerializedReflectables <= 65535);
  bs.WriteAt((uint16_t)numSerializedReflectables, writeNumReflPos);

  return numSerializedReflectables;
}

int serializeAllReflectables(BitStream &bs, uint16_t flags_to_have, bool fth_all, uint16_t flags_to_ignore, bool fti_all,
  bool do_reset_changed_flag)
{
  WinAutoLock l(danet::reflection_lists_critical_section);
  if (!all_reflectables.head)
    return 0;

  BitSize_t writeNumReflPos = bs.GetWriteOffset();
  bs.Write((uint16_t)0); // make room for num serialized reflectables

  int numSerializedReflectables = 0;
  for (ReflectableObject *r = all_reflectables.head; r;)
  {
    ReflectableObject *next = r->next;

    if ((r->isRelfectionFlagSet(ReflectableObject::EXCLUDED)) == 0 && r->isNeedSerialize())
    {
      BitSize_t pos2_before_write = bs.GetWriteOffset();
      (void)pos2_before_write;
      if (r->serialize(bs, flags_to_have, fth_all, flags_to_ignore, fti_all, do_reset_changed_flag))
      {
        G_ASSERT(bs.GetWriteOffset() - pos2_before_write); // nothing was written, but returned true?
        bs.AlignWriteToByteBoundary();
        uint32_t data_written = BITS_TO_BYTES(bs.GetWriteOffset() - pos2_before_write);
        (void)data_written;
        G_ASSERTF(data_written <= 65535, "%s 0x%p type='%s' writed more than 65535 byte data (%d)", __FUNCTION__, r, r->getClassName(),
          data_written);
        ++numSerializedReflectables;
      }
    }
    else if (do_reset_changed_flag)
      r->resetChangeFlag();

    r = next;
  }

  // write reflectable count
  G_ASSERT(numSerializedReflectables <= 65535);
  bs.WriteAt((uint16_t)numSerializedReflectables, writeNumReflPos);

  return numSerializedReflectables;
}

int forceFullSyncForAllReflectables()
{
  WinAutoLock l(danet::reflection_lists_critical_section);
  int ret = 0;
  for (ReflectableObject *r = all_reflectables.head; r; r = r->next, ++ret)
    r->forceFullSync();
  return ret;
}

bool ReflectableObject::deserialize(BitStream &bs, int /* data_size */)
{
  checkWatermark();

  BitSize_t end_pos = 0;
  IdFieldSerializer255 idFieldSerializer;
  // read var sizes
  uint32_t numVars = idFieldSerializer.readFieldsSizeAndCount(bs, end_pos);
  G_ASSERT(numVars <= DANET_REFLECTION_MAX_VARS_PER_OBJECT);

  ReflectionVarMeta *varsToRead[DANET_REFLECTION_MAX_VARS_PER_OBJECT];
  G_STATIC_ASSERT(sizeof(varsToRead) <= 4096); // to much stack?
  G_STATIC_ASSERT(DANET_REFLECTION_MAX_VARS_PER_OBJECT <= IdFieldSerializer255::MAX_FIELDS_NUM);

  // read var indexes
  idFieldSerializer.readFieldsIndex(bs);
  ReflectionVarMeta *v = varList.head;
  for (uint32_t j = 0; j < numVars; ++j)
  {
    v = getVarByPersistentId(idFieldSerializer.getFieldId(j));
    if (v)
    {
      G_ASSERT((v->flags & RVF_NEED_DESERIALIZE) == 0);
      v->flags |= RVF_NEED_DESERIALIZE; // mark vars that we going to deserialize
    }
    varsToRead[j] = v;
  }

  // allow objects choose what flags to deserialize
  onBeforeVarsDeserialization();

  // read var data
  bool ret = true;
  for (uint32_t j = 0; j < numVars; ++j)
  {
    v = varsToRead[j];
#if DEBUG_BORDERS_REFLECTION
    uint8_t dv = 255;
    bs.Read(dv);
    G_ASSERT(dv == 117);
#endif
    int old_value = v ? v->getValue<int>() : 0;
    BitSize_t ppp = bs.GetReadOffset();
    if (!v)
    {
      idFieldSerializer.skipReadingField(j, bs); // skip
#if DEBUG_BORDERS_REFLECTION
      bs.Read(dv);
      G_ASSERT(dv == 118);
#endif
      continue;
    }
    G_ASSERT(v->coder);
    (void)ppp;
    if (!(*v->coder)(DANET_REFLECTION_OP_DECODE, v, this, &bs))
    {
      debug("can't decode value for var '%s' in obj 0x%p (type = '%s')", v->getVarName(), this, getClassName());
      idFieldSerializer.skipReadingField(j, bs); // skip
#if DEBUG_BORDERS_REFLECTION
      bs.Read(dv);
      G_ASSERT(dv == 118);
#endif
      continue;
    }
#if DAGOR_DBGLEVEL > 0
    else if ((option_flags & DUMP_REFLECTION) || (v->flags & RVF_DEBUG) || (reflectionFlags & DEBUG_REFLECTION))
      debug("%s deserialized var 0x%p '%s' %u bytes (0x%x -> 0x%x)", __FUNCTION__, v, v->getVarName(),
        BITS_TO_BYTES(bs.GetReadOffset() - ppp), old_value, v->getValue<int>());
#else
    (void)(old_value);
#endif
    if (bs.GetReadOffset() - ppp != idFieldSerializer.getFieldSize(j))
    {
      // e.g. both Ship and HVM have the same MPI_OID_GROUND_MODEL and the same reflection variables persistent ids
      // but WarShip fields really differ from those of HeavyVehicleModel
      ret = false;
      break;
    }
#if DEBUG_BORDERS_REFLECTION
    bs.Read(dv);
    G_ASSERT(dv == 118);
#endif
    G_ASSERT(!(v->flags & RVF_DESERIALIZED));
    if (v->flags & RVF_NEED_DESERIALIZE)
      v->flags |= RVF_DESERIALIZED;
  }

  bs.SetReadOffset(end_pos);

  // tell to object that we done
  onAfterVarsDeserialization();

  // reset service flags
  for (v = varList.head; v; v = v->next)
    v->flags &= ~(RVF_DESERIALIZED | RVF_NEED_DESERIALIZE);

  return ret;
}

int deserializeReflectables(BitStream &bs, mpi::object_dispatcher resolver)
{
  G_ASSERT(resolver);

  uint16_t numReflectables = 65535;
  if (!bs.Read(numReflectables))
    return -1;

  for (int i = 0; i < numReflectables; ++i)
  {
    uint16_t data_written = 0xffff;
    if (!bs.Read(data_written))
      return -2;

    G_ASSERTF(BITS_TO_BYTES(bs.GetNumberOfUnreadBits()) >= data_written,
      "%s unexpected stream end - unreaded %d bytes, data_written=%d bytes", __FUNCTION__, BITS_TO_BYTES(bs.GetNumberOfUnreadBits()),
      data_written);
    if (BITS_TO_BYTES(bs.GetNumberOfUnreadBits()) < data_written)
      return -3;

    BitSize_t start_pos = bs.GetReadOffset();
    mpi::ObjectID oid = mpi::INVALID_OBJECT_ID;
    mpi::ObjectExtUID extUid = mpi::INVALID_OBJECT_EXT_UID;
    mpi::read_object_ext_uid(bs, oid, extUid);
    bs.AlignReadToByteBoundary();
    BitSize_t startPosAfterId = bs.GetReadOffset();
    ReflectableObject *refl = (oid == mpi::INVALID_OBJECT_ID && extUid == mpi::INVALID_OBJECT_EXT_UID)
                                ? NULL
                                : static_cast<ReflectableObject *>(resolver(oid, extUid));
    if (refl && refl->debugWatermark != DANET_WATERMARK)
      debug("%s can't resolve (or bad reflection object) 0x%p by oid 0x%x", __FUNCTION__, refl, oid);
#if DAGOR_DBGLEVEL > 0
    if (option_flags & DUMP_REFLECTION)
      debug("%s 0x%x(%s) %d(+2) bytes", __FUNCTION__, oid, refl ? refl->getClassName() : "<unknown>", data_written);
#endif
    bool read_fail = false;
    if (!refl || refl->debugWatermark != DANET_WATERMARK ||
        !refl->deserialize(bs, int(data_written) - BITS_TO_BYTES(int(startPosAfterId - start_pos))))
      read_fail = true;
    else
    {
      bs.AlignReadToByteBoundary();
      uint32_t real_readed = bs.GetReadOffset() - start_pos;
      real_readed = BITS_TO_BYTES(real_readed);
      read_fail = real_readed != data_written;
      G_ASSERTF(real_readed == data_written,
        "%s integrity check for reflection packet failed, "
        "written (%d) != readed (%d) bytes, obj = %p (0x%x, %s)",
        __FUNCTION__, data_written, real_readed, refl, (int)oid, refl ? refl->getClassName() : "");
    }
    if (read_fail)
    {
      bs.SetReadOffset(start_pos);
      bs.IgnoreBytes(data_written);
    }
  }

  return numReflectables;
}

void ReplicatedObject::genReplicationEvent(BitStream &out_bs) const
{
  G_ASSERT(getClassId() <= 255);

#if DEBUG_BORDERS_REPLICATION
  out_bs.Write((uint8_t)113);
#endif

  out_bs.Write((uint8_t)getClassId());
  serializeReplicaCreationData(out_bs);

#if DEBUG_BORDERS_REPLICATION
  out_bs.Write((uint8_t)114);
#endif
}

/* static */
ReplicatedObject *ReplicatedObject::onRecvReplicationEvent(BitStream &bs)
{
#if DEBUG_BORDERS_REPLICATION
  uint8_t border = 0;
  bs.Read(border);
  G_ASSERT(border == 113);
#endif

  uint8_t classId = 255;
  if (!bs.Read(classId))
  {
    debug("%s can't read classId", __FUNCTION__);
    return NULL;
  }
  if (classId >= num_registered_obj_creators)
  {
    debug("%s invalid classId %d, num_registered_obj_creators = %d", __FUNCTION__, classId, num_registered_obj_creators);
    return NULL;
  }
  ReplicatedObject *ret = (*registered_repl_obj_creators[classId].create)(bs);

#if DEBUG_BORDERS_REPLICATION
  bs.Read(border);
  G_ASSERT(border == 114);
#endif

  return ret;
}

void ReplicatedObject::genReplicationEventForAll(BitStream &bs)
{
  WinAutoLock l(danet::reflection_lists_critical_section);
  BitSize_t pos = bs.GetWriteOffset();
  bs.Write((uint16_t)0);
  int numSerialized = 0;
  for (ReflectableObject *obj = all_reflectables.head; obj; obj = obj->next)
    if (obj->isRelfectionFlagSet(ReflectableObject::REPLICATED))
    {
      static_cast<ReplicatedObject *>(obj)->genReplicationEvent(bs);
      ++numSerialized;
    }
  G_ASSERT(numSerialized <= 65535);
  bs.WriteAt((uint16_t)numSerialized, pos);
}

bool ReplicatedObject::onRecvReplicationEventForAll(BitStream &bs)
{
  uint16_t n;
  if (!bs.Read(n))
    return false;
  for (int i = 0; i < n; ++i)
    if (!ReplicatedObject::onRecvReplicationEvent(bs))
      return false;
  return true;
}

int default_fvec_coder(DANET_ENCODER_SIGNATURE)
{
  (void)ro;
  uint32_t numBytes = BITS_TO_BYTES(meta->numBits);
  G_ASSERT(numBytes >= sizeof(Point2));
  G_ASSERT(numBytes <= sizeof(Point4));
  if (op == DANET_REFLECTION_OP_ENCODE)
  {
    Point4 &p4 = meta->getValue<Point4>();
    if (meta->flags & RVF_NORMALIZED)
    {
#if DAGOR_DBGLEVEL > 0
      if (numBytes == sizeof(Point2))
        G_ASSERT(length(meta->getValue<Point2>()) < 1.00001f);
      else if (numBytes == sizeof(Point3))
        G_ASSERT(length(meta->getValue<Point3>()) < 1.00001f);
      else if (numBytes == sizeof(Point4))
        G_ASSERT(length(meta->getValue<Point4>()) < 1.00001f);
#endif

      bs->Write((short)clamp(p4.x * 32767.f, -32767.f, 32767.f));
      bs->Write((short)clamp(p4.y * 32767.f, -32767.f, 32767.f));
      if (numBytes >= sizeof(Point3))
        bs->Write((short)clamp(p4.z * 32767.f, -32767.f, 32767.f));
      if (numBytes >= sizeof(Point4))
        bs->Write((short)clamp(p4.w * 32767.f, -32767.f, 32767.f));
    }
    else
      bs->WriteBits(&meta->getValue<uint8_t>(), meta->numBits);
  }
  else if (op == DANET_REFLECTION_OP_DECODE)
  {
    Point4 p4;

    if (meta->flags & RVF_NORMALIZED)
    {
      short x, y, z, w;
      if (!bs->Read(x) || !bs->Read(y))
        return 0;

      p4.x = float(x) / 32767.f;
      p4.y = float(y) / 32767.f;

      if (numBytes >= sizeof(Point3))
      {
        if (bs->Read(z))
          p4.z = float(z) / 32767.f;
        else
          return 0;

        if (numBytes >= sizeof(Point4))
        {
          if (bs->Read(w))
            p4.w = float(w) / 32767.f;
          else
            return 0;
          p4.normalize();
        }
        else
          reinterpret_cast<Point3 *>(&p4.x)->normalize();
      }
      else
        reinterpret_cast<Point2 *>(&p4.x)->normalize();
    }
    else
    {
      if (!bs->ReadBits((uint8_t *)&p4.x, meta->numBits))
        return 0;
    }
    if (!(meta->flags & RVF_NEED_DESERIALIZE))
      return 1;

    Point4 &val = meta->getValue<Point4>();
    if (memcmp(&val.x, &p4.x, numBytes) != 0)
    {
      if (meta->flags & (RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE))
        const_cast<ReflectableObject *>(ro)->onReflectionVarChanged(meta, &p4.x);
      memcpy(&val.x, &p4.x, numBytes);
    }
  }
  return 1;
}

int default_tm_coder(DANET_ENCODER_SIGNATURE)
{
  TMatrix &tm = meta->getValue<TMatrix>();
  if (op == DANET_REFLECTION_OP_ENCODE)
  {
    G_ASSERT(length(tm.getcol(0)) < 1.00001f);
    G_ASSERT(length(tm.getcol(1)) < 1.00001f);
    G_ASSERT(length(tm.getcol(2)) < 1.00001f);

    Quat q = normalize(Quat(tm));
    bs->Write((short)clamp(q.x * 32767.f, -32767.f, 32767.f));
    bs->Write((short)clamp(q.y * 32767.f, -32767.f, 32767.f));
    bs->Write((short)clamp(q.z * 32767.f, -32767.f, 32767.f));
    bs->Write((short)clamp(q.w * 32767.f, -32767.f, 32767.f));
    bs->Write(*(const Point3 *)tm.m[3]);
  }
  else if (op == DANET_REFLECTION_OP_DECODE)
  {
    short qs[4];
    if (!bs->ReadArray(qs, 4))
      return 0;
    if (meta->flags & RVF_NEED_DESERIALIZE)
    {
      Quat q;
      q.x = qs[0] / 32767.f;
      q.y = qs[1] / 32767.f;
      q.z = qs[2] / 32767.f;
      q.w = qs[3] / 32767.f;
      if (!(meta->flags & (RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE)))
      {
        tm = makeTM(normalize(q));
        return (int)bs->Read(*(Point3 *)tm.m[3]);
      }
      else
      {
        TMatrix ntm = makeTM(normalize(q));
        if (!bs->Read(*(Point3 *)ntm.m[3]))
          return 0;
        if (tm != ntm)
        {
          const_cast<ReflectableObject *>(ro)->onReflectionVarChanged(meta, &ntm);
          tm = ntm;
        }
      }
    }
    else
      bs->IgnoreBytes(sizeof(Point3));
  }
  return 1;
}

static void writeIntBits(danet::BitStream &bs, uint32_t val, uint32_t numBits) { bs.WriteBits((const uint8_t *)&val, numBits); }

static void writeNormFloat(danet::BitStream &bs, float normVal, uint32_t numBits)
{
  G_ASSERT(fabsf(normVal) < 1.000001f);
  writeIntBits(bs, (uint32_t)(float((1 << numBits) - 1) * normVal), numBits);
}

static bool readIntBits(danet::BitStream &bs, uint32_t &retVal, uint32_t numBits)
{
  retVal = 0;
  return bs.ReadBits((uint8_t *)&retVal, numBits);
}

static bool readNormFloat(danet::BitStream &bs, float &normVal, uint32_t numBits)
{
  uint32_t val;
  bool r = readIntBits(bs, val, numBits);
  if (!r)
    return true;
  normVal = float(val) / float((1 << numBits) - 1);
  return true;
}

int default_int_coder(DANET_ENCODER_SIGNATURE)
{
  uint32_t numBits = meta->numBits;
  if (numBits >= BYTES_TO_BITS(sizeof(int)))
    return readwrite_var_raw<int>(op, meta, ro, bs);
  if (op == DANET_REFLECTION_OP_ENCODE)
    writeIntBits(*bs, meta->getValue<uint32_t>(), numBits);
  else if (op == DANET_REFLECTION_OP_DECODE)
  {
    if (!(meta->flags & RVF_NEED_DESERIALIZE))
      bs->IgnoreBits(numBits);
    else
    {
      int dummy;
      if (!readIntBits(*bs, (uint32_t &)dummy, numBits))
        return 0;
      int hiBit = 1 << (meta->numBits - 1);
      if (!(meta->flags & RVF_UNSIGNED) && (dummy & hiBit)) // extend sign if need to
        dummy = dummy - (1 << meta->numBits);
      int &val = meta->getValue<int>();
      if (val != dummy)
      {
        if (meta->flags & (RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE))
          const_cast<ReflectableObject *>(ro)->onReflectionVarChanged(meta, &dummy);
        val = dummy;
      }
    }
  }
  return 1;
}

static void writeFloatMinMax(danet::BitStream &bs, float val, float min_, float max_, uint32_t numBits)
{
  writeNormFloat(bs, safediv(clamp(val, min_, max_) - min_, max_ - min_), numBits);
}

static bool readFloatMinMax(danet::BitStream &bs, float &val, float min_, float max_, uint32_t numBits)
{
  float normVal = 0.f;
  if (!readNormFloat(bs, normVal, numBits))
    return false;
  val = min_ + normVal * (max_ - min_);
  return true;
}

static int default_float_min_max_coder(DANET_ENCODER_SIGNATURE)
{
  G_ASSERT(meta->flags & RVF_MIN_MAX_SPECIFIED);
  uint32_t numBits = (uint32_t)meta->numBits;
  G_ASSERT(numBits && numBits < BYTES_TO_BITS(sizeof(float)));
  float *fa = &meta->getValue<float>(); // [value, min, max]
  if (op == DANET_REFLECTION_OP_ENCODE)
    writeFloatMinMax(*bs, fa[0], fa[1], fa[2], numBits);
  else if (op == DANET_REFLECTION_OP_DECODE)
  {
    if (!(meta->flags & RVF_NEED_DESERIALIZE))
      bs->IgnoreBits(numBits);
    else if (!(meta->flags & (RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE)))
      return (int)readFloatMinMax(*bs, fa[0], fa[1], fa[2], numBits);
    else
    {
      float dummy;
      if (!readFloatMinMax(*bs, dummy, fa[1], fa[2], numBits))
        return 0;
      if (!is_equal_float(dummy, fa[0]))
      {
        if (meta->flags & (RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE))
          const_cast<ReflectableObject *>(ro)->onReflectionVarChanged(meta, &dummy);
        fa[0] = dummy;
      }
    }
  }
  return 1;
}

int default_float_coder(DANET_ENCODER_SIGNATURE)
{
  uint16_t f = meta->flags;
  uint32_t numBits = meta->numBits ? meta->numBits : BYTES_TO_BITS(sizeof(short));

  if (numBits >= BYTES_TO_BITS(sizeof(float)))
    return readwrite_var_raw<float>(op, meta, ro, bs);

  if (f & RVF_MIN_MAX_SPECIFIED)
    return default_float_min_max_coder(op, meta, ro, bs);

  if (!(f & RVF_NORMALIZED))
    return readwrite_var_raw<float>(op, meta, ro, bs);

  float &fval = meta->getValue<float>();

  if (op == DANET_REFLECTION_OP_ENCODE)
  {
    G_ASSERT(fabsf(fval) < 1.000001f);
    float nval;
    if (f & RVF_UNSIGNED) // 0 .. 1
    {
      G_ASSERT(fval >= 0.f);
      nval = fval;
    }
    else // -1 .. 1
      nval = (fval + 1.f) * 0.5f;
    writeNormFloat(*bs, nval, numBits);
  }
  else if (op == DANET_REFLECTION_OP_DECODE)
  {
    if (!(meta->flags & RVF_NEED_DESERIALIZE))
      bs->IgnoreBits(numBits);
    else
    {
      float normVal = 0.f;
      if (!readNormFloat(*bs, normVal, numBits))
        return 0;
      if (meta->flags & (RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE))
        fval = (f & RVF_UNSIGNED) ? normVal : (normVal * 2.f - 1.f);
      else
      {
        float new_val = (f & RVF_UNSIGNED) ? normVal : (normVal * 2.f - 1.f);
        if (!is_equal_float(new_val, fval))
        {
          if (meta->flags & (RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE))
            const_cast<ReflectableObject *>(ro)->onReflectionVarChanged(meta, &new_val);
          fval = new_val;
        }
      }
    }
  }
  return 1;
}
}; // namespace danet
