#pragma once

#include <cstdint>
#include <atomic>

// Enable DAS_GC_DEBUG to keep swept nodes alive for use-after-sweep detection.
// Swept nodes get GC_MAGIC_SWEPT stamped but memory is not freed until gc_root destructor.
#ifndef DAS_GC_DEBUG
#define DAS_GC_DEBUG 0
#endif

// Alive magic: 0x1ee7TTTT where TTTT is a type tag.
// gc_is_alive() checks the upper 16 bits; gc_type_tag() reads the lower 16.
#define GC_MAGIC_ALIVE_PREFIX   0x1ee70000u
#define GC_TAG_TYPEDECL         0x0001u
#define GC_TAG_EXPRESSION       0x0002u
#define GC_TAG_MAKEFIELDDECL   0x0003u
#define GC_TAG_MAKESTRUCT      0x0004u
#define GC_TAG_FUNCTION        0x0005u
#define GC_TAG_ENUMERATION     0x0006u
#define GC_TAG_STRUCTURE       0x0007u
#define GC_TAG_VARIABLE        0x0008u
#define GC_MAGIC_TYPEDECL       (GC_MAGIC_ALIVE_PREFIX | GC_TAG_TYPEDECL)       // 0x1ee70001
#define GC_MAGIC_EXPRESSION     (GC_MAGIC_ALIVE_PREFIX | GC_TAG_EXPRESSION)     // 0x1ee70002
#define GC_MAGIC_MAKEFIELDDECL (GC_MAGIC_ALIVE_PREFIX | GC_TAG_MAKEFIELDDECL)  // 0x1ee70003
#define GC_MAGIC_MAKESTRUCT    (GC_MAGIC_ALIVE_PREFIX | GC_TAG_MAKESTRUCT)     // 0x1ee70004
#define GC_MAGIC_FUNCTION      (GC_MAGIC_ALIVE_PREFIX | GC_TAG_FUNCTION)       // 0x1ee70005
#define GC_MAGIC_ENUMERATION  (GC_MAGIC_ALIVE_PREFIX | GC_TAG_ENUMERATION)    // 0x1ee70006
#define GC_MAGIC_STRUCTURE    (GC_MAGIC_ALIVE_PREFIX | GC_TAG_STRUCTURE)      // 0x1ee70007
#define GC_MAGIC_VARIABLE     (GC_MAGIC_ALIVE_PREFIX | GC_TAG_VARIABLE)       // 0x1ee70008
#define GC_MAGIC_DEAD           0xdeadc0deu
#define GC_MAGIC_SWEPT_PREFIX   0x5ee90000u

namespace das {

    struct gc_root;

    // gc_node — base class for GC-managed AST nodes.
    // Each node lives on exactly one gc_root's doubly-linked list.
    // New nodes are linked to the thread-local root by default.
    // Module::gc_collect() moves reachable nodes to the module's root;
    // whatever remains on the thread-local root is garbage and gets swept.
    struct DAS_API gc_node {
        uint32_t    gc_magic = GC_MAGIC_ALIVE_PREFIX;
        gc_node *   gc_prev = nullptr;
        gc_node *   gc_next = nullptr;
        gc_root *   gc_owner = nullptr;
        uint64_t    gc_id = 0;

        gc_node();                              // links to thread-local root, assigns gc_id
        gc_node ( const gc_node & );            // new node, new ID, links to thread-local root
        gc_node ( gc_node && src );             // takes over source's list position
        gc_node & operator = ( const gc_node & ) = delete;
        gc_node & operator = ( gc_node && src );
        virtual ~gc_node();                     // unlinks from current root

        void gc_unlink();                       // remove from current root (sets gc_owner=nullptr)
        void gc_link ( gc_root * root );        // add to root's list
        void gc_assign ( gc_root * new_root );  // O(1) unlink + relink
        bool gc_is_alive() const { return (gc_magic & 0xFFFF0000u) == GC_MAGIC_ALIVE_PREFIX; }
        uint16_t gc_type_tag() const { return gc_magic & 0xFFFFu; }
        const char * gc_type_name() const;      // "TypeDecl", "Expression", or "gc_node"
        void gc_verify() const;                 // asserts gc_is_alive(), reports gc_id on failure

        // Compatibility stubs for smart_ptr_raw<T>::marshal().
        // Expression/TypeDecl are no longer refcounted, but ast_gen.inc
        // calls marshal() which instantiates smart_ptr_policy<T>::delRef.
        __forceinline void addRef() {}
        __forceinline bool delRef() { return false; }
        __forceinline unsigned int use_count() const { return 1; }
    };

    // gc_root — owns a doubly-linked list of gc_nodes.
    // Thread-local root is the default allocation target.
    // Module has its own root. gc_sweep() deletes all remaining nodes.
    struct DAS_API gc_root {
        gc_node *   gc_first = nullptr;
        gc_node *   gc_last = nullptr;
        uint64_t    gc_count = 0;
        bool        gc_collecting = false;      // delete guard — set during gc_sweep

        gc_root() = default;
        gc_root ( const gc_root & ) = delete;
        gc_root & operator = ( const gc_root & ) = delete;
        ~gc_root();                             // sweeps remaining nodes

        void gc_link ( gc_node * node );
        void gc_unlink ( gc_node * node );
        void gc_sweep();                        // deletes all nodes on this root
        void gc_dump_to_thread_root();          // move all nodes from this root to thread root
        void gc_report() const;                 // debug: print node IDs and count

        static gc_root &     gc_get_thread_root();   // permanent per-thread root
        static gc_root * &   gc_get_active_root();   // current target for new nodes
        static std::atomic<uint64_t>    gc_next_id;
    };

    // gc_local<T> — RAII holder for temporary gc_node-derived objects.
    // Unlinks from the GC root on construction, deletes on destruction.
    // Use when code creates a gc_node (e.g. TypeDecl) that is only needed locally
    // and should not participate in GC collection/sweep.
    // Moveable, so it works in vectors for complex cases.
    template<typename T>
    struct gc_local {
        T * ptr;
        gc_local ( T * p = nullptr ) : ptr(p) { if (ptr) ptr->gc_unlink(); }
        ~gc_local() { delete ptr; }
        gc_local ( gc_local && o ) noexcept : ptr(o.ptr) { o.ptr = nullptr; }
        gc_local & operator = ( gc_local && o ) noexcept {
            if ( this != &o ) { delete ptr; ptr = o.ptr; o.ptr = nullptr; }
            return *this;
        }
        gc_local ( const gc_local & ) = delete;
        gc_local & operator = ( const gc_local & ) = delete;
        operator T * () const { return ptr; }
        T * operator -> () const { return ptr; }
        T & operator * () const { return *ptr; }
    };

    // gc_guard — RAII scoped cleanup.
    // Redirects the thread-local root to a local root for the duration.
    // On destruction, sweeps the local root (deletes unclaimed nodes).
    // Nodes moved to a module root during the scope survive.
    struct DAS_API gc_guard {
        gc_root     guard_root;
        gc_root *   saved_thread_root;

        gc_guard();
        ~gc_guard();

        gc_guard ( const gc_guard & ) = delete;
        gc_guard & operator = ( const gc_guard & ) = delete;
    };

}
