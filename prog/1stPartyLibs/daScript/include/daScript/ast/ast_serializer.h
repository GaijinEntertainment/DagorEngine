#pragma once

#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast.h"

namespace das {
    struct ForReading {};

    struct AstSerializer {
        ~AstSerializer ();
        AstSerializer ( void );
        AstSerializer ( ForReading, vector<uint8_t> && buffer_ );

        AstSerializer ( const AstSerializer & from ) = delete;
        AstSerializer ( AstSerializer && from ) = default;
        AstSerializer & operator = ( const AstSerializer & from ) = delete;
        AstSerializer & operator = ( AstSerializer && from ) = default;

        ModuleLibrary *     moduleLibrary = nullptr;
    // some passes require module group (it's passed from top-level)
        ModuleGroup *       thisModuleGroup = nullptr;
        Module *            thisModule = nullptr;
        Module *            astModule = nullptr;
        bool                writing = false;
        bool                failed = false;
        size_t              readOffset = 0;
        vector<uint8_t>     buffer;
        vector<uint8_t>     metadata;
        bool                seenNewModule = false;
    // file info clean up
        vector<FileInfo*>         deleteUponFinish; // these pointers are for builtins (which we don't serialize) and need to be cleaned manually
        das_hash_set<FileInfo*>   doNotDelete;
    // profile data
        uint64_t totMacroTime = 0;
    // pointers
        das_hash_map<uint64_t, ExprBlock*>          exprBlockMap;
        using DataOffset = uint64_t;
        das_hash_map<FileInfo*, DataOffset>         writingFileInfoMap;
        das_hash_map<DataOffset, FileInfo*>         readingFileInfoMap;
        das_hash_map<uint64_t, FileAccess*>         fileAccessMap;
    // smart pointers
        das_hash_map<uint64_t, MakeFieldDeclPtr>    smartMakeFieldDeclMap;
        das_hash_map<uint64_t, EnumerationPtr>      smartEnumerationMap;
        das_hash_map<uint64_t, StructurePtr>        smartStructureMap;
        das_hash_map<uint64_t, VariablePtr>         smartVariableMap;
        das_hash_map<uint64_t, FunctionPtr>         smartFunctionMap;
        das_hash_map<uint64_t, MakeStructPtr>       smartMakeStructMap;
        das_hash_map<uint64_t, TypeDeclPtr>         smartTypeDeclMap;
    // refs
        vector<pair<ExprBlock**,uint64_t>>          blockRefs;
        vector<pair<Function **,uint64_t>>          functionRefs;
        vector<pair<Variable **,uint64_t>>          variableRefs;
        vector<pair<Structure **,uint64_t>>         structureRefs;
        vector<pair<Enumeration **,uint64_t>>       enumerationRefs;
        // fieldRefs tuple contains: fieldptr, module, structname, fieldname
        vector<tuple<const Structure::FieldDeclaration **, Module *, string, string>>       fieldRefs;
    // tracking for shared modules
        das_hash_set<Module *>                      writingReadyModules;
        void tag ( const char * name );
        void read  ( void * data, size_t size );
        void write ( const void * data, size_t size );
        void serializeAdaptiveSize64 ( uint64_t & size );
        void serializeAdaptiveSize32 ( uint32_t & size );
        void serialize ( void * data, size_t size );
        void patch ();
        AstSerializer & operator << ( string & str );
        AstSerializer & operator << ( const char * & value );
        AstSerializer & operator << ( bool & value ) { serialize(&value, sizeof(value)); return *this; }
        AstSerializer & operator << ( vec4f & value ) { serialize(&value, sizeof(value)); return *this; }
        AstSerializer & operator << ( float & value ) { serialize(&value, sizeof(value)); return *this; }
        AstSerializer & operator << ( void * & value ) { serialize(&value, sizeof(value)); return *this; }
        AstSerializer & operator << ( uint8_t & value ) { serialize(&value, sizeof(value)); return *this; }
        AstSerializer & operator << ( int32_t & value ) { serialize(&value, sizeof(value)); return *this; }
        AstSerializer & operator << ( int64_t & value ) { serialize(&value, sizeof(value)); return *this; }
        AstSerializer & operator << ( uint16_t & value ) { serialize(&value, sizeof(value)); return *this; }
        AstSerializer & operator << ( uint32_t & value ) { serialize(&value, sizeof(value)); return *this; }
        AstSerializer & operator << ( uint64_t & value ) { serialize(&value, sizeof(value)); return *this; }
        AstSerializer & operator << ( pair<uint32_t,uint32_t> & value ) { serialize(&value, sizeof(value)); return *this; }
        AstSerializer & operator << ( pair<uint64_t,uint64_t> & value ) { serialize(&value, sizeof(value)); return *this; }
        AstSerializer & operator << ( pair<string,bool> & value ) { *this << value.first << value.second; return *this; }
        AstSerializer & operator << ( tuple<Module *, string, string, bool, LineInfo> & value );
        AstSerializer & operator << ( CodeOfPolicies & ser );
        AstSerializer & operator << ( TypeDeclPtr & type );
        AstSerializer & operator << ( AnnotationArgument & arg );
        AstSerializer & operator << ( AnnotationDeclarationPtr & annotation_decl );
        AstSerializer & operator << ( AnnotationPtr & anno );
        AstSerializer & operator << ( Structure::FieldDeclaration & field_declaration );
        AstSerializer & operator << ( ExpressionPtr & expr );
        AstSerializer & operator << ( FunctionPtr & func );
        AstSerializer & operator << ( Function * & func );
        AstSerializer & operator << ( Type & baseType );
        AstSerializer & operator << ( LineInfo & at );
        AstSerializer & operator << ( Module * & module );
        AstSerializer & operator << ( FileInfo * & info );
        AstSerializer & operator << ( FileInfoPtr & ptr );
        AstSerializer & operator << ( FileAccessPtr & ptr );
        AstSerializer & operator << ( Structure * & struct_ );
        AstSerializer & operator << ( StructurePtr & struct_ );
        AstSerializer & operator << ( Enumeration * & enum_type );
        AstSerializer & operator << ( EnumerationPtr & enum_type );
        AstSerializer & operator << ( Enumeration::EnumEntry & entry );
        AstSerializer & operator << ( TypeAnnotationPtr & type_anno );
        AstSerializer & operator << ( TypeAnnotation * & type_anno );
        AstSerializer & operator << ( VariablePtr & var );
        AstSerializer & operator << ( Variable * & var );
        AstSerializer & operator << ( Function::AliasInfo & alias_info );
        AstSerializer & operator << ( InferHistory & history );
        AstSerializer & operator << ( ReaderMacroPtr & ptr );
        AstSerializer & operator << ( TypeInfoMacro * & ptr );
        AstSerializer & operator << ( ExprBlock * & block );
        AstSerializer & operator << ( CallMacro * & ptr );
        AstSerializer & operator << ( CaptureEntry & entry );
        AstSerializer & operator << ( MakeFieldDeclPtr & ptr );
        AstSerializer & operator << ( MakeStructPtr & ptr );
   // Top-level
        AstSerializer & operator << ( Module & module );

