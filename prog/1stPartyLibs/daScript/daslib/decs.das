options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options multiple_contexts
options strict_smart_pointers = true

module decs shared public

//! daslang Entity Component System (DECS).
//!
//! A pure-daslang ECS runtime: entities with dynamic component
//! composition, archetypal storage, queries, commit/deferred
//! create/delete, and serialization. Use with ``decs_boost`` for
//! the query macro syntax.

require daslib/templates

require daslib/rtti public
require daslib/algorithm
// require daslib/sort_boost
require daslib/functional
require daslib/archive public
require daslib/contracts
require math
require strings
require daslib/defer

typedef ComponentHash = uint64  //! Hash value of the ECS component type
typedef TypeHash = uint64       //! Hash value of the individual type

struct CTypeInfo {
    //! Type information for the individual component subtype.
    //! Consists of type name and collection of type-specific routines to control type values during its lifetime, serialization, etc.
    basicType : Type        //! basic type of the component
    mangledName : string    //! mangled name of the type
    fullName : string       //! full name of the type
    hash : TypeHash         //! hash of the type
    size : uint             //! size of the type
    eraser     : function<(var arr : array<uint8>) : void> //! function to erase component value
    clonner    : function<(var dst : array<uint8>; src : array<uint8>) : void> //! function to clone component value
    serializer : function<(var arch : Archive; var arr : array<uint8>; name : string) : void> //! function to serialize component value
    dumper     : function<(elem : void?) : string> //! function to dump component value as text
    mkTypeInfo : function<() : TypeInfo const?> //! function to make TypeInfo for the component type
    gc         : function<(var src : array<uint8>) : lambda> //! function to perform GC marking on the component value
}

struct public Component {
    //! Single ECS component. Contains component name, data, and data layout.
    name : string           //! name of the component
    hash : ComponentHash    //! hash of the component
    stride : int            //! stride of the component data
    data : array<uint8>     //! raw data of the component
    info : CTypeInfo        //! type information of the component
    gc_dummy : lambda       //! this is here so that GC can find real representation of data
}

struct public EntityId {
    //! Unique identifier of the entity. Consists of id (index in the data array) and generation.
    id : uint               //! index of the entity
    generation : int        /// generation of the entity
}

struct public Archetype {
    //! ECS archetype. Archetype is unique combination of components.
    hash : ComponentHash            //! hash of the archetype (combination of component hashes)
    components : array<Component>   //! list of components in the archetype
    size : int                      //! number of entities in the archetype
    eidIndex : int                  //! index of the 'eid' component in the components array
}

struct public ComponentValue {
    //! Value of the component during creation or transformation.
    name : string                   //! name of the component
    info : CTypeInfo                //! type information of the component
    data : float4[4]                //! raw data of the component
}

typedef DeferEval = lambda<(var act : DeferAction) : void>//! Lambda which holds deferred action. Typically creation of destruction of an entity.

struct private DeferAction {
    //! Individual deferred action.
    eid : EntityId              //! entity id involved in the action
    action : DeferEval          //! deferred action lambda
}

typedef ComponentMap = array<ComponentValue> //! Table of component values for individual entity.

struct EcsRequestPos {
    //! Location of the ECS request in the code (source file and line number).
    file : string           //! source file
    line : uint             //! line number
}

def private normalize_file_name(name : string) : string {
    //! Strips directory path, keeping only filename and extension.
    //! This ensures the same canonical name regardless of working directory,
    //! which is critical for AOT hash stability.
    var s = replace(name, "\\", "/")
    var lastSlash = -1
    var pos = 0
    while (true) {
        let idx = find(s, "/", pos)
        if (idx < 0) {
            break
        }
        lastSlash = idx
        pos = idx + 1
    }
    if (lastSlash >= 0) {
        return slice(s, lastSlash + 1)
    }
    return s
}

def EcsRequestPos(at : rtti::LineInfo) : EcsRequestPos {
    //! Constructs EcsRequestPos from rtti::LineInfo.
    return EcsRequestPos(file = at.fileInfo != null ? normalize_file_name(string(at.fileInfo.name)) : "", line = at.line)
}

struct public EcsRequest {
    //! Individual ECS requests. Contains list of required components, list of components which are required to be absent.
    //! Caches list of archetypes, which match the request.
    hash : ComponentHash        //! hash of the request
    req : array<string>         //! required components
    reqn : array<string>        //! required components which are not present
    archetypes : array<int>     //! sorted list of matching archetypes
    at : EcsRequestPos          //! location of the request in the code
}

struct public DecsState {
    //! Entire state of the ECS system.
    //! Contains archetypes, entities and entity free-list, entity lookup table, all archetypes and archetype lookups, etc.
    archetypeLookup : table<ComponentHash; int>     //! lookup of archetype by its hash
    allArchetypes : array<Archetype>                //! all archetypes in the system
    entityFreeList : array<EntityId>                //! list of free entity IDs
    entityLookup : array<tuple<generation : int; archetype : ComponentHash; index : int>>   //! lookup of entity by its ID
    componentTypeCheck : table<string; CTypeInfo>   //! lookup of component type info by its name
    ecsQueries : array<EcsRequest>                  //! all ECS requests
    queryLookup : table<ComponentHash; int>         //! lookup of ECS request by its hash
}

typedef PassFunction = function<() : void>  //! One of the callbacks which form individual pass.

struct public DecsPass {
    //! Individual pass of the update of the ECS system.
    //! Contains pass name and list of all pass callbacks.
    name : string               //! name of the pass
    calls : array<PassFunction> //! list of all pass callbacks
}

var public decsState : DecsState    //! Full state of the ECS system.
var private deferActions : array<DeferAction>
var private decsPasses : array<DecsPass>
var private insideQuery : int

let INVALID_ENTITY_ID = decs::EntityId()  //! Entity ID which represents invalid entity.

def operator ==(a, b : decs::EntityId implicit) {
    //! Equality operator for entity IDs.
    return a.id == b.id && a.generation == b.generation
}

