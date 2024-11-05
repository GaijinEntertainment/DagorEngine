#pragma once

#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast.h"
#include <optional>
#include <utility>

namespace das {
    struct SerializationStorage {
        vector<uint8_t> buffer;
        size_t bufferPos = 0;
        template<typename T>
        __forceinline bool read ( T & data ) {
            if ( bufferPos + sizeof(T) < buffer.size() ) {
                data = *(T*)(buffer.data() + bufferPos);
                bufferPos += sizeof(T);
                return true;
            }
            return readOverflow(&data, sizeof(T));
        }
        __forceinline bool read ( vec4f & data ) {
            if ( bufferPos + sizeof(vec4f) < buffer.size() ) {
                data = v_ldu((const float*)(buffer.data() + bufferPos));
                bufferPos += sizeof(vec4f);
                return true;
            }
            return readOverflow(&data, sizeof(vec4f));
        }
        __forceinline bool read ( void * data, size_t size ) {
            if ( bufferPos + size < buffer.size() ) {
                memcpy(data, buffer.data() + bufferPos, size);
                bufferPos += size;
                return true;
            }
            return readOverflow(data, size);
        }
        virtual bool readOverflow ( void * data, size_t size ) = 0;
        virtual void write ( const void * data, size_t size ) = 0;
        virtual size_t writingSize() const = 0;
        virtual ~SerializationStorage() {}
    };

    struct SerializationStorageVector : SerializationStorage {
        virtual size_t writingSize() const override {
            return buffer.size();
        }
        virtual bool readOverflow ( void * data, size_t size ) override {
            if ( bufferPos + size > buffer.size() ) return false;
            memcpy(data, buffer.data() + bufferPos, size);
            bufferPos += size;
            return true;
        }
        virtual void write ( const void * data, size_t size ) override {
            auto at = buffer.size();
            buffer.resize(at + size);
            memcpy(buffer.data() + at, data, size);
        }
    };

    struct AstSerializer {
        ~AstSerializer ();
        AstSerializer ( SerializationStorage * storage, bool isWriting );

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
        SerializationStorage * buffer = nullptr;
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
        // parseModule tuple contains: moduleName, mtime, thisModule, thisModule
        vector<tuple<string, uint64_t, ProgramPtr, Module*>> parsedModules;
    // tracking for shared modules
        das_hash_set<Module *>                      writingReadyModules;
        void tag   ( const char * name );
        template<typename T>
        void read  ( T & data ) { buffer->read(data); }
        void read  ( void * data, size_t size );
        void write ( const void * data, size_t size );
        template<typename T>
        void serialize ( T & data ) {
            if ( writing ) {
                write(&data, sizeof(data));
            } else {
                read(data);
            }
        }
        void serialize ( void * data, size_t size );
        void serializeAdaptiveSize64 ( uint64_t & size );
        void serializeAdaptiveSize32 ( uint32_t & size );
        void collectFileInfo ( vector<FileInfoPtr> & orphanedFileInfos );
        void getCompiledModules ( );
        void patch ();
        AstSerializer & operator << ( string & str );
        AstSerializer & operator << ( const char * & value );
        AstSerializer & operator << ( bool & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( vec4f & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( float & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( void * & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( uint8_t & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( int32_t & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( int64_t & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( uint16_t & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( uint32_t & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( uint64_t & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( pair<uint32_t,uint32_t> & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( pair<uint64_t,uint64_t> & value ) { serialize(value); return *this; }
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
        AstSerializer & serializeModule ( Module & module, bool already_exists );

        uint32_t getVersion ();

        void serializeProgram ( ProgramPtr program, ModuleGroup & libGroup ) noexcept;
        bool serializeScript ( ProgramPtr program ) noexcept;

        template<typename T>
        void serializeSmartPtr( smart_ptr<T> & obj, das_hash_map<uint64_t, smart_ptr<T>> & objMap );

        template <uint64_t n>
        AstSerializer& operator << ( int (&value)[n] ) {
            serialize(value, n * sizeof(int)); return *this;
        }

        ___noinline bool trySerialize ( const callable<void(AstSerializer&)> &cb ) noexcept;

        template <typename TT>
        AstSerializer & operator << ( vector<TT> & value ) {
            tag("Vector");
            if ( writing ) {
                uint64_t size = value.size();
                serializeAdaptiveSize64(size);
            } else {
                uint64_t size = 0;
                serializeAdaptiveSize64(size);
                value.resize(size);
            }
            for ( TT & v : value ) {
                *this << v;
            }
            return *this;
        }

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
