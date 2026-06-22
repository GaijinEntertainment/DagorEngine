#include "daScript/misc/platform.h"
#include "daScript/misc/gc_node.h"
#include "daScript/misc/crash_handler.h"

#include <inttypes.h>
#include <stdlib.h>

namespace das {

    // debugging id - to break on

    static uint64_t gc_break_on_id = 0;

    // installed by the AST layer (ast_gc_report.cpp); see gc_node.h
    DAS_API gc_describe_fn gc_node_describe_hook = nullptr;

    // ---- gc_root statics ----

    static thread_local gc_root     s_gc_thread_root;
    static thread_local gc_root *   s_gc_active_root = &s_gc_thread_root;
    std::atomic<uint64_t>           gc_root::gc_next_id { 1 };

    static void gc_check_break ( uint64_t id );

    gc_root & gc_root::gc_get_thread_root() {
        return s_gc_thread_root;
    }

    gc_root * & gc_root::gc_get_active_root() {
        return s_gc_active_root;
    }

    // ---- gc_root ----

    gc_root::~gc_root() {
        if ( gc_first ) {
            gc_sweep();
        }
    }

    void gc_root::gc_link ( gc_node * node ) {
        DAS_ASSERT(node && "gc_link: null node");
        DAS_ASSERT(node->gc_owner == nullptr && "gc_link: node already on a root");
        // gc_check_break(node->gc_id);
        node->gc_owner = this;
        node->gc_prev = gc_last;
        node->gc_next = nullptr;
        if ( gc_last ) {
            gc_last->gc_next = node;
        } else {
            gc_first = node;
        }
        gc_last = node;
        gc_count++;
    }

    void gc_root::gc_unlink ( gc_node * node ) {
        DAS_ASSERT(node && "gc_unlink: null node");
        DAS_ASSERT(node->gc_owner == this && "gc_unlink: node not on this root");
        // gc_check_break(node->gc_id);
        if ( node->gc_prev ) {
            node->gc_prev->gc_next = node->gc_next;
        } else {
            gc_first = node->gc_next;
        }
        if ( node->gc_next ) {
            node->gc_next->gc_prev = node->gc_prev;
        } else {
            gc_last = node->gc_prev;
        }
        node->gc_prev = nullptr;
        node->gc_next = nullptr;
        node->gc_owner = nullptr;
        DAS_ASSERT(gc_count > 0);
        gc_count--;
    }

    void gc_root::gc_sweep() {
        gc_collecting = true;
        while ( gc_first ) {
            auto node = gc_first;
            gc_unlink(node);
            uint32_t swept_magic = GC_MAGIC_SWEPT_PREFIX | (node->gc_magic & 0xFFFFu);
#if DAS_GC_DEBUG
            // !!! DO NOT REMOVE !!!
            // Memory poisoning mode for debugging use-after-sweep.
            // Destroys the object but keeps memory allocated with 0xCD fill.
            // gc_magic (with type tag) and gc_id are restored so crashed nodes can be identified.
            // Set DAS_GC_DEBUG=1 to enable, then use DAS_GC_BREAK_ON_ID to find the creator.
            {
                uint64_t saved_id = node->gc_id;
                node->~gc_node();
#ifdef _MSC_VER
                auto sz = _msize(node);
#elif defined(__APPLE__)
                auto sz = malloc_size(node);
#else
                auto sz = malloc_usable_size(node);
#endif
                memset(node, 0xCD, sz);
                node->gc_magic = swept_magic;
                node->gc_id = saved_id;
            }
#else
            node->gc_magic = swept_magic;
            delete node;
#endif
        }
        gc_collecting = false;
    }

    void gc_root::gc_dump_to_thread_root() {
        auto & threadRoot = gc_get_thread_root();
        while ( gc_first ) {
            auto node = gc_first;
            gc_unlink(node);
            threadRoot.gc_link(node);
        }
    }

    void gc_root::gc_report() const {
        DAS_FATAL_LOG("gc_root %p: count=%" PRIu64 "\n", (const void *)this, gc_count);
        char atbuf[256];
        auto node = gc_first;
        while ( node ) {
            atbuf[0] = 0;
            if ( gc_node_describe_hook ) gc_node_describe_hook(node, atbuf, (int)sizeof(atbuf));
            DAS_FATAL_LOG("  node %p: id=%" PRIu64 " type=%s magic=0x%08x %s\n",
                (const void *)node, node->gc_id, node->gc_type_name(), node->gc_magic, atbuf);
            node = node->gc_next;
        }
    }

    // ---- gc_node debug break ----
    // Set gc_break_on_id via debugger or env DAS_GC_BREAK_ON_ID to break
    // when a specific gc_id is assigned. Useful for tracing where a leaked node was born.

    static uint64_t gc_init_break_on_id() {
        return 0;
    }

    static void gc_check_break ( uint64_t id ) {
        if ( gc_break_on_id == 0 ) {
            static bool once = true;
            if ( once ) { once = false; gc_break_on_id = gc_init_break_on_id(); }
        }
        if ( gc_break_on_id && id == gc_break_on_id ) {
            DAS_FATAL_LOG("gc_node break: id=%" PRIu64 "\n", id);
            print_current_stack_trace();
            os_debug_break();
        }
    }

    // ---- gc_node ----

    gc_node::gc_node() {
        gc_id = gc_root::gc_next_id.fetch_add(1, std::memory_order_relaxed);
        gc_check_break(gc_id);
        gc_root::gc_get_active_root()->gc_link(this);
    }

