// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/aotBitStream.h"


struct BitStreamAnnotation final : das::ManagedStructureAnnotation<danet::BitStream, false>
{
  BitStreamAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("BitStream", ml) { cppName = " ::danet::BitStream"; }
};

namespace bind_dascript
{

class BitStreamModule final : public das::Module
{
public:
  BitStreamModule() : das::Module("BitStream")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("DagorMath"));

    addAnnotation(das::make_smart<BitStreamAnnotation>(lib));

    das::addUsing<danet::BitStream>(*this, lib, "danet::BitStream");

#define ADD_EXTERN_BITSTREAM_READ(type)                                                 \
  das::addExtern<DAS_BIND_FUN(bind_dascript::bitstream_read<type>)>(*this, lib, "Read", \
    das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::bitstream_read<" #type ">");

#define ADD_EXTERN_BITSTREAM_WRITE(type)                                                       \
  das::addExtern<DAS_BIND_FUN(bind_dascript::bitstream_write<type>)>(*this, lib, "Write",      \
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::bitstream_write<" #type ">"); \
  das::addExtern<DAS_BIND_FUN(bind_dascript::bitstream_write_at<type>)>(*this, lib, "WriteAt", \
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::bitstream_write_at<" #type ">");

#define ADD_EXTERN_BITSTREAM_WRITE_REF(type)                                                       \
  das::addExtern<DAS_BIND_FUN(bind_dascript::bitstream_write_ref<type>)>(*this, lib, "Write",      \
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::bitstream_write_ref<" #type ">"); \
  das::addExtern<DAS_BIND_FUN(bind_dascript::bitstream_write_at_ref<type>)>(*this, lib, "WriteAt", \
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::bitstream_write_at_ref<" #type ">");

#define ADD_EXTERN_BITSTREAM_READ_WRITE(type) \
  ADD_EXTERN_BITSTREAM_READ(type);            \
  ADD_EXTERN_BITSTREAM_WRITE(type)
#define ADD_EXTERN_BITSTREAM_READ_WRITE_REF(type) \
  ADD_EXTERN_BITSTREAM_READ(type);                \
  ADD_EXTERN_BITSTREAM_WRITE_REF(type)

    ADD_EXTERN_BITSTREAM_READ_WRITE(int8_t);
    ADD_EXTERN_BITSTREAM_READ_WRITE(uint8_t);
    ADD_EXTERN_BITSTREAM_READ_WRITE(int16_t);
    ADD_EXTERN_BITSTREAM_READ_WRITE(uint16_t);
    ADD_EXTERN_BITSTREAM_READ_WRITE(int32_t);
    ADD_EXTERN_BITSTREAM_READ_WRITE(uint32_t);
    ADD_EXTERN_BITSTREAM_READ_WRITE(float);
    ADD_EXTERN_BITSTREAM_READ_WRITE(bool);
    ADD_EXTERN_BITSTREAM_READ_WRITE_REF(Point2);
    ADD_EXTERN_BITSTREAM_READ_WRITE_REF(Point3);
    ADD_EXTERN_BITSTREAM_READ_WRITE_REF(Point4);
    ADD_EXTERN_BITSTREAM_READ_WRITE_REF(danet::BitStream);


#define ADD_EXTERN_BITSTREAM_READ_WRITE_COMPRESSED(type)                                                       \
  das::addExtern<DAS_BIND_FUN(bind_dascript::bitstream_read_compressed<type>)>(*this, lib, "ReadCompressed",   \
    das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::bitstream_read_compressed<" #type ">"); \
  das::addExtern<DAS_BIND_FUN(bind_dascript::bitstream_write_compressed<type>)>(*this, lib, "WriteCompressed", \
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::bitstream_write_compressed<" #type ">");

    ADD_EXTERN_BITSTREAM_READ_WRITE_COMPRESSED(int16_t);
    ADD_EXTERN_BITSTREAM_READ_WRITE_COMPRESSED(uint16_t);
    ADD_EXTERN_BITSTREAM_READ_WRITE_COMPRESSED(int32_t);
    ADD_EXTERN_BITSTREAM_READ_WRITE_COMPRESSED(uint32_t);

    das::addExtern<DAS_BIND_FUN(bind_dascript::bitstream_read_eid)>(*this, lib, "Read",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::bitstream_read_eid");
    das::addExtern<DAS_BIND_FUN(bind_dascript::bitstream_write_eid)>(*this, lib, "Write", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::bitstream_write_eid");

    das::addInterop<bitstream_read_write_any<BitStreamWalkerMode::Read>, bool, const danet::BitStream &, vec4f>(*this, lib, "Read",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::bitstream_read_write_any<BitStreamWalkerMode::Read>");
    das::addInterop<bitstream_read_write_any<BitStreamWalkerMode::Write>, bool, danet::BitStream &, const vec4f>(*this, lib, "Write",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::bitstream_read_write_any<BitStreamWalkerMode::Write>");

    das::addExtern<DAS_BIND_FUN(bind_dascript::bitstream_addr)>(*this, lib, "ecs_addr", das::SideEffects::none,
      "bind_dascript::bitstream_addr");

    das::addExtern<DAS_BIND_FUN(bind_dascript::bitstream_data_ctor)>(*this, lib, "using", das::SideEffects::invoke,
      "bind_dascript::bitstream_data_ctor");

    das::addExtern<DAS_BIND_FUN(bind_dascript::bitstream_data)>(*this, lib, "GetData", das::SideEffects::invoke,
      "bind_dascript::bitstream_data");


#define DAS_BIND_MEMBER(fn, side_effect, name)                                                       \
  {                                                                                                  \
    using method = DAS_CALL_MEMBER(fn);                                                              \
    das::addExtern<DAS_CALL_METHOD(method)>(*this, lib, name, side_effect, DAS_CALL_MEMBER_CPP(fn)); \
  }
    DAS_BIND_MEMBER(danet::BitStream::GetNumberOfBitsUsed, das::SideEffects::none, "GetNumberOfBitsUsed")
    DAS_BIND_MEMBER(danet::BitStream::GetWriteOffset, das::SideEffects::none, "GetWriteOffset")
    DAS_BIND_MEMBER(danet::BitStream::GetNumberOfBytesUsed, das::SideEffects::none, "GetNumberOfBytesUsed")
    DAS_BIND_MEMBER(danet::BitStream::GetReadOffset, das::SideEffects::none, "GetReadOffset")
    DAS_BIND_MEMBER(danet::BitStream::SetReadOffset, das::SideEffects::worstDefault, "SetReadOffset") // const operator!
    DAS_BIND_MEMBER(danet::BitStream::GetNumberOfUnreadBits, das::SideEffects::none, "GetNumberOfUnreadBits")
    DAS_BIND_MEMBER(danet::BitStream::GetNumberOfAllocatedBits, das::SideEffects::none, "GetNumberOfAllocatedBits")
    DAS_BIND_MEMBER(danet::BitStream::AlignWriteToByteBoundary, das::SideEffects::modifyArgument, "AlignWriteToByteBoundary")
    DAS_BIND_MEMBER(danet::BitStream::AlignReadToByteBoundary, das::SideEffects::worstDefault, "AlignReadToByteBoundary") // const
                                                                                                                          // operator!
    DAS_BIND_MEMBER(danet::BitStream::ResetWritePointer, das::SideEffects::modifyArgument, "ResetWritePointer")
    DAS_BIND_MEMBER(danet::BitStream::ResetReadPointer, das::SideEffects::worstDefault, "ResetReadPointer") // const operator!

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/aotBitStream.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(BitStreamModule, bind_dascript);
