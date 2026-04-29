#pragma once

#include "daScript/simulate/simulate.h"
#include "daScript/misc/string_writer.h"
#include "daScript/misc/safebox.h"
#include "daScript/misc/vectypes.h"
#include "daScript/misc/platform.h"
#include "daScript/misc/arraytype.h"
#include "daScript/misc/rangetype.h"
#include "daScript/simulate/data_walker.h"
#include "daScript/simulate/debug_info.h"
#include "daScript/ast/ast_serialize_macro.h"
#include "daScript/ast/compilation_errors.h"
#include "daScript/ast/ast_typedecl.h"
#include "daScript/simulate/aot_library.h"

#ifndef DAS_ALLOW_ANNOTATION_LOOKUP
#define DAS_ALLOW_ANNOTATION_LOOKUP     1
#endif

#ifndef DAS_THREAD_SAFE_ANNOTATIONS
#define DAS_THREAD_SAFE_ANNOTATIONS    1
#endif


namespace das
{
    struct AstSerializer;

    class Function;
    typedef smart_ptr<Function> FunctionPtr;

    struct Variable;
    typedef smart_ptr<Variable> VariablePtr;

    class Program;
    typedef smart_ptr<Program> ProgramPtr;

    struct FunctionAnnotation;
    typedef smart_ptr<FunctionAnnotation> FunctionAnnotationPtr;

    struct Expression;
    typedef smart_ptr<Expression> ExpressionPtr;

    struct PassMacro;
    typedef smart_ptr<PassMacro> PassMacroPtr;

    struct VariantMacro;
    typedef smart_ptr<VariantMacro> VariantMacroPtr;

    struct ReaderMacro;
    typedef smart_ptr<ReaderMacro> ReaderMacroPtr;

    struct CommentReader;
    typedef smart_ptr<CommentReader> CommentReaderPtr;

    struct CallMacro;
    typedef smart_ptr<CallMacro> CallMacroPtr;

    struct ForLoopMacro;
    typedef smart_ptr<ForLoopMacro> ForLoopMacroPtr;

    struct CaptureMacro;
    typedef smart_ptr<CaptureMacro> CaptureMacroPtr;

    struct SimulateMacro;
    typedef smart_ptr<SimulateMacro> SimulateMacroPtr;

    struct TypeMacro;
    typedef smart_ptr<TypeMacro> TypeMacroPtr;

    struct AnnotationArgumentList;
    struct AnnotationDeclaration;
    typedef smart_ptr<AnnotationDeclaration> AnnotationDeclarationPtr;

    enum class LogicAnnotationOp { And, Or, Xor, Not };
    AnnotationPtr newLogicAnnotation ( LogicAnnotationOp op );
    AnnotationPtr newLogicAnnotation ( LogicAnnotationOp op,
        const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 );


    //      [annotation (value,value,...,value)]
    //  or  [annotation (key=value,key,value,...,key=value)]
    struct DAS_API AnnotationArgument {
        Type    type;       // only tInt, tFloat, tBool, and tString are allowed
        string  name;
        string  sValue;
        union {
            bool    bValue;
            int     iValue;
            float   fValue;
            AnnotationArgumentList * aList; // only used during parsing
        };
        LineInfo    at;
        AnnotationArgument () : type(Type::tVoid), iValue(0) {}
        //explicit copy is required to avoid copying union as float and cause FPE
        AnnotationArgument ( const AnnotationArgument & a )
            : type(a.type), name(a.name), sValue(a.sValue), iValue(a.iValue), at(a.at) {}
        AnnotationArgument & operator = ( const AnnotationArgument & a ) {
            type=a.type; name=a.name; sValue=a.sValue; iValue=a.iValue; at=a.at; return *this;
        }
        AnnotationArgument ( const string & n, const string & s, const LineInfo & loc = LineInfo() )
            : type(Type::tString), name(n), sValue(s), iValue(0), at(loc) {}
        AnnotationArgument ( const string & n, bool b, const LineInfo & loc = LineInfo() )
            : type(Type::tBool), name(n), bValue(b), at(loc) {}
        AnnotationArgument ( const string & n, int i, const LineInfo & loc = LineInfo() )
            : type(Type::tInt), name(n), iValue(i), at(loc) {}
        AnnotationArgument ( const string & n, float f, const LineInfo & loc = LineInfo() )
            : type(Type::tFloat), name(n), fValue(f), at(loc) {}
        AnnotationArgument ( const string & n, AnnotationArgumentList * al, const LineInfo & loc = LineInfo() )
            : type(Type::none), name(n), aList(al), at(loc) {}
        void serialize ( AstSerializer & ser );
    };

    typedef vector<AnnotationArgument> AnnotationArguments;

    struct DAS_API AnnotationArgumentList : AnnotationArguments {
        const AnnotationArgument * find ( const string & name, Type type ) const;
        bool getBoolOption(const string & name, bool def = false) const;
        int32_t getIntOption(const string & name, int32_t def = false) const;
        uint64_t getUInt64Option(const string & name, uint64_t def = false) const;
        uint64_t getUInt64OptionEx (const string & name, const string & name2, uint64_t def = false) const;
        void serialize ( AstSerializer & ser );
    };

    struct DAS_API Annotation : BasicAnnotation {
        Annotation ( const string & n, const string & cpn = "" ) : BasicAnnotation(n,cpn) {}
        virtual ~Annotation() {}
        virtual void seal( Module * m ) { module = m; }
        virtual bool rtti_isHandledTypeAnnotation() const { return false; }
        virtual bool rtti_isStructureAnnotation() const { return false; }
        virtual bool rtti_isStructureTypeAnnotation() const { return false; }
        virtual bool rtti_isEnumerationAnnotation() const { return false; }
        virtual bool rtti_isFunctionAnnotation() const { return false; }
        virtual bool rtti_isBasicStructureAnnotation() const { return false;  }
        string describe() const { return name; }
        string getMangledName() const;
        virtual void log ( TextWriter & ss, const AnnotationDeclaration & decl ) const;
        virtual void serialize( AstSerializer & ) { }
        Module *    module = nullptr;
    };

    struct DAS_API AnnotationDeclaration : ptr_ref_count {
        AnnotationPtr           annotation;
        AnnotationArgumentList  arguments;
        LineInfo                at;
        union {
            struct {
                bool inherited : 1;
            };
            uint32_t            flags;
        };
        string getMangledName() const;
        void serialize( AstSerializer & ser );
    };

    typedef vector<AnnotationDeclarationPtr> AnnotationList;

    AnnotationList cloneAnnotationList ( const AnnotationList & list );

    class DAS_API Enumeration : public ptr_ref_count {
    public:
        struct EnumEntry {
            string          name;
            string          cppName;
            LineInfo        at;
            ExpressionPtr   value;
            void serialize ( AstSerializer & ser );
        };
    public:
        Enumeration() = default;
        Enumeration( const string & na ) : name(na) {}
        bool add ( const string & f, const LineInfo & at );
        bool add ( const string & f, const ExpressionPtr & expr, const LineInfo & at );
        bool addEx ( const string & f, const string & fcpp, const ExpressionPtr & expr, const LineInfo & at );
        bool addI ( const string & f, int64_t value, const LineInfo & at );
        bool addIEx ( const string & f, const string & fcpp, int64_t value, const LineInfo & at );
        string describe() const { return name; }
        string getMangledName() const;
        int64_t find ( const string & na, int64_t def ) const;
        string find ( int64_t va, const string & def ) const;
        pair<ExpressionPtr,bool> find ( const string & f ) const;
        TypeDeclPtr makeBaseType() const;
        Type getEnumType() const;
        TypeDeclPtr makeEnumType() const;
        void serialize ( AstSerializer & ser );
    public:
        string              name;
        string              cppName;
        LineInfo            at;
        vector<EnumEntry>   list;
        Module *            module = nullptr;
        bool                external = false;
        Type                baseType = Type::tInt;
        AnnotationList      annotations;
        bool                isPrivate = false;
#if DAS_MACRO_SANITIZER
    public:
        void* operator new ( size_t count ) { return das_aligned_alloc16(count); }
        void operator delete  ( void* ptr ) { auto size = das_aligned_memsize(ptr);
            memset(ptr, 0xcd, size); das_aligned_free16(ptr); }
#endif
    };

    class DAS_API Structure : public ptr_ref_count {
    public:
        struct FieldDeclaration {
            string                  name;
            TypeDeclPtr             type;
            ExpressionPtr           init;
            AnnotationArgumentList  annotation;
            LineInfo                at;
            int                     offset = 0;
            union {
                struct {
                    bool            moveSemantics : 1;
                    bool            parentType : 1;
                    bool            capturedConstant : 1;
                    bool            generated : 1;
                    bool            capturedRef : 1;
                    bool            doNotDelete : 1;
                    bool            privateField : 1;
                    bool            sealed : 1;
                    bool            implemented : 1;
                    bool            classMethod : 1;
                };
                uint32_t            flags = 0;
            };
            FieldDeclaration() = default;
            FieldDeclaration(const string & n, const TypeDeclPtr & t,  const ExpressionPtr & i,
                             const AnnotationArgumentList & alist, bool ms, const LineInfo & a )
                : name(n), type(t), init(i), annotation(alist), at(a) {
                moveSemantics = ms;
                for ( auto & ann : annotation ) {
                    if ( ann.name=="do_not_delete" ) {
                        doNotDelete = true;
                        break;
                    }
                }
            }
            void serialize ( AstSerializer & ser );
        };
        struct FieldDeclarationRef {
            Structure *            owner = nullptr;
            int32_t                index = -1;
            __forceinline FieldDeclaration * operator -> () const { return &owner->fields[index]; }
            __forceinline FieldDeclaration & operator * () const { return owner->fields[index]; }
            __forceinline FieldDeclaration * get() const { return (index >= 0 && owner != nullptr) ? &owner->fields[index] : nullptr; }
            __forceinline operator bool() const { return index>=0 && owner!=nullptr; }
            __forceinline bool operator ! () const { return index<0 || owner==nullptr; }
        };
    public:
        Structure() {}
        Structure ( const string & n ) : name(n) {}
        StructurePtr clone() const;
        bool isCompatibleCast ( const Structure & castS ) const;
        const FieldDeclaration * findField ( const string & name ) const;
        const Structure * findFieldParent ( const string & name ) const;
        FieldDeclarationRef findFieldRef ( const string & name ) const;
        int getSizeOf() const;
        uint64_t getSizeOf64() const;
        uint64_t getSizeOf64(bool & failed) const;
        int getAlignOf() const;
        int getAlignOfFailed(bool & failed) const;
        __forceinline bool canCopy() const { return canCopy(false); }
        bool canCopy(bool tempMatters) const;
        bool canCloneFromConst() const;
        bool canClone() const;
        bool canMove() const;
        bool canAot() const;
        bool canAot( das_set<Structure *> & recAot ) const;
        bool isNoHeapType() const;
        bool isPod() const;
        bool isRawPod() const;
        bool isExprTypeAnywhere( das_set<Structure *> & dep ) const;
        bool isSafeToDelete( das_set<Structure *> & dep ) const;
        bool isLocal( das_set<Structure *> & dep ) const;
        bool isTemp( das_set<Structure *> & dep ) const;
        bool isShareable ( das_set<Structure *> & dep ) const;
        bool hasClasses( das_set<Structure *> & dep ) const;
        bool hasStringData( das_set<void*> & dep ) const;
        bool hasNonTrivialCtor ( das_set<Structure *> & dep ) const;
        bool hasNonTrivialDtor ( das_set<Structure *> & dep ) const;
        bool hasNonTrivialCopy ( das_set<Structure *> & dep ) const;
        bool canBePlacedInContainer ( das_set<Structure *> & dep ) const;
        bool needInScope ( das_set<Structure *> & dep ) const;
        bool unsafeInit ( das_set<Structure *> & dep ) const;
        string describe() const { return name; }
        string getMangledName() const;
        bool hasAnyInitializers() const;
        void serialize( AstSerializer & ser );
        uint64_t getOwnSemanticHash(HashBuilder & hb,das_set<Structure *> & dep, das_set<Annotation *> & adep) const;
        TypeDeclPtr findAlias ( const string & name ) const;
    public:
        string                          name;
        vector<FieldDeclaration>        fields;
        das_hash_map<string,int32_t>    fieldLookup;
        safebox<TypeDecl>               aliases;
        LineInfo                        at;
        Module *                        module = nullptr;
        Structure *                     parent = nullptr;
        AnnotationList                  annotations;
        uint64_t                        ownSemanticHash = 0;
        union {
            struct {
                bool    isClass : 1;
                bool    genCtor : 1;
                bool    cppLayout : 1;
                bool    cppLayoutNotPod : 1;
                bool    generated : 1;
                bool    persistent : 1;
                bool    isLambda : 1;
                bool    privateStructure : 1;
                bool    macroInterface : 1;
                bool    sealed : 1;
                bool    circular : 1;
                bool    generator : 1;
                bool    hasStaticMembers : 1;
                bool    hasStaticFunctions : 1;
                bool    hasInitFields : 1;
                bool    safeWhenUninitialized : 1;
                bool    isTemplate : 1;
                bool    hasDefaultInitializer : 1;
                bool    noGenCtor : 1;
            };
            uint32_t    flags = 0;
        };
        mutable bool circularGuard = false;   // we prevent circular lookups with this guard. Do not serialize, do not expose to daslang
#if DAS_MACRO_SANITIZER
    public:
        void* operator new ( size_t count ) { return das_aligned_alloc16(count); }
        void operator delete  ( void* ptr ) { auto size = das_aligned_memsize(ptr);
            memset(ptr, 0xcd, size); das_aligned_free16(ptr); }
#endif
    };

