#pragma once

#include <cstdint>

#include "daScript/misc/gc_node.h"
#include "daScript/misc/platform.h"

namespace das
{

#ifndef DAS_ENABLE_PROFILER
#define DAS_ENABLE_PROFILER 0
#endif

#ifndef DAS_ENABLE_KEEPALIVE
#define DAS_ENABLE_KEEPALIVE 1
#endif

    enum Type : int32_t {
        none,
        autoinfer,
        alias,
        option,
        typeDecl,
        typeMacro,
        fakeContext,
        fakeLineInfo,
        anyArgument,
        tVoid,
        tBool,
        tInt8,
        tUInt8,
        tInt16,
        tUInt16,
        tInt64,
        tUInt64,
        tInt,
        tInt2,
        tInt3,
        tInt4,
        tUInt,
        tUInt2,
        tUInt3,
        tUInt4,
        tFloat,
        tFloat2,
        tFloat3,
        tFloat4,
        tDouble,
        tRange,
        tURange,
        tRange64,
        tURange64,
        tString,
        tStructure,
        tHandle,
        tEnumeration,
        tEnumeration8,
        tEnumeration16,
        tEnumeration64,
        tBitfield,
        tBitfield8,
        tBitfield16,
        tBitfield64,
        tPointer,
        tFunction,
        tLambda,
        tIterator,
        tArray,
        tTable,
        tBlock,
        tTuple,
        tVariant,
        tFixedArray,    // AST-only (FIXED_ARRAY_REWORK.md): structural fixed-array TypeDecl node.
                        //  Runtime TypeInfo never carries it — fixed arrays stay flattened to dim[]
                        //  at the single AST->TypeInfo conversion point (ast_debug_info_helper.cpp).
        tDistinct,      // AST-only: nominal `distinct Foo = int` type. ABI-identical to firstType
                        //  (the underlying workhorse type); annotation points at DistinctTypeAnnotation.
                        //  Erased to the underlying type at the same AST->TypeInfo conversion point.
        // ===== 16/8-bit type lattice — appended (NEVER insert above: das_base_type in
        // daScriptC.h pins the numeric values of everything before this point). Vectors are
        // byte-packed in the low bytes of the vec4f slot; scalars ride widened in lane 0.
        tFloat16,       // IEEE-754 binary16 scalar (das name "float16"; "half" is a builtin alias)
        tHalf2,         // 16-bit fp vectors: {2,3,4,8} lanes, 8-lane form fills the 128-bit slot
        tHalf3,
        tHalf4,
        tHalf8,
        tShort2,        // int16 vectors (storage + convert only, no closed arithmetic)
        tShort3,
        tShort4,
        tShort8,
        tUShort2,       // uint16 vectors
        tUShort3,
        tUShort4,
        tUShort8,
        tByte2,         // int8 vectors — byte = SIGNED int8 (house naming: bare=signed)
        tByte3,
        tByte4,
        tByte8,
        tByte16,
        tUByte2,        // uint8 vectors
        tUByte3,
        tUByte4,
        tUByte8,
        tUByte16
        // (reserved next: bfloat16 family — bfloat, bfloat2/3/4/8 — decided OUT for now)
    };

    enum class RefMatters {
        no
    ,   yes
    };

    enum class ConstMatters {
        no
    ,   yes
    };

    enum class TemporaryMatters {
        no
    ,   yes
    };

    enum class AllowSubstitute   {
        no,
        yes
    };

    template <typename T>
    struct isCloneable  {
        template<typename U>
        static decltype(declval<U&>() = declval<const U&>(), U (declval<const U&>()), true_type{}) func (das::remove_reference<U>*);
        template<typename U>
        static false_type func (...);
        using  type = decltype(func<T>(nullptr));
        static constexpr bool value { type::value };
    };

    struct StructInfo;
    struct Annotation;
    struct TypeAnnotation;
    struct EnumInfo;

    // POD mirrors of AST annotation data, deep-copied into the DebugInfoAllocator so that
    // debug info never outlives its strings (a Context can outlive its Program).
    struct AnnotationArgumentInfo {
        Type            type;       // only tBool, tInt, tFloat, tString
        const char *    name;
        const char *    sValue;
        union {
            bool        bValue;
            int32_t     iValue;
            float       fValue;
        };
        AnnotationArgumentInfo() = default;
        AnnotationArgumentInfo ( const char * n, bool b )
            : type(Type::tBool), name(n), sValue(nullptr), bValue(b) {}
        AnnotationArgumentInfo ( const char * n, int32_t i )
            : type(Type::tInt), name(n), sValue(nullptr), iValue(i) {}
        AnnotationArgumentInfo ( const char * n, float f )
            : type(Type::tFloat), name(n), sValue(nullptr), fValue(f) {}
        AnnotationArgumentInfo ( const char * n, const char * s )
            : type(Type::tString), name(n), sValue(s), iValue(0) {}
    };