def operator !=(a, b : decs::EntityId implicit) {
    //! Inequality operator for entity IDs.
    return a.id != b.id || a.generation != b.generation
}

def public describe(info : CTypeInfo) {
    //! Returns textual description of the type.
    return "{info.fullName} {info.basicType} MNH={info.mangledName} hash={info.hash} size={info.size}"
}

def public operator .(var cmp : ComponentMap; name : string) : ComponentValue& {
    //! Access to component value by name. For example::
    //!
    //!     create_entity <| @ ( eid, cmp )
    //!         cmp.pos := float3(i)    // same as cmp |> set("pos",float3(i))
    let idx = lower_bound(cmp, ComponentValue(name = name)) <| $(x, y) => x.name < y.name
    if (!(idx < length(cmp) && cmp[idx].name == name)) {
        cmp |> push(ComponentValue(name = name), idx)
    }
    unsafe {
        return cmp[idx]
    }
}

def public set(var cv : ComponentValue; val : auto) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    var cval = make_component(cv.name, val)
    assert(cv.info.hash == 0ul || cv.info.hash == cval.info.hash)
    cv = cval
}

def public operator :=(var cv : ComponentValue; val : EntityId) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : bool) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val);
}

def public operator :=(var cv : ComponentValue; val : range) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : urange) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : range64) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : urange64) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : string) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : int) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : int8) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : int16) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : int64) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : int2) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : int3) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : int4) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : uint) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : uint8) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : uint16) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : uint64) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : uint2) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : uint3) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : uint4) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : float) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : float2) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : float3) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : float4) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : float3x3) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : float3x4) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : float4x4) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public operator :=(var cv : ComponentValue; val : double) {
    //! Sets individual component value. Verifies that the value is of the correct type.
    set(cv, val)
}

def public clone(var dst : Component; src : Component) {
    //! Clones component value.
    dst.name := src.name
    dst.hash = src.hash
    dst.stride = src.stride
    dst.info := src.info
    if (src.info.clonner != null) {
        invoke(src.info.clonner, dst.data, src.data)
    } else {
        dst.data := src.data
    }
}

def public serialize(var arch : Archive; var src : Component) {
    //! Serializes component value.
    arch |> serialize(src.name)
    arch |> serialize(src.hash)
    arch |> serialize(src.stride)
    arch |> serialize(src.info)
    if (src.info.serializer != null) {
        invoke(src.info.serializer, arch, src.data, src.name)
    } else {
        panic("decs: unable to serialize component '{src.name}'")
    }
}

def public register_decs_stage_call(name : string; pcall : PassFunction) {
    //! Registration of a single pass callback. This is a low-level function, used by decs_boost macros.
    var dpass <- DecsPass(name = name)
    let idx = lower_bound(decsPasses, dpass) <| $(x, y) => x.name < y.name
    if (idx < length(decsPasses) && decsPasses[idx].name == name) {
        decsPasses[idx].calls |> push(pcall)
    } else {
        dpass.calls |> push(pcall)
        decsPasses |> emplace(dpass, idx)    // insert new one
    }
}

def public decs_stage(name : string) {
    //! Invokes specific ECS pass.
    //! `commit` is called before and after the invocation.
    commit()
    let idx = lower_bound(decsPasses, DecsPass(name = name)) <| $(x, y) => x.name < y.name
    if (idx < length(decsPasses) && decsPasses[idx].name == name) {
        for (cll in decsPasses[idx].calls) {
            invoke(cll)
        }
    }
    commit()
}

def operator delete(var cmp : Component) {
    //! Deletes component.
    if (cmp.info.eraser != null) {
        invoke(cmp.info.eraser, cmp.data)
    }
    delete cmp.data
    delete cmp.info
}

def public restart {
    //! Restarts ECS by erasing all deferred actions and entire state.
    if (insideQuery != 0) {
        panic("decs: can't call `restart` from inside query")
    }
    delete deferActions
    delete decsState
}

def private new_entity_id() {
    //! Creates new entity ID.
    var eid : EntityId
    if (!empty(decsState.entityFreeList)) {
        eid = decsState.entityFreeList[length(decsState.entityFreeList) - 1]
        decsState.entityFreeList |> pop
    } else {
        eid.id = uint(length(decsState.entityLookup))
        decsState.entityLookup |> push((0, 0ul, 0))
    }
    eid.generation ++
    return eid
}

def public before_gc {
    //! Low level callback to be called before the garbage collection.
    //! This is a low-level function typically used by `live`.
    if (insideQuery != 0) {
        panic("decs: can't call 'before_gc' from inside query")
    }
    for (arch in decsState.allArchetypes) {
        if (arch.size > 0) {
            for (comp in arch.components) {
                if (comp.info.gc != null) {
                    comp.gc_dummy <- invoke(comp.info.gc, comp.data)
                }
            }
        }
    }
}

def public after_gc {
    //! Low level callback to be called after the garbage collection.
    //! This is a low-level function typically used by `live`.
    if (insideQuery != 0) {
        panic("decs: can't call 'after_gc' from inside query")
    }
    for (arch in decsState.allArchetypes) {
        if (arch.size > 0) {
            for (comp in arch.components) {
                if (comp.gc_dummy != null) {
                    delete comp.gc_dummy
                }
            }
        }
    }
}

def public debug_dump_string : string {
    //! Returns state of the ECS system as a string.
    return build_string() <| $(var writer) {
        debug_dump_writer(writer)
    }
}

def public debug_dump_writer(var writer : StringBuilderWriter) {
    //! Writes state of the ECS system to a StringBuilderWriter.
    for (arch in decsState.allArchetypes) {
        writer |> write("archetype {arch.hash} : {arch.size}\n")
        for (index in range(arch.size)) {
            writer |> write("\tentity[{index}]\n")
            for (c in arch.components) {
                if (c.info.dumper != null) {
                    let offset = index * c.stride
                    unsafe {
                        let dump = invoke(c.info.dumper, addr(c.data[offset]))
                        writer |> write("\t\t{c.name} : {describe(c.info)} = {dump}\n")
                    }
                } else {
                    writer |> write("\t\t{c.name} : {describe(c.info)}\n")
                }
            }
        }
    }
    for (erq in decsState.ecsQueries) {
        writer |> write("query {erq.hash}\n")
        writer |> write("\treq = {erq.req}\n\treqn = {erq.reqn}\n\tarchetypes = {erq.archetypes}\n\tat = {erq.at.file}:{int(erq.at.line)}\n")
    }
}