    struct DAS_API Variable : ptr_ref_count {
        VariablePtr clone() const;
        string getMangledName() const;
        uint64_t getMangledNameHash() const;
        static uint64_t getMNHash(const string &mangledName);
        bool isAccessUnused() const;
        bool isAccessDummy() const;
        bool isCtorInitialized() const;
        void serialize ( AstSerializer & ser );
        string          name;
        string          aka;        // name alias
        TypeDeclPtr     type;
        ExpressionPtr   init;
        ExpressionPtr   source;     // if its interator variable, this is where the source is
        LineInfo        at;
        int             index = -1;
        uint32_t        stackTop = 0;
        uint32_t        extraLocalOffset = 0;   // this is here for fake variables only
        Module *        module = nullptr;
        das_set<Function *> useFunctions;
        das_set<Variable *> useGlobalVariables;
        uint32_t        initStackSize = 0;
        union {
            struct {
                bool    init_via_move : 1;
                bool    init_via_clone : 1;
                bool    used : 1;
                bool    aliasCMRES : 1;
                bool    marked_used : 1;
                bool    global_shared : 1;
                bool    do_not_delete : 1;
                bool    generated : 1;

                bool    capture_as_ref : 1;
                bool    can_shadow : 1;             // can shadow block or function arguments, as block argument
                bool    private_variable : 1;
                bool    tag : 1;
                bool    global : 1;
                bool    inScope : 1;
                bool    no_capture : 1;
                bool    early_out : 1;              // this variable is potentially uninitialized in the finally section

                bool    used_in_finally : 1;        // this variable is used in the finally section
                bool    static_class_member : 1;    // this is a static class member
                bool    bitfield_constant : 1;      // this is a bitfield constant
                bool    pod_delete : 1;             // this variable can be deleted as POD
                bool    pod_delete_gen : 1;         // pod delete has been generated
                bool    single_return_via_move : 1; // this variable is returned via move, where function has only 1 return path (only set if force_pod_inscope is set)
                bool    consumed : 1;               // this variable has been passed via consume (only set if force_pod_inscope is set)
            };
            uint32_t flags = 0;
        };
        union {
            struct {
                bool    access_extern : 1;
                bool    access_get : 1;
                bool    access_ref : 1;
                bool    access_init : 1;
                bool    access_pass : 1;
                bool    access_fold : 1;
            };
            uint32_t access_flags = 0;
        };


        union {
            struct {
                bool    aliasesResolved : 1;
                bool    stackResolved : 1;
            };
            uint32_t     stackFlags = 0;
        };

        AnnotationArgumentList  annotation;
#if DAS_MACRO_SANITIZER
    public:
        void* operator new ( size_t count ) { return das_aligned_alloc16(count); }
        void operator delete  ( void* ptr ) { auto size = das_aligned_memsize(ptr);
            memset(ptr, 0xcd, size); das_aligned_free16(ptr); }
#endif
    };

    struct VarLessPred {
        __forceinline bool operator () ( const VariablePtr & a, const VariablePtr & b ) const {
            if ( a==b) {
                return false;
            } else if ( a->name != b->name ) {
                return a->name < b->name;
            } else if ( a->at != b->at ) {
                return a->at < b->at;
            } else {
                return false;
            }
        }
    };

    typedef das_safe_set<VariablePtr,VarLessPred> safe_var_set;

    struct ExprBlock;
    struct ExprCallFunc;

    struct DAS_API FunctionAnnotation : Annotation {
        FunctionAnnotation ( const string & n ) : Annotation(n) {}
        virtual bool rtti_isFunctionAnnotation() const override { return true; }
        virtual bool apply ( const FunctionPtr & func, ModuleGroup & libGroup,
                            const AnnotationArgumentList & args, string & err ) = 0;
        virtual bool generic_apply ( const FunctionPtr &, ModuleGroup &,
                            const AnnotationArgumentList &, string & ) { return true; };
        virtual bool finalize ( const FunctionPtr & func, ModuleGroup & libGroup,
                               const AnnotationArgumentList & args,
                               const AnnotationArgumentList & progArgs, string & err ) = 0;
        virtual bool apply ( ExprBlock * block, ModuleGroup & libGroup,
                            const AnnotationArgumentList & args, string & err ) = 0;
        virtual bool finalize ( ExprBlock * block, ModuleGroup & libGroup,
                               const AnnotationArgumentList & args,
                               const AnnotationArgumentList & progArgs, string & err ) = 0;
        virtual bool patch ( const FunctionPtr &, ModuleGroup &,
                               const AnnotationArgumentList &,
                               const AnnotationArgumentList &, string &, bool & ) { return true; }
        virtual bool fixup ( const FunctionPtr &, ModuleGroup &,
                               const AnnotationArgumentList &,
                               const AnnotationArgumentList &, string & ) { return true; }
        virtual bool lint ( const FunctionPtr &, ModuleGroup &,
                               const AnnotationArgumentList &,
                               const AnnotationArgumentList &, string & ) { return true; }
        virtual SimNode * simulate ( Context *, Function *,
                            const AnnotationArgumentList &, string & ) { return nullptr; }
        virtual void complete ( Context *, const FunctionPtr & ) { }
        virtual bool simulate ( Context *, SimFunction * ) { return true; }
        virtual bool verifyCall ( ExprCallFunc * /*call*/, const AnnotationArgumentList & /*args*/,
            const AnnotationArgumentList & /*progArgs */, string & /*err*/ ) { return true; }
        virtual ExpressionPtr transformCall ( ExprCallFunc * /*call*/, string & /*err*/ ) { return nullptr; }
        virtual string aotName ( ExprCallFunc * call );
        virtual string aotArgumentPrefix ( ExprCallFunc * /*call*/, int /*argIndex*/ ) { return ""; }
        virtual string aotArgumentSuffix ( ExprCallFunc * /*call*/, int /*argIndex*/ ) { return ""; }
        virtual void aotPrefix ( TextWriter &, ExprCallFunc * ) { }
        virtual bool isGeneric() const { return false; }
        virtual bool isCompatible ( const FunctionPtr &, const vector<TypeDeclPtr> &, const AnnotationDeclaration &, string &  ) const { return true; }
        virtual bool isSpecialized() const { return false; }
        virtual void appendToMangledName( const FunctionPtr &, const AnnotationDeclaration &, string & /* mangledName */ ) const { }
    };

