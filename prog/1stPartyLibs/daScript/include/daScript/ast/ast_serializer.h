#pragma once

#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast.h"
#include "daScript/misc/hash.h"
#include <optional>
#include <utility>

// enable this one if serialization breaks on tag, to get more info
#ifndef DAS_SERIALIZE_DTAG
#define DAS_SERIALIZE_DTAG      0
#endif

#ifndef DAS_SERIALIZE_PROFILE
#define DAS_SERIALIZE_PROFILE   0
#endif

namespace das {
    struct SerializationStorage {
        vector<uint8_t> buffer;
        size_t bufferPos = 0;
        template<typename T>
        __forceinline bool read ( T & data ) {
            if ( bufferPos + sizeof(T) < buffer.size() ) {
                memcpy((void*)&data, buffer.data() + bufferPos, sizeof(T));  // buffer offsets are arbitrary — no aligned punning
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
        // bytes left to read, when the storage knows: lets the reader reject a corrupted
        // length BEFORE allocating (a serialized length can never exceed the bytes left).
        // Streaming storages that cannot know keep the default "unbounded" - they simply
        // do not get the early rejection.
        virtual uint64_t readRemaining() const { return ~0ull; }
        // overwrite already-written bytes (record-length backpatch). A storage that cannot
        // seek keeps the default false — record lengths then stay 0 and the reader falls
        // back to the legacy stop-at-first-failure cutoff instead of skip-and-resume.
        virtual bool patch ( size_t /*at*/, const void * /*data*/, size_t /*size*/ ) { return false; }
        //! called at the end of every program's bytes: a storage that buffers ahead settles what it holds here; a storage that never wrote is left alone
        virtual void flush () {}
        virtual struct SerializationStorageVector * asVector () { return nullptr; }
        virtual ~SerializationStorage() {}
    };

    struct SerializationStorageVector : SerializationStorage {
        size_t writePos = 0;
        bool   wrote = false;
        virtual size_t writingSize() const override {
            return writePos;
        }
        virtual bool readOverflow ( void * data, size_t size ) override {
            if ( bufferPos + size > buffer.size() ) return false;
            memcpy(data, buffer.data() + bufferPos, size);
            bufferPos += size;
            return true;
        }
        void grow ( size_t need ) {
            size_t cap = buffer.size();
            size_t next = cap ? cap : 4096;
            while ( next < need ) {
                if ( next > (size_t(-1) >> 1) ) { next = need; break; }
                next *= 2;
            }
            buffer.resize(next);
        }
        __forceinline void append ( const void * data, size_t size ) {
            if ( writePos + size > buffer.size() ) grow(writePos + size);
            memcpy(buffer.data() + writePos, data, size);
            writePos += size;
            wrote = true;
        }
        virtual void write ( const void * data, size_t size ) override {
            append(data, size);
        }
        virtual void flush () override {
            if ( wrote ) buffer.resize(writePos);
        }
        virtual SerializationStorageVector * asVector () override { return this; }
        virtual bool patch ( size_t at, const void * data, size_t size ) override {
            if ( at + size > writePos ) return false;
            memcpy(buffer.data() + at, data, size);
            return true;
        }
        virtual uint64_t readRemaining() const override {
            return uint64_t(buffer.size() - bufferPos);
        }
    };

    // A node's identity in a record: the writer numbers nodes from 1 in first-mention order,
    // the reader indexes a vector with the number. 0 is the null pointer. Per record - the
    // tables reset with the maps at the end of every program.
    struct SerializeNodeId {
        uint32_t index = 0;
        bool operator == ( const SerializeNodeId & o ) const noexcept { return index == o.index; }
        bool operator != ( const SerializeNodeId & o ) const noexcept { return index != o.index; }
    };

    struct DAS_API AstSerializer {
        ~AstSerializer ();
        AstSerializer ( SerializationStorage * storage, bool isWriting );

        AstSerializer ( const AstSerializer & from ) = delete;
        AstSerializer ( AstSerializer && from ) = default;
        AstSerializer & operator = ( const AstSerializer & from ) = delete;
        AstSerializer & operator = ( AstSerializer && from ) = default;

        ModuleLibrary *     moduleLibrary = nullptr;
    // some passes require module group (it's passed from top-level)
        ModuleGroup *       thisModuleGroup = nullptr;
        FileAccess *        fileAccess = nullptr;
        Module *            thisModule = nullptr;
        Module *            astModule = nullptr;
        bool                writing = false;
        bool                failed = false;
        SerializationStorage * buffer = nullptr;
        SerializationStorageVector * bufferAsVector = nullptr;
        bool                seenNewModule = false;
    // module-cache resume state (trySerializeProgramModule)
        bool                checkedStreamHeader = false;
        uint64_t            resumedModules = 0;     // records skipped + reparsed in place
        uint64_t            resumedCorrupt = 0;     // of those, failures a rewrite REPAIRS (anything but builtinHashDrift)
        bool                builtinHashDrift = false;   // last record failed on a builtin cumulative-hash mismatch (lazily populated builtin, e.g. dasbind) - deterministic per process, a rewrite changes nothing
        bool                quietCache = false;
        int32_t             recordDepth = 0;
        uint64_t            servedModules = 0;
        int                 readingRecord = 0;      // >0 while a record's payload deserializes (a late require nested in it cannot read)
        string              cutoffFile;
        string              cutoffReason;
        bool                policyMismatch = false;
        bool                readJitEnabled = false; // the compile's jit_enabled, for the macro program a served record reinstantiates (the cold path's program carries it)
        AnnotationArgumentList readOptions;         // the served module's own `options`, for the same macro program (its fusion opt-in reads them)
    // expression lookup
        das_hash_map<uint32_t, Annotation *> rttiHash2Annotation;
    // file info clean up
        vector<FileInfo*>         deleteUponFinish; // these pointers are for builtins (which we don't serialize) and need to be cleaned manually
        das_hash_set<FileInfo*>   doNotDelete;
    // per-record tables, cleared in clearNodeIds (src/builtin/ARCHITECTURE.md#module-cache-record-bytes)
        das_hash_map<FileInfo *, uint32_t>          writeFileInfos;
        vector<FileInfo *>                          readFileInfos;
        FileInfo *          lastWriteFileInfo = nullptr;
        uint32_t            lastWriteFileInfoNumber = 0;
        FileInfo *          lineBaseFile = nullptr;
        uint32_t            lineBaseLine = 0;
        struct TypeHash  { size_t operator () ( const TypeDecl * t ) const noexcept; };
        struct TypeEqual { bool operator () ( const TypeDecl * a, const TypeDecl * b ) const noexcept; };
        das_hash_map<const TypeDecl *, uint32_t, TypeHash, TypeEqual>  writeTypes;
        uint32_t                                    writeFreshTypeCount = 0;
        vector<TypeDecl *>                          readTypes;
        das_hash_map<uint32_t, uint32_t>            writeExprClasses;
        vector<Annotation *>                        readExprClasses;
        das_hash_map<Module *, uint32_t>            writeModules;
        vector<pair<Module *, uint64_t>>            readModules;
        das_hash_map<Function *, string>            writeMangledNames;
        struct StringView {
            const char *    data = nullptr;
            uint32_t        length = 0;
        };
        struct StringViewHash {
            size_t operator () ( const StringView & v ) const noexcept { return size_t(hash_block64((const uint8_t *) v.data, v.length)); }
        };
        struct StringViewEqual {
            bool operator () ( const StringView & a, const StringView & b ) const noexcept {
                return a.length == b.length && memcmp(a.data, b.data, a.length) == 0;
            }
        };
        das_hash_map<StringView, uint32_t, StringViewHash, StringViewEqual>  writeStrings;
        int32_t                                     emptyStringNumber = -1;
        vector<StringView>                          readStrings;
        vector<unique_ptr<string>>                  stringArena;
        AstSerializer & serializeString ( string & str, bool temp );
        //! a string with no home past the call: the table gets a copy of it; any other value streams as usual
        AstSerializer & serializeTemp ( string & str ) { return serializeString(str, true); }
        template <typename T>
        void serializeTemp ( T & value ) { *this << value; }
    // profile data
        uint64_t totMacroTime = 0;
        uint64_t totFinalizeTime = 0;
        uint64_t totSetupTime = 0;
    // node identity (SerializeNodeId): the writer numbers a node at its first mention and
    // remembers which numbers have had their payload written; the reader keeps the node
    // each number resolved to, null until its payload is read (a forward reference is
    // patched in patch()). Per record.
        das_hash_map<const void *, uint32_t>        writeIds;
        vector<uint8_t>                             writtenIds;     // indexed by SerializeNodeId::index
        vector<void *>                              readNodes;      // indexed by SerializeNodeId::index
        vector<pair<void *, SerializeNodeId>>       pendingRefs;    // storage of a TT * slot, and the number it waits for
        // fieldRefs tuple contains: fieldptr, module, structname, fieldname
        vector<tuple<Structure::FieldDeclarationRef*, Module *, string, string>>       fieldRefs;
        // parsedModules record: fileName, source content hash, source size, program, thisModule, the collector's require names
        vector<tuple<string, uint64_t, int64_t, ProgramPtr, Module*, vector<string>>> parsedModules;
        size_t                                      writtenModules = 0;     // parsedModules already in the stream (writebackModules)
    // tracking for shared modules
        das_hash_set<Module *>                      writingReadyModules;
        bool                                        ignoreEmptyExternal = false;
        void tag   ( const char * name, uint32_t hash );
#if DAS_SERIALIZE_PROFILE
        struct ProfAgg {
            uint64_t selfBytes = 0;
            uint64_t outermostInclBytes = 0;
            uint64_t count = 0;
            int64_t  outermostInclTicks = 0;
        };
        struct ProfNode {
            uint32_t nameId = 0;
            uint32_t parent = 0;
            uint64_t inclBytes = 0;
            uint64_t count = 0;
            int64_t  inclTicks = 0;
            vector<uint32_t> children;
        };
        struct ProfFrame {
            uint32_t nameId;
            uint32_t node;
            uint64_t startBytes;
            uint64_t childBytes;
            int64_t  startTicks;
        };
        struct ProfRecord {
            string   file;
            uint64_t lengthWordBytes = 0;
            uint64_t payloadBytes = 0;
            int64_t  usec = 0;
        };
        vector<pair<const char *, uint32_t>>    profNameIds;
        vector<string>                          profNames;
        vector<ProfAgg>                         profAgg;
        vector<ProfNode>                        profNodes;
        vector<ProfFrame>                       profStack;
        das_hash_map<string, uint32_t>          profStrings;
        das_hash_map<string, uint32_t>          profTypes;
        uint64_t                                profTypeBytes = 0;
        vector<ProfRecord>                      profRecords;
        int64_t                                 profStartTicks = 0;
        uint64_t profPosition () const { return writing ? buffer->writingSize() : buffer->bufferPos; }
        void profBegin ( const char * name );
        void profEnd ();
        void profString ( const string & str );
        void profType ( const TypeDecl * type, uint64_t bytes );
        void profReport ( TextWriter & tw ) const;
#endif
#if DAS_SERIALIZE_DTAG
        __forceinline void dtag ( const char * name, uint32_t hash ) { tag(name,hash); }
#else
        __forceinline void dtag ( const char *, uint32_t ) {}
#endif
        template<typename T>
        void read  ( T & data ) {
            // zero-filled "success" on a truncated stream would silently corrupt every
            // field after it - fail like the sized read does (recoverable: the module
            // cache resumes past the record)
            if ( !buffer->read(data) ) onReadFailure();
        }
        void read  ( void * data, size_t size );
        [[noreturn]] void onReadFailure ();     // throws dasException ("read overflow")
        __forceinline void write ( const void * data, size_t size ) {
            if ( bufferAsVector ) bufferAsVector->append(data, size);
            else buffer->write(data, size);
        }
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
        //! encodes into a caller's buffer of at least 5 bytes and answers the bytes used
        static __forceinline uint32_t zigzag32 ( int32_t v ) { return (uint32_t(v) << 1) ^ uint32_t(v >> 31); }
        static __forceinline int32_t unzigzag32 ( uint32_t v ) { return int32_t(v >> 1) ^ -int32_t(v & 1); }
        static __forceinline size_t encodeAdaptiveSize32 ( uint8_t * out, uint32_t size ) {
            size_t n = 0;
            while ( size >= 0x80 ) {
                out[n++] = uint8_t(size) | 0x80;
                size >>= 7;
            }
            out[n++] = uint8_t(size);
            return n;
        }
        // reject a deserialized element count that exceeds the bytes left in the stream
        // BEFORE it gates an allocation (throws; reading only)
        void verifyLength ( uint64_t size );
        void collectFileInfo ( vector<FileInfoPtr> & orphanedFileInfos );
        void getCompiledModules ( );
        void patch ();
        AstSerializer & operator << ( string & str ) { return serializeString(str, recordDepth == 0); }
        AstSerializer & operator << ( const char * & value );
        AstSerializer & operator << ( bool & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( vec4f & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( float & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( void * & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( uint8_t & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( int32_t & value ) {
            if ( writing ) {
                uint32_t z = zigzag32(value);
                serializeAdaptiveSize32(z);
            } else {
                uint32_t z = 0;
                serializeAdaptiveSize32(z);
                value = unzigzag32(z);
            }
            return *this;
        }
        AstSerializer & operator << ( int64_t & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( uint16_t & value ) { serialize(value); return *this; }
        AstSerializer & operator << ( uint32_t & value ) { serializeAdaptiveSize32(value); return *this; }
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
        AstSerializer & operator << ( Type & baseType );
        AstSerializer & operator << ( LineInfo & at );
        AstSerializer & operator << ( Module * & module );
        AstSerializer & operator << ( FileInfo * & info );
        AstSerializer & operator << ( FileInfoPtr & ptr );
        AstSerializer & operator << ( FileAccessPtr & ptr );
        AstSerializer & operator << ( StructurePtr & struct_ );
        AstSerializer & operator << ( EnumerationPtr & enum_type );
        AstSerializer & operator << ( Enumeration::EnumEntry & entry );
        AstSerializer & operator << ( TypeAnnotationPtr & type_anno );
        AstSerializer & operator << ( VariablePtr & var );
        AstSerializer & operator << ( Function::AliasInfo & alias_info );
        AstSerializer & operator << ( InferHistory & history );
        AstSerializer & operator << ( ReaderMacroPtr & ptr );
        AstSerializer & operator << ( TypeInfoMacro * & ptr );
        AstSerializer & operator << ( ExprBlock * & block );
        AstSerializer & operator << ( CallMacro * & ptr );
        AstSerializer & operator << ( CaptureEntry & entry );
        AstSerializer & operator << ( MakeFieldDeclPtr & ptr );
        AstSerializer & operator << ( MakeStructPtr & ptr );
        AstSerializer & operator << ( SerializeNodeId & value );
   // Top-level
        AstSerializer & operator << ( Module & module );
        AstSerializer & serializeModule ( Module & module, bool already_exists );

        static constexpr uint32_t getVersion () {
            return 218;   // 218: max_fast_call_depth joins the module-cache policy stream
        }

        void serializeProgram ( ProgramPtr program, ModuleGroup & libGroup ) noexcept;
        void serializeProgramImpl ( ProgramPtr program, ModuleGroup & libGroup ); // throws dasException; called via the noexcept wrapper above
        bool serializeScript ( ProgramPtr program ) noexcept;

        template <uint64_t n>
        AstSerializer& operator << ( int (&value)[n] ) {
            serialize(value, n * sizeof(int)); return *this;
        }

        ___noinline bool trySerialize ( const callable<void(AstSerializer&)> &cb ) noexcept;

        template <typename TT>
        AstSerializer & operator << ( vector<TT> & value ) {
            dtag("Vector",hash_tag("Vector"));
#if DAS_SERIALIZE_PROFILE
            profBegin("Vector");
#endif
            if ( writing ) {
                uint64_t size = value.size();
                serializeAdaptiveSize64(size);
            } else {
                uint64_t size = 0;
                serializeAdaptiveSize64(size);
                verifyLength(size);
                value.resize(size);
            }
            for ( TT & v : value ) {
                *this << v;
            }
#if DAS_SERIALIZE_PROFILE
            profEnd();
#endif
            return *this;
        }

        template <typename K, typename V, typename H, typename E>
        void serialize_hash_map ( das_hash_map<K, V, H, E> & value );

        template <typename K, typename V, typename H, typename E>
        AstSerializer & operator << ( das_hash_map<K, V, H, E> & value );

        // Guarded: under DAS_CUSTOM_HASH=0 das_insert_only_hash_map aliases to
        // std::unordered_map (same as das_hash_map), so the regular overloads
        // above already match insert-only args.
#if DAS_CUSTOM_HASH
        template <typename K, typename V, typename H, typename E>
        void serialize_hash_map ( das_insert_only_hash_map<K, V, H, E> & value );

        template <typename K, typename V, typename H, typename E>
        AstSerializer & operator << ( das_insert_only_hash_map<K, V, H, E> & value );
#endif

        template <typename V>
        AstSerializer & operator << ( safebox_map<V> & box );

        template <typename V, typename VT>
        AstSerializer & operator << ( safebox<V,VT> & box );

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

        // reading: the node a number resolved to, or a patch-later slot when its payload is still ahead
        template <typename TT>
        void fillOrPatchLater ( TT * & ptr, SerializeNodeId id ) {
            if ( auto node = readNode<TT>(id) ) {
                ptr = node;
            } else {
                ptr = ( TT * ) 1;
                pendingRefs.emplace_back(&ptr, id);     // the slot's storage; patch() writes it bytewise, whatever TT is
            }
        }
        template <typename TT>
        __forceinline TT * readNode ( SerializeNodeId id ) const { return (TT *) readNodes[id.index]; }
        __forceinline void setReadNode ( SerializeNodeId id, void * node ) { readNodes[id.index] = node; }
        // writing: has this number's payload gone into the stream already
        __forceinline bool isWritten ( SerializeNodeId id ) const { return id.index < writtenIds.size() && writtenIds[id.index]; }
        void markWritten ( SerializeNodeId id ) {
            if ( id.index >= writtenIds.size() ) writtenIds.resize(id.index + 1, 0);
            writtenIds[id.index] = 1;
        }
        void clearNodeIds ();

        auto readModuleAndName () -> pair<Module *, string>;
        auto readModuleAndNameHash () -> pair<Module *, uint64_t>;

        void findExternal ( Function * & func );
        void findExternal ( Enumeration * & ptr );
        void findExternal ( Structure * & ptr );
        void findExternal ( Variable * & ptr );
        void findExternal ( TypeInfoMacro * & ptr );

        // writing: the node's number, assigned at its first mention; reading: a placeholder the
        // stream overwrites
        SerializeNodeId getSerializeId ( const void * ptr ) {
            if ( !writing || !ptr ) return {0};
            auto & slot = writeIds[ptr];
            if ( !slot ) slot = uint32_t(writeIds.size());   // the fresh entry counts itself: numbers run from 1
            return {slot};
        }

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

#if DAS_SERIALIZE_PROFILE
    struct SerProfileScope {
        AstSerializer & ser;
        SerProfileScope ( AstSerializer & s, const char * name ) : ser(s) { ser.profBegin(name); }
        ~SerProfileScope () { ser.profEnd(); }
    };
    #define DAS_SER_PROFILE_CAT2(a,b)   a##b
    #define DAS_SER_PROFILE_CAT(a,b)    DAS_SER_PROFILE_CAT2(a,b)
    #define DAS_SER_PROFILE(ser, name)  das::SerProfileScope DAS_SER_PROFILE_CAT(_serProfScope_, __LINE__)((ser), (name))
#else
    #define DAS_SER_PROFILE(ser, name)
#endif

    // File-backed driver for the env module-cache rail (daScriptEnvironment::serializer_read
    // / serializer_write): install() binds a reader (when the file exists and is non-empty)
    // and/or a writer around a compile; finish() unbinds, classifies what the reader saw,
    // and saves the refreshed stream (write-to-temp + rename, creating missing parent
    // directories) when the writeback fired.
    // The writeback fires when there was no usable reader, the stream went stale, or a
    // resumed record failed the THROWING way (damaged bytes - the rewrite repairs it); a
    // drift-flavored 'partial' (a lazily populated builtin like dasbind, deterministic per
    // process) deliberately does NOT rewrite - that reparse cost recurs per run by design.
    // Pass the same path as both readFrom and writeTo for a self-maintaining cache; quiet
    // silences the reader's per-record diagnostics. Under DAS_NO_FILEIO the cache
    // degrades to a stub: install() binds nothing and finish() answers 'unavailable'.
    struct DAS_API ModuleFileCache {
        enum class ReadVerdict {
            none,       // no reader installed, or nothing was ever read (e.g. debugger disabled the rail)
            clean,      // every module came from the cache
            partial,    // resumed: N unchanged-file records reparsed in place, the rest served
            fallback,   // the stream was cut - the prefix before the cutoff served, the rest reparsed
            unavailable // DAS_NO_FILEIO build - a file-backed cache cannot exist on this target
        };
        struct Result {
            ReadVerdict verdict = ReadVerdict::none;
            uint64_t    resumed = 0;
            uint64_t    served = 0;
            string      cutoffFile;
            string      cutoffReason;
            bool        wrote = false;          // refreshed stream saved to writeTo
            bool        saveFailed = false;     // had bytes to save but could not write the file
            uint64_t    wroteBytes = 0;
        };
        void install ( const string & readFrom, const string & writeTo, bool quiet = false );
        Result finish ();
        static string defaultPath ( const string & scriptPath, const string & hostBinary, const string & hostOptions );
        static string embeddedHostOptions ( const CodeOfPolicies & policies );   // hostOptions for a script's compile_file: argv up to `--` + policies hash
        SerializationStorageVector  readStorage, writeStorage;
        unique_ptr<AstSerializer>   reader, writer;
        string                      writePath;
        bool                        requested = false;  // install() was asked to read or write
        // deserialized FileInfos not claimed by any module (collectFileInfo): LineInfos in
        // the deserialized AST point at them, so this cache object MUST outlive the program
        // it fed - declare it before the program/context in the embedding scope
        vector<FileInfoPtr>         orphanedFileInfos;
    };

    // Opaque handle to expose serializer to daslang.
    struct AstSerializerState {
        unique_ptr<SerializationStorageVector> storage;
        unique_ptr<AstSerializer> serializer;
    };

    // Create a writing serializer.
    AstSerializerState * rtti_create_ast_serializer ();

    // Create a reading serializer from a blob.
    AstSerializerState * rtti_create_ast_deserializer ( const TArray<uint8_t> & data );

    // Delete a serializer state.
    void rtti_delete_ast_serializer ( AstSerializerState * state );

    // Serialize one program (writing mode). Returns false on failure.
    bool rtti_ast_serializer_serialize_program (
            AstSerializerState * state,
            const smart_ptr<Program> & program );

    // Deserialize one program (reading mode).
    void rtti_ast_serializer_deserialize_program (
            AstSerializerState * state, ModuleGroup * libGroup,
            const TBlock<void,bool,smart_ptr<Program>,const string> & block,
            Context * context, LineInfoArg * at );
    // the same, with the access the restored program's late requires walk through
    void rtti_ast_serializer_deserialize_program_ex (
            AstSerializerState * state, smart_ptr<FileAccess> access, ModuleGroup * libGroup,
            const TBlock<void,bool,smart_ptr<Program>,const string> & block,
            Context * context, LineInfoArg * at );

    // Get serialized data from a writing serializer.
    void rtti_ast_serializer_get_data (
            AstSerializerState * state,
            const TBlock<void,TTemporary<TArray<uint8_t> const>> & block,
            Context * context, LineInfoArg * at );

    //! microseconds this state's reads spent inside finalizeModule, over every program it read
    int64_t rtti_ast_serializer_finalize_usec ( AstSerializerState * state );
    //! microseconds this state's reads spent in the setup Program::serialize runs after the stream
    int64_t rtti_ast_serializer_setup_usec ( AstSerializerState * state );
}
