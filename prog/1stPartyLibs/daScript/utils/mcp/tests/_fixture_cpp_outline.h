// Test fixture for cpp_outline v2 (utils/mcp/tools/cpp_outline.das).
// Each section deliberately exercises one of the v2 TODO items.

#define MY_EXPORT __attribute__((visibility("default")))

namespace fixns {

// item 6 — template specializations show distinct args in the outline
template <typename T>
struct Trait {
    static const int value = 0;
};

template <>
struct Trait<int> {
    static const int value = 1;
};

template <>
struct Trait<float> {
    static const int value = 2;
};

// item 4 — `class MY_EXPORT Outer` triggers the tree-sitter-cpp misparse
// that emits both a class_specifier and a bogus function_definition. The
// post-filter must drop the function entry; the surviving class entry must
// have its end_line transferred from the misparsed range.
class MY_EXPORT Outer {
public:
    int x;

    Outer() : x(0) {}

    // items 1, 5 — multi-line declaration with default args; signature must
    // collapse to a single line and preserve every parameter.
    void describe(
        const char * name,
        int count = 0,
        bool verbose = false
    ) const;

    // item 2 — Inner is nested inside Outer; tree mode indents it
    class Inner {
    public:
        void method();
    };

    // item 3 — anonymous union + anonymous enum must NOT appear in the outline.
    // Templates often hide an `enum { value = ... }` inside a struct body; that
    // also has no extractable name and should be filtered out.
    union {
        int i;
        float f;
    };

    enum {
        kFlagA = 1,
        kFlagB = 2,
    };
};

// item 7 — out-of-class definition; qualified name `Outer::describe` should
// be preserved and not double-prefixed in flat mode.
void Outer::describe(
    const char * name,
    int count,
    bool verbose
) const {
    (void)name;
    (void)count;
    (void)verbose;
}

// baseline — top-level free function for the flat-render comparison
void freeFunction(int x);

// ─── cpp_grep_usage corpus ───────────────────────────────────────────────
// The lines below exist purely for `do_cpp_grep_usage` to find. Each one
// uses `Outer` (the class declared above) in a different syntactic context
// that the previous `sg run -p` invocation could not match.

class GrepDerived : public Outer {  // base-class usage (namespace_identifier under base_class_clause)
public:
    Outer makeOuter();              // type_identifier in return position
    void takeOuter(const Outer & o); // type_identifier in parameter position
};

void usesQualifiedCall() {
    Outer x;                        // type_identifier in declaration
    x.describe("hi", 1, false);
    Outer::Inner inner;             // namespace_identifier in qualified type expr
}

}  // namespace fixns