def public debug_dump {
    //! Prints out state of the ECS system to LOG_DEBUG.
    to_log(LOG_DEBUG, debug_dump_string())
}

def private with_archetype(hash : ComponentHash; blk : block<(var arch : Archetype; idx : int; isNew : bool) : void>) {
    //! Invokes block for the archetype with specified hash.
    var afound & = unsafe(decsState.archetypeLookup[hash])
    if (afound == 0) {
        decsState.allArchetypes |> emplace(Archetype(hash = hash))
        afound = length(decsState.allArchetypes)
        ++insideQuery; invoke(blk, decsState.allArchetypes[afound - 1], afound - 1, true); --insideQuery
    } else {
        ++insideQuery; invoke(blk, decsState.allArchetypes[afound - 1], afound - 1, false); --insideQuery
    }
}

def private create_archetype(var arch : Archetype; cmp : ComponentMap; idx : int) {
    //! Creates archetype from the component map.
    assert(length(arch.components) == 0)
    arch.eidIndex = -1
    for (kv, kvi in cmp, count()) {
        var ct & = unsafe(decsState.componentTypeCheck[kv.name])
        if (ct.hash != 0ul) {
            if (kv.info.hash != ct.hash) {
                panic("decs: component {kv.name} type mismatch {ct} vs {kv.info}")
            }
        } else {
            ct = kv.info
        }
        let chash = hash(kv.name)
        arch.components |> emplace <| Component(
            name = kv.name,
            hash = chash,
            stride = int(kv.info.size),
            info = kv.info
        )
        if (kv.name == "eid") {
            assert(arch.eidIndex == -1)
            arch.eidIndex = kvi
        }
    }
    assert(arch.eidIndex != -1)
    for (erq in decsState.ecsQueries) {
        if (erq |> can_process_request(arch)) {
            erq.archetypes |> push(idx)
        }
    }
}

def private get_eid(var arch : Archetype; index : int) : EntityId& {
    //! Gets entity ID component for the specific entity index in the archetype.
    unsafe {
        var ceid & = arch.components[arch.eidIndex]
        return *(reinterpret<EntityId?> addr(ceid.data[index * ceid.stride]))
    }
}

def private create_entity(var arch : Archetype; eid : EntityId; cmp : ComponentMap) {
    //! Creates new entity in the archetype with specified component map.
    let eidx = arch.size++
    for (c, comp in arch.components, cmp) {
        c.data |> resize(length(c.data) + c.stride)
        unsafe {
            memcpy(addr(c.data[eidx * c.stride]), addr(comp.data), c.stride)
        }
    }
    return eidx
}

def private remove_entity(var arch : Archetype; di : int) {
    //! Removes entity at specified index from the archetype.
    arch.size --
    if (di != arch.size) {// copy last one in the hole
        var eid_last_id = get_eid(arch, arch.size).id
        decsState.entityLookup[eid_last_id].index = di
        for (c in arch.components) {
            unsafe {
                memcpy(addr(c.data[di * c.stride]), addr(c.data[arch.size * c.stride]), c.stride)
            }
        }
    }
    for (c in arch.components) {
        c.data |> resize(arch.size * c.stride)
    }
}

def private cmp_archetype_hash(cmp : ComponentMap) {
    //! Computes archetype hash from the component map.
    var ahash : ComponentHash
    for (kv in cmp) {
        ahash = (ahash <<< 2ul) ^ hash(kv.name)
    }
    return ahash
}

def private req_hash(erq : EcsRequest) {
    var ahash : ComponentHash
    for (kv in erq.req) {
        ahash = (ahash <<< 2ul) ^ hash(kv)
    }
    for (kv in erq.reqn) {
        ahash = (ahash <<< 2ul) ^ ~hash(kv)
    }
    return ahash
}

def public has(arch : Archetype; name : string) {
    //! Returns true if object has specified subobjec.
    return arch.components |> binary_search(Component(name = name)) <| $(x, y) => x.name < y.name
}

def private can_process_request(var erq : EcsRequest; var arch : Archetype) {
    if (erq.hash == arch.hash) {
        return true
    }
    for (r in erq.req) {
        if (!arch |> has(r)) {
            return false
        }
    }
    for (r in erq.reqn) {
        if (arch |> has(r)) {
            return false
        }
    }
    return true
}

def public verify_request(var erq : EcsRequest) : tuple<ok : bool; error : string> {
    //! Verifies ECS request. Returns pair of boolean (true for OK) and error message.
    if (erq.hash == 0ul || (empty(erq.req) && empty(erq.reqn))) {
        return (true, "")     // this queries just about everything
    }
    for (N in erq.reqn) {// assuming require_not is typically shorter
        if (erq.req |> binary_search(N)) {
            return (false, "duplicate req and neq {N}")
        }
    }
    return (true, "")
}

def public compile_request(var erq : EcsRequest) {
    //! Compiles ECS request, by creating request hash.
    sort_unique(erq.req)
    sort_unique(erq.reqn)
    erq.hash = req_hash(erq)
}

def public lookup_request(var erq : EcsRequest) {
    //! Looks up ECS request in the request cache.
    if (erq.hash == 0ul) {
        compile_request(erq)
    }
    var ql & = unsafe(decsState.queryLookup[erq.hash])
    if (ql == 0) {
        for (arch, archi in decsState.allArchetypes, count()) {
            if (erq |> can_process_request(arch)) {
                erq.archetypes |> push(archi)
            }
        }
        decsState.ecsQueries |> push_clone(erq)
        ql = length(decsState.ecsQueries)
    }
    return ql - 1
}