    struct AnnotationInfo {
        const char *                name;           // annotation name
        const char *                module_name;    // module where the annotation is declared
        AnnotationArgumentInfo *    arguments;      // flat array
        uint32_t                    count;
        mutable Annotation *        resolved;       // lazy environment-lookup cache. WARNING: use Module::resolveAnnotation
        AnnotationInfo() = default;
        AnnotationInfo ( const char * _name, const char * _module_name,
                AnnotationArgumentInfo * _arguments, uint32_t _count )
            : name(_name), module_name(_module_name)
            , arguments(_arguments), count(_count), resolved(nullptr) {}
    };

    struct BasicAnnotation : gc_node {
        BasicAnnotation ( const string & n, const string & cpn = "" ) : name(n), cppName(cpn) {}
        virtual ~BasicAnnotation() {}
        string      name;
        string      cppName;
    };

    struct DAS_API FileInfo {
    public:
        virtual void freeSourceData() { lineOffsets.clear(); lineIndexBuilt = false; }
        virtual ~FileInfo() { freeSourceData(); }
        void reserveProfileData();
        virtual void getSourceAndLength ( const char * & src, uint32_t & len ) { src=nullptr; len=0; }
        virtual void serialize ( AstSerializer & ser );
        // Lazy byte-offset index of line starts. lineOffsets[i] = start of line i+1.
        // Built on first getLine call via getSourceAndLength. O(N) one-time, O(1) per query.
        void buildLineIndex();
        bool getLine ( uint32_t line, const char * & begin, uint32_t & len );
        string                name;
        int32_t               tabSize = 4;
#if DAS_ENABLE_PROFILER
    public:
        vector<uint64_t>      profileData;
#endif
    protected:
        vector<uint32_t>      lineOffsets;
        bool                  lineIndexBuilt = false;
    };
    typedef unique_ptr<FileInfo> FileInfoPtr;

    class DAS_API TextFileInfo : public FileInfo {
    public:
        TextFileInfo ( ) = default;
        TextFileInfo ( const char * src, uint32_t len, bool own )
            : source(src), sourceLength(len), owner(own) {}
        virtual ~TextFileInfo() { if ( owner ) freeSourceData(); }
        virtual void freeSourceData() override;
        virtual void getSourceAndLength ( const char * & src, uint32_t & len ) override;
        virtual void serialize ( AstSerializer & ser ) override;
    protected:
        const char *          source = nullptr;
        uint32_t              sourceLength = 0;
        bool                  owner = true;
    };

    struct ModuleInfo {
        string  moduleName;
        string  fileName;
        string  importName;
        bool extraDepModule = false;
        string  requireName;    // the require string that produced this info (e.g. "daslib/fio"); carried so a promoted shared module records its canonical, directory-independent require identity
    };

    struct BaseRequireRecord {
        string              name;
        int32_t             line;
        vector<FileInfo *>  chain;
    };

    struct RequireRecord : BaseRequireRecord {
        bool                isPublic;
    };

    enum class MissingHint {
        ModuleInfoNotFound,
        WrongModuleName,
        FileNotFound,
        DuplicateModule,
    };

    struct MissingRecord : BaseRequireRecord {
        MissingHint         hintType;
        string              hintName;
        string              hintName2;
    };

    struct NamelessModuleReq {
        string              name;
        string              moduleName;
        string              fileName;
        string              fromFile;
    };

    struct NamelessMismatch {
        vector<FileInfo *>  chain;
        int32_t             line;
        string              name1;
        string              fileName1;
        string              fromFile1;
        string              name2;
        string              fileName2;
        string              fromFile2;
    };

    typedef smart_ptr<class FileAccess> FileAccessPtr;
    class DAS_API FileAccess : public ptr_ref_count {
    public:
        FileAccess() { ref_count_magic = TRACK_PTR_FILE_ACCESS; }
        virtual ~FileAccess() {}

        FileAccess& operator=(const FileAccess&) = delete;
        FileAccess(const FileAccess&) = delete;

