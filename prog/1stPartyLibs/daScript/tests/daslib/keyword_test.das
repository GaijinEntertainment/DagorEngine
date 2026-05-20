options gen2
options no_aot
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require ast_core
require dastest/testing_boost public

[test]
def test_is_cpp_keyword(t : T?) {
    // positive
    t |> success(is_cpp_keyword("int"), "int is a C++ keyword")
    t |> success(is_cpp_keyword("class"), "class is a C++ keyword")
    t |> success(is_cpp_keyword("virtual"), "virtual is a C++ keyword")
    t |> success(is_cpp_keyword("constexpr"), "constexpr is a C++ keyword")
    t |> success(is_cpp_keyword("co_await"), "co_await is a C++ keyword")
    t |> success(is_cpp_keyword("override"), "override is a C++ keyword")
    t |> success(is_cpp_keyword("nullptr"), "nullptr is a C++ keyword")
    t |> success(is_cpp_keyword("template"), "template is a C++ keyword")
    // negative
    t |> success(!is_cpp_keyword("foo"), "foo is not a C++ keyword")
    t |> success(!is_cpp_keyword(""), "empty string is not a C++ keyword")
    t |> success(!is_cpp_keyword("def"), "def is not a C++ keyword")
    t |> success(!is_cpp_keyword("yield"), "yield is not a C++ keyword")
    t |> success(!is_cpp_keyword("recover"), "recover is not a C++ keyword")
}

[test]
def test_is_das_keyword(t : T?) {
    // positive
    t |> success(is_das_keyword("def"), "def is a daScript keyword")
    t |> success(is_das_keyword("let"), "let is a daScript keyword")
    t |> success(is_das_keyword("struct"), "struct is a daScript keyword")
    t |> success(is_das_keyword("yield"), "yield is a daScript keyword")
    t |> success(is_das_keyword("smart_ptr"), "smart_ptr is a daScript keyword")
    t |> success(is_das_keyword("static_if"), "static_if is a daScript keyword")
    t |> success(is_das_keyword("float4"), "float4 is a daScript keyword")
    t |> success(is_das_keyword("recover"), "recover is a daScript keyword")
    t |> success(is_das_keyword("fixed_array"), "fixed_array is a daScript keyword")
    // negative
    t |> success(!is_das_keyword("foo"), "foo is not a daScript keyword")
    t |> success(!is_das_keyword(""), "empty string is not a daScript keyword")
    t |> success(!is_das_keyword("virtual"), "virtual is not a daScript keyword")
    t |> success(!is_das_keyword("constexpr"), "constexpr is not a daScript keyword")
    t |> success(!is_das_keyword("namespace"), "namespace is not a daScript keyword")
}

[test]
def test_overlap_keywords(t : T?) {
    // keywords in both languages
    t |> success(is_cpp_keyword("class") && is_das_keyword("class"), "class is in both")
    t |> success(is_cpp_keyword("struct") && is_das_keyword("struct"), "struct is in both")
    t |> success(is_cpp_keyword("return") && is_das_keyword("return"), "return is in both")
    t |> success(is_cpp_keyword("enum") && is_das_keyword("enum"), "enum is in both")
    t |> success(is_cpp_keyword("template") && is_das_keyword("template"), "template is in both")
    // C++ only
    t |> success(is_cpp_keyword("virtual") && !is_das_keyword("virtual"), "virtual is C++ only")
    t |> success(is_cpp_keyword("namespace") && !is_das_keyword("namespace"), "namespace is C++ only")
    // daScript only
    t |> success(!is_cpp_keyword("def") && is_das_keyword("def"), "def is daScript only")
    t |> success(!is_cpp_keyword("recover") && is_das_keyword("recover"), "recover is daScript only")
    t |> success(!is_cpp_keyword("smart_ptr") && is_das_keyword("smart_ptr"), "smart_ptr is daScript only")
}
