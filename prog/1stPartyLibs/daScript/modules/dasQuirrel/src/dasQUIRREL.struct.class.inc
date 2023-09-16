// this file is generated via daScript automatic C++ binder
// all user modifications will be lost after this file is re-generated

// from D:\Work\daScript/Modules/dasQuirrel/src/cb_dasQuirrel.h:3:8
struct SQWeakRef_GeneratedAnnotation : ManagedStructureAnnotation<SQWeakRef> {
	SQWeakRef_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("SQWeakRef", ml, "SQWeakRef") {
	}
	void init () {
	}
};

// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:140:15
struct tagSQObjectValue_GeneratedAnnotation : ManagedStructureAnnotation<tagSQObjectValue> {
	tagSQObjectValue_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("tagSQObjectValue", ml, "tagSQObjectValue") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(pTable)>("pTable","pTable");
		addField<DAS_BIND_MANAGED_FIELD(pArray)>("pArray","pArray");
		addField<DAS_BIND_MANAGED_FIELD(pClosure)>("pClosure","pClosure");
		addField<DAS_BIND_MANAGED_FIELD(pOuter)>("pOuter","pOuter");
		addField<DAS_BIND_MANAGED_FIELD(pGenerator)>("pGenerator","pGenerator");
		addField<DAS_BIND_MANAGED_FIELD(pNativeClosure)>("pNativeClosure","pNativeClosure");
		addField<DAS_BIND_MANAGED_FIELD(pString)>("pString","pString");
		addField<DAS_BIND_MANAGED_FIELD(pUserData)>("pUserData","pUserData");
		addField<DAS_BIND_MANAGED_FIELD(nInteger)>("nInteger","nInteger");
		addField<DAS_BIND_MANAGED_FIELD(fFloat)>("fFloat","fFloat");
		addField<DAS_BIND_MANAGED_FIELD(pUserPointer)>("pUserPointer","pUserPointer");
		addField<DAS_BIND_MANAGED_FIELD(pFunctionProto)>("pFunctionProto","pFunctionProto");
		addField<DAS_BIND_MANAGED_FIELD(pRefCounted)>("pRefCounted","pRefCounted");
		addField<DAS_BIND_MANAGED_FIELD(pDelegable)>("pDelegable","pDelegable");
		addField<DAS_BIND_MANAGED_FIELD(pThread)>("pThread","pThread");
		addField<DAS_BIND_MANAGED_FIELD(pClass)>("pClass","pClass");
		addField<DAS_BIND_MANAGED_FIELD(pInstance)>("pInstance","pInstance");
		addField<DAS_BIND_MANAGED_FIELD(pWeakRef)>("pWeakRef","pWeakRef");
		addField<DAS_BIND_MANAGED_FIELD(raw)>("raw","raw");
	}
};

// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:164:16
struct tagSQObject_GeneratedAnnotation : ManagedStructureAnnotation<tagSQObject> {
	tagSQObject_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("tagSQObject", ml, "tagSQObject") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(_type)>("_type","_type");
		addField<DAS_BIND_MANAGED_FIELD(_flags)>("_flags","_flags");
		addField<DAS_BIND_MANAGED_FIELD(_unVal)>("_unVal","_unVal");
	}
};

// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:171:17
struct tagSQMemberHandle_GeneratedAnnotation : ManagedStructureAnnotation<tagSQMemberHandle> {
	tagSQMemberHandle_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("tagSQMemberHandle", ml, "tagSQMemberHandle") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(_static)>("_static","_static");
		addField<DAS_BIND_MANAGED_FIELD(_index)>("_index","_index");
	}
};

// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:176:16
struct tagSQStackInfos_GeneratedAnnotation : ManagedStructureAnnotation<tagSQStackInfos> {
	tagSQStackInfos_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("tagSQStackInfos", ml, "tagSQStackInfos") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(funcname)>("funcname","funcname");
		addField<DAS_BIND_MANAGED_FIELD(source)>("source","source");
		addField<DAS_BIND_MANAGED_FIELD(line)>("line","line");
	}
};

// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:195:16
struct tagSQRegFunction_GeneratedAnnotation : ManagedStructureAnnotation<tagSQRegFunction> {
	tagSQRegFunction_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("tagSQRegFunction", ml, "tagSQRegFunction") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
		addField<DAS_BIND_MANAGED_FIELD(nparamscheck)>("nparamscheck","nparamscheck");
		addField<DAS_BIND_MANAGED_FIELD(typemask)>("typemask","typemask");
	}
};

// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\squirrel.h:202:16
struct tagSQFunctionInfo_GeneratedAnnotation : ManagedStructureAnnotation<tagSQFunctionInfo> {
	tagSQFunctionInfo_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("tagSQFunctionInfo", ml, "tagSQFunctionInfo") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(funcid)>("funcid","funcid");
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
		addField<DAS_BIND_MANAGED_FIELD(source)>("source","source");
		addField<DAS_BIND_MANAGED_FIELD(line)>("line","line");
	}
};

// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdio.h:9:8
struct SQStream_GeneratedAnnotation : ManagedStructureAnnotation<SQStream> {
	SQStream_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("SQStream", ml, "SQStream") {
	}
	void init () {
	}
};

// from D:\Work\daScript\Modules\dasQuirrel\libquirrel\include\sqstdstring.h:13:9
struct SQRexMatch_GeneratedAnnotation : ManagedStructureAnnotation<SQRexMatch> {
	SQRexMatch_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("SQRexMatch", ml, "SQRexMatch") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(begin)>("begin","begin");
		addField<DAS_BIND_MANAGED_FIELD(len)>("len","len");
	}
};