        void reset() { files.clear(); }
        FileInfo * setFileInfo ( const string & fileName, FileInfoPtr && info );
        FileInfo * getFileInfo ( const string & fileName );
        virtual bool invalidateFileInfo ( const string & fileName );
        virtual string getIncludeFileName ( const string & fileName, const string & incFileName ) const;
        void freeSourceData();
        virtual int64_t getFileMtime ( const string & fileName ) const;
        FileInfoPtr letGoOfFileInfo ( const string & fileName );
        virtual ModuleInfo getModuleInfo ( const string & req, const string & from ) const;
        virtual string getDynModulesFolder () const { return ""; }
        virtual bool isPodInScopeAllowed ( const string & /*moduleName*/, const string & /*fileName*/ ) const { return true; };
        virtual bool isModuleAllowed ( const string &, const string & ) const { return true; };
        virtual bool canModuleBeUnsafe ( const string &, const string & ) const { return true; };
        virtual bool isWithModuleUnsafe ( const string & /*targetModule*/, const string & /*fileName*/ ) const { return false; };
        virtual bool canBeRequired ( const string &, const string &, bool ) const { return true; };
        virtual bool addFsRoot ( const string & , const string & ) { return false; }
        virtual void serialize ( AstSerializer & ser );
        virtual bool isSameFileName ( const string & f1, const string & f2 ) const;
        virtual bool isOptionAllowed ( const string & /*opt*/, const string & /*from*/ ) const { return true; }
        virtual bool isOptionBlocked ( const string & /*opt*/, const string & /*from*/ ) const { return false; }
        virtual bool isAnnotationAllowed ( const string & /*ann*/, const string & /*from*/ ) const { return true; }
        // must stop at word boundary
        virtual bool parseCustomRequire(const char *& /*src*/, const char * /*srcEnd*/,
                                        FileInfo *& /*info*/,
                                        const FileAccessPtr & /*access*/,
                                        vector<RequireRecord> & /*req*/,
                                        vector<FileInfo *> & /*chain*/ ) const {
            return false;
        }

        void addExtraModule ( const string & modName, const string & modFile ) { extraModules.emplace_back(modName, modFile); }
        const vector<pair<string,string>> & getExtraModules () const { return extraModules; }

        void addAutoRequiredModule ( const string & modName ) { autoRequiredModules.insert(modName); }
        const das_set<string> & getAutoRequiredModules () const { return autoRequiredModules; }

        void lock() { locked = true; }
        void unlock() { locked = false; }
        bool isLocked() const { return locked; }
    protected:
        virtual FileInfo * getNewFileInfo ( const string & ) { return nullptr; }
    protected:
        das_hash_map<string, FileInfoPtr>    files;
        vector<pair<string,string>>          extraModules;
        das_set<string>                      autoRequiredModules;
        bool    locked = false;
    };
    template <> struct isCloneable<FileAccess> : false_type {};

    struct SimFunction;
    class Context;
    class Program;

    class DAS_API ModuleFileAccess : public FileAccess {
    public:
        ModuleFileAccess();
        ModuleFileAccess ( const string & pak, const smart_ptr<Program> & program );
        virtual ~ModuleFileAccess();
        bool failed() const { return !context || !modGet; }
        virtual ModuleInfo getModuleInfo ( const string & req, const string & from ) const override;
        virtual string getDynModulesFolder () const override;
        virtual string getIncludeFileName ( const string & fileName, const string & incFileName ) const override;
        virtual bool isModuleAllowed ( const string &, const string & ) const override;
        virtual bool canModuleBeUnsafe ( const string &, const string & ) const override;
        virtual bool isWithModuleUnsafe ( const string &, const string & ) const override;
        virtual bool canBeRequired ( const string &, const string &, bool ) const override;
        virtual void serialize ( AstSerializer & ser ) override;
        virtual bool isSameFileName ( const string & f1, const string & f2 ) const override;
        virtual bool isOptionAllowed ( const string & opt, const string & from ) const override;
        virtual bool isOptionBlocked ( const string & opt, const string & from ) const override;
        virtual bool isAnnotationAllowed ( const string & /*ann*/, const string & /*from*/ ) const override;
        virtual bool isPodInScopeAllowed ( const string & /*moduleName*/, const string & /*fileName*/ ) const override;
    protected:
        Context *           context = nullptr;
        SimFunction *       modGet = nullptr;
        SimFunction *       includeGet = nullptr;
        SimFunction *       moduleAllowed = nullptr;
        SimFunction *       moduleUnsafe = nullptr;
        SimFunction *       withModuleUnsafe = nullptr;
        SimFunction *       canModuleBeRequired = nullptr;
        SimFunction *       sameFileName = nullptr;
        SimFunction *       optionAllowed = nullptr;
        SimFunction *       optionBlocked = nullptr;
        SimFunction *       annotationAllowed = nullptr;
        SimFunction *       podInScopeAllowed = nullptr;
        SimFunction *       dynModulesFolderGet = nullptr;
    };
    template <> struct isCloneable<ModuleFileAccess> : false_type {};

