// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/stcode/callbackTable.h>

#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_sampler.h>

#include "../shStateBlock.h"

struct RaytraceTopAccelerationStructure;

namespace stcode::dbg
{

enum class RecordType
{
  REFERENCE,
  CPP
};

#if CPP_STCODE && VALIDATE_CPP_STCODE && DAGOR_DBGLEVEL > 0

void reset();

void record_set_tex(RecordType type, int stage, cpp::uint reg, BaseTexture *tex);
void record_set_buf(RecordType type, int stage, cpp::uint reg, Sbuffer *buf);
void record_set_const_buf(RecordType type, int stage, cpp::uint reg, Sbuffer *buf);
void record_set_tlas(RecordType type, int stage, cpp::uint reg, RaytraceTopAccelerationStructure *tlas);
void record_set_sampler(RecordType type, int stage, cpp::uint reg, d3d::SamplerHandle handle);
void record_set_rwtex(RecordType type, int stage, cpp::uint reg, BaseTexture *tex);
void record_set_rwbuf(RecordType type, int stage, cpp::uint reg, Sbuffer *buf);
void record_request_sampler(RecordType type, const d3d::SamplerInfo &base_info);
void record_set_const(RecordType type, int stage, unsigned int id, const void *val, int cnt);
void record_reg_bindless(RecordType type, unsigned int id, TEXTUREID tid);
void record_set_static_tex(RecordType type, int stage, cpp::uint reg, TEXTUREID tid);
void record_multidraw_support(RecordType type, bool is_enabled);

void require_exact_record_matches();
void validate_accumulated_records(int cpp_routine_id, int ref_routine_id, const char *shname, bool dynamic);

#else

inline void reset() {}

inline void record_set_tex(RecordType, int, cpp::uint, BaseTexture *) {}
inline void record_set_buf(RecordType, int, cpp::uint, Sbuffer *) {}
inline void record_set_const_buf(RecordType, int, cpp::uint, Sbuffer *) {}
inline void record_set_tlas(RecordType, int, cpp::uint, RaytraceTopAccelerationStructure *) {}
inline void record_set_sampler(RecordType, int, cpp::uint, d3d::SamplerHandle) {}
inline void record_set_rwtex(RecordType, int, cpp::uint, BaseTexture *) {}
inline void record_set_rwbuf(RecordType, int, cpp::uint, Sbuffer *) {}
inline void record_request_sampler(RecordType, const d3d::SamplerInfo &) {}
inline void record_set_const(RecordType, int, unsigned int, const void *, int) {}
inline void record_reg_bindless(RecordType, unsigned int, TEXTUREID) {}
inline void record_set_static_tex(RecordType, int, cpp::uint, TEXTUREID) {}
inline void record_multidraw_support(RecordType, bool) {}

inline void require_exact_record_matches() {}
inline void validate_accumulated_records(int, int, const char *, bool) {}

#endif

} // namespace stcode::dbg
