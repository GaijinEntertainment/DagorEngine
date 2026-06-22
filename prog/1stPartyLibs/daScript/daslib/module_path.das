options gen2
options indenting = 4
options strict_smart_pointers = true

module module_path shared public

//! Runtime resolution of *where this module's directory currently lives*.
//!
//! daslang code that ships runtime assets alongside its source files
//! (SVGs, fonts, shaders, data files) typically computes the asset
//! directory at startup with::
//!
//!     let DIR = dir_name(get_module_file_name("my_module"))
//!     let svg = "{DIR}/cards.svg"
//!
//! That works in dev (interpreted from the source tree) and in an SDK /
//! cmake-install layout, but breaks for ``daspkg release`` standalone
//! bundles: ``get_module_file_name`` returns the path baked at compile
//! time, which doesn't exist next to the redistributable exe.
//!
//! ``get_this_module_dir()`` solves this by capturing the *call-site*
//! source-file path at macro expansion, then doing a 3-tier walk at
//! runtime — same policy PR #2579 introduced for ``.shared_module``
//! resolution:
//!
//! 1. ``<exe_dir>/<rel>``     — daspkg release standalone bundle
//! 2. ``<das_root>/<rel>``    — SDK / cmake install
//! 3. ``dir_name(baked)``     — dev (interpreted from source tree)
//!
//! ``<rel>`` is the substring of the call-site directory starting at
//! the last ``/modules/`` segment (e.g. ``modules/dasCards/cards``).
//! Tiers 1+2 are skipped when the call site has no ``/modules/`` segment
//! (project-local code outside the package layout); tier 3 then returns
//! the baked dir if it still exists on disk (dev) or ``<exe_dir>`` if it
//! doesn't (relocated bundle). The result is never empty.
//!
//! Usage::
//!
//!     require daslib/module_path
//!
//!     def load_assets() {
//!         let dir = get_this_module_dir()
//!         let svg = "{dir}/cards.svg"
//!         // ...
//!     }
//!
//! Call from inside a function — *not* a top-level ``let`` initializer.
//! daslang's ``-exe`` JIT path has a known issue with module-init ``let``
//! initializers that call Context-allocating builtins: the codegen-time
//! address gets baked and ASLR makes it wrong in the standalone exe.
//! The same call works fine inside any function body. Also affects
//! ``let X = get_das_root()`` and ``let X = dir_name(get_module_file_name(M))``.

require fio_core public
require daslib/ast_boost
require daslib/macro_boost
require daslib/templates_boost

[call_macro(name="get_this_module_dir")]
class private GetThisModuleDirMacro : AstCallMacro {
    //! Expands to a call into a 3-tier directory resolver, baking the
    //! call site's source-file path as a string literal so the resolver
    //! can derive the modules-relative suffix at runtime.
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        macro_verify(empty(call.arguments), prog, call.at, "expecting get_this_module_dir()")
        let baked = string(call.at.fileInfo.name)
        return qmacro(__builtin_resolve_this_module_dir($v(baked)))
    }
}