    struct DAS_API LineInfo {
        LineInfo() = default;
        LineInfo(FileInfo * fi, int c, int l, int lc, int ll)
            : fileInfo(fi)
            , column(uint32_t(c)), line(uint32_t(l))
            , last_column(uint32_t(lc)), last_line(uint32_t(ll)) {}
        bool operator < ( const LineInfo & info ) const;
        bool operator == ( const LineInfo & info ) const;
        bool operator != ( const LineInfo & info ) const;
        bool inside ( const LineInfo & info ) const;
        bool empty() const;
        string describe(bool fully = false) const;
        FileInfo *  fileInfo = nullptr;
        uint32_t    column = 0, line = 0;
        uint32_t    last_column = 0, last_line = 0;
        static LineInfo g_LineInfoNULL;
    };

    struct LineInfoArg : LineInfo {};

    struct DAS_API TypeInfo {
        enum {
            flag_ref = 1<<0,
            flag_refType = 1<<1,
            flag_canCopy = 1<<2,
            flag_isPod = 1<<3,
            flag_isRawPod = 1<<4,
            flag_isConst = 1<<5,
            flag_isTemp = 1<<6,
            flag_isImplicit = 1<<7,
            flag_refValue = 1<<8,
            flag_hasInitValue = 1<<9,
            flag_isSmartPtr = 1<<10,
            flag_isSmartPtrNative = 1<<11,
            flag_isHandled = 1<<12,
            flag_heapGC = 1<<13,
            flag_stringHeapGC = 1<<14,
            flag_private = 1<<15,
            flag_classMethod = 1<<16,   // struct field is a class method (set on the field VarInfo)
        };
        union {
            StructInfo *                structType;
            EnumInfo *                  enumType;
            AnnotationInfo *            annotation_info;        // WARNING: unresolved. use 'getAnnotation'
        };
        TypeInfo *                  firstType;              // map  from, or array
        TypeInfo *                  secondType;             // map  to
        TypeInfo **                 argTypes;
        const char **               argNames;
        uint32_t *                  dim;
        uint64_t                    hash;
        Type                        type;
        uint32_t                    flags;
        uint32_t                    size;
        uint32_t                    argCount;
        uint32_t                    dimSize;
        TypeInfo() = default;
        TypeInfo (  Type _type, StructInfo * _structType, EnumInfo * _enumType, AnnotationInfo * _annotation_info,
                    TypeInfo * _firstType, TypeInfo * _secondType, TypeInfo ** _argTypes, const char ** _argNames, uint32_t _argCount,
                    uint32_t _dimSize, uint32_t * _dim, uint32_t _flags, uint32_t _size, uint64_t _hash ) {
            type               = _type;
            if ( _structType )    { structType = _structType; DAS_ASSERT(!_enumType && !_annotation_info); }
            else if ( _enumType ) { enumType = _enumType; DAS_ASSERT(!_structType && !_annotation_info); }
            else                  { annotation_info = _annotation_info; DAS_ASSERT(!_structType && !_enumType); }
            firstType          = _firstType;
            secondType         = _secondType;
            argTypes           = _argTypes;
            argNames           = _argNames;
            argCount           = _argCount;
            dimSize            = _dimSize;
            dim                = _dim;
            flags              = _flags;
            size               = _size;
            hash               = _hash;
        }
        __forceinline bool isRef() const { return flags & flag_ref; }
        __forceinline bool isRefType() const { return flags & flag_refType; }
        __forceinline bool isRefValue() const { return flags & flag_refValue; }
        __forceinline bool canCopy() const { return flags & flag_canCopy; }
        __forceinline bool isPod() const { return flags & flag_isPod; }
        __forceinline bool isRawPod() const { return flags & flag_isRawPod; }
        __forceinline bool isConst() const { return flags & flag_isConst; }
        __forceinline bool isTemp() const { return flags & flag_isTemp; }
        __forceinline bool isImplicit() const { return flags & flag_isImplicit; }
        __forceinline bool isSmartPtr() const { return flags & flag_isSmartPtr; }
        __forceinline static bool isSimpleBaseType(Type t) {
            switch ( t ) {
                case Type::tString:
                case Type::tStructure:
                case Type::tHandle:
                case Type::tTuple:
                case Type::tVariant:
                case Type::tArray:
                case Type::tTable:
                case Type::tLambda:
                case Type::tIterator:
                case Type::tBlock:
                case Type::tPointer:
                    return false;
                default:
                    return true;
            }
        }
        __forceinline bool isSimpleType() const {
            return dimSize == 0 && isSimpleBaseType(type);
        }
        __forceinline bool isDimOfSimpleType() const {
            return dimSize == 1 && isSimpleBaseType(type);
        }
        __forceinline bool isArrayOfSimpleType() const {
            return firstType && firstType->isSimpleType();
        }
        __forceinline bool isTableOfSimpleTypes() const {
            return firstType && secondType
                && firstType->isSimpleType()
                && secondType->isSimpleType();
        }
        __forceinline bool isTupleOfSimpleTypes() const {
            for ( uint32_t i=0, is=argCount; i!=is; ++i ) {
                if ( !argTypes[i]->isSimpleType() ) return false;
            }
            return true;
        }
        __forceinline bool isVariantOfSimpleTypes() const {
            for ( uint32_t i=0, is=argCount; i!=is; ++i ) {
                if ( !argTypes[i]->isSimpleType() ) return false;
            }
            return true;
        }
        TypeAnnotation * getAnnotation() const;
        StructInfo * getStructType() const;
        EnumInfo * getEnumType() const;
        void resolveAnnotation() const;
    };