def public for_each_archetype(var erq : EcsRequest; blk : block<(arch : Archetype) : void>) {
    //! Invokes block for each entity of each archetype that can be processed by the request.
    let qi = lookup_request(erq)
    var inscope aclone := decsState.ecsQueries[qi].archetypes
    for (aidx in aclone) {
        var arch & = unsafe(decsState.allArchetypes[aidx])
        if (arch.size > 0) {
            ++insideQuery; invoke(blk, arch); --insideQuery
        }
    }
}

def public for_eid_archetype(eid : EntityId implicit; hash : ComponentHash; var erq : function<() : EcsRequest>; blk : block<(arch : Archetype; index : int) : void>) {
    //! Invokes block for the specific entity id, given request.
    //! Request is returned by a specified function.
    var lookup = decsState.entityLookup[eid.id]
    if (lookup.generation != eid.generation) {
        return false
    }
    var qi = -1
    decsState.queryLookup |> get(hash) $(ql) {
        qi = ql - 1
    }
    if (qi == -1) {
        qi = lookup_request(invoke(erq))
    }
    let aidx = decsState.archetypeLookup[lookup.archetype]
    if (aidx == 0) {
        return false
    }
    if (binary_search(decsState.ecsQueries[qi].archetypes, aidx - 1)) {
        var arch & = unsafe(decsState.allArchetypes[aidx - 1])
        assert(arch.size > 0)
        ++insideQuery; invoke(blk, arch, lookup.index); -- insideQuery
        return true
    } else {
        return false
    }
}

def public for_each_archetype(hash : ComponentHash; var erq : function<() : EcsRequest>; blk : block<(arch : Archetype) : void>) {
    //! Invokes block for each entity of each archetype that can be processed by the request.
    //! Request is returned by a specified function.
    var qi = -1
    decsState.queryLookup |> get(hash) $(ql) {
        qi = ql - 1
    }
    if (qi == -1) {
        qi = lookup_request(invoke(erq))
    }
    var inscope aclone := decsState.ecsQueries[qi].archetypes
    for (aidx in aclone) {
        var arch & = unsafe(decsState.allArchetypes[aidx])
        if (arch.size > 0) {
            ++insideQuery; invoke(blk, arch); --insideQuery
        }
    }
}

def public for_each_archetype_find(hash : ComponentHash; var erq : function<() : EcsRequest>; blk : block<(arch : Archetype) : bool>) {
    //! Invokes block for each entity of each archetype that can be processed by the request.
    //! Request is returned by a specified function.
    //! If block returns true, iteration is stopped.
    var qi = -1
    decsState.queryLookup |> get(hash) $(ql) {
        qi = ql - 1
    }
    if (qi == -1) {
        qi = lookup_request(invoke(erq))
    }
    var inscope aclone := decsState.ecsQueries[qi].archetypes
    for (aidx in aclone) {
        var arch & = unsafe(decsState.allArchetypes[aidx])
        if (arch.size > 0) {
            ++insideQuery; let res = invoke(blk, arch); --insideQuery
            if (res) {
                return true
            }
        }
    }
    return false
}

// [template(atype)]
[unsafe_operation]
def decs_array(atype : auto(TT); src : array<uint8>; capacity : int) {
    //! Low level function returns temporary array of component given specific type of component.
    assert(length(src) > 0)
    static_if (typeinfo is_dim(atype)) {
        var dest : array<TT[typeinfo dim(atype)] -const -& -#>
        unsafe {
            _builtin_make_temp_array(dest, addr(src[0]), capacity)
            return <- dest
        }
    } else {
        var dest : array<TT -const -& -#>
        unsafe {
            _builtin_make_temp_array(dest, addr(src[0]), capacity)
            return <- dest
        }
    }
}

