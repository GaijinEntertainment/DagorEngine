#pragma once

namespace das
{
    enum class CompilationError : int
    {
        unspecified

// lexer errors

    ,   mismatching_parentheses                 =   10001
    ,   mismatching_curly_bracers               =   10002
    ,   string_constant_exceeds_file            =   10003
    ,   string_constant_exceeds_line            =   10004
    ,   unexpected_close_comment                =   10005       //  */ out of the bule
    ,   integer_constant_out_of_range           =   10006
    ,   comment_contains_eof                    =   10007
    ,   invalid_escape_sequence                 =   10008       //  blah/yblah
    ,   invalid_line_directive                  =   10009       // #row,col,"filename" is bad somehow

// parser errors

    ,   syntax_error                            =   20000
    ,   malformed_ast                           =   20001       // macro generated something, which is not a valid ast

// semantic erros

    ,   invalid_type                            =   30101       //  int & a[], int&&, int&*
    ,   invalid_return_type                     =   30102       //  func():blah&
    ,   invalid_argument_type                   =   30103       //  func(a:boxed&)
    ,   invalid_structure_field_type            =   30104       //  struct a:void
    ,   invalid_array_type                      =   30105       //  array<int&>
    ,   invalid_table_type                      =   30106       //  table<wtf,int> table<string&,int> table<int,string&>
    ,   invalid_argument_count                  =   30107       //  assert(), assert(blah,....)
    ,   invalid_variable_type                   =   30108       //  a:void
    ,   invalid_new_type                        =   30109       //  new int&, new int
    ,   invalid_index_type                      =   30110       //  a[wtf]
    ,   invalid_annotation                      =   30111       //  [wtf] a
    ,   invalid_swizzle_mask                    =   30112       //  vec.xxxxx or vec.xAz or vec.wtf
    ,   invalid_initialization_type             =   30113       //  int a = "b"
    ,   invalid_with_type                       =   30114       //  with int
    ,   invalid_override                        =   30115       //  override new_field:blah, or old_field:blah without override
    ,   invalid_name                            =   30116       //  __anything
    ,   invalid_array_dimension                 =   30117       //  int blah[non-const]
    ,   invalid_iteration_source                =   30118       //  for x in 10
    ,   invalid_loop                            =   30119       //  for x, y in a etc
    ,   invalid_label                           =   30120       //  label not found etc
    ,   invalid_enumeration                     =   30121       //  enum foo = "blah" etc
    ,   invalid_option                          =   30122       //  option wtf = wth
    ,   invalid_member_function                 =   30123       //  member function in struct
    ,   invalid_capture                         =   30124       //  capture section in non-lambda, capture on non-variable, unused catpure, etc
    ,   invalid_private                         =   30125       //  struct member private, etc
    ,   invalid_aka                             =   30126       //  aka with global let, typedecl, etc

    ,   function_already_declared               =   30201       //  func x .... func x
    ,   argument_already_declared               =   30202       //  func(...,a,...,a,....)
    ,   local_variable_already_declared         =   30203       //  let(...,x,...,x,...)
    ,   global_variable_already_declared        =   30204       //  let(...,x,...,x,...)
    ,   structure_field_already_declared        =   30205       //  struct ... x ... x
    ,   structure_already_declared              =   30206       //  ... struct x ... struct x ...
    ,   structure_already_has_initializer       =   30207       //  struct Foo x = 5; def Foo() ...
    ,   enumeration_already_declared            =   30208       //  enum A; enumA
    ,   enumeration_value_already_declared      =   30209       //  enum A { x; x }
    ,   type_alias_already_declared             =   30210       //  typdef A = b; typedef A = ...;
    ,   field_already_initialized               =   30211       //  typdef A = b; typedef A = ...;
    ,   too_many_arguments                      =   30212       //  func(x,y,z,....) with more than DAS_MAX_FUNCTION_ARGUMENTS arguments

    ,   type_not_found                          =   30301       //  a:wtf
    ,   structure_not_found                     =   30302       //  new wtf
    ,   operator_not_found                      =   30303       //  1 + 1.0
    ,   function_not_found                      =   30304       //  wtf(x)
    ,   variable_not_found                      =   30305       //  wtf
    ,   handle_not_found                        =   30306       //  external type has colliding name
    ,   annotation_not_found                    =   30307       //  [wtf] struct or [wtf] function
    ,   enumeration_not_found                   =   30308       //  WTF WTF enum
    ,   enumeration_value_not_found             =   30309       //  enumt WTF
    ,   type_alias_not_found                    =   30310       //  typedef A =; typedef A =; ...
    ,   bitfield_not_found                      =   30311       //  bitfield<one;two> a; a.three

    ,   cant_initialize                         =   30401       //  block type declaration, default values
    ,   cant_be_null                            =   30402       //  expression can't be null
    ,   exception_during_macro                  =   30403       //  exception during macro