    struct VarInfo : TypeInfo {
        union {
            vec4f                   value;
            char *                  sValue;
        };
        const char *                name;
        AnnotationArgumentInfo *    annotation_arguments = nullptr; // flat array
        uint32_t                    annotation_argument_count = 0;
        uint32_t                    offset;
        uint32_t                    nextGcField;
        VarInfo() = default;
        VarInfo(Type _type, StructInfo * _structType, EnumInfo * _enumType, AnnotationInfo * _annotation_info,
                TypeInfo * _firstType, TypeInfo * _secondType, TypeInfo ** _argTypes, const char ** _argNames, uint32_t _argCount,
                uint32_t _dimSize, uint32_t * _dim, uint32_t _flags, uint32_t _size,
                uint64_t _hash, const char * _name, uint32_t _offset, uint32_t _nextGcField,
                AnnotationArgumentInfo * _annotation_arguments = nullptr, uint32_t _annotation_argument_count = 0 ) :
            TypeInfo(_type,_structType,_enumType,_annotation_info,
                    _firstType,_secondType,_argTypes,_argNames,_argCount,
                     _dimSize,_dim,_flags,_size,_hash) {
                name               = _name;
                offset             = _offset;
                nextGcField        = _nextGcField;
                annotation_arguments       = _annotation_arguments;
                annotation_argument_count  = _annotation_argument_count;
                value = v_zero();
        }
    };

    struct StructInfo {
        enum {
            flag_class =        (1<<0)
        ,   flag_lambda =       (1<<1)
        ,   flag_heapGC =       (1<<2)
        ,   flag_stringHeapGC = (1<<3)
        };
        const char* name;
        const char* module_name;
        VarInfo **  fields;
        AnnotationInfo * annotations;   // flat array
        uint64_t    hash;
        uint64_t    init_mnh;
        uint32_t    flags;
        uint32_t    count;
        uint32_t    size;
        uint32_t    firstGcField;
        uint32_t    annotation_count;
        StructInfo() = default;
        StructInfo(
            const char * _name, const char * _module_name, uint32_t _flags, VarInfo ** _fields, uint32_t _count,
            uint32_t _size, uint64_t _init_mnh, AnnotationInfo * _annotations, uint32_t _annotation_count,
            uint64_t _hash, uint32_t _firstGcField ) {
                name =            _name;
                module_name =     _module_name;
                flags =           _flags;
                fields =          _fields;
                count =           _count;
                size =            _size;
                init_mnh =        _init_mnh;
                annotations =     _annotations;
                annotation_count = _annotation_count;
                hash =            _hash;
                firstGcField =    _firstGcField;
        }
    };