    gc_node::gc_node ( const gc_node & ) {
        // New node with new ID, linked to the active root. Does NOT copy list position.
        gc_id = gc_root::gc_next_id.fetch_add(1, std::memory_order_relaxed);
        gc_check_break(gc_id);
        gc_root::gc_get_active_root()->gc_link(this);
    }

    gc_node::gc_node ( gc_node && src ) {
        // Take over source's list position
        gc_magic = src.gc_magic;
        gc_id = src.gc_id;
        gc_prev = src.gc_prev;
        gc_next = src.gc_next;
        gc_owner = src.gc_owner;
        // Patch neighbors to point to us
        if ( gc_prev ) {
            gc_prev->gc_next = this;
        } else if ( gc_owner ) {
            gc_owner->gc_first = this;
        }
        if ( gc_next ) {
            gc_next->gc_prev = this;
        } else if ( gc_owner ) {
            gc_owner->gc_last = this;
        }
        // Detach source
        src.gc_prev = nullptr;
        src.gc_next = nullptr;
        src.gc_owner = nullptr;
        src.gc_magic = GC_MAGIC_DEAD;
    }

    gc_node & gc_node::operator = ( gc_node && src ) {
        if ( this == &src ) return *this;
        // Unlink self from current root
        if ( gc_owner ) {
            gc_owner->gc_unlink(this);
        }
        // Take over source's list position
        gc_magic = src.gc_magic;
        gc_id = src.gc_id;
        gc_prev = src.gc_prev;
        gc_next = src.gc_next;
        gc_owner = src.gc_owner;
        if ( gc_prev ) {
            gc_prev->gc_next = this;
        } else if ( gc_owner ) {
            gc_owner->gc_first = this;
        }
        if ( gc_next ) {
            gc_next->gc_prev = this;
        } else if ( gc_owner ) {
            gc_owner->gc_last = this;
        }
        src.gc_prev = nullptr;
        src.gc_next = nullptr;
        src.gc_owner = nullptr;
        src.gc_magic = GC_MAGIC_DEAD;
        return *this;
    }

    gc_node::~gc_node() {
        gc_check_break(gc_id);
        if ( gc_owner ) {
            // Node is being deleted outside of gc_sweep.
            // During the migration period, this can happen legitimately when old code
            // paths delete TypeDecl. In DAS_GC_DEBUG mode, assert to catch bugs.
#if DAS_GC_DEBUG
            if ( !gc_owner->gc_collecting ) {
                DAS_FATAL_LOG("gc_node id=%" PRIu64 " deleted outside of gc_sweep\n", gc_id);
                DAS_ASSERTF(false, "gc_node id=%" PRIu64 " deleted outside of gc_sweep", gc_id);
            }
#endif
            gc_owner->gc_unlink(this);
        }
        if ( gc_is_alive() ) {
            gc_magic = GC_MAGIC_DEAD;
        }
    }

    void gc_node::gc_unlink() {
        if ( gc_owner ) {
            gc_owner->gc_unlink(this);
        }
    }

    void gc_node::gc_link ( gc_root * root ) {
        DAS_ASSERT(gc_owner == nullptr && "gc_link: already linked");
        root->gc_link(this);
    }

    void gc_node::gc_assign ( gc_root * new_root ) {
        if ( gc_owner == new_root ) return;
        if ( gc_owner ) {
            gc_owner->gc_unlink(this);
        }
        new_root->gc_link(this);
    }

    const char * gc_node::gc_type_name() const {
        uint16_t tag = gc_magic & 0xFFFFu;
        switch ( tag ) {
            case GC_TAG_TYPEDECL:       return "TypeDecl";
            case GC_TAG_EXPRESSION:     return "Expression";
            case GC_TAG_MAKEFIELDDECL:  return "MakeFieldDecl";
            case GC_TAG_MAKESTRUCT:     return "MakeStruct";
            case GC_TAG_FUNCTION:       return "Function";
            case GC_TAG_ENUMERATION:    return "Enumeration";
            case GC_TAG_STRUCTURE:      return "Structure";
            case GC_TAG_VARIABLE:       return "Variable";
            default:                    return "gc_node";
        }
    }

    void gc_node::gc_verify() const {
        if ( !gc_is_alive() ) {
            const char * typeName = gc_type_name();
            if ( (gc_magic & 0xFFFF0000u) == GC_MAGIC_SWEPT_PREFIX ) {
                DAS_FATAL_ERROR("%s id=%" PRIu64 " was already swept (use-after-sweep)", typeName, gc_id);
            } else if ( gc_magic == GC_MAGIC_DEAD ) {
                DAS_FATAL_ERROR("%s id=%" PRIu64 " was already deleted (use-after-free)", typeName, gc_id);
            } else {
                DAS_FATAL_ERROR("%s id=%" PRIu64 " has corrupt magic=0x%08x", typeName, gc_id, gc_magic);
            }
        }
    }

    // ---- gc_guard ----

    gc_guard::gc_guard() {
        saved_thread_root = gc_root::gc_get_active_root();
        gc_root::gc_get_active_root() = &guard_root;
    }

    gc_guard::~gc_guard() {
        gc_root::gc_get_active_root() = saved_thread_root;
        guard_root.gc_sweep();
    }

    gc_active_scope::gc_active_scope ( gc_root * use ) {
        saved_active = gc_root::gc_get_active_root();
        gc_root::gc_get_active_root() = use;
    }

    gc_active_scope::~gc_active_scope() {
        gc_root::gc_get_active_root() = saved_active;
    }

}
