#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_gc_report.h"

#include <algorithm>
#include <stdlib.h>
#include <string.h>

namespace das {

    // ---- per-tag downcast ----
    // Only the AST layer knows the concrete subclass behind each gc tag, so this is the
    // one place that can recover a node's source `at`. MakeStruct (and any untagged node)
    // carries no `at` of its own -> nullptr.
    static const LineInfo * gc_node_at ( const gc_node * node ) {
        switch ( node->gc_type_tag() ) {
            case GC_TAG_TYPEDECL:       return &static_cast<const TypeDecl*>(node)->at;
            case GC_TAG_EXPRESSION:     return &static_cast<const Expression*>(node)->at;
            case GC_TAG_MAKEFIELDDECL:  return &static_cast<const MakeFieldDecl*>(node)->at;
            case GC_TAG_FUNCTION:       return &static_cast<const Function*>(node)->at;
            case GC_TAG_ENUMERATION:    return &static_cast<const Enumeration*>(node)->at;
            case GC_TAG_STRUCTURE:      return &static_cast<const Structure*>(node)->at;
            case GC_TAG_VARIABLE:       return &static_cast<const Variable*>(node)->at;
            default:                    return nullptr;     // MakeStruct, untagged, builtins
        }
    }

    static const char * gc_tag_name ( uint16_t tag ) {
        switch ( tag ) {
            case GC_TAG_TYPEDECL:       return "TypeDecl";
            case GC_TAG_EXPRESSION:     return "Expr";
            case GC_TAG_MAKEFIELDDECL:  return "MakeField";
            case GC_TAG_MAKESTRUCT:     return "MakeStruct";
            case GC_TAG_FUNCTION:       return "Function";
            case GC_TAG_ENUMERATION:    return "Enum";
            case GC_TAG_STRUCTURE:      return "Struct";
            case GC_TAG_VARIABLE:       return "Variable";
            default:                    return "other";
        }
    }

    // ---- describe hook (installed into gc_node base) ----

    static void gc_describe_impl ( const gc_node * node, char * buf, int buflen ) {
        const LineInfo * li = gc_node_at(node);
        if ( li && li->fileInfo ) {
            snprintf(buf, buflen, "%s:%u:%u", li->fileInfo->name.c_str(), li->line, li->column);
        } else {
            buf[0] = 0;
        }
    }

    // The hook variable is statically zero-initialized before any dynamic init, so setting
    // it here (dynamic init) has no ordering hazard. gc_report is a manual diagnostic and is
    // never called during static init, so lazy-install is unnecessary.
    static struct GcReportHookInstaller {
        GcReportHookInstaller () { gc_node_describe_hook = &gc_describe_impl; }
    } g_gcReportHookInstaller;

    // ---- histogram ----

    namespace {
        struct TagCounts {
            uint64_t n[16] = {};
            uint64_t total = 0;
            void add ( uint16_t tag ) { n[tag & 0xf]++; total++; }       // tags are < 16
        };
        struct FileBucket {
            TagCounts total;
            map<uint32_t, TagCounts> byLine;
        };
        typedef map<string, FileBucket> Histogram;
    }

    static void gc_build_histogram ( const gc_root & root, Histogram & histo ) {
        for ( auto node = root.gc_first; node; node = node->gc_next ) {
            uint16_t tag = node->gc_type_tag();
            const LineInfo * li = gc_node_at(node);
            string key;
            uint32_t line = 0;
            if ( li && li->fileInfo ) { key = li->fileInfo->name; line = li->line; }
            else { key = "<no source>"; }
            auto & fb = histo[key];
            fb.total.add(tag);
            fb.byLine[line].add(tag);
        }
    }

    // "[Expr 1200, TypeDecl 900]" — nonzero tags, descending by count.
    static void gc_format_breakdown ( const TagCounts & c, TextWriter & out ) {
        vector<pair<uint16_t,uint64_t>> v;
        for ( uint16_t t=0; t<16; ++t ) if ( c.n[t] ) v.push_back({t, c.n[t]});
        sort(v.begin(), v.end(), [](const pair<uint16_t,uint64_t> & a, const pair<uint16_t,uint64_t> & b){
            return a.second > b.second;
        });
        out << "[";
        for ( size_t i=0; i<v.size(); ++i ) {
            if ( i ) out << ", ";
            out << gc_tag_name(v[i].first) << " " << v[i].second;
        }
        out << "]";
    }

    void gcReportHistogram ( const gc_root & root, const char * label, TextWriter & out, uint64_t minCount ) {
        Histogram histo;
        gc_build_histogram(root, histo);
        // files, descending by total count
        vector<const pair<const string, FileBucket>*> files;
        for ( auto & kv : histo ) files.push_back(&kv);
        sort(files.begin(), files.end(),
            [](const pair<const string, FileBucket>* a, const pair<const string, FileBucket>* b){
                return a->second.total.total > b->second.total.total;
            });
        out << "=== AST gc histogram: " << (label ? label : "") << " — " << root.gc_count << " nodes ===\n";
        for ( auto fp : files ) {
            if ( fp->second.total.total < minCount ) continue;
            out << "  " << fp->first << "  " << fp->second.total.total << "  ";
            gc_format_breakdown(fp->second.total, out);
            out << "\n";
            vector<const pair<const uint32_t, TagCounts>*> lines;
            for ( auto & lkv : fp->second.byLine ) lines.push_back(&lkv);
            sort(lines.begin(), lines.end(),
                [](const pair<const uint32_t, TagCounts>* a, const pair<const uint32_t, TagCounts>* b){
                    return a->second.total > b->second.total;
                });
            for ( auto lp : lines ) {
                if ( lp->second.total < minCount ) continue;
                out << "    :" << lp->first << "  " << lp->second.total << "  ";
                gc_format_breakdown(lp->second, out);
                out << "\n";
            }
        }
    }