    struct EnumValueInfo {
        const char * name;
        int64_t      value;
    };

    struct EnumInfo {
        enum {
            flag_unsigned = (1<<0)    // underlying type is uint8/uint16/uint32/uint64 (not int*)
        };
        const char *        name;
        const char *        module_name;
        EnumValueInfo **    fields;
        uint32_t            count;
        uint64_t            hash;
        uint32_t            flags;
        AnnotationInfo *    annotations;        // flat array
        uint32_t            annotation_count;
    };

    struct LocalVariableInfo : TypeInfo {
        LineInfo        visibility;
        const char *    name;
        uint32_t        stackTop;
        union {
            struct {
                bool    cmres : 1;
            };
            uint32_t    localFlags;
        };
    };

    struct FuncInfo {
        enum {
            flag_init = (1<<0)
        ,   flag_builtin = (1<<1)
        ,   flag_private = (1<<2)
        ,   flag_shutdown = (1<<3)
        ,   flag_late_init = (1<<5)
        ,   flag_late_shutdown = (1<<6)
        };
        const char *            name;
        const char *            cppName;
        VarInfo **              fields;
        TypeInfo *              result;
        LocalVariableInfo **    locals;
        VarInfo **              globals;
        AnnotationInfo *        annotations;    // flat array
        uint64_t                hash;
        uint32_t                flags;
        uint32_t                count;
        uint32_t                stackSize;
        uint32_t                localCount;
        uint32_t                globalCount;
        uint32_t                annotation_count;
        FuncInfo() = default;
        FuncInfo( const char * _name, const char * _cppName, VarInfo ** _fields, uint32_t _count, uint32_t _stackSize,
                TypeInfo * _result, LocalVariableInfo ** _locals, uint32_t _localCount, uint64_t _hash, uint32_t _flags,
                AnnotationInfo * _annotations = nullptr, uint32_t _annotation_count = 0 ) {
            name =       _name;
            cppName =    _cppName;
            fields =     _fields;
            count =      _count;
            stackSize =  _stackSize;
            result =     _result;
            locals =     _locals;
            localCount = _localCount;
            hash =       _hash;
            flags =      _flags;
            globals =    nullptr;
            globalCount = 0;
            annotations = _annotations;
            annotation_count = _annotation_count;
        }
    };

    DAS_API string das_to_string ( Type t );
    DAS_API Type nameToBasicType(const string & name);

    DAS_API int getTypeBaseSize ( Type type );
    DAS_API int getTypeBaseAlign ( Type type );
    DAS_API int getTypeBaseSize ( TypeInfo * info );
    DAS_API int getDimSize ( TypeInfo * info );
    DAS_API int getTypeSize ( TypeInfo * info );
    DAS_API int getTypeAlign ( TypeInfo * info );
    DAS_API int getTupleFieldOffset ( TypeInfo * info, int index );
    DAS_API int getVariantFieldOffset ( TypeInfo * info, int index );

    DAS_API bool isSameType ( const TypeInfo * THIS, const TypeInfo * decl, RefMatters refMatters, ConstMatters constMatters, TemporaryMatters temporaryMatters, bool topLevel );
    DAS_API bool isCompatibleCast ( const StructInfo * THIS, const StructInfo * castS );
    DAS_API bool isValidArgumentType ( TypeInfo * argType, TypeInfo * passType );
    DAS_API bool isMatchingArgumentType ( TypeInfo * argType, TypeInfo * passType);

    enum class PrintFlags : uint32_t {
        none =                  0
    ,   escapeString =          (1<<0)
    ,   namesAndDimensions =    (1<<1)
    ,   typeQualifiers =        (1<<2)
    ,   refAddresses =          (1<<3)
    ,   singleLine =            (1<<4)
    ,   fixedFloatingPoint =    (1<<5)
    ,   fullTypeInfo =          (1<<6)

    ,   string_builder  =   PrintFlags::none
    ,   debugger        =   PrintFlags::escapeString | PrintFlags::namesAndDimensions
            | PrintFlags::typeQualifiers | PrintFlags::refAddresses | PrintFlags::fixedFloatingPoint
    ,   stackwalker     =   PrintFlags::escapeString | PrintFlags::namesAndDimensions
            | PrintFlags::typeQualifiers | PrintFlags::fixedFloatingPoint
    };

    DAS_API string debug_type ( const TypeInfo * info );
    DAS_API string getTypeInfoMangledName ( TypeInfo * info );
}