        void serializeProgram ( ProgramPtr program, ModuleGroup & libGroup );

        template<typename T>
        void serializeSmartPtr( smart_ptr<T> & obj, das_hash_map<uint64_t, smart_ptr<T>> & objMap );

        template <uint64_t n>
        AstSerializer& operator << ( int (&value)[n] ) {
            serialize(value, n * sizeof(int)); return *this;
        }

        template <typename TT>
        AstSerializer & operator << ( vector<TT> & value );

        template <typename K, typename V, typename H, typename E>
        void serialize_hash_map ( das_hash_map<K, V, H, E> & value );

        template <typename K, typename V>
        AstSerializer & operator << ( das_hash_map<K, V> & value );

        template <typename V>
        AstSerializer & operator << ( safebox_map<V> & box );

        template <typename V>
        AstSerializer & operator << ( safebox<V> & box );

        template<typename TT>
        AstSerializer & serializePointer ( TT * & ptr );

        bool isInThisModule ( Function * & ptr );
        bool isInThisModule ( Enumeration * & ptr );
        bool isInThisModule ( Structure * & ptr );
        bool isInThisModule ( Variable * & ptr );
        bool isInThisModule ( TypeInfoMacro * & ptr );

        void writeIdentifications ( Function * & func );
        void writeIdentifications ( Enumeration * & ptr );
        void writeIdentifications ( Structure * & ptr );
        void writeIdentifications ( Variable * & ptr );
        void writeIdentifications ( TypeInfoMacro * & ptr );

        void fillOrPatchLater ( Function * & func, uint64_t id );
        void fillOrPatchLater ( Enumeration * & ptr, uint64_t id );
        void fillOrPatchLater ( Structure * & ptr, uint64_t id );
        void fillOrPatchLater ( Variable * & ptr, uint64_t id );

        auto readModuleAndName () -> pair<Module *, string>;

        void findExternal ( Function * & func );
        void findExternal ( Enumeration * & ptr );
        void findExternal ( Structure * & ptr );
        void findExternal ( Variable * & ptr );
        void findExternal ( TypeInfoMacro * & ptr );

        template <typename EnumType>
        void serialize_small_enum ( EnumType & baseType ) {
            if ( writing ) {
                uint8_t bt = (uint8_t) baseType;
                *this << bt;
            } else {
                uint8_t bt = 0;
                *this << bt;
                baseType = (EnumType) bt;
            }
        }

        template <typename EnumType>
        void serialize_enum ( EnumType & baseType ) {
            if ( writing ) {
                uint32_t bt = (uint32_t) baseType;
                *this << bt;
            } else {
                uint32_t bt = 0;
                *this << bt;
                baseType = (EnumType) bt;
            }
        }
    };
}
