#pragma once

namespace das
{

#ifndef DAS_ENABLE_PROFILER
#define DAS_ENABLE_PROFILER 0
#endif

    enum Type : int32_t {
        none,
        autoinfer,
        alias,
        option,
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
        tBitfield,
        tPointer,
        tFunction,
        tLambda,
        tIterator,
        tArray,
        tTable,
        tBlock,
        tTuple,
        tVariant
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
        static decltype(declval<U&>() = declval<const U&>(), U (declval<const U&>()), true_type{}) func (remove_reference_t<U>*);
        template<typename U>
        static false_type func (...);
        using  type = decltype(func<T>(nullptr));
        static constexpr bool value { type::value };
    };

    struct StructInfo;
    struct TypeAnnotation;
    struct EnumInfo;

    struct BasicAnnotation : ptr_ref_count {
        BasicAnnotation ( const string & n, const string & cpn = "" ) : name(n), cppName(cpn) {}
        string      name;
        string      cppName;
    };

    struct FileInfo {
    public:
        virtual void freeSourceData() { }
        virtual ~FileInfo() { freeSourceData(); }
        void reserveProfileData();
        virtual void getSourceAndLength ( const char * & src, uint32_t & len ) { src=nullptr; len=0; }
        virtual void serialize ( AstSerializer & ser );
        string                name;
        int32_t               tabSize = 4;
#if DAS_ENABLE_PROFILER
    public:
        vector<uint64_t>      profileData;
#endif
    };
    typedef unique_ptr<FileInfo> FileInfoPtr;

    class TextFileInfo : public FileInfo {
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
    };

    class FileAccess : public ptr_ref_count {
    public:
        virtual ~FileAccess() {}
        void reset() { files.clear(); }
        FileInfo * setFileInfo ( const string & fileName, FileInfoPtr && info );
        FileInfo * getFileInfo ( const string & fileName );
        virtual bool invalidateFileInfo ( const string & fileName );
        virtual string getIncludeFileName ( const string & fileName, const string & incFileName ) const;
        void freeSourceData();
        virtual int64_t getFileMtime ( const string & fileName ) const;
        FileInfoPtr letGoOfFileInfo ( const string & fileName );
        virtual ModuleInfo getModuleInfo ( const string & req, const string & from ) const;
        virtual bool isModuleAllowed ( const string &, const string & ) const { return true; };
        virtual bool canModuleBeUnsafe ( const string &, const string & ) const { return true; };
        virtual bool addFsRoot ( const string & , const string & ) { return false; }
        virtual void serialize ( AstSerializer & ser );
    protected:
        virtual FileInfo * getNewFileInfo ( const string & ) { return nullptr; }
    protected:
        das_hash_map<string, FileInfoPtr>    files;
    };
    typedef smart_ptr<FileAccess> FileAccessPtr;
    template <> struct isCloneable<FileAccess> : false_type {};

    struct SimFunction;
    class Context;

    class ModuleFileAccess : public FileAccess {
    public:
        ModuleFileAccess();
        ModuleFileAccess ( const string & pak, const FileAccessPtr & access );
        virtual ~ModuleFileAccess();
        bool failed() const { return !context || !modGet; }
        virtual ModuleInfo getModuleInfo ( const string & req, const string & from ) const override;
        virtual string getIncludeFileName ( const string & fileName, const string & incFileName ) const override;
        virtual bool isModuleAllowed ( const string &, const string & ) const override;
        virtual bool canModuleBeUnsafe ( const string &, const string & ) const override;
        virtual void serialize ( AstSerializer & ser ) override;
    protected:
        Context *           context = nullptr;
        SimFunction *       modGet = nullptr;
        SimFunction *       includeGet = nullptr;
        SimFunction *       moduleAllowed = nullptr;
        SimFunction *       moduleUnsafe = nullptr;
    };
    template <> struct isCloneable<ModuleFileAccess> : false_type {};

    struct LineInfo {
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

    struct TypeInfo {
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
            flag_lockCheck = 1<<15,
        };
        union {
            StructInfo *                structType;
            EnumInfo *                  enumType;
            mutable TypeAnnotation *    annotation_or_name;     // WARNING: unresolved. use 'getAnnotation'
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
        TypeInfo (  Type _type, StructInfo * _structType, EnumInfo * _enumType, TypeAnnotation * _annotation_or_name,
                    TypeInfo * _firstType, TypeInfo * _secondType, TypeInfo ** _argTypes, const char ** _argNames, uint32_t _argCount,
                    uint32_t _dimSize, uint32_t * _dim, uint32_t _flags, uint32_t _size, uint64_t _hash ) {
            type               = _type;
            if ( _structType )    { structType = _structType; DAS_ASSERT(!_enumType && !_annotation_or_name); }
            else if ( _enumType ) { enumType = _enumType; DAS_ASSERT(!_structType && !_annotation_or_name); }
            else                  { annotation_or_name = _annotation_or_name; DAS_ASSERT(!_structType && !_enumType); }
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
        void *                      annotation_arguments = nullptr;
        uint32_t                    offset;
        uint32_t                    nextGcField;
        VarInfo() = default;
        VarInfo(Type _type, StructInfo * _structType, EnumInfo * _enumType, TypeAnnotation * _annotation_or_name,
                TypeInfo * _firstType, TypeInfo * _secondType, TypeInfo ** _argTypes, const char ** _argNames, uint32_t _argCount,
                uint32_t _dimSize, uint32_t * _dim, uint32_t _flags, uint32_t _size,
                uint64_t _hash, const char * _name, uint32_t _offset, uint32_t _nextGcField ) :
            TypeInfo(_type,_structType,_enumType,_annotation_or_name,
                    _firstType,_secondType,_argTypes,_argNames,_argCount,
                     _dimSize,_dim,_flags,_size,_hash) {
                name               = _name;
                offset             = _offset;
                nextGcField        = _nextGcField;
                value = v_zero();
        }
    };