    ,   cant_dereference                        =   30501
    ,   cant_index                              =   30502       //  wtf[a]
    ,   cant_get_field                          =   30503       //  wtf.field
    ,   cant_write_to_const                     =   30504       //  const int & a = 5
    ,   cant_move_to_const                      =   30505       //  const array<int> & a <- b
    ,   cant_write_to_non_reference             =   30506       //  1 = blah
    ,   cant_copy                               =   30507       //  a = array<int>(x), expecting <-
    ,   cant_move                               =   30508       //  int a; a <- 1
    ,   cant_pass_temporary                     =   30509       //  let t : int? = q // where q is int?#

    ,   condition_must_be_bool                  =   30601       //  if ( 10 ) ...
    ,   condition_must_be_static                =   30602       //  if ( constexpr ) only

    ,   cant_pipe                               =   30701       //  wtf <| arg

    ,   invalid_block                           =   30801       //  fn({ return; }), fn ({ break; })
    ,   return_or_break_in_finally              =   30802       //  if a {} finally { return blah; }

    ,   module_not_found                        =   30901       //  require wtf
    ,   module_already_has_a_name               =   30902       //  module a; module aa
    ,   module_does_not_export_unused_symbols   =   30903       //  options remove_unused_symbols = false is missing from the module
    ,   module_does_not_have_a_name             =   30904       //  missing module name for AOT module
    ,   module_required_from_shared             =   30905       //  require non shared foo from shared bar

    ,   cant_new_handle                         =   31001       //  new Handle
    ,   bad_delete                              =   31002       //  delete ;

    ,   cant_infer_generic                      =   31100       // TEMPORARY, REMOVE ONCE GENERICS ARE SUPPORTED
    ,   cant_infer_missing_initializer          =   31101       //  let x = 5
    ,   cant_infer_mismatching_restrictions     =   31102       //  let x : auto [5] = int[4][3]

    ,   invalid_cast                            =   31200       //  cast<Goo> ...
    ,   incompatible_cast                       =   31201       //  cast<NotBarParent> bar

    ,   unsafe                                  =   31300       // unsafe operation in safe function

    ,   index_out_of_range                      =   31400       // a:int[3] a[4]

    ,   expecting_return_value                  =   32101       // def blah:int without return
    ,   not_expecting_return_value              =   32102       // def blah:void ... return 12
    ,   invalid_return_semantics                =   32103       // return <- required
    ,   invalid_yield                           =   32104       // yield in block, yield in non generator, etc

    ,   unsupported_read_macro                  =   33100       // #what ""
    ,   unsupported_call_macro                  =   33101       // apply failed etc
    ,   unbound_macro_tag                       =   33102       // macro tag in the regular code

    ,   typeinfo_reference                      =   39901       //  typeinfo(sizeof type int&)
    ,   typeinfo_auto                           =   39902       //  typeinfo(typename type auto)
    ,   typeinfo_undefined                      =   39903       //  typeinfo(??? ...)
    ,   typeinfo_dim                            =   39904       //  typeinfo(dim non_array)
    ,   typeinfo_macro_error                    =   39905       //  typeinfo(custom ...) returns errors

// logic errors

    ,   static_assert_failed                    =   40100       // static_assert(false)
    ,   run_failed                              =   40101       //  [run]def fn; ..... fn(nonconst)
    ,   annotation_failed                       =   40102       // annotation->verifyCall failed
    ,   concept_failed                          =   40103       // similar to static_assert(false), but error reported 1up on stack
    ,   macro_failed                            =   40104       // user did something in the macro, it went kaboom

    ,   not_all_paths_return_value              =   40200       // def a() { if true return 1; }
    ,   assert_with_side_effects                =   40201       // assert(i++)
    ,   only_fast_aot_no_cpp_name               =   40202       // blah() of the function without cppName
    ,   aot_side_effects                        =   40203       // eval(a++,b++,a+b)
    ,   no_global_heap                          =   40204       // let a = {{ }}
    ,   no_global_variables                     =   40205       // var a = ...
    ,   unused_function_argument                =   40206       // def foo ( a ) ..... /* no a here */
    ,   unsafe_function                         =   40207       // [unsafe] when code of policies prohibits
    ,   unused_block_argument                   =   40208       // foo() <| $ ( a ) ..... /* no a here */
    ,   top_level_no_sideeffect_operation       =   40209       // a == b
    ,   deprecated_function                     =   40210       // [deprecated] function
    ,   argument_aliasing                       =   40211       // a = fun(a) with some form of potential cmres
    ,   make_local_aliasing                     =   40212       // a = [[... a.x ...]] with some form of potential cmres
    ,   in_scope_in_the_loop                    =   40213       // for ( a in b ) { let in scope ... ; }
    ,   no_init                                 =   40214       // [init] disabled via options or CodeOfPolicies

    ,   duplicate_key                           =   40300       // { 1:1, ..., 1:* }

    ,   too_many_infer_passes                   =   41000

// integration errors

    ,   missing_node                            =   50100       // handled type requires simulateGetXXX
    ,   missing_aot                             =   50101       // AOT hash function missing
    };
}
