// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:114:14
class Enumeration_tagSQObjectType : public das::Enumeration{
public:
	Enumeration_tagSQObjectType() : das::Enumeration("tagSQObjectType") {
		external = true;
		cppName = "tagSQObjectType";
		baseType = (das::Type) das::ToBasicType<int>::type;
		addIEx("OT_NULL", "OT_NULL", int64_t(tagSQObjectType::OT_NULL), das::LineInfo());
		addIEx("OT_INTEGER", "OT_INTEGER", int64_t(tagSQObjectType::OT_INTEGER), das::LineInfo());
		addIEx("OT_FLOAT", "OT_FLOAT", int64_t(tagSQObjectType::OT_FLOAT), das::LineInfo());
		addIEx("OT_BOOL", "OT_BOOL", int64_t(tagSQObjectType::OT_BOOL), das::LineInfo());
		addIEx("OT_STRING", "OT_STRING", int64_t(tagSQObjectType::OT_STRING), das::LineInfo());
		addIEx("OT_TABLE", "OT_TABLE", int64_t(tagSQObjectType::OT_TABLE), das::LineInfo());
		addIEx("OT_ARRAY", "OT_ARRAY", int64_t(tagSQObjectType::OT_ARRAY), das::LineInfo());
		addIEx("OT_USERDATA", "OT_USERDATA", int64_t(tagSQObjectType::OT_USERDATA), das::LineInfo());
		addIEx("OT_CLOSURE", "OT_CLOSURE", int64_t(tagSQObjectType::OT_CLOSURE), das::LineInfo());
		addIEx("OT_NATIVECLOSURE", "OT_NATIVECLOSURE", int64_t(tagSQObjectType::OT_NATIVECLOSURE), das::LineInfo());
		addIEx("OT_GENERATOR", "OT_GENERATOR", int64_t(tagSQObjectType::OT_GENERATOR), das::LineInfo());
		addIEx("OT_USERPOINTER", "OT_USERPOINTER", int64_t(tagSQObjectType::OT_USERPOINTER), das::LineInfo());
		addIEx("OT_THREAD", "OT_THREAD", int64_t(tagSQObjectType::OT_THREAD), das::LineInfo());
		addIEx("OT_FUNCPROTO", "OT_FUNCPROTO", int64_t(tagSQObjectType::OT_FUNCPROTO), das::LineInfo());
		addIEx("OT_CLASS", "OT_CLASS", int64_t(tagSQObjectType::OT_CLASS), das::LineInfo());
		addIEx("OT_INSTANCE", "OT_INSTANCE", int64_t(tagSQObjectType::OT_INSTANCE), das::LineInfo());
		addIEx("OT_WEAKREF", "OT_WEAKREF", int64_t(tagSQObjectType::OT_WEAKREF), das::LineInfo());
		addIEx("OT_OUTER", "OT_OUTER", int64_t(tagSQObjectType::OT_OUTER), das::LineInfo());
	}
};