    struct StructInfo {
        enum {
            flag_class =        (1<<0)
        ,   flag_lambda =       (1<<1)
        ,   flag_heapGC =       (1<<2)
        ,   flag_stringHeapGC = (1<<3)
        ,   flag_lockCheck =    (1<<4)
        };
        const char* name;
        const char* module_name;
        VarInfo **  fields;
        void *      annotation_list;
        uint64_t    hash;
        uint64_t    init_mnh;
        uint32_t    flags;
        uint32_t    count;
        uint32_t    size;
        uint32_t    firstGcField;
        StructInfo() = default;
        StructInfo(
            const char * _name, const char * _module_name, uint32_t _flags, VarInfo ** _fields, uint32_t _count,
            uint32_t _size, uint64_t _init_mnh, void * _annotation_list, uint64_t _hash, uint32_t _firstGcField ) {
                name =            _name;
                module_name =     _module_name;
                flags =           _flags;
                fields =          _fields;
                count =           _count;
                size =            _size;
                init_mnh =        _init_mnh;
                annotation_list = _annotation_list;
                hash =            _hash;
                firstGcField =    _firstGcField;
        }
    };

    struct EnumValueInfo {
        const char * name;
        int64_t      value;
    };

    struct EnumInfo {
        const char *        name;
        const char *        module_name;
        EnumValueInfo **    fields;
        uint32_t            count;
        uint64_t            hash;
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
        };
        const char *            name;
        const char *            cppName;
        VarInfo **              fields;
        TypeInfo *              result;
        LocalVariableInfo **    locals;
        VarInfo **              globals;
        uint64_t                hash;
        uint32_t                flags;
        uint32_t                count;
        uint32_t                stackSize;
        uint32_t                localCount;
        uint32_t                globalCount;
        FuncInfo() = default;
        FuncInfo( const char * _name, const char * _cppName, VarInfo ** _fields, uint32_t _count, uint32_t _stackSize,
                TypeInfo * _result, LocalVariableInfo ** _locals, uint32_t _localCount, uint64_t _hash, uint32_t _flags ) {
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
        }
    };

    string das_to_string ( Type t );
    Type nameToBasicType(const string & name);

    int getTypeBaseSize ( Type type );
    int getTypeBaseAlign ( Type type );
    int getTypeBaseSize ( TypeInfo * info );
    int getDimSize ( TypeInfo * info );
    int getTypeSize ( TypeInfo * info );
    int getTypeAlign ( TypeInfo * info );
    int getTupleFieldOffset ( TypeInfo * info, int index );
    int getVariantFieldOffset ( TypeInfo * info, int index );

    bool isSameType ( const TypeInfo * THIS, const TypeInfo * decl, RefMatters refMatters, ConstMatters constMatters, TemporaryMatters temporaryMatters, bool topLevel );
    bool isCompatibleCast ( const StructInfo * THIS, const StructInfo * castS );
    bool isValidArgumentType ( TypeInfo * argType, TypeInfo * passType );
    bool isMatchingArgumentType ( TypeInfo * argType, TypeInfo * passType);

    enum class PrintFlags : uint32_t {
        none =                  0
    ,   escapeString =          (1<<0)
    ,   namesAndDimensions =    (1<<1)
    ,   typeQualifiers =        (1<<2)
    ,   refAddresses =          (1<<3)
    ,   humanReadable =         (1<<4)
    ,   singleLine =            (1<<5)
    ,   fixedFloatingPoint =    (1<<6)

    ,   string_builder  =   PrintFlags::fixedFloatingPoint
    ,   debugger        =   PrintFlags::escapeString | PrintFlags::namesAndDimensions
            | PrintFlags::humanReadable | PrintFlags::typeQualifiers | PrintFlags::refAddresses | PrintFlags::fixedFloatingPoint
    ,   stackwalker     =   PrintFlags::escapeString | PrintFlags::namesAndDimensions
            | PrintFlags::typeQualifiers | PrintFlags::humanReadable | PrintFlags::fixedFloatingPoint
    };

    string debug_type ( const TypeInfo * info );
    string getTypeInfoMangledName ( TypeInfo * info );
}