    // ---- per-stage delta ----

    bool gcStageReportEnabled () {
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
        static int cached = -1;
        if ( cached < 0 ) cached = getenv("DAS_GC_STAGE_REPORT") ? 1 : 0;
        return cached != 0;
#else
        return false;   // consoles have no environment variables
#endif
    }

    // signed per-(file,line) delta: cur - prev. n[] can go negative when a stage reclaims.
    namespace {
        struct SignedCounts {
            int64_t n[16] = {};
            int64_t total = 0;
        };
    }

    static void gc_format_signed ( const SignedCounts & c, TextWriter & out ) {
        vector<pair<uint16_t,int64_t>> v;
        for ( uint16_t t=0; t<16; ++t ) if ( c.n[t] ) v.push_back({t, c.n[t]});
        sort(v.begin(), v.end(), [](const pair<uint16_t,int64_t> & a, const pair<uint16_t,int64_t> & b){
            return llabs(a.second) > llabs(b.second);
        });
        out << "[";
        for ( size_t i=0; i<v.size(); ++i ) {
            if ( i ) out << ", ";
            out << gc_tag_name(v[i].first) << " " << (v[i].second>0?"+":"") << v[i].second;
        }
        out << "]";
    }

    void gcStageReportDelta ( const char * moduleName, const char * fileName, const char * stage, TextWriter & out ) {
        if ( !gcStageReportEnabled() ) return;
        static thread_local Histogram prev;          // baseline from the previous stage
        const bool isParse = stage && strcmp(stage, "parse")==0;
        if ( isParse ) prev.clear();                 // new module: drop the stale baseline
        const gc_root & root = *gc_root::gc_get_active_root();
        const char * unit = (moduleName && moduleName[0]) ? moduleName
                          : (fileName && fileName[0]) ? fileName : "<main>";
        Histogram cur;
        gc_build_histogram(root, cur);
        // union of file keys present in cur or prev
        set<string> fileKeys;
        for ( auto & kv : cur )  fileKeys.insert(kv.first);
        for ( auto & kv : prev ) fileKeys.insert(kv.first);
        // per-file signed totals, descending by magnitude
        struct FileDelta { string name; SignedCounts total; vector<pair<uint32_t,SignedCounts>> lines; };
        vector<FileDelta> deltas;
        for ( auto & fk : fileKeys ) {
            auto ci = cur.find(fk);
            auto pi = prev.find(fk);
            const FileBucket * cb = ci!=cur.end()  ? &ci->second : nullptr;
            const FileBucket * pb = pi!=prev.end() ? &pi->second : nullptr;
            set<uint32_t> lineKeys;
            if ( cb ) for ( auto & lk : cb->byLine ) lineKeys.insert(lk.first);
            if ( pb ) for ( auto & lk : pb->byLine ) lineKeys.insert(lk.first);
            FileDelta fd;
            fd.name = fk;
            for ( auto ln : lineKeys ) {
                SignedCounts sc;
                const TagCounts * cc = nullptr;
                const TagCounts * pc = nullptr;
                if ( cb ) { auto it = cb->byLine.find(ln); if ( it!=cb->byLine.end() ) cc = &it->second; }
                if ( pb ) { auto it = pb->byLine.find(ln); if ( it!=pb->byLine.end() ) pc = &it->second; }
                for ( int t=0; t<16; ++t ) {
                    int64_t d = (cc?(int64_t)cc->n[t]:0) - (pc?(int64_t)pc->n[t]:0);
                    sc.n[t] = d; sc.total += d;
                    fd.total.n[t] += d;
                }
                fd.total.total += sc.total;
                if ( sc.total != 0 ) fd.lines.push_back({ln, sc});
            }
            if ( fd.total.total != 0 ) deltas.push_back(std::move(fd));
        }
        sort(deltas.begin(), deltas.end(), [](const FileDelta & a, const FileDelta & b){
            return llabs(a.total.total) > llabs(b.total.total);
        });
        if ( deltas.empty() ) {
            out << "=== gc delta @ " << unit << " : " << (stage ? stage : "")
                << " — no net change (" << root.gc_count << " nodes) ===\n";
            prev = std::move(cur);
            return;
        }
        out << "=== gc delta @ " << unit << " : " << (stage ? stage : "")
            << " (active root: " << root.gc_count << " nodes) ===\n";
        for ( auto & fd : deltas ) {
            out << "  " << fd.name << "  " << (fd.total.total>0?"+":"") << fd.total.total << "  ";
            gc_format_signed(fd.total, out);
            out << "\n";
            sort(fd.lines.begin(), fd.lines.end(),
                [](const pair<uint32_t,SignedCounts> & a, const pair<uint32_t,SignedCounts> & b){
                    return llabs(a.second.total) > llabs(b.second.total);
                });
            for ( auto & lp : fd.lines ) {
                out << "    :" << lp.first << "  " << (lp.second.total>0?"+":"") << lp.second.total << "  ";
                gc_format_signed(lp.second, out);
                out << "\n";
            }
        }
        prev = std::move(cur);                        // new baseline for the next stage
    }

}
