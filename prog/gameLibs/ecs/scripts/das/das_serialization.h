// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <daScript/ast/ast_serializer.h>

namespace bind_dascript
{
extern bool debugSerialization;
extern bool enableSerialization;
extern bool serializationReading;
extern bool suppressSerialization;
extern eastl::string deserializationFileName;
extern eastl::string serializationFileName;
extern das::unique_ptr<das::SerializationStorage> initSerializerStorage;
extern das::unique_ptr<das::SerializationStorage> initDeserializerStorage;
extern das::unique_ptr<das::AstSerializer> initSerializer;
extern das::unique_ptr<das::AstSerializer> initDeserializer;
} // namespace bind_dascript