    struct TransformFunctionAnnotation : FunctionAnnotation {
        TransformFunctionAnnotation ( const string & n ) : FunctionAnnotation(n) {}
        virtual ExpressionPtr transformCall ( ExprCallFunc * /*call*/, string & /*err*/ ) override = 0;
        virtual bool apply ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, string & ) override {
            return false;
        }
        virtual bool finalize ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string & ) override {
            return false;
        }
        virtual bool apply ( ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, string & ) override {
            return false;
        }
        virtual bool finalize ( ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string & ) override {
            return false;
        }
    };

    struct TypeAnnotation : Annotation {
        TypeAnnotation ( const string & n, const string & cpn = "" ) : Annotation(n,cpn) {}
        virtual TypeAnnotationPtr clone ( const TypeAnnotationPtr & p = nullptr ) const {
            DAS_ASSERTF(p, "can only clone real type %s", name.c_str());
            p->name = name;
            p->cppName = cppName;
            return p;
        }
        virtual void seal( Module * m ) override {
            Annotation::seal(m);
            HashBuilder hb;
            das_set<Structure *> dep;
            das_set<Annotation *> adep;
            ownSemanticHash = getOwnSemanticHash(hb,dep,adep);
        }
        virtual uint64_t getOwnSemanticHash ( HashBuilder & hb, das_set<Structure *> &, das_set<Annotation *> & ) const {
            hb.updateString(getMangledName());
            return hb.getHash();
        }
        virtual int32_t getGcFlags(das_set<Structure *> &, das_set<Annotation *> &) const { return 0; }
        virtual bool canAot(das_set<Structure *> &) const { return true; }
        virtual bool canMove() const {
            return !hasNonTrivialCopy();
        }
        virtual bool canCopy() const {
            return !hasNonTrivialCopy();
        }
        virtual bool canClone() const { return false; }
        virtual bool isPod() const { return false; }
        virtual bool isRawPod() const { return false; }
        virtual bool isRefType() const { return false; }
        virtual bool hasStringData(das_set<void *> &) const { return false; }
        virtual bool hasNonTrivialCtor() const { return true; }
        virtual bool hasNonTrivialDtor() const { return true; }
        virtual bool hasNonTrivialCopy() const { return true; }
        virtual bool canBePlacedInContainer() const {
            return !hasNonTrivialCtor() && !hasNonTrivialDtor() && !hasNonTrivialCopy();
        }
        virtual bool isLocal() const {
            return isPod() && !hasNonTrivialCtor() && !hasNonTrivialDtor() && !hasNonTrivialCopy();
        }
        virtual bool needInScope() const { return false; }
        virtual bool canNew() const { return false; }
        virtual bool canDelete() const { return false; }
        virtual bool needDelete() const { return canDelete(); }
        virtual bool canDeletePtr() const { return false; }
        virtual bool isIndexable ( const TypeDeclPtr & ) const { return false; }
        virtual bool isIndexMutable ( const TypeDeclPtr & ) const { return false; }
        virtual bool isIterable ( ) const { return false; }
        virtual bool isShareable ( ) const { return true; }
        virtual bool isSmart() const { return false; }
        virtual bool avoidNullPtr() const { return false; }
        virtual bool canSubstitute ( TypeAnnotation * /* passType */ ) const { return false; }
        virtual bool canBeSubstituted ( TypeAnnotation * /* passType */ ) const { return false; }
        virtual string getSmartAnnotationCloneFunction () const { return ""; }
        virtual size_t getSizeOf() const { return sizeof(void *); }
        virtual size_t getAlignOf() const { return 1; }
        virtual uint32_t getFieldOffset ( const string & ) const { return -1U; }
        virtual TypeInfo * getFieldType ( const string & ) const { return nullptr; }
        virtual TypeDeclPtr makeValueType() const { return nullptr; }
        virtual TypeDeclPtr makeFieldType ( const string &, bool ) const { return nullptr; }
        virtual TypeDeclPtr makeSafeFieldType ( const string &, bool ) const { return nullptr; }
        virtual TypeDeclPtr makeIndexType ( const ExpressionPtr & /*src*/, const ExpressionPtr & /*idx*/ ) const { return nullptr; }
        virtual TypeDeclPtr makeIteratorType ( const ExpressionPtr & /*src*/ ) const { return nullptr; }
        // aot
        virtual void aotPreVisitGetField ( TextWriter &, const string & ) { }
        virtual void aotPreVisitGetFieldPtr ( TextWriter &, const string & ) { }
        virtual void aotVisitGetField ( TextWriter & ss, const string & fieldName ) { ss << "." << fieldName; }
        virtual void aotVisitGetFieldPtr ( TextWriter & ss, const string & fieldName ) { ss << "->" << fieldName; }
        // simulate
        virtual SimNode * simulateDelete ( Context &, const LineInfo &, SimNode *, uint32_t ) const { return nullptr; }
        virtual SimNode * simulateDeletePtr ( Context &, const LineInfo &, SimNode *, uint32_t ) const { return nullptr; }
        virtual SimNode * simulateClone ( Context &, const LineInfo &, SimNode *, SimNode * ) const { return nullptr; }
        virtual SimNode * simulateNullCoalescing ( Context &, const LineInfo &, SimNode *, SimNode * ) const { return nullptr; }
        virtual SimNode * simulateGetNew ( Context &, const LineInfo & ) const { return nullptr; }
        virtual SimNode * simulateGetAt ( Context &, const LineInfo &, const TypeDeclPtr &,
                                         const ExpressionPtr &, const ExpressionPtr &, uint32_t ) const { return nullptr; }
        virtual SimNode * simulateGetAtR2V ( Context &, const LineInfo &, const TypeDeclPtr &,
                                            const ExpressionPtr &, const ExpressionPtr &, uint32_t ) const { return nullptr; }
        virtual SimNode * simulateGetIterator ( Context &, const LineInfo &, const ExpressionPtr & ) const { return nullptr; }
        // data walker
        virtual void walk ( DataWalker &, void * ) { }
        // familiar patterns
        virtual bool isYetAnotherVectorTemplate() const { return false; }   // has [], there is length(x), data is linear in memory
        // factory
        virtual void * factory () const { return nullptr; }
        // new, delete, and clone, jit versions
        virtual void * jitGetNew () const { return nullptr; }
        virtual void * jitGetDelete () const { return nullptr; }
        virtual void * jitGetClone () const { return nullptr; }
        uint64_t ownSemanticHash = 0;
    };

    struct StructureAnnotation : Annotation {
        StructureAnnotation ( const string & n ) : Annotation(n) {}
        virtual bool rtti_isStructureAnnotation() const override { return true; }
        virtual bool touch ( const StructurePtr & st, ModuleGroup & libGroup,
                            const AnnotationArgumentList & args, string & err ) = 0;    // this one happens before infer. u can change structure here
        virtual bool look (const StructurePtr & st, ModuleGroup & libGroup,
            const AnnotationArgumentList & args, string & err ) = 0;                    // this one happens after infer. structure is read-only, or at-least infer-safe
        virtual bool patch (const StructurePtr &, ModuleGroup &,
            const AnnotationArgumentList &, string &, bool & /*astChanged*/ ) { return true; } // this one happens after infer. this can restart infer by setting astChange
        virtual void complete ( Context *, const StructurePtr & ) { }
        virtual void aotPrefix ( const StructurePtr &, const AnnotationArgumentList &, TextWriter & ) { }
        virtual void aotBody   ( const StructurePtr &, const AnnotationArgumentList &, TextWriter & ) { }
        virtual void aotSuffix ( const StructurePtr &, const AnnotationArgumentList &, TextWriter & ) { }
    };
    typedef smart_ptr<StructureAnnotation> StructureAnnotationPtr;

    struct EnumerationAnnotation : Annotation {
        EnumerationAnnotation ( const string & n ) : Annotation(n) {}
        virtual bool rtti_isEnumerationAnnotation() const override { return true; }
        virtual bool touch ( const EnumerationPtr & st, ModuleGroup & libGroup,
                            const AnnotationArgumentList & args, string & err ) = 0;    // this one happens before infer. u can change enum here
    };
    typedef smart_ptr<EnumerationAnnotation> EnumerationAnnotationPtr;


    // annotated structure
    //  needs to override
    //      create,clone
    struct StructureTypeAnnotation : TypeAnnotation {
        StructureTypeAnnotation ( const string & n ) : TypeAnnotation(n) {}
        virtual bool rtti_isStructureTypeAnnotation() const override { return true; }
        virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
        virtual bool canCopy() const override { return false; }
        virtual bool isPod() const override { return false; }
        virtual bool isRawPod() const override { return false; }
        virtual bool isRefType() const override { return false; }
        virtual bool create ( const StructurePtr & st, const AnnotationArgumentList & /*args*/, string & /*err*/ ) {
            structureType = st;
            return true;
        }
        virtual TypeAnnotationPtr clone ( const TypeAnnotationPtr & p = nullptr ) const override {
            smart_ptr<StructureTypeAnnotation> cp =  p ? static_pointer_cast<StructureTypeAnnotation>(p) : make_smart<StructureTypeAnnotation>(name);
            cp->structureType = structureType;
            return TypeAnnotation::clone(cp);
        }
        smart_ptr<Structure>   structureType;
    };

    struct DAS_API Expression : ptr_ref_count {
        Expression() = default;
        Expression(const LineInfo & a) : at(a) {}
        string describe() const;
        virtual ~Expression() {}
        friend StringWriter& operator<< (StringWriter& stream, const Expression & func);
        virtual ExpressionPtr visit(Visitor & /*vis*/ )  { DAS_ASSERT(0); return this; };
        virtual ExpressionPtr clone( const ExpressionPtr & expr = nullptr ) const;
        static ExpressionPtr autoDereference ( const ExpressionPtr & expr );
        virtual SimNode * simulate (Context & /*context*/ ) const { DAS_ASSERT(0); return nullptr; };
        virtual SimNode * trySimulate (Context & context, uint32_t extraOffset, const TypeDeclPtr & r2vType ) const;
        virtual void markNoDiscard() { }
        virtual bool rtti_isAssume() const { return false; }
        virtual bool rtti_isSequence() const { return false; }
        virtual bool rtti_isConstant() const { return false; }
        virtual bool rtti_isStringConstant() const { return false; }
        virtual bool rtti_isStringBuilder() const { return false; }
        virtual bool rtti_isCall() const { return false; }
        virtual bool rtti_isInvoke() const { return false; }
        virtual bool rtti_isCallLikeExpr() const { return false; }
        virtual bool rtti_isCallFunc() const { return false; }
        virtual bool rtti_isNamedCall() const { return false; }
        virtual bool rtti_isLet() const { return false; }
        virtual bool rtti_isReturn() const { return false; }
        virtual bool rtti_isBreak() const { return false; }
        virtual bool rtti_isContinue() const { return false; }
        virtual bool rtti_isBlock() const { return false; }
        virtual bool rtti_isWith() const { return false; }
        virtual bool rtti_isUnsafe() const { return false; }
        virtual bool rtti_isVar() const { return false; }
        virtual bool rtti_isR2V() const { return false; }
        virtual bool rtti_isRef2Ptr() const { return false; }
        virtual bool rtti_isPtr2Ref() const { return false; }
        virtual bool rtti_isCast() const { return false; }
        virtual bool rtti_isField() const { return false; }
        virtual bool rtti_isAsVariant() const { return false; }
        virtual bool rtti_isIsVariant() const { return false; }
        virtual bool rtti_isSafeAsVariant() const { return false; }
        virtual bool rtti_isSwizzle() const { return false; }
        virtual bool rtti_isSafeField() const { return false; }
        virtual bool rtti_isAt() const { return false; }
        virtual bool rtti_isSafeAt() const { return false; }
        virtual bool rtti_isOp1() const { return false; }
        virtual bool rtti_isOp2() const { return false; }
        virtual bool rtti_isOp3() const { return false; }
        virtual bool rtti_isNullCoalescing() const { return false; }
        virtual bool rtti_isValues() const { return false; }
        virtual bool rtti_isMakeBlock() const { return false; }
        virtual bool rtti_isMakeLocal() const { return false; }
        virtual bool rtti_isMakeStruct() const { return false; }
        virtual bool rtti_isMakeTuple() const { return false; }
        virtual bool rtti_isMakeVariant() const { return false; }
        virtual bool rtti_isMakeArray() const { return false; }
        virtual bool rtti_isIfThenElse() const { return false; }
        virtual bool rtti_isFor() const { return false; }
        virtual bool rtti_isWhile() const { return false; }
        virtual bool rtti_isAddr() const { return false; }
        virtual bool rtti_isLabel() const { return false; }
        virtual bool rtti_isGoto() const { return false; }
        virtual bool rtti_isFakeContext() const { return false; }
        virtual bool rtti_isFakeLineInfo() const { return false; }
        virtual bool rtti_isAscend() const { return false; }
        virtual bool rtti_isTypeDecl() const { return false; }
        virtual bool rtti_isNullPtr() const { return false; }
        virtual bool rtti_isCopy() const { return false; }
        virtual bool rtti_isClone() const { return false; }
        virtual Expression * tail() { return this; }
        virtual bool swap_tail ( Expression *, Expression * ) { return false; }
        virtual uint32_t getEvalFlags() const { return 0; }
        virtual void serialize ( AstSerializer & ser );
        LineInfo    at;
        TypeDeclPtr type;
        const char * __rtti = nullptr;
        union{
            struct {
                bool    alwaysSafe : 1;
                bool    generated : 1;
                bool    userSaidItsSafe : 1;
            };
            uint32_t    genFlags = 0;
        };
        union {
            struct {
                bool    constexpression : 1;
                bool    noSideEffects : 1;
                bool    noNativeSideEffects : 1;
                bool    isForLoopSource : 1;
                bool    isCallArgument : 1;
            };
            uint32_t    flags = 0;
        };
        union {
            struct {
                bool    topLevel :  1;
                bool    argLevel : 1;
                bool    bottomLevel : 1;
            };
            uint32_t    printFlags = 0;
        };
#if DAS_MACRO_SANITIZER
    public:
        void* operator new ( size_t count ) { return das_aligned_alloc16(count); }
        void operator delete  ( void* ptr ) { auto size = das_aligned_memsize(ptr);
            memset(ptr, 0xcd, size); das_aligned_free16(ptr); }
#endif
    };

    struct ExprLooksLikeCall;
    typedef function<ExprLooksLikeCall * (const LineInfo & info)> ExprCallFactory;

    template <typename ExprType>
    inline smart_ptr<ExprType> clonePtr ( const ExpressionPtr & expr ) {
        return expr ? static_pointer_cast<ExprType>(expr) : make_smart<ExprType>();
    }

    bool isLocalOrGlobal ( const ExpressionPtr & expr );

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4324)
#endif
    struct DAS_API ExprConst : Expression {
        ExprConst ( ) : baseType(Type::none) { __rtti = "ExprConst"; }
        ExprConst ( Type t ) : baseType(t) { __rtti = "ExprConst"; }
        ExprConst ( const LineInfo & a, Type t ) : Expression(a), baseType(t) { __rtti = "ExprConst"; }
        virtual SimNode * simulate (Context & context) const override;
        virtual bool rtti_isConstant() const override { return true; }
        template <typename QQ> QQ & cvalue() { return *((QQ *)&value); }
        template <typename QQ> const QQ & cvalue() const { return *((const QQ *)&value); }
        virtual void serialize ( AstSerializer & ser ) override;
        Type    baseType = Type::none;
        vec4f   value = v_zero();
        bool    foldedNonConst = false;
      };
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    template <typename TT, typename ExprConstExt>
    struct ExprConstT : ExprConst {
        ExprConstT ( TT val, Type bt ) : ExprConst(bt) {
            __rtti = "ExprConstT";
            value = cast<TT>::from(val);
        }
        ExprConstT ( const LineInfo & a, TT val, Type bt ) : ExprConst(a,bt) {
            __rtti = "ExprConstT";
            value = v_zero();
            memcpy(&value, &val, sizeof(TT));
        }
        virtual ExpressionPtr clone( const ExpressionPtr & expr ) const override {
            auto cexpr = clonePtr<ExprConstExt>(expr);
            Expression::clone(cexpr);
            cexpr->value = value;
            return cexpr;
        }
        virtual ExpressionPtr visit(Visitor & vis) override;
        TT getValue() const { return cast<TT>::to(value); }
    };

    enum class SideEffects : uint32_t {
        none =              0
    ,   unsafe =            (1<<0)
    ,   userScenario =      (1<<1)
    ,   modifyExternal =    (1<<2)
    ,   accessExternal =    modifyExternal
    ,   modifyArgument =    (1<<3)
    ,   accessGlobal =      (1<<4)
    ,   invoke =            (1<<5)
    ,   captureString =     (1<<6)
    ,   modifyArgumentAndExternal   = modifyArgument | modifyExternal
    ,   modifyArgumentAndAccessExternal = modifyArgument | accessExternal

    // use this as 'default' bind if you don't know what are side effects of your function, or if you don't undersand what are SideEffects
    // note: should this 'captureString' be here? C++ code which requires string and does not invoke is SO rare, that it is not worth to have it here
    ,   worstDefault =  modifyArgumentAndExternal

    ,   invokeAndAccessExternal = invoke | accessExternal
    ,   inferredSideEffects = uint32_t(SideEffects::modifyArgument) | uint32_t(SideEffects::accessGlobal) | uint32_t(SideEffects::invoke)
    };

    inline SideEffects operator |(SideEffects lhs, SideEffects rhs)
    {
      return static_cast<SideEffects>(
          static_cast<das::underlying_type<SideEffects>::type>(lhs) |
          static_cast<das::underlying_type<SideEffects>::type>(rhs));
    }

    typedef fragile_bit_set AstFuncLookup;

    struct InferHistory {
        LineInfo    at;
        Function *  func = nullptr;
        InferHistory() = default;
        InferHistory(const LineInfo & a, const FunctionPtr & p) : at(a), func(p.get()) {}
        void serialize ( AstSerializer & ser );
    };
    class DAS_API Function : public ptr_ref_count {
    public:
        enum class DescribeExtra     { no, yes };
        enum class DescribeModule    { no, yes };
    public:
        virtual ~Function() {}
        friend StringWriter& operator<< (StringWriter& stream, const Function & func);
        void getMangledName(TextWriter & ss) const;
        string getMangledName() const;
        uint64_t getMangledNameHash() const;
        VariablePtr findArgument(const string & name);
        SimNode * simulate (Context & context) const;
        virtual SimNode * makeSimNode ( Context & context, const vector<ExpressionPtr> & arguments );
        string describeName(DescribeModule moduleName = DescribeModule::no) const;
        string describe(DescribeModule moduleName = DescribeModule::no, DescribeExtra extra = DescribeExtra::no) const;
        virtual FunctionPtr visit(Visitor & vis);
        FunctionPtr setSideEffects ( SideEffects seFlags );
        bool isGeneric() const;
        FunctionPtr clone() const;
        string getLocationExtra() const;
        LineInfo getConceptLocation(const LineInfo & at) const;
        virtual string getAotBasicName() const { return name; }
        string getAotName(ExprCallFunc * call) const;
        string getAotArgumentPrefix(ExprCallFunc * call, int index) const;
        string getAotArgumentSuffix(ExprCallFunc * call, int index) const;
        FunctionPtr setAotTemplate();
        FunctionPtr setAnyTemplate();
        FunctionPtr setTempResult();
        FunctionPtr setCaptureString();
        FunctionPtr setNoDiscard();
        FunctionPtr setDeprecated(const string & message);
        FunctionPtr arg_init ( int argIndex, const ExpressionPtr & initValue ) {
            arguments[argIndex]->init = initValue;
            return this;
        }
        FunctionPtr arg_type ( int argIndex, const TypeDeclPtr & td ) {
            DAS_ASSERTF(!module, "cannot change argument type after function is added to module, use makeExtern/addToModule instead");
            arguments[argIndex]->type = td;
            return this;
        }
        FunctionPtr res_type ( const TypeDeclPtr & td ) {
            result = td;
            return this;
        }
        FunctionPtr addToModule ( Module & mod, SideEffects seFlags );
        FunctionPtr getOrigin() const;
        Function * getOriginPtr() const;
        void serialize ( AstSerializer & ser );
        void notInferred();
    public:
        AnnotationList      annotations;
        string              name;
        vector<VariablePtr> arguments;
        TypeDeclPtr         result;
        ExpressionPtr       body;
        int32_t             index = -1;
        uint32_t            totalStackSize = 0;
        int32_t             totalGenLabel = 0;
        LineInfo            at, atDecl;
        Module *            module = nullptr;
        AstFuncLookup       lookup;
        das_set<Function *>     useFunctions;
        das_set<Variable *>     useGlobalVariables;
        Structure *         classParent = nullptr;
    // this is what we use for alias checking
        vector<int>         resultAliases;
        vector<vector<int>> argumentAliases;
        struct AliasInfo {
            Variable *  var = nullptr;
            Function *  func = nullptr;
            bool        viaPointer = false;
            void serialize ( AstSerializer & ser );
        };
        vector<AliasInfo>  resultAliasesGlobals;
    // end of what we use for alias checking
        union {
            struct {
                bool    builtIn : 1;
                bool    policyBased : 1;
                bool    callBased : 1;
                bool    interopFn : 1;
                bool    hasReturn: 1;
                bool    copyOnReturn : 1;
                bool    moveOnReturn : 1;
                bool    exports : 1;

                bool    init : 1;
                bool    addr : 1;
                bool    used : 1;
                bool    fastCall : 1;
                bool    knownSideEffects : 1;
                bool    hasToRunAtCompileTime : 1;
                bool    unsafeOperation : 1;
                bool    unsafeDeref : 1;

                bool    hasMakeBlock : 1;
                bool    aotNeedPrologue : 1;
                bool    noAot : 1;
                bool    aotHybrid : 1;
                bool    aotTemplate : 1;
                bool    generated : 1;
                bool    privateFunction : 1;
                bool    generator : 1;

                bool    lambda : 1;
                bool    firstArgReturnType : 1;
                bool    noPointerCast : 1;
                bool    isClassMethod : 1;
                bool    isTypeConstructor : 1;
                bool    shutdown : 1;
                bool    anyTemplate : 1;
                bool    macroInit : 1;
            };
            uint32_t flags = 0;
        };

        union {
            struct {
                bool    macroFunction : 1;
                bool    needStringCast : 1;
                bool    aotHashDeppendsOnArguments : 1;
                bool    lateInit : 1;
                bool    requestJit : 1;
                bool    unsafeOutsideOfFor : 1;
                // empty spot
                bool    safeImplicit : 1;

                bool    deprecated : 1;
                bool    aliasCMRES : 1;
                bool    neverAliasCMRES : 1;
                bool    addressTaken : 1;
                bool    propertyFunction : 1;
                bool    pinvoke : 1;
                bool    jitOnly : 1;
                bool    isStaticClassMethod : 1;

                bool    requestNoJit : 1;
                bool    jitContextAndLineInfo : 1;
                bool    nodiscard : 1;
                bool    captureString : 1;
                bool    callCaptureString : 1;
                bool    hasStringBuilder : 1;
                bool    recursive : 1;              // this one is detected by the updateKeepAlive during the simulate, if enabled
                bool    isTemplate : 1;

                bool    unsafeWhenNotCloneArray : 1; // this one is used to mark functions which are unsafe when not cloning arrays
                bool    stub : 1;                    // skip stack allocation, optimizations, etc
                bool    lateShutdown : 1;
                bool    hasTryRecover : 1;           // has try { } recover { }
                bool    hasUnsafe : 1;               // has unsafe { }
                bool    isConstClassMethod : 1;      // method is const
            };
            uint32_t moreFlags = 0;
        };

        union {
            struct {
                bool unsafeFunction : 1;
                bool userScenario : 1;
                bool modifyExternal : 1;
                bool modifyArgument : 1;
                bool accessGlobal : 1;
                bool invoke : 1;
            };
            uint32_t    sideEffectFlags = 0;
        };

        union {
            struct {
                bool    aliasesResolved : 1;
                bool    stackResolved : 1;
            };
            uint32_t     stackFlags = 0;
        };

        vector<InferHistory> inferStack;
        FunctionPtr fromGeneric = nullptr;
        uint64_t hash = 0;
        uint64_t aotHash = 0;

        int32_t optimizationRound = 0;
        bool isFullyInferred = false;
        string inferredSource;

#if DAS_MACRO_SANITIZER
    public:
        void* operator new ( size_t count ) { return das_aligned_alloc16(count); }
        void operator delete  ( void* ptr ) { auto size = das_aligned_memsize(ptr);
            memset(ptr, 0xcd, size); das_aligned_free16(ptr); }
#endif
    };

    uint64_t getFunctionHash ( Function * fun, SimNode * node, Context * context );

    uint64_t getFunctionAotHash ( const Function * fun );
    string getAotHashComment ( const Function * fun );
    uint64_t getVariableListAotHash ( const vector<const Variable *> & globs, uint64_t initHash );

    class DAS_API BuiltInFunction : public Function {
    public:
        BuiltInFunction ( const char *fn, const char * fnCpp );
        virtual string getAotBasicName() const override {
            return cppName.empty() ? name : cppName;
        }
        FunctionPtr arg ( const char * argName ) {
            DAS_VERIFYF(arguments.size() == 1, "during a processing of the '%s' function\n", cppName.c_str());
            arguments[0]->name = argName;
            return this;
        }
        FunctionPtr args ( std::initializer_list<const char *> argList ) {
            if ( argList.size()==0 ) return this;
            DAS_VERIFYF(argList.size() == arguments.size(), "during a processing of the '%s' function\n", cppName.c_str());
            int argIndex = 0;
            for ( const char * arg : argList ) {
                arguments[argIndex++]->name = arg;
            }
            return this;
        }
        virtual void * getBuiltinAddress() const { return nullptr; }
    public:
        void construct (const vector<TypeDeclPtr> & args );
        void constructExternal (const vector<TypeDeclPtr> & args );
        void constructInterop (const vector<TypeDeclPtr> & args );
    public:
        string cppName;
    };

    template <typename RetT, typename ...Args>
    ___noinline vector<TypeDeclPtr> makeBuiltinArgs ( const ModuleLibrary & lib ) {
        return { makeType<RetT>(lib), makeArgumentType<Args>(lib)... };
    }

    struct TypeInfoMacro : public ptr_ref_count {
        TypeInfoMacro ( const string & n )
            : name(n) {
        }
        virtual void seal( Module * m ) { module = m; }
        virtual ExpressionPtr getAstChange ( const ExpressionPtr &, string & ) { return nullptr; }
        virtual TypeDeclPtr getAstType ( ModuleLibrary &, const ExpressionPtr &, string & ) { return nullptr; }
        virtual SimNode * simluate ( Context *, const ExpressionPtr &, string & ) { return nullptr; }
        virtual void aotPrefix ( TextWriter &, const ExpressionPtr & ) { }
        virtual void aotSuffix ( TextWriter &, const ExpressionPtr & ) { }
        virtual bool aotInfix ( TextWriter &, const ExpressionPtr & ) { return false; }
        virtual bool aotNeedTypeInfo ( const ExpressionPtr & ) const { return false; }
        virtual bool noAot ( const ExpressionPtr & ) const { return false; }
        string name;
        Module * module = nullptr;
    };
    typedef smart_ptr<TypeInfoMacro> TypeInfoMacroPtr;

    struct Error {
        Error () {}
        Error ( const string & w, const string & e, const string & f, LineInfo a, CompilationError ce )
            : what(w), extra(e), fixme(f), at(a), cerr(ce)  {}
        __forceinline bool operator < ( const Error & err ) const {
            if (at == err.at) {
                if (what == err.what) {
                    if (extra == err.extra) {
                        return fixme < err.fixme;
                    } else {
                        return extra < err.extra;
                    }
                } else {
                    return what < err.what;
                }
            } else {
                return at < err.at;
            }
        };
        string              what;
        string              extra;
        string              fixme;
        LineInfo            at;
        CompilationError    cerr = CompilationError::unspecified;
    };

    enum class ModuleAotType {
        no_aot,
        hybrid,
        cpp
    };

    enum VerifyBuiltinFlags {
        verifyAliasTypes = (1<<0),
        verifyHandleTypes = (1<<1),
        verifyGlobals = (1<<2),
        verifyFunctions = (1<<3),
        verifyGenerics = (1<<4),
        verifyStructures = (1<<5),
        verifyStructureFields = (1<<6),
        verifyEnums = (1<<7),
        verifyEnumFields = (1<<8),
        verifyAll = 0xffffffff
    };

    bool isValidBuiltinName ( const string & name, bool canPunkt = false );

    class DAS_API Module {
    public:
        Module ( const string & n = "" );
        void promoteToBuiltin(const FileAccessPtr & access);
        virtual ~Module();
        virtual void addPrerequisits ( ModuleLibrary & ) const {}
        virtual ModuleAotType aotRequire ( TextWriter & ) const { return ModuleAotType::no_aot; }
        virtual Type getOptionType ( const string & ) const;
        virtual bool initDependencies() { return true; }
        bool addAlias ( const TypeDeclPtr & at, bool canFail = false );
        bool addVariable ( const VariablePtr & var, bool canFail = false );
        bool addStructure ( const StructurePtr & st, bool canFail = false );
        bool removeStructure ( const StructurePtr & st );
        bool addEnumeration ( const EnumerationPtr & st, bool canFail = false );
        bool addFunction ( const FunctionPtr & fn, bool canFail = false );
        bool addGeneric ( const FunctionPtr & fn, bool canFail = false );
        bool addAnnotation ( const AnnotationPtr & ptr, bool canFail = false );
        void registerAnnotation ( const AnnotationPtr & ptr );
        bool addTypeInfoMacro ( const TypeInfoMacroPtr & ptr, bool canFail = false );
        bool addReaderMacro ( const ReaderMacroPtr & ptr, bool canFail = false );
        bool addTypeMacro ( const TypeMacroPtr & ptr, bool canFail = false );
        bool addCommentReader ( const CommentReaderPtr & ptr, bool canFail = false );
        bool addKeyword ( const string & kwd, bool needOxfordComma, bool canFail = false );
        bool addTypeFunction ( const string & name, bool canFail = false );
        TypeDeclPtr findAlias ( const string & name ) const;
        VariablePtr findVariable ( const string & name ) const;
        FunctionPtr findFunction ( const string & mangledName ) const;
        FunctionPtr findFunctionByMangledNameHash ( uint64_t hash ) const;
        FunctionPtr findGeneric ( const string & mangledName ) const;
        FunctionPtr findUniqueFunction ( const string & name ) const;
        StructurePtr findStructure ( const string & name ) const;
        StructurePtr findStructureByMangledNameHash ( uint64_t hash ) const;
        AnnotationPtr findAnnotation ( const string & name ) const;
        EnumerationPtr findEnum ( const string & name ) const;
        EnumerationPtr findEnumByMangledNameHash ( uint64_t hash ) const;
        ReaderMacroPtr findReaderMacro ( const string & name ) const;
        TypeInfoMacroPtr findTypeInfoMacro ( const string & name ) const;
        ExprCallFactory * findCall ( const string & name ) const;
        __forceinline bool isVisibleDirectly ( Module * objModule ) const {
            if ( objModule==this ) return true;
            if ( objModule->visibleEverywhere ) return true;
            return requireModule.find(objModule) != requireModule.end();
        }
        bool compileBuiltinModule ( const string & name, const unsigned char * const str, unsigned int str_len );//will replace last symbol to 0
        static Module * require ( const string & name );
        static Module * requireEx ( const string & name, bool allowPromoted );
        static void Initialize();
        static void CollectFileInfo(das::vector<FileInfoPtr> &accesses);
        static void Shutdown();
        static void Reset(bool debAg);
        static void ClearSharedModules();
        static void CollectSharedModules();
        static TypeAnnotation * resolveAnnotation ( const TypeInfo * info );
        static Type findOption ( const string & name );
        static void foreach(const callable<bool(Module * module)> & func);
        virtual uintptr_t rtti_getUserData() {return uintptr_t(0);}
        void verifyAotReady();
        void verifyBuiltinNames(uint32_t flags);
        void addDependency ( Module * mod, bool pub );
        void addBuiltinDependency ( ModuleLibrary & lib, Module * mod, bool pub = false );
        void serialize( AstSerializer & ser, bool already_exists );
        void setModuleName ( const string & n );
        FileInfo * getFileInfo() const;
    public:
        template <typename RecAnn>
        void initRecAnnotation ( const smart_ptr<RecAnn> & rec, ModuleLibrary & lib ) {
            rec->mlib = &lib;
            rec->init();
            rec->mlib = nullptr;
        }
    public:
        template <typename TT, typename ...TARG>
        __forceinline void addCall ( const string & fnName, TARG ...args ) {
            if ( callThis.find(fnName)!=callThis.end() ) {
                DAS_FATAL_ERROR("addCall(%s) failed. duplicate call\n", fnName.c_str());
            }
            callThis[fnName] = [fnName,args...](const LineInfo & at) {
                return new TT(at, fnName, args...);
            };
        }
        __forceinline bool addCallMacro ( const string & fnName, ExprCallFactory && factory ) {
            if ( callThis.find(fnName)!=callThis.end() ) {
                return false;
            }
            callThis[fnName] = das::move(factory);
            return true;
        }
    public:
        smart_ptr<Context>                          macroContext;
        safebox<TypeDecl>                           aliasTypes;
        safebox<Annotation>                         handleTypes;
        safebox<Structure>                          structures;
        safebox<Enumeration>                        enumerations;
        safebox<Variable>                           globals;
        safebox<Function>                           functions;          // mangled name 2 function name
        fragile_hash<vector<Function*>>             functionsByName;    // all functions of the same name
        safebox<Function>                           generics;           // mangled name 2 generic name
        fragile_hash<vector<Function*>>             genericsByName;     // all generics of the same name
        mutable das_map<string, ExprCallFactory>    callThis;
        das_map<string, TypeInfoMacroPtr>           typeInfoMacros;
        das_map<uint64_t, uint64_t>                 annotationData;
        das_hash_map<Module *,bool>                 requireModule;      // visibility modules
        vector<PassMacroPtr>                        macros;             // infer macros (clean infer, assume no errors)
        vector<PassMacroPtr>                        inferMacros;        // infer macros (dirty infer, assume half-way-there tree)
        vector<PassMacroPtr>                        optimizationMacros; // optimization macros
        vector<PassMacroPtr>                        lintMacros;         // lint macros (assume read-only)
        vector<PassMacroPtr>                        globalLintMacros;   // lint macros which work everywhere
        vector<VariantMacroPtr>                     variantMacros;      //  X is Y, X as Y expression handler
        vector<ForLoopMacroPtr>                     forLoopMacros;      // for loop macros (for every for loop)
        vector<CaptureMacroPtr>                     captureMacros;      // lambda capture macros
        vector<SimulateMacroPtr>                    simulateMacros;     // simulate macros (every time we simulate context)
        das_map<string,TypeMacroPtr>                typeMacros;         // type macros (every time we infer type)
        das_map<string,ReaderMacroPtr>              readMacros;         // %foo "blah"
        CommentReaderPtr                            commentReader;      // /* blah */ or // blah
        vector<pair<string,bool>>                   keywords;           // keywords (and if they need oxford comma)
        vector<string>                              typeFunctions;      // type functions
        das_hash_map<string,Type>                   options;            // options
        uint64_t                                    cumulativeHash = 0; // hash of all mangled names in this module (for builtin modules)
        string                                      name;
        uint64_t                                    nameHash = 0;
        string                                      fileName;           // where the module was found, if not built-in
        union {
            struct {
                bool    builtIn : 1;
                bool    promoted : 1;
                bool    isPublic : 1;
                bool    isModule : 1;
                bool    isSolidContext : 1;
                bool    fromExtraDependency : 1;
                bool    doNotAllowUnsafe : 1;
                bool    wasParsedNameless : 1;
                bool    visibleEverywhere : 1;
                bool    allowPodInscope : 1;
            };
            uint32_t        moduleFlags = 0;
        };
    private:
        Module * next = nullptr;
        unique_ptr<FileInfo>    ownFileInfo;
        FileAccessPtr           promotedAccess;
    };

    #define REGISTER_MODULE(ClassName) \
        DAS_EXPORT_DLL das::Module * register_##ClassName () { \
            das::daScriptEnvironment::ensure(); \
            ClassName * module_##ClassName = new ClassName(); \
            return module_##ClassName; \
        }

    #if DAS_ENABLE_DLL
        #define REGISTER_DYN_MODULE(ClassName, ExtName) \
        extern "C" { \
            DAS_EXPORT_DLL das::Module * register_dyn_##ExtName ( int buildId ) { \
                if ( buildId != DAS_BUILD_ID ) return nullptr; \
                das::daScriptEnvironment::ensure(); \
                ClassName * module_##ClassName = new ClassName(); \
                return module_##ClassName; \
            } \
        }
    #else
        #define REGISTER_DYN_MODULE(ClassName, ExtName)
    #endif

    static inline string getDynModuleRegistratorName(const string &mod) {
        return "register_dyn_" + mod;
    }

    #define REGISTER_MODULE_IN_NAMESPACE(ClassName,Namespace) \
        DAS_EXPORT_DLL das::Module * register_##ClassName () { \
            das::daScriptEnvironment::ensure(); \
            Namespace::ClassName * module_##ClassName = new Namespace::ClassName(); \
            return module_##ClassName; \
        }

    using module_pull_t = das::Module*(*)();
    struct ModulePullHelper
    {
        ModulePullHelper(module_pull_t pull);
    };

    void pull_all_auto_registered_modules();

    #define AUTO_REGISTER_MODULE(ClassName) \
        REGISTER_MODULE(ClassName)          \
        static das::ModulePullHelper ClassName##RegisterHelper(&register_##ClassName);

    #define AUTO_REGISTER_MODULE_IN_NAMESPACE(ClassName,Namespace) \
        REGISTER_MODULE_IN_NAMESPACE(ClassName, Namespace)         \
        static das::ModulePullHelper ClassName##RegisterHelper(&register_##ClassName);

    class ModuleDas : public Module {
    public:
        ModuleDas(const string & n = "") : Module(n) {}
        virtual ModuleAotType aotRequire ( TextWriter & ) const { return ModuleAotType::cpp; }
    };

    class DAS_API ModuleLibrary {
        friend class Module;
        friend class Program;
    public:
        ModuleLibrary() {}
        ModuleLibrary( Module * this_module );
        virtual ~ModuleLibrary() {};
        void addBuiltInModule ();
        bool addModule ( Module * module );
        void foreach ( const callable<bool (Module * module)> & func, const string & name ) const;
        void foreach_in_order ( const callable<bool (Module * module)> & func, Module * thisM ) const;
        void findWithCallback ( const string & name, Module * inWhichModule, const callable<void (Module * pm, const string &name, Module * inWhichModule)> & func ) const;
        void findAlias ( vector<TypeDeclPtr> & ptr, Module * pm, const string & aliasName, Module * inWhichModule ) const;
        vector<TypeDeclPtr> findAlias ( const string & name, Module * inWhichModule ) const;
        void findAnnotation ( vector<AnnotationPtr> & ptr, Module * pm, const string & annotationName, Module * inWhichModule ) const;
        vector<AnnotationPtr> findAnnotation ( const string & name, Module * inWhichModule ) const;
        vector<AnnotationPtr> findStaticAnnotation ( const string & name ) const;
        vector<TypeInfoMacroPtr> findTypeInfoMacro ( const string & name, Module * inWhichModule ) const;
        void findEnum ( vector<EnumerationPtr> & ptr, Module * pm, const string & enumName, Module * inWhichModule ) const;
        vector<EnumerationPtr> findEnum ( const string & name, Module * inWhichModule ) const;
        void findStructure ( vector<StructurePtr> & ptr, Module * pm, const string & funcName, Module * inWhichModule ) const;
        vector<StructurePtr> findStructure ( const string & name, Module * inWhichModule ) const;
        Module * findModule ( const string & name ) const;
        Module * findModuleByMangledNameHash ( uint64_t hash ) const;
        TypeDeclPtr makeStructureType ( const string & name ) const;
        TypeDeclPtr makeHandleType ( const string & name ) const;
        TypeDeclPtr makeEnumType ( const string & name ) const;
        Module* front() const { return modules.front(); }
        Module* back() const { return modules.back(); }
        vector<Module *> & getModules() { return modules; }
        Module* getThisModule() const { return thisModule; }
        void reset();
        void renameModule ( Module * module, const string & newName );
    protected:
        vector<Module *>                modules;
        safebox_map<Module *>           moduleLookupByHash;
        Module *                        thisModule = nullptr;
    };

    struct DAS_API ModuleGroupUserData {
        ModuleGroupUserData ( const string & n ) : name(n) {}
        virtual ~ModuleGroupUserData() {}
        string name;
    };
    typedef shared_ptr<ModuleGroupUserData> ModuleGroupUserDataPtr;

    class DAS_API ModuleGroup : public ModuleLibrary {
    public:
        virtual ~ModuleGroup();
        ModuleGroupUserData * getUserData ( const string & dataName ) const;
        bool setUserData ( ModuleGroupUserData * data );
        void collectMacroContexts();
    protected:
        das_map<string,ModuleGroupUserDataPtr>  userData;
    };
    template <> struct isCloneable<ModuleGroup> : false_type {};

    struct PassMacro : ptr_ref_count {
        PassMacro ( const string na = "" ) : name(na) {}
        virtual bool apply( Program *, Module * ) { return false; }
        string name;
    };

    struct ExprReader;
    struct ReaderMacro : ptr_ref_count {
        ReaderMacro ( const string na = "" ) : name(na) {}
        virtual bool accept ( Program *, Module *, ExprReader *, int, const LineInfo & ) { return false; }
        virtual char * suffix ( Program *, Module *, ExprReader *, int &, FileInfo * &, const LineInfo & ) { return nullptr; }
        virtual ExpressionPtr visit (  Program *, Module *, ExprReader * ) { return nullptr; }
        virtual void seal( Module * m ) { module = m; }
        string name;
        Module * module = nullptr;
    };

    struct ExprCallMacro;
    struct CallMacro : ptr_ref_count {
        CallMacro ( const string & na = "" ) : name(na) {}
        virtual void preVisit (  Program *, Module *, ExprCallMacro * ) { }
        virtual ExpressionPtr visit (  Program *, Module *, ExprCallMacro * ) { return nullptr; }
        virtual void seal( Module * m ) { module = m; }
        virtual bool canVisitArguments ( ExprCallMacro *, int ) { return true; }
        virtual bool canFoldReturnResult ( ExprCallMacro * ) { return true; }
        string name;
        Module * module = nullptr;
    };

    struct ExprFor;
    struct ForLoopMacro : ptr_ref_count {
        ForLoopMacro ( const string & na = "" ) : name(na) {}
        virtual ExpressionPtr visit ( Program *, Module *, ExprFor * ) { return nullptr; }
        string name;
    };

    struct CaptureMacro : ptr_ref_count {
        CaptureMacro ( const string & na = "" ) : name(na) {}
        virtual ExpressionPtr captureExpression ( Program *, Module *, Expression *, TypeDecl * ) { return nullptr; }
        virtual void captureFunction ( Program *, Module *, Structure *, Function * ) { }
        virtual void releaseFunction ( Program *, Module *, Structure *, Function * ) { }
        string name;
    };

    struct TypeMacro : ptr_ref_count {
        TypeMacro ( const string & na = "" ) : name(na) {}
        virtual TypeDeclPtr visit ( Program *, Module *, const TypeDeclPtr &, const TypeDeclPtr & ) { return nullptr; }
        string name;
    };

    struct ExprIsVariant;
    struct ExprAsVariant;
    struct ExprSafeAsVariant;
    struct VariantMacro : ptr_ref_count {
        VariantMacro ( const string na = "" ) : name(na) {}
        virtual ExpressionPtr visitIs     (  Program *, Module *, ExprIsVariant * ) { return nullptr; }
        virtual ExpressionPtr visitAs     (  Program *, Module *, ExprAsVariant * ) { return nullptr; }
        virtual ExpressionPtr visitSafeAs (  Program *, Module *, ExprSafeAsVariant * ) { return nullptr; }
        string name;
    };

    struct SimulateMacro : ptr_ref_count {
        SimulateMacro ( const string na = "" ) : name(na) {}
        virtual bool preSimulate ( Program *, Context * ) { return true; }
        virtual bool simulate ( Program *, Context * ) { return true; }
        string name;
    };

    class DAS_API DebugInfoHelper : public ptr_ref_count {
    public:
        DebugInfoHelper () { debugInfo = make_shared<DebugInfoAllocator>(); }
        DebugInfoHelper ( const shared_ptr<DebugInfoAllocator> & di ) : debugInfo(di) {}
    public:
        TypeInfo * makeTypeInfo ( TypeInfo * info, const TypeDeclPtr & type );
        VarInfo * makeVariableDebugInfo ( const Variable & var );
        VarInfo * makeVariableDebugInfo ( const Structure & st, const Structure::FieldDeclaration & var );
        StructInfo * makeStructureDebugInfo ( const Structure & st );
        FuncInfo * makeFunctionDebugInfo ( const Function & fn );
        EnumInfo * makeEnumDebugInfo ( const Enumeration & en );
        FuncInfo * makeInvokeableTypeDebugInfo ( const TypeDeclPtr & blk, const LineInfo & at );
        void appendLocalVariables ( FuncInfo * info, const ExpressionPtr & body );
        void appendGlobalVariables ( FuncInfo * info, const FunctionPtr & body );
        void logMemInfo ( TextWriter & tw );
    public:
        shared_ptr<DebugInfoAllocator>  debugInfo;
        bool                            rtti = false;
    public:
        das_hash_map<string,StructInfo *>        smn2s;
        das_hash_map<string,TypeInfo *>          tmn2t;
        das_hash_map<string,VarInfo *>           vmn2v;
        das_hash_map<string,FuncInfo *>          fmn2f;
        das_hash_map<string,EnumInfo *>          emn2e;

        das_hash_map<TypeInfo *,string>          t2cppTypeName;
        das_hash_map<StructInfo *,string>        s2cppTypeName;
    };

    struct CodeOfPolicies {
    // Aot config
        bool        aot = false;                        // enable AOT
        /*option*/ bool        aot_lib = false;
        /*option*/ bool        standalone_context = false;         // generate standalone context class in aot mode
        bool        aot_module = false;                 // this is how AOT tool knows module is module, and not an entry point
        bool        aot_macros = false;                 // enables aot of macro code (like 'qmacro_block')
        bool        paranoid_validation = false;        // todo
        bool        cross_platform = false;             // aot supports platform independent mode
        string      aot_result;                         // Path where to store cpp-result of aot
    // End aot config
        bool        completion = false;                 // this code is being compiled for 'completion' mode
        bool        export_all = false;                 // when user compiles, export all (public?) functions
        bool        serialize_main_module = true;       // if false, then we recompile main module each time
        bool        keep_alive = false;                 // produce keep-alive noodes
        /*option*/ bool        very_safe_context = false;          // context is very safe (does not release old memory from array or table grow, leaves it to GC)
    // error reporting
        int32_t     always_report_candidates_threshold = 6; // always report candidates if there are less than this number
    // infer passes
        /*option*/ int32_t     max_infer_passes = 50;              // maximum number of infer passes
        /*option*/ int32_t     max_call_depth = 50;                // maximum call expression nesting depth during inference
        bool verify_infer_types = false;                       // verify inferred types (to see if there is internal consistency). note - this adds errors to failing tests
    // memory
        /*option*/ uint32_t    stack = 16*1024;                    // 0 for unique stack
        /*option*/ bool        intern_strings = false;             // use string interning lookup for regular string heap
        /*option*/ bool        persistent_heap = false;
        /*option*/ bool        multiple_contexts = false;          // code supports context safety
        /*option*/ uint32_t    heap_size_hint = 65536;
        /*option*/ uint32_t    string_heap_size_hint = 65536;
        /*option*/ bool        solid_context = false;              // all access to varable and function lookup to be context-dependent (via index)
                                                        // this is slightly faster, but prohibits AOT or patches
        bool        macro_context_persistent_heap = true;   // if true, then persistent heap is used for macro context
        bool        macro_context_collect = false;          // GC collect macro context after major passes
        uint64_t    max_static_variables_size = 0x100000000;   // 4GB
        /*option*/ uint64_t    max_heap_allocated = 0;
        /*option*/ uint64_t    max_string_heap_allocated = 0;
    // rtti
        /*option*/ bool rtti = false;                              // create extended RTTI
    // language
        /*option*/ bool unsafe_table_lookup = false;                // table lookup (tab[key]) to be unsafe
        /*option*/ bool relaxed_pointer_const = false;             // allow const correctness to be relaxed on pointers
        bool version_2_syntax = false;                  // use syntax version 2
        bool gen2_make_syntax = false;                  // only new make syntax is allowed (no [[...]] or [{...}])
        /*option*/ bool relaxed_assign = true;                     // allow = to <- substitution, in certain expressions
        bool no_unsafe = false;
        bool local_ref_is_unsafe = true;                // var a & = ... unsafe. should be
        /*option*/ bool no_global_variables = false;
        /*option*/ bool no_global_variables_at_all = false;
        /*option*/ bool no_global_heap = false;
        /*option*/ bool only_fast_aot = false;
        /*option*/ bool aot_order_side_effects = false;
        /*option*/ bool no_unused_function_arguments = false;
        /*option*/ bool no_unused_block_arguments = false;
        bool allow_block_variable_shadowing = false;
        bool allow_local_variable_shadowing = false;
        bool allow_shared_lambda = false;
        bool ignore_shared_modules = false;
        bool default_module_public = true;              // by default module is 'public', not 'private'
        /*option*/ bool no_deprecated = false;
        /*option*/ bool no_aliasing = false;                       // if true, aliasing will be reported as error, otherwise will turn off optimization
        /*option*/ bool strict_smart_pointers = true;              // collection of tests for smart pointers, like van inscope for any local, etc
        /*option*/ bool no_init = false;                           // if true, then no [init] is allowed in any shape or form
        /*option*/ bool strict_unsafe_delete = false;              // if true, delete of type which contains 'unsafe' delete is unsafe // TODO: enable when need be
        bool no_members_functions_in_struct = false;    // structures can't have member functions
        /*option*/ bool no_local_class_members = true;             // members of the class can't be classes
        /*option*/ bool report_invisible_functions = true;         // report invisible functions (report functions not visible from current module)
        /*option*/ bool report_private_functions = true;           // report private functions (report functions which are not accessible due to private module)
        /*option*/ bool no_unsafe_uninitialized_structures = true; // if true, then unsafe uninitialized structures are not allowed
        /*option*/ bool strict_properties = false;                 // if true, then properties are strict, i.e. a.prop = b does not get promoted to a.prop := b
        /*option*/ bool no_writing_to_nameless = true;             // if true, then writing to nameless variables (intermediate on the stack) is not allowed
        /*option*/ bool always_call_super = false;                  // if true, then super() needs to be called from every class constructor
    // environment
        /*option*/ bool no_optimizations = false;                  // disable optimizations, regardless of settings
        /*option*/ bool no_infer_time_folding = false;             // disable infer-time constant folding
        bool fail_on_no_aot = true;                     // AOT link failure is error
        bool fail_on_lack_of_aot_export = false;        // remove_unused_symbols = false is missing in the module, which is passed to AOT
        /*option*/ bool log_compile_time = false;                  // if true, then compile time will be printed at the end of the compilation
        /*option*/ bool log_total_compile_time = false;            // if true, then detailed compile time will be printed at the end of the compilation
        /*option*/ bool no_fast_call = false;                      // disable fastcall
        /*option*/ bool scoped_stack_allocator = true;             // reuse stack memory after variables out of scope
        /*option*/ bool force_inscope_pod = false;                 // force in-scope for POD-like types
        /*option*/ bool log_inscope_pod = false;                   // log in-scope for POD-like types
    // debugger
        //  when enabled
        //      1. disables [fastcall]
        //      2. invoke of blocks will have extra prologue overhead
        //      3. context always has context mutex
        bool debugger = false;
        /*option*/ bool debug_infer_flag = false;  // set this to true to debug macros for missing "not_inferred"
    // profiler
        // only enabled if profiler is disabled
        // when enabled
        //      1. disables [fastcall]
        bool profiler = false;
    // pinvoke
        /*option*/ bool threadlock_context = false;               // has context mutex
    // jit
        bool jit_enabled = false;                // enable JIT
        // todo: add this params to serialization?
        bool jit_jit_all_functions = true;       // JIT all functions by default
        bool jit_debug_info = false;             // Add debug info to generate binary code
        bool jit_dll_mode = true;                // Create if missing and reuse DLL or JIT compile
        bool jit_exe_mode = false;                // Create executable
        bool jit_emit_prologue = false;          // Emit prologue for all functions and blocks
        string jit_output_path;                  // Folder to store compiled dll's. By default it'll be _das_root_/.jitted_scripts
        int32_t jit_opt_level = 3u;              // Opt level for LLVM to codegen and IR optimizations
        int32_t jit_size_level = 3u;             // Opt level for LLVM for binary size
        string jit_path_to_shared_lib;           // Path to libDaScript. Optional, we'll try to find it in _das_root_/lib/ if not provided.
        string jit_path_to_linker;               // Path to linker. Optional, we'll use clang-cl from LLVM on Windows and cc otherwise.
    // dll loading
        vector<string> dll_search_paths;          // additional search paths for dll loading
    // one-liners
        /*option*/ bool temp_one_liner_warning = false;
        /*option*/ bool temp_table_lint_warning = false;
    };

    struct CommentReader : public ptr_ref_count {
        virtual void open ( bool cppStyle, const LineInfo & at ) = 0;
        virtual void accept ( int Ch, const LineInfo & at ) = 0;
        virtual void close ( const LineInfo & at ) = 0;
        virtual void beforeStructure ( const LineInfo & at ) = 0;
        virtual void afterStructure ( Structure * st, const LineInfo & at ) = 0;
        virtual void beforeStructureFields ( const LineInfo & at ) = 0;
        virtual void afterStructureFields ( const LineInfo & at ) = 0;
        virtual void afterStructureField ( const char * name, const LineInfo & at ) = 0;
        virtual void beforeFunction ( const LineInfo & at ) = 0;
        virtual void afterFunction ( Function * fun, const LineInfo & at ) = 0;
        virtual void beforeGlobalVariables ( const LineInfo & at ) = 0;
        virtual void afterGlobalVariable ( const char * name, const LineInfo & at ) = 0;
        virtual void afterGlobalVariables ( const LineInfo & at ) = 0;
        virtual void beforeVariant ( const LineInfo & at ) = 0;
        virtual void beforeVariantEntries ( const LineInfo & at ) = 0;
        virtual void afterVariantEntry ( const char * name, const LineInfo & at ) = 0;
        virtual void afterVariantEntries ( const LineInfo & at ) = 0;
        virtual void afterVariant ( const char * name, const LineInfo & at ) = 0;
        virtual void beforeTuple ( const LineInfo & at ) = 0;
        virtual void beforeTupleEntries ( const LineInfo & at ) = 0;
        virtual void afterTupleEntry ( const char * name, const LineInfo & at ) = 0;
        virtual void afterTupleEntries ( const LineInfo & at ) = 0;
        virtual void afterTuple ( const char * name, const LineInfo & at ) = 0;
        virtual void beforeBitfield ( const LineInfo & at ) = 0;
        virtual void beforeBitfieldEntries ( const LineInfo & at ) = 0;
        virtual void afterBitfieldEntry ( const char * name, const LineInfo & at ) = 0;
        virtual void afterBitfieldEntries ( const LineInfo & at ) = 0;
        virtual void afterBitfield ( const char * name, const LineInfo & at ) = 0;
        virtual void beforeEnumeration ( const LineInfo & at ) = 0;
        virtual void beforeEnumerationEntries ( const LineInfo & at ) = 0;
        virtual void afterEnumerationEntry ( const char * name, const LineInfo & at ) = 0;
        virtual void afterEnumerationEntries ( const LineInfo & at ) = 0;
        virtual void afterEnumeration ( const char * name, const LineInfo & at ) = 0;
        virtual void beforeAlias ( const LineInfo & at ) = 0;
        virtual void afterAlias ( const char * name, const LineInfo & at ) = 0;
    };

    typedef function<void (const TypeDeclPtr & argType, const TypeDeclPtr & passType)> UpdateAliasMapCallback;

    class DAS_API Program : public ptr_ref_count {
    public:
        Program();
        int getContextStackSize() const;
        friend DAS_API StringWriter& operator<< (StringWriter& stream, const Program & program);
        vector<StructurePtr> findStructure ( const string & name ) const;
        vector<AnnotationPtr> findAnnotation ( const string & name ) const;
        vector<TypeInfoMacroPtr> findTypeInfoMacro ( const string & name ) const;
        vector<EnumerationPtr> findEnum ( const string & name ) const;
        vector<TypeDeclPtr> findAlias ( const string & name ) const;
        bool addAlias ( const TypeDeclPtr & at );
        bool addVariable ( const VariablePtr & var );
        bool addStructure ( const StructurePtr & st );
        bool addEnumeration ( const EnumerationPtr & st );
        bool addStructureHandle ( const StructurePtr & st, const TypeAnnotationPtr & ann, const AnnotationArgumentList & arg );
        bool addFunction ( const FunctionPtr & fn );
        FunctionPtr findFunction(const string & mangledName) const;
        bool addGeneric ( const FunctionPtr & fn );
        Module * addModule ( const string & name );
        void finalizeAnnotations();
        bool patchAnnotations();
        void fixupAnnotations();
        void normalizeOptionTypes ();
        void inferTypes(TextWriter & logs, ModuleGroup & libGroup);
        void inferTypesDirty(TextWriter & logs, bool verbose);
        bool relocatePotentiallyUninitialized(TextWriter & logs);
        void lint (TextWriter & logs, ModuleGroup & libGroup );
        void inferLint(TextWriter & logs);
        void checkSideEffects();
        void foldUnsafe();
        bool optimizationRefFolding(int32_t round);
        bool optimizationConstFolding(int32_t round);
        bool optimizationBlockFolding(int32_t round);
        bool optimizationCondFolding(int32_t round);
        bool optimizationUnused(TextWriter & logs, int32_t round);
        void fusion ( Context & context, TextWriter & logs );
        void buildAccessFlags(TextWriter & logs);
        bool verifyAndFoldContracts();
        void optimize(TextWriter & logs, ModuleGroup & libGroup);
        bool inScopePodAnalysis(TextWriter & logs);
        void markSymbolUse(bool builtInSym, bool forceAll, bool initThis, Module * macroModule, TextWriter * logs = nullptr);
        void markModuleSymbolUse(TextWriter * logs = nullptr);
        void markMacroSymbolUse(TextWriter * logs = nullptr);
        void markExecutableSymbolUse(TextWriter * logs = nullptr);
        void markFoldingSymbolUse(const vector<Function *> & needRun, TextWriter * logs = nullptr);
        void removeUnusedSymbols();
        void clearSymbolUse();
        void dumpSymbolUse(TextWriter & logs);
        void allocateStack(TextWriter & logs, bool permanent, bool everything);
        void deriveAliases(TextWriter & logs, bool permanent, bool everything);
        void updateSemanticHash();
        void updateKeepAliveFlags();
        bool simulate ( Context & context, TextWriter & logs, StackAllocator * sharedStack = nullptr );
        uint64_t getInitSemanticHashWithDep( uint64_t initHash );
        void error ( const string & str, const string & extra, const string & fixme, const LineInfo & at, CompilationError cerr = CompilationError::unspecified );
        bool failed() const { return failToCompile || macroException; }
        static ExpressionPtr makeConst ( const LineInfo & at, const TypeDeclPtr & type, vec4f value );
        ExprLooksLikeCall * makeCall ( const LineInfo & at, const string & name );
        ExprLooksLikeCall * makeCall ( const LineInfo & at, const LineInfo & atEnd, const string & name );
        TypeDecl * makeTypeDeclaration ( const LineInfo & at, const string & name );
        StructurePtr visitStructure(Visitor & vis, Structure *);
        EnumerationPtr visitEnumeration(Visitor & vis, Enumeration *);
        void visitModule(Visitor & vis, Module * thatModule, bool visitGenerics = false, bool sortStructures = false);
        void visitModulesInOrder(Visitor & vis, bool visitGenerics = false);
        void visitModules(Visitor & vis, bool visitGenerics = false);
        void visit(Visitor & vis, bool visitGenerics = false, bool sortStructures = false);
        void setPrintFlags();
        void aotCpp ( Context & context, TextWriter & logs, bool cross_platform = false );
        void registerAotCpp ( TextWriter & logs, Context & context, bool headers = true, bool allModules = false );
        void validateAotCpp ( TextWriter & logs, Context & context );
        void buildMNLookup ( Context & context, const vector<FunctionPtr> & lookupFunctions, TextWriter & logs );
        void buildGMNLookup ( Context & context, TextWriter & logs );
        void buildADLookup ( Context & context, TextWriter & logs );
        bool getOptimize() const;
        bool getDebugger() const;
        bool getProfiler() const;
        Module *getThisModule() const { return thisModule.get(); }
        void makeMacroModule( TextWriter & logs );
        vector<ReaderMacroPtr> getReaderMacro ( const string & markup ) const;
        void serialize ( AstSerializer & ser );
    protected:
        // this is no longer the way to link AOT
        //  set CodeOfPolicies::aot instead
        void linkCppAot ( Context & context, AotLibrary & aotLib, TextWriter & logs );
        void linkError ( const string & str, const string & extra );
    public:
        UpdateAliasMapCallback updateAliasMapCallback;
    public:
        template <typename TT>
        string describeCandidates ( const TT & result, bool needHeader = true ) const {
            if ( !result.size() ) return "";
            TextWriter ss;
            if ( needHeader ) ss << "candidates are:";
            for ( auto & fn : result ) {
                ss << "\n\t";
                if ( fn->module && !fn->module->name.empty() && !(fn->module->name=="builtin") )
                    ss << fn->module->name << "::";
                ss << fn->describe();
            }
            return ss.str();
        }
    public:
        string                      thisNamespace;
        string                      thisModuleName;
        unique_ptr<Module>          thisModule;
        ModuleLibrary               library;
        ModuleGroup *               thisModuleGroup = nullptr;
        int                         totalFunctions = 0;
        int                         totalVariables = 0;
        int                         newLambdaIndex = 1;
        vector<Error>               errors;
        vector<Error>               aotErrors;
        uint32_t                    globalInitStackSize = 0;
        uint32_t                    globalStringHeapSize = 0;
        bool                        folding = false;
        bool                        reportingInferErrors = false;
        uint64_t                    initSemanticHashWithDep = 0;
        union {
            struct {
                bool    failToCompile : 1;
                bool    unsafe : 1;
                bool    isCompiling : 1;
                bool    isSimulating : 1;
                bool    isCompilingMacros : 1;
                bool    needMacroModule : 1;
                bool    promoteToBuiltin : 1;
                bool    isDependency : 1;
                bool    macroException : 1;
            };
            uint32_t    flags = 0;
        };
        das_map<CompilationError,int>   expectErrors;
        AnnotationArgumentList      options;
        CodeOfPolicies              policies;
        vector<tuple<Module *,string,string,bool,LineInfo>> allRequireDecl;
        das_hash_map<uint64_t,TypeDecl *> astTypeInfo;
    };

    // module parsing routines
    string getModuleName ( const string & nameWithDots );
    string getModuleFileName ( const string & nameWithDots );

    // template name parsing routines
    bool starts_with ( const string & name, const char * template_name );

    // access function from class adapter
    DAS_API int adapt_field_offset ( const char * fName, const StructInfo * info );
    DAS_API int adapt_field_offset_ex ( const char * fName, const StructInfo * info, uint32_t & i );
    DAS_API char * adapt_field ( const char * fName, char * pClass, const StructInfo * info );
    DAS_API Func adapt ( const char * funcName, char * pClass, const StructInfo * info );

    // this one works for single module only
    DAS_API ProgramPtr parseDaScript ( const string & fileName, const string & moduleName, const FileAccessPtr & access,
        TextWriter & logs, ModuleGroup & libGroup, bool exportAll = false, bool isDep = false, CodeOfPolicies policies = CodeOfPolicies() );

    // this one collectes dependencies and compiles with modules
    DAS_API ProgramPtr compileDaScript ( const string & fileName, const FileAccessPtr & access,
        TextWriter & logs, ModuleGroup & libGroup, CodeOfPolicies policies = CodeOfPolicies() );
    DAS_API ProgramPtr compileDaScriptSerialize ( const string & fileName, const FileAccessPtr & access,
        TextWriter & logs, ModuleGroup & libGroup, CodeOfPolicies policies = CodeOfPolicies() );

    // collect script prerequisits
    DAS_API bool getPrerequisits ( const string & fileName,
                          const FileAccessPtr & access,
                          string &modName,
                          vector<ModuleInfo> & req,
                          vector<MissingRecord> & missing,
                          vector<RequireRecord> & circular,
                          vector<RequireRecord> & notAllowed,
                          vector<FileInfo *> & chain,
                          das_set<string> & dependencies,
                          das_hash_map<string, NamelessModuleReq> & namelessReq,
                          vector<NamelessMismatch> & namelessMismatches,
                          ModuleGroup & libGroup,
                          TextWriter * log,
                          int tab,
                          bool allowPromoted,
                          const string & reqNameHint = "",
                          int32_t reqLineHint = 0 );

    void getAllRequireReq ( FileInfo * fi, const FileAccessPtr & access, das::string &modName, vector<RequireRecord> & req, vector<FileInfo *> & chain, das_set<FileInfo *> & collected );


    // note: this has sifnificant performance implications
    //      i.e. this is ok for the load time \ map time
    //      it is not ok to use for every call
    template <typename ReturnType, typename ...Args>
    inline bool verifyCall ( FuncInfo * info, const ModuleLibrary & lib ) {
        vector<TypeDeclPtr> args = { makeType<Args>(lib)... };
        if ( args.size() != info->count ) {
            return false;
        }
        DebugInfoHelper helper;
        for ( uint32_t index=0, indexs=info->count; index!=indexs; ++ index ) {
            auto argType = info->fields[index];
            if ( argType->type==Type::anyArgument ) {
                continue;
            }
            auto passType = helper.makeTypeInfo(nullptr, args[index]);
            if ( !isValidArgumentType(argType, passType) ) {
                return false;
            }
        }
        // ok, now for the return type
        auto resType = helper.makeTypeInfo(nullptr, makeType<ReturnType>(lib));
        if ( !isValidArgumentType(resType, info->result) ) {
            return false;
        }
        return true;
    }

    struct DynamicModuleInfo {
        ~DynamicModuleInfo();
        string name;
        vector<void*> dll_handlers;         // all c++ binded dlls.
        vector<pair<string, string>> paths; // from -> to
    };

    struct DAS_API daScriptEnvironment {
        ProgramPtr      g_Program;
        bool            g_isInAot = false;
        Module *        modules = nullptr;
        int             das_def_tab_size = 4;
        bool            g_resolve_annotations = true;
        TextWriter *    g_compilerLog = nullptr;
        const char *    g_compilingFileName = nullptr;
        const char *    g_compilingModuleName = nullptr;
        int64_t         macroTimeTicks = 0;
        AstSerializer * serializer_read = nullptr;
        AstSerializer * serializer_write = nullptr;

        vector<DynamicModuleInfo> *g_dyn_modules_resolve = nullptr;
        inline static DAS_THREAD_LOCAL(DebugAgentInstance *) g_threadLocalDebugAgent;
        uint64_t        dataWalkerStringLimit = 0;

        static daScriptEnvironment *getBound();
        static void setBound(daScriptEnvironment *bnd);
        static daScriptEnvironment *exchangeBound(daScriptEnvironment *bnd);

        static daScriptEnvironment *getOwned();
        static void setOwned(daScriptEnvironment *bnd);
        static daScriptEnvironment *exchangeOwned(daScriptEnvironment *bnd);

        static void ensure();

    /*private:*/
        inline static DAS_THREAD_LOCAL(daScriptEnvironment *) bound;
        inline static DAS_THREAD_LOCAL(daScriptEnvironment *) owned;
    };

    struct DAS_API daScriptEnvironmentGuard {
        das::daScriptEnvironment *initialBound;
        das::daScriptEnvironment *initialOwned;

        daScriptEnvironmentGuard(das::daScriptEnvironment *bound = nullptr, das::daScriptEnvironment *owned = nullptr);
        ~daScriptEnvironmentGuard();
    };
}