[unsafe_outside_of_for]
def public get(arch : Archetype; name : string; value : auto(TT)) {
    //! Creates temporary array of component given specific name and type of component.
    //! If component is not found - panic.
    let idx = arch.components |> lower_bound(Component(name = name)) <| $(x, y) => x.name < y.name
    if (idx < length(arch.components)) {
        let comp & = unsafe(arch.components[idx])
        if (comp.name == name) {
            unsafe {
                var cvinfo : TypeInfo const?
                static_if (typeinfo is_dim(value)) {
                    cvinfo  = addr(typeinfo rtti_typeinfo(type<TT[typeinfo dim(value)] -const -& -#>))
                } else {
                    cvinfo  = addr(typeinfo rtti_typeinfo(type<TT -const -& -#>))
                }
                if (comp.info.hash != cvinfo.hash) {
                    panic("decs: component array {name} type mismatch, expecting {describe(comp.info)} vs {describe(cvinfo)} MNH={get_mangled_name(cvinfo)} hash={cvinfo.hash} size={cvinfo.size}")
                }
                static_if (typeinfo is_dim(value)) {
                    return <- decs_array(type<TT[typeinfo dim(value)]>, comp.data, arch.size)
                } else {
                    return <- decs_array(type<TT>, comp.data, arch.size)
                }
            }
        }
    }
    panic("decs: component array {name} not found")
    unsafe {
        static_if (typeinfo is_dim(value)) {
            return <- array<TT[typeinfo dim(value)] -const -& -#>()
        } else {
            return <- array<TT -const -& -#>()
        }
    }
}

[expect_dim(value), unsafe_outside_of_for]
def public get_ro(arch : Archetype; name : string; value : auto(TT)[]) : array<TT[typeinfo sizeof(value)] -const -& -#> const {
    //! Returns const temporary array of component given specific name and type of component for array components.
    unsafe {
        return <- get(arch, name, value)
    }
}

[!expect_dim(value), unsafe_outside_of_for]
def public get_ro(arch : Archetype; name : string; value : auto(TT)) : array<TT -const -& -#> const {
    //! Returns const temporary array of component given specific name and type of component for regular components.
    unsafe {
        return <- get(arch, name, value)
    }
}

def public get_default_ro(arch : Archetype; name : string; value : auto(TT)) : iterator<TT const&> {
    //! Returns const iterator of component given specific name and type of component.
    //! If component is not found - iterator will keep returning the specified value.
    let idx = arch.components |> lower_bound(Component(name = name)) <| $(x, y) => x.name < y.name
    if (idx < length(arch.components)) {
        let comp & = unsafe(arch.components[idx])
        if (comp.name == name) {
            unsafe {
                var cvinfo : TypeInfo const?
                static_if (typeinfo is_dim(value)) {
                    cvinfo  = addr(typeinfo rtti_typeinfo(type<TT[typeinfo dim(value)] -const -& -#>))
                } else {
                    cvinfo  = addr(typeinfo rtti_typeinfo(type<TT -const -& -#>))
                }
                if (comp.info.hash != cvinfo.hash) {
                    panic("decs: component array {name} type mismatch, expecting {describe(comp.info)} vs {describe(cvinfo)} MNH={get_mangled_name(cvinfo)} hash={cvinfo.hash} size={cvinfo.size}")
                }
                static_if (typeinfo is_dim(value)) {
                    var it : iterator<TT[typeinfo dim(value)] const&>
                    _builtin_make_fixed_array_iterator(it, addr(comp.data[0]), arch.size, comp.stride)
                    return <- it
                } else {
                    var it : iterator<TT const&>
                    _builtin_make_fixed_array_iterator(it, addr(comp.data[0]), arch.size, comp.stride)
                    return <- it
                }
            }
        }
    }
    return <- repeat_ref(value, arch.size)
}

def public get_optional(arch : Archetype; name : string; value : auto(TT)?) : iterator<TT -const -& -#?> {
    //! Returns const iterator of component given specific name and type of component.
    //! If component is not found - iterator will keep returning default value for the component type.
    let idx = arch.components |> lower_bound(Component(name = name)) <| $(x, y) => x.name < y.name
    if (idx < length(arch.components)) {
        let comp & = unsafe(arch.components[idx])
        if (comp.name == name) {
            unsafe {
                let cvinfo = addr(typeinfo rtti_typeinfo(type<TT -const -& -#>))
                if (comp.info.hash != cvinfo.hash) {
                    panic("decs: component array {name} type mismatch, expecting {describe(comp.info)} vs {describe(cvinfo)} MNH={get_mangled_name(cvinfo)} hash={cvinfo.hash} size={cvinfo.size}")
                }
            }
            var it : iterator<TT -const -& -#?>
            unsafe {
                _builtin_make_fixed_array_iterator(it, addr(comp.data[0]), arch.size, comp.stride)
            }
            return <- it
        }
    }
    return <- repeat(default<TT -const -& -#?>, arch.size)
}

def private update_entity_imm(eid : EntityId; blk : lambda<(eid : EntityId; var cmp : ComponentMap) : void>) {
    var lookup = decsState.entityLookup[eid.id]
    if (lookup.generation != eid.generation) {
        return
    }
    var cmp : ComponentMap
    let arch_index = unsafe(decsState.archetypeLookup[lookup.archetype]) - 1
    let eidx = decsState.entityLookup[eid.id].index
    var old_ahash = 0ul
    {// note - this scope is for arch &
        var arch & = unsafe(decsState.allArchetypes[arch_index])
        cmp |> reserve(length(arch.components))
        for (c in arch.components) {
            var value = ComponentValue(name = c.name, info = c.info)
            unsafe {
                memcpy(addr(value.data), addr(c.data[eidx * c.stride]), c.stride)
            }
            cmp |> push(value)
        }
        old_ahash = arch.hash
    }
    invoke(blk, eid, cmp)
    cmp |> set("eid", eid)  // necessary?
    var new_ahash = cmp_archetype_hash(cmp)
    if (old_ahash == new_ahash) {
        for (c, comp in decsState.allArchetypes[arch_index].components, cmp) {
            unsafe {
                memcpy(addr(c.data[eidx * c.stride]), addr(comp.data), c.stride)
            }
        }
    } else {
        remove_entity(decsState.allArchetypes[arch_index], eidx)
        with_archetype(new_ahash) $(var narch; idx; isNew) {
            if (isNew) {
                narch |> create_archetype(cmp, idx)
            }
            let neidx = narch |> create_entity(eid, cmp)
            decsState.entityLookup[eid.id] =  (eid.generation, new_ahash, neidx)
        }
    }
    delete cmp
}

def private create_entity_imm(eid : EntityId; blk : lambda<(eid : EntityId; var cmp : ComponentMap) : void>) {
    var cmp : ComponentMap
    cmp |> push <| make_component("eid", eid)
    invoke(blk, eid, cmp)
    cmp |> set("eid", eid)       // necessary?
    var ahash = cmp_archetype_hash(cmp)
    with_archetype(ahash) $(var arch; idx; isNew) {
        if (isNew) {
            arch |> create_archetype(cmp, idx)
        }
        let eidx = arch |> create_entity(eid, cmp)
        decsState.entityLookup[eid.id] =  (eid.generation, ahash, eidx)
    }
    delete cmp
}

def private delete_entity_imm(eid : EntityId) {
    var lookup = decsState.entityLookup[eid.id]
    if (lookup.generation == eid.generation) {
        let di = decsState.entityLookup[eid.id].index
        decsState.entityLookup[eid.id].generation = 0
        decsState.entityFreeList |> push(eid)
        remove_entity(decsState.allArchetypes[unsafe(decsState.archetypeLookup[lookup.archetype]) - 1], di)
    }
}

def public update_entity(entityid : EntityId implicit; var blk : lambda<(eid : EntityId; var cmp : ComponentMap) : void>) {
    //! Creates deferred action to update entity specified by id.
    var deval <- @  capture(<- blk) (var act : DeferAction) {
        update_entity_imm(act.eid, blk)
    }
    deferActions |> emplace(DeferAction(action <- deval, eid = entityid))
}

def public create_entity(var blk : lambda<(eid : EntityId; var cmp : ComponentMap) : void>) {
    //! Creates deferred action to create entity.
    var deval <- @  capture(<- blk) (var act : DeferAction) {
        create_entity_imm(act.eid, blk)
    }
    var eid = new_entity_id()
    deferActions |> emplace(DeferAction(action <- deval, eid = eid))
    return eid
}

def public create_entities(count : int; blk : block<(eid : EntityId; i : int; var cmp : ComponentMap) : void>) {
    //! Creates `count` entities in bulk. All entities must have the same archetype
    //! (same set of components). This is much faster than calling `create_entity` in
    //! a loop because it does a single archetype lookup and a single resize per
    //! component array for the entire batch.
    //!
    //! Unlike `create_entity`, this is immediate — entities are visible to queries
    //! right after the call returns (no `commit` needed).
    //!
    //! Example::
    //!
    //!     create_entities(100) <| $(eid : EntityId; i : int; var cmp : ComponentMap)
    //!         apply_decs_template(cmp, Particle(pos = origin, vel = compute_vel(i)))
    //!
    if (count <= 0) {
        return
    }
    if (insideQuery != 0) {
        panic("decs: can't call `create_entities` from inside query")
    }
    // First entity: build component map to establish archetype
    var cmp : ComponentMap
    var first_eid = new_entity_id()
    cmp |> push <| make_component("eid", first_eid)
    invoke(blk, first_eid, 0, cmp)
    cmp |> set("eid", first_eid)
    var ahash = cmp_archetype_hash(cmp)
    with_archetype(ahash) <| $(var arch; arch_idx; isNew) {
        if (isNew) {
            arch |> create_archetype(cmp, arch_idx)
        }
        // Pre-resize all component arrays for count entities at once
        let base = arch.size
        for (c in arch.components) {
            c.data |> resize(length(c.data) + c.stride * count)
        }
        // Fill first entity
        for (c, comp in arch.components, cmp) {
            unsafe {
                memcpy(addr(c.data[base * c.stride]), addr(comp.data), c.stride)
            }
        }
        decsState.entityLookup[first_eid.id] = (first_eid.generation, ahash, base)
        // Fill remaining entities (reuse cmp — set overwrites existing entries)
        for (i in range(1, count)) {
            var eid = new_entity_id()
            cmp |> set("eid", eid)
            invoke(blk, eid, i, cmp)
            cmp |> set("eid", eid)
            if (cmp_archetype_hash(cmp) != ahash) {
                panic("decs: create_entities — all entities must have the same components (entity {i} differs from entity 0)")
            }
            let eidx = base + i
            for (c, comp in arch.components, cmp) {
                unsafe {
                    memcpy(addr(c.data[eidx * c.stride]), addr(comp.data), c.stride)
                }
            }
            decsState.entityLookup[eid.id] = (eid.generation, ahash, eidx)
        }
        arch.size += count
    }
    delete cmp
}

def public create_entities_from_cmp(count : int; var cmp : ComponentMap;
                                    fill_blk : block<(var arch : Archetype; eidx : int; eid : EntityId; i : int) : void>) {
    //! Creates `count` entities in bulk with maximum performance. Takes a pre-built
    //! ComponentMap (used once to establish the archetype shape), then calls `fill_blk`
    //! for each entity to write directly into archetype storage, bypassing ComponentMap.
    //!
    //! Typically called by macro-generated ``create_entities`T`` functions rather than
    //! directly. See `create_entities` for the simpler ComponentMap-based API.
    if (count <= 0) {
        return
    }
    if (insideQuery != 0) {
        panic("decs: can't call `create_entities_from_cmp` from inside query")
    }
    // Ensure eid component is present for archetype hash computation
    cmp |> set("eid", INVALID_ENTITY_ID)
    var ahash = cmp_archetype_hash(cmp)
    with_archetype(ahash) <| $(var arch; arch_idx; isNew) {
        if (isNew) {
            arch |> create_archetype(cmp, arch_idx)
        }
        let base = arch.size
        for (c in arch.components) {
            c.data |> resize(length(c.data) + c.stride * count)
        }
        let id_base = uint(length(decsState.entityLookup))
        decsState.entityLookup |> resize(int(id_base) + count)
        for (i in range(count)) {
            let eid = EntityId(id = id_base + uint(i), generation = 1)
            let eidx = base + i
            invoke(fill_blk, arch, eidx, eid, i)
            decsState.entityLookup[eid.id] = (eid.generation, ahash, eidx)
        }
        arch.size += count
    }
}

def public delete_entity(entityid : EntityId implicit) {
    //! Creates deferred action to delete entity specified by id.
    var deval <- @(var act : DeferAction) {
        delete_entity_imm(act.eid)
    }
    deferActions |> emplace(DeferAction(action <- deval, eid = entityid))
}

def public commit {
    //! Finishes all deferred actions.
    if (insideQuery != 0) {
        panic("decs: can't call `commit` from inside query")
    }
    var actions <- deferActions
    for (da in actions) {
        da.action |> invoke(da)
    }
    delete actions
}

def public is_alive(eid : EntityId) : bool {
    //! Returns true if the entity is alive (exists and has not been deleted).
    //! An entity is alive when its id is within bounds and its generation matches the lookup table.
    if (eid == INVALID_ENTITY_ID) {
        return false
    }
    if (int(eid.id) >= length(decsState.entityLookup)) {
        return false
    }
    return decsState.entityLookup[eid.id].generation == eid.generation
}

def public entity_count() : int {
    //! Returns the total number of alive entities across all archetypes.
    var total = 0
    for (arch in decsState.allArchetypes) {
        total += arch.size
    }
    return total
}

def public get_component(eid : EntityId; name : string; defval : auto(TT)) : TT -const -& -# {
    //! Returns a copy of the named component for the given entity.
    //! If the entity is dead or the component is not found, returns ``defval``.
    //! The type of the component is inferred from the type of ``defval``.
    //! Panics if the component exists but its type does not match.
    if (!is_alive(eid)) {
        return defval
    }
    var lookup = decsState.entityLookup[eid.id]
    let arch_index = unsafe(decsState.archetypeLookup[lookup.archetype]) - 1
    var arch & = unsafe(decsState.allArchetypes[arch_index])
    let cidx = arch.components |> lower_bound(Component(name = name)) <| $(x, y) => x.name < y.name
    if (cidx < length(arch.components)) {
        var comp & = unsafe(arch.components[cidx])
        if (comp.name == name) {
            unsafe {
                var cvinfo : TypeInfo const?
                static_if (typeinfo is_dim(defval)) {
                    cvinfo = addr(typeinfo rtti_typeinfo(type<TT[typeinfo dim(defval)] -const -& -#>))
                } else {
                    cvinfo = addr(typeinfo rtti_typeinfo(type<TT -const -& -#>))
                }
                if (comp.info.hash != cvinfo.hash) {
                    panic("decs: get_component {name} type mismatch, expecting {describe(comp.info)} vs {describe(cvinfo)} MNH={get_mangled_name(cvinfo)} hash={cvinfo.hash} size={cvinfo.size}")
                }
                var result : TT -const -& -#
                memcpy(addr(result), addr(comp.data[lookup.index * comp.stride]), comp.stride)
                return result
            }
        }
    }
    return defval
}

def public get(var cmp : ComponentMap; name : string; var value : auto(TT)) {
    //! Gets component value specified by name and type.
    //! Will panic if name matches but type does not.
    let idx = lower_bound(cmp, ComponentValue(name = name)) <| $(x, y) => x.name < y.name
    if (idx < length(cmp) && cmp[idx].name == name) {
        unsafe {
            var cvinfo : TypeInfo const?
            static_if (typeinfo is_dim(value)) {
                cvinfo = addr(typeinfo rtti_typeinfo(type<TT[typeinfo dim(value)] -const -& -#>))
            } else {
                cvinfo = addr(typeinfo rtti_typeinfo(type<TT -const -& -#>))
            }
            if (cmp[idx].info.hash != cvinfo.hash) {
                panic("decs: get component {name} type mismatch, expecting {describe(cmp[idx].info)} vs {describe(cvinfo)} MNH={get_mangled_name(cvinfo)} hash={cvinfo.hash} size={cvinfo.size}")
            }
            memcpy(addr(value), addr(cmp[idx].data), typeinfo sizeof(type<TT const -&>))
        }
    }
    return value
}

def private make_callbacks(var cv : ComponentValue; value : auto(TT)) {
    cv.info.mkTypeInfo = @@() : TypeInfo const? {
        static_if (typeinfo is_dim(value)) {
            return unsafe(addr(typeinfo rtti_typeinfo(type<array<TT[typeinfo dim(value)] -const -& -#>>)))
        } else {
            return unsafe(addr(typeinfo rtti_typeinfo(type<array<TT -const -& -#>>)))
        }
    }
    cv.info.dumper = @@(elem : void?) {
        if (elem == null) {
            return ""
        }
        unsafe {
            static_if (typeinfo is_dim(value)) {
                var pTT = reinterpret<TT[typeinfo dim(value)] -const -& -#?> elem
                return "{*pTT}"
            } else {
                var pTT = reinterpret<TT -const -& -#?> elem
                return "{*pTT}"
            }
        }
    }
    cv.info.gc = @@(var src : array<uint8>) : lambda {
        // the idea with this one is that we make a lambda
        //  which would capture an array of the right size and type
        //  which points to the right data
        //  and when lambda is deleted - we memzero the temp array
        static_if (typeinfo is_dim(value)) {
            var gc_dummy : array<TT[typeinfo dim(value)] -const -& -#>
            let stride = typeinfo sizeof(type<TT[typeinfo dim(value)] -const -& -#>)
            let size = length(src) / stride
            unsafe {
                _builtin_make_temp_array(gc_dummy, addr(src[0]), size)
            }
            return <- @  capture(<- gc_dummy) {
                pass
            } finally {
                memzero(gc_dummy)
            }
        } else {
            var gc_dummy : array<TT -const -& -#>
            let stride = typeinfo sizeof(type<TT -const -& -#>)
            let size = length(src) / stride
            unsafe {
                _builtin_make_temp_array(gc_dummy, addr(src[0]), size)
            }
            return <- @  capture(<- gc_dummy) {
                pass
            } finally {
                memzero(gc_dummy)
            }
        }
    }
    cv.info.clonner = @@(var dst : array<uint8>; src : array<uint8>) {
        if (length(src) == 0) {
            return
        }
        static_if (typeinfo is_dim(value)) {
            let stride = typeinfo sizeof(type<TT[typeinfo dim(value)] -const -& -#>)
            dst |> resize(length(src))
            let size = length(src) / stride
            var dsrc, ssrc : array<TT[typeinfo dim(value)] -const -& -#>
            unsafe {
                _builtin_make_temp_array(dsrc, addr(dst[0]), size)
                _builtin_make_temp_array(ssrc, addr(src[0]), size)
            }
            for (d, s in dsrc, ssrc) {
                d := s
            }
        } else {
            let stride = typeinfo sizeof(type<TT -const -& -#>)
            dst |> resize(length(src))
            let size = length(src) / stride
            var dsrc, ssrc : array<TT -const -& -#>
            unsafe {
                _builtin_make_temp_array(dsrc, addr(dst[0]), size)
                _builtin_make_temp_array(ssrc, addr(src[0]), size)
            }
            for (d, s in dsrc, ssrc) {
                d := s
            }
        }
    }
    cv.info.serializer = @@(var arch : Archive; var src : array<uint8>; name : string) {
        static_if (typeinfo is_dim(value)) {
            var stride = typeinfo sizeof(type<TT[typeinfo dim(value)] -const -& -#>)
            if (arch.reading) {
                var wasStride : int
                arch |> _::serialize(wasStride)
                if (wasStride != stride) {
                    panic("decs: component '{name}' stride mismatch, expecting {stride}, got {wasStride}")
                } else {
                    var temp : array<TT[typeinfo dim(value)] -const -& -#>
                    arch |> _::serialize(temp)
                    let size = length(temp)
                    src |> resize(size * stride)
                    if (size > 0) {
                        unsafe {
                            memcpy(addr(src[0]), addr(temp[0]), size * stride)
                        }
                    }
                }
            } else {
                arch |> _::serialize(stride)
                var ssrc : array<TT[typeinfo dim(value)] -const -& -#>
                let size = length(src) / stride
                unsafe {
                    if (length(src) > 0) {
                        _builtin_make_temp_array(ssrc, addr(src[0]), size)
                    }
                }
                arch |> serialize(ssrc)
            }
        } else {
            var stride : int = typeinfo sizeof(type<TT -const -& -#>)
            if (arch.reading) {
                var wasStride : int
                arch |> _::serialize(wasStride)
                if (wasStride != stride) {
                    panic("decs: component '{name}' stride mismatch, expecting {stride}, got {wasStride}")
                } else {
                    var temp : array<TT -const -& -#>
                    arch |> _::serialize(temp)
                    let size = length(temp)
                    src |> resize(size * stride)
                    if (size > 0) {
                        unsafe {
                            memcpy(addr(src[0]), addr(temp[0]), size * stride)
                        }
                    }
                }
            } else {
                arch |> _::serialize(stride)
                var ssrc : array<TT -const -& -#>
                let size = length(src) / stride
                unsafe {
                    if (length(src) > 0) {
                        _builtin_make_temp_array(ssrc, addr(src[0]), size)
                    }
                }
                arch |> serialize(ssrc)
            }
        }
    }
    static_if (typeinfo can_delete(type<TT -const -& -#>)) {
        cv.info.eraser = @@(var arr : array<uint8>) {
            if (length(arr) == 0) {
                return
            }
            static_if (typeinfo is_dim(value)) {
                let stride = typeinfo sizeof(type<TT[typeinfo dim(value)] -const -& -#>)
                for (i in range(length(arr) / stride)) {
                    let offset = i * stride
                    unsafe {
                        var adel = reinterpret<TT[typeinfo dim(value)] -const -& -#?> addr(arr[offset])
                        delete * adel
                    }
                }
            } else {
                let stride = typeinfo sizeof(type<TT -const -& -#>)
                for (i in range(length(arr) / stride)) {
                    let offset = i * stride
                    unsafe {
                        var adel = reinterpret<TT -const -& -#?> addr(arr[offset])
                        delete * adel
                    }
                }
            }
        }
    }
}

def private make_component(name : string; value : auto(TT)) {
    var cv = ComponentValue(name = name)
    unsafe {
        var tinfo : TypeInfo const?
        var tname : string
        static_if (typeinfo is_dim(value)) {
            tinfo = addr(typeinfo rtti_typeinfo(type<TT[typeinfo dim(value)] -const -& -#>))
            tname = typeinfo fulltypename(type<TT[typeinfo dim(value)] -const -& -#>)
        } else {
            tinfo = addr(typeinfo rtti_typeinfo(type<TT -const -& -#>))
            tname = typeinfo fulltypename(type<TT -const -& -#>)
        }
        cv.info = CTypeInfo(basicType = tinfo.basicType, mangledName = get_mangled_name(tinfo),
            fullName = tname, hash = tinfo.hash, size = tinfo.size)
        make_callbacks(cv, value)
        memcpy(addr(cv.data), addr(value), typeinfo sizeof(value))
    }
    return cv
}

def public set(var cmp : ComponentMap; name : string; value : auto(TT)) {
    //! Set component value specified by name and type.
    //! If value already exists, it is overwritten. If already existing value type is not the same - panic.
    concept_assert(typeinfo sizeof(value) <= typeinfo sizeof(cmp[0].data), "unsupported component type {typeinfo typename(value)} (its too big)")
    let cv = make_component(name, value)
    let idx = lower_bound(cmp, ComponentValue(name = name)) <| $(x, y) => x.name < y.name
    if (idx < length(cmp) && cmp[idx].name == name) {
        if (cmp[idx].info.hash != cv.info.hash) {
            panic("decs: set component {name} type mismatch, expecting {describe(cmp[idx].info)} vs vs {describe(cv.info)} MNH={cv.info.mangledName} hash={cv.info.hash} size={cv.info.size}")
        }
        cmp[idx] = cv           // overwrite
    } else {
        cmp |> push(cv, idx)    // insert new one
    }
}

def public find_component_index(arch : Archetype; name : string) : int {
    //! Returns the index of a named component within the archetype's component array.
    //! Used to resolve component locations once, then write many entities via `set_direct_at`.
    for (i in range(length(arch.components))) {
        if (arch.components[i].name == name) {
            return i
        }
    }
    panic("decs: find_component_index — component '{name}' not found in archetype")
    return -1
}

def public set_direct_at(var arch : Archetype; comp_idx : int; eidx : int; value : auto(TT)) {
    //! Writes a component value directly into archetype storage by pre-resolved component index.
    //! O(1) per write — use `find_component_index` once, then this for each entity.
    unsafe {
        memcpy(addr(arch.components[comp_idx].data[eidx * arch.components[comp_idx].stride]), addr(value), typeinfo sizeof(value))
    }
}

def public set_direct(var arch : Archetype; name : string; eidx : int; value : auto(TT)) {
    //! Writes a component value directly into archetype storage at the given entity index.
    //! Convenience wrapper — does a name lookup per call. For bulk writes, prefer
    //! `find_component_index` + `set_direct_at` to resolve the name once.
    set_direct_at(arch, find_component_index(arch, name), eidx, value)
}

def public has(var cmp : ComponentMap; name : string) {
    //! Returns true if component map has specified component.
    return binary_search(cmp, ComponentValue(name = name)) <| $(x, y) => x.name < y.name
}

def public remove(var cmp : ComponentMap; name : string) {
    //! Removes specified value from the component map.
    let idx = lower_bound(cmp, ComponentValue(name = name)) <| $(x, y) => x.name < y.name
    if (idx < length(cmp) && cmp[idx].name == name) {
        cmp |> erase(idx)
    }
}

