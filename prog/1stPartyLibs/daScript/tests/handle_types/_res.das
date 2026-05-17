options gen2

module _res shared public

require daslib/typemacro_boost
require daslib/class_boost

//! Helper for ``failed_position_template_ptr.das`` — verbatim copy of the
//! user-reported ``res`` module. Renamed from ``res`` to ``_res`` to match the
//! daslang requirement that helper module filenames start with ``_``.

[template_structure(T)]
struct template Res {
    value_ptr : T?
}
