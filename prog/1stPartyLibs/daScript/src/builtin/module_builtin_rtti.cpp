#include "daScript/ast/ast_simulate.h"
#include "daScript/misc/platform.h"

#include "module_builtin_rtti.h"

#include "daScript/simulate/simulate_nodes.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/simulate/sim_policy.h"
#include "daScript/simulate/fs_file_info.h"
#include "daScript/simulate/simulate_visit_op.h"

#include "daScript/misc/performance_time.h"
#include "daScript/ast/ast_serializer.h"
#include "daScript/misc/gc_node.h"

using namespace das;

IMPLEMENT_EXTERNAL_TYPE_FACTORY(FileInfo,FileInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(LineInfo,LineInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Annotation,Annotation)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(TypeAnnotation,TypeAnnotation)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(BasicStructureAnnotation,BasicStructureAnnotation)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(StructInfo,StructInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(EnumInfo,EnumInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(EnumValueInfo,EnumValueInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(TypeInfo,TypeInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(VarInfo,VarInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(LocalVariableInfo,LocalVariableInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(FuncInfo,FuncInfo)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(AnnotationArgument,AnnotationArgument)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(AnnotationArguments,AnnotationArguments)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(AnnotationArgumentList,AnnotationArgumentList)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(AnnotationDeclaration,AnnotationDeclaration)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(AnnotationList,AnnotationList)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(DebugInfoHelper,DebugInfoHelper)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Program,Program)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Module,Module)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Error,Error)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(FileAccess,FileAccess)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(Context,Context)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(SimFunction,SimFunction)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(CodeOfPolicies,CodeOfPolicies)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(ModuleGroup,ModuleGroup)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(recursive_mutex,das::recursive_mutex)
IMPLEMENT_EXTERNAL_TYPE_FACTORY(AstSerializer,das::AstSerializerState)

class EnumerationCompilationError : public das::Enumeration {
private:
    inline static const char *enumArrayName[] = {
    "unspecified", "invalid_line", "invalid_string", "mismatching_curly_bracers", "mismatching_module_name", "mismatching_parens",
    "exceeds_constant", "exceeds_file", "invalid_aka", "invalid_capture", "invalid_escape", "invalid_field",
    "invalid_field_static", "invalid_function", "invalid_function_annotation", "invalid_function_static", "invalid_global_aka", "invalid_macro",
    "invalid_module", "invalid_module_require", "invalid_name", "invalid_type_aka", "invalid_type_alias", "missing_module_name",
    "exceeds_bitfield", "ambiguous_function_argument_type", "already_declared_enumeration", "already_declared_enumerator", "already_declared_field", "already_declared_field_static",
    "already_declared_function_argument", "already_declared_global", "already_declared_global_bitfield", "already_declared_local", "already_declared_module", "already_declared_module_name",
    "already_declared_structure", "already_declared_type_alias", "lookup_annotation", "lookup_enumeration", "lookup_file", "lookup_module",
    "lookup_structure", "cant_structure", "runtime_annotation", "internal_module", "invalid_annotation", "invalid_annotation_field",
    "invalid_annotation_macro", "invalid_annotation_type", "invalid_argument", "invalid_argument_global", "invalid_argument_name", "invalid_argument_type",
    "invalid_array", "invalid_array_dimension", "invalid_array_dimension_type", "invalid_array_element_type", "invalid_array_type", "invalid_as",
    "invalid_ascend_array_handle_type", "invalid_ascend_handle_type", "invalid_assert_argument_count", "invalid_assert_comment_type", "invalid_assert_condition_type", "invalid_bitfield",
    "invalid_bitfield_cast_argument_count", "invalid_block_argument", "invalid_block_argument_count", "invalid_block_argument_init_type", "invalid_block_argument_type", "invalid_block_break",
    "invalid_block_continue", "invalid_block_finally", "invalid_break", "invalid_capture_variable", "invalid_cast_function", "invalid_cast_structure",
    "invalid_cast_structure_pointer", "invalid_cast_type", "invalid_class", "invalid_class_local", "invalid_class_tuple", "invalid_class_variant",
    "invalid_clone_smart_pointer_type", "invalid_comprehension_element_type", "invalid_continue", "invalid_debug_argument_count", "invalid_debug_comment_type", "invalid_delete_size_type",
    "invalid_delete_super_self_type", "invalid_empty_name", "invalid_enumeration", "invalid_enumeration_name", "invalid_enumerator", "invalid_enumerator_name", "invalid_enumerator_type",
    "invalid_erase_argument_count", "invalid_expression", "invalid_field_name", "invalid_field_syntax", "invalid_field_type", "invalid_finally_in_generator_if",
    "invalid_find_argument_count", "invalid_for_iterator_count", "invalid_for_iterator_tuple", "invalid_function_argument", "invalid_function_argument_count", "invalid_function_argument_type",
    "invalid_function_argument_type_block", "invalid_function_name", "invalid_function_options", "invalid_function_result", "invalid_function_result_discarded", "invalid_function_result_type",
    "invalid_function_type", "invalid_generator", "invalid_generator_argument_count", "invalid_generator_argument_type", "invalid_generator_result_type", "invalid_global",
    "invalid_global_init_options", "invalid_global_init_type", "invalid_global_options", "invalid_global_self_init", "invalid_global_shared", "invalid_global_type",
    "invalid_global_type_shared", "invalid_handle_index_type", "invalid_handle_safe_index_type", "invalid_if_condition_type", "invalid_index_type", "invalid_insert_argument_count",
    "invalid_invoke_argument_count", "invalid_invoke_argument_type", "invalid_invoke_method_syntax", "invalid_invoke_target_type", "invalid_is", "invalid_is_expression",
    "invalid_iteration_source_type", "invalid_key_exists_argument_count", "invalid_label_type", "invalid_local_in_scope", "invalid_local_in_scope_finally", "invalid_local_init",
    "invalid_local_init_block", "invalid_local_init_constructor", "invalid_local_init_type", "invalid_local_tuple_expansion", "invalid_local_type", "invalid_macro_context",
    "invalid_macro_read", "invalid_macro_tag", "invalid_macro_type", "invalid_memzero_argument", "invalid_memzero_argument_count", "invalid_memzero_argument_type",
    "invalid_module_name", "invalid_new_class_syntax", "invalid_new_initializer_result_type", "invalid_new_initializer_type", "invalid_new_type", "invalid_null_coalescing_type",
    "invalid_op3_expression", "invalid_pointer_arithmetic", "invalid_quote_argument_count", "invalid_result", "invalid_result_type", "invalid_return_semantics",
    "invalid_safe_as", "invalid_safe_dereference_type", "invalid_safe_field_type", "invalid_static_assert_argument_count", "invalid_static_assert_comment_type", "invalid_static_assert_condition_type",
    "invalid_static_if_condition", "invalid_storage_type_op", "invalid_string_builder_argument", "invalid_structure", "invalid_structure_annotation", "invalid_structure_array",
    "invalid_structure_block_pipe", "invalid_structure_field_init", "invalid_structure_field_type", "invalid_structure_initializer_required", "invalid_structure_local", "invalid_structure_name",
    "invalid_structure_template", "invalid_structure_tuple", "invalid_structure_type", "invalid_structure_variant", "invalid_super_call", "invalid_swizzle_mask",
    "invalid_swizzle_type", "invalid_table", "invalid_table_argument_type", "invalid_table_expression", "invalid_table_index_type", "invalid_table_key_type",
    "invalid_table_safe_index_type", "invalid_table_type", "invalid_tuple", "invalid_tuple_argument_type", "invalid_tuple_block", "invalid_tuple_key",
    "invalid_tuple_key_type", "invalid_tuple_type", "invalid_tuple_variant", "invalid_type", "invalid_type_dimension", "invalid_type_expression",
    "invalid_typeinfo", "invalid_typeinfo_annotation", "invalid_typeinfo_annotation_argument_type", "invalid_typeinfo_dim", "invalid_typeinfo_dim_table", "invalid_typeinfo_dim_table_type",
    "invalid_typeinfo_function", "invalid_typeinfo_has_field_type", "invalid_typeinfo_mangled_subexpression", "invalid_typeinfo_module_subexpression", "invalid_typeinfo_offsetof_type", "invalid_typeinfo_struct_get_annotation_argument_type",
    "invalid_typeinfo_struct_has_annotation_argument_type", "invalid_typeinfo_struct_has_annotation_type", "invalid_typeinfo_struct_modulename", "invalid_typeinfo_struct_name", "invalid_typeinfo_variant_index_type", "invalid_variable_name",
    "invalid_variable_private", "invalid_variant", "invalid_variant_array", "invalid_variant_block", "invalid_variant_initializer_count", "invalid_variant_tuple",
    "invalid_variant_type", "invalid_variant_unique", "invalid_while_condition_type", "invalid_with_array_type", "invalid_with_type", "invalid_yield",
    "invalid_yield_in_block", "missing_annotation", "missing_assume_type", "missing_bitfield_init", "missing_block_argument_init", "missing_block_result",
    "missing_enumeration_zero", "missing_finalizer", "missing_for_iterator", "missing_function_body", "missing_function_name", "missing_function_result",
    "missing_global", "missing_global_init", "missing_global_shared_init", "missing_local", "missing_local_block_init", "missing_local_init",
    "missing_local_reference_init", "missing_new_default_initializer", "missing_result", "missing_structure_field", "missing_typeinfo_subexpression", "mismatching_array_dimension",
    "mismatching_array_element_type", "mismatching_block_argument_type", "mismatching_clone_type", "mismatching_function_argument", "mismatching_function_argument_count", "mismatching_numeric_type",
    "mismatching_result_type", "mismatching_structure_dimension", "mismatching_tuple_argument_count", "mismatching_tuple_field_names", "mismatching_type", "mismatching_variant_dimension",
    "exceeds_argument", "exceeds_array_index", "exceeds_call_depth", "exceeds_expression_recursion", "exceeds_function_argument", "exceeds_infer_passes",
    "exceeds_local", "exceeds_new_argument", "exceeds_structure", "exceeds_tuple_index", "exceeds_type", "exceeds_type_alias",
    "exceeds_typeinfo_sizeof", "exceeds_constant_range", "ambiguous_annotation", "ambiguous_bitfield", "ambiguous_call_macro", "ambiguous_enumeration", "ambiguous_field",
    "ambiguous_field_lookup", "ambiguous_finalizer", "ambiguous_function", "ambiguous_macro", "ambiguous_structure", "ambiguous_super_call",
    "ambiguous_super_constructor", "ambiguous_type", "ambiguous_type_alias", "ambiguous_typeinfo_macro", "ambiguous_variable", "already_declared_assume_alias",
    "already_declared_block_argument", "already_declared_function", "already_declared_label", "already_declared_local_variable", "already_declared_structure_field_init", "already_declared_table",
    "already_declared_tuple_field", "already_declared_variable", "already_declared_with_shadow", "lookup_annotation_field", "lookup_argument_type", "lookup_bitfield",
    "lookup_block_argument_type", "lookup_block_result_type", "lookup_cast_type", "lookup_constructor", "lookup_expression_type", "lookup_field",
    "lookup_field_type", "lookup_function", "lookup_function_address_type", "lookup_function_argument_type", "lookup_function_mangled_name", "lookup_function_result_type",
    "lookup_function_type", "lookup_generator_type", "lookup_global_type", "lookup_is_expression_type", "lookup_label", "lookup_local_type",
    "lookup_macro", "lookup_method", "lookup_new_type", "lookup_safe_field_type", "lookup_structure_field", "lookup_structure_field_type",
    "lookup_super_class", "lookup_super_constructor", "lookup_super_finalizer", "lookup_super_method", "lookup_tuple_field", "lookup_type",
    "lookup_typeinfo_annotation", "lookup_typeinfo_annotation_argument", "lookup_typeinfo_macro", "lookup_typeinfo_offsetof_type", "lookup_typeinfo_type", "lookup_variable",
    "lookup_variant_field", "lookup_variant_type", "cant_access_private_field", "cant_access_private_structure", "cant_address_function", "cant_address_template_function",
    "cant_annotation_field", "cant_apply_op", "cant_argument", "cant_argument_structure", "cant_array_element", "cant_ascend",
    "cant_assign_op", "cant_block", "cant_capture_variable", "cant_clone", "cant_clone_type", "cant_copy",
    "cant_create_structure_annotation", "cant_delete", "cant_delete_local", "cant_delete_smart_pointer", "cant_delete_super", "cant_dereference",
    "cant_expression", "cant_field_class", "cant_finalize_block_annotation", "cant_finalize_function_annotation", "cant_finalize_structure_annotation", "cant_function",
    "cant_get_field", "cant_get_field_pointer", "cant_global", "cant_index", "cant_index_key", "cant_index_pointer",
    "cant_index_table", "cant_initialize_array_element", "cant_initialize_private_field", "cant_initialize_structure_field", "cant_initialize_variant_field", "cant_iterate_iterator",
    "cant_local", "cant_move", "cant_pointer", "cant_result", "cant_safe_get_field", "cant_safe_index",
    "cant_safe_index_table", "cant_structure_field", "cant_take_pointer", "cant_tuple", "cant_type", "cant_variant_field",
    "cant_write", "unsafe_address", "unsafe_argument", "unsafe_array_safe_index", "unsafe_capture_variable", "unsafe_cast",
    "unsafe_class", "unsafe_class_initializer", "unsafe_class_local", "unsafe_class_stack_construction", "unsafe_delete", "unsafe_delete_pointer",
    "unsafe_fixed_array_safe_index", "unsafe_function", "unsafe_function_call", "unsafe_global", "unsafe_global_pointer", "unsafe_local",
    "unsafe_local_class", "unsafe_local_in_scope_required", "unsafe_local_reference", "unsafe_local_type", "unsafe_move", "unsafe_operator",
    "unsafe_pointer_index", "unsafe_pointer_safe_index", "unsafe_return_block", "unsafe_return_function", "unsafe_return_reference", "unsafe_return_smart_pointer",
    "unsafe_smart_pointer", "unsafe_structure_field", "unsafe_structure_uninitialized", "unsafe_structure_visibility", "unsafe_table_index", "unsafe_table_safe_index",
    "unsafe_variant_field", "unsafe_variant_safe_as", "recursion_argument", "recursion_assume_alias", "recursion_function", "recursion_function_argument",
    "recursion_global", "recursion_structure", "recursion_type_alias", "runtime_annotation_transform", "runtime_call_macro", "runtime_expression",
    "runtime_macro_exception", "runtime_macro_infer", "runtime_macro_performance", "runtime_macro_style", "runtime_structure_annotation", "runtime_typeinfo_macro",
    "not_resolved_yet_argument_type", "not_resolved_yet_array_dimension", "not_resolved_yet_array_type", "not_resolved_yet_bitfield_type", "not_resolved_yet_block", "not_resolved_yet_block_argument",
    "not_resolved_yet_block_argument_init", "not_resolved_yet_class_type", "not_resolved_yet_comprehension_type", "not_resolved_yet_delete_size_type", "not_resolved_yet_dereference_type", "not_resolved_yet_enumerator",
    "not_resolved_yet_expression_type", "not_resolved_yet_function", "not_resolved_yet_function_block", "not_resolved_yet_generator_block", "not_resolved_yet_index_type", "not_resolved_yet_invoke_argument_type",
    "not_resolved_yet_is_expression_type", "not_resolved_yet_label_type", "not_resolved_yet_lambda_block", "not_resolved_yet_local_type", "not_resolved_yet_method_call", "not_resolved_yet_new_type",
    "not_resolved_yet_null_coalescing_type", "not_resolved_yet_structure", "not_resolved_yet_structure_field", "not_resolved_yet_structure_type", "not_resolved_yet_table_type", "not_resolved_yet_tuple_type",
    "not_resolved_yet_type", "not_resolved_yet_type_alias", "not_resolved_yet_typeinfo", "not_resolved_yet_typeinfo_alignof", "not_resolved_yet_typeinfo_dim", "not_resolved_yet_typeinfo_dim_table",
    "not_resolved_yet_typeinfo_sizeof", "concept_failed", "condition_must_be_bool", "not_expecting_result", "static_assert_failed", "invalid_options",
    "mismatching_runtime_type", "mismatching_runtime_type_hash", "exceeds_array", "exceeds_global", "exceeds_runtime_buffer", "cant_hash_block",
    "cant_hash_context", "cant_hash_iterator", "cant_serialize_block", "cant_serialize_iterator", "cant_serialize_null_pointer", "cant_serialize_pointer",
    "cant_serialize_table", "runtime_function", "runtime_function_annotation", "runtime_global", "runtime_macro", "internal_annotation",
    "internal_array", "internal_array_type", "internal_block", "internal_block_missing_return_type", "internal_class", "internal_enumeration",
    "internal_expression", "internal_field", "internal_function", "internal_function_annotation", "internal_function_changed", "internal_function_name",
    "internal_function_not_resolved_yet", "internal_function_refresh", "internal_generator", "internal_generator_finalizer", "internal_generator_finalizer_multiple", "internal_generator_finalizer_name",
    "internal_generator_function_name", "internal_generator_structure_name", "internal_global", "internal_label", "internal_lambda_finalizer_name", "internal_lambda_function_name",
    "internal_lambda_in_scope_conversion", "internal_lambda_structure_name", "internal_macro", "internal_name", "internal_options", "internal_pod_analysis_infer",
    "internal_relocate_infer", "internal_structure", "internal_structure_block", "internal_table", "internal_tuple", "internal_tuple_type",
    "internal_type", "internal_type_alias", "internal_typeinfo_macro", "internal_variable", "missing_aot", "syntax_error",
    "function_not_found", "function_already_declared", "invalid_return_type", "invalid_initialization_type", "invalid_private", "invalid_static",
    "aot_side_effects", "cant_pipe", "enumeration_value_already_declared", "invalid_escape_sequence", "invalid_option", "module_already_has_a_name",
    "type_alias_already_declared", "unsupported_read_macro"
    };
public:
    EnumerationCompilationError() : das::Enumeration("CompilationError") {
        external = true;
        cppName = "das::CompilationError";
        baseType = (das::Type) das::ToBasicType< das::underlying_type< das::CompilationError >::type >::type;
        das::CompilationError enumArray[] = {
    das::CompilationError::unspecified, das::CompilationError::invalid_line, das::CompilationError::invalid_string, das::CompilationError::mismatching_curly_bracers,
    das::CompilationError::mismatching_module_name, das::CompilationError::mismatching_parens, das::CompilationError::exceeds_constant, das::CompilationError::exceeds_file,
    das::CompilationError::invalid_aka, das::CompilationError::invalid_capture, das::CompilationError::invalid_escape, das::CompilationError::invalid_field,
    das::CompilationError::invalid_field_static, das::CompilationError::invalid_function, das::CompilationError::invalid_function_annotation, das::CompilationError::invalid_function_static,
    das::CompilationError::invalid_global_aka, das::CompilationError::invalid_macro, das::CompilationError::invalid_module, das::CompilationError::invalid_module_require,
    das::CompilationError::invalid_name, das::CompilationError::invalid_type_aka, das::CompilationError::invalid_type_alias, das::CompilationError::missing_module_name,
    das::CompilationError::exceeds_bitfield, das::CompilationError::ambiguous_function_argument_type, das::CompilationError::already_declared_enumeration, das::CompilationError::already_declared_enumerator,
    das::CompilationError::already_declared_field, das::CompilationError::already_declared_field_static, das::CompilationError::already_declared_function_argument, das::CompilationError::already_declared_global,
    das::CompilationError::already_declared_global_bitfield, das::CompilationError::already_declared_local, das::CompilationError::already_declared_module, das::CompilationError::already_declared_module_name,
    das::CompilationError::already_declared_structure, das::CompilationError::already_declared_type_alias, das::CompilationError::lookup_annotation, das::CompilationError::lookup_enumeration,
    das::CompilationError::lookup_file, das::CompilationError::lookup_module, das::CompilationError::lookup_structure, das::CompilationError::cant_structure,
    das::CompilationError::runtime_annotation, das::CompilationError::internal_module, das::CompilationError::invalid_annotation, das::CompilationError::invalid_annotation_field,
    das::CompilationError::invalid_annotation_macro, das::CompilationError::invalid_annotation_type, das::CompilationError::invalid_argument, das::CompilationError::invalid_argument_global,
    das::CompilationError::invalid_argument_name, das::CompilationError::invalid_argument_type, das::CompilationError::invalid_array, das::CompilationError::invalid_array_dimension,
    das::CompilationError::invalid_array_dimension_type, das::CompilationError::invalid_array_element_type, das::CompilationError::invalid_array_type, das::CompilationError::invalid_as,
    das::CompilationError::invalid_ascend_array_handle_type, das::CompilationError::invalid_ascend_handle_type, das::CompilationError::invalid_assert_argument_count, das::CompilationError::invalid_assert_comment_type,
    das::CompilationError::invalid_assert_condition_type, das::CompilationError::invalid_bitfield, das::CompilationError::invalid_bitfield_cast_argument_count, das::CompilationError::invalid_block_argument,
    das::CompilationError::invalid_block_argument_count, das::CompilationError::invalid_block_argument_init_type, das::CompilationError::invalid_block_argument_type, das::CompilationError::invalid_block_break,
    das::CompilationError::invalid_block_continue, das::CompilationError::invalid_block_finally, das::CompilationError::invalid_break, das::CompilationError::invalid_capture_variable,
    das::CompilationError::invalid_cast_function, das::CompilationError::invalid_cast_structure, das::CompilationError::invalid_cast_structure_pointer, das::CompilationError::invalid_cast_type,
    das::CompilationError::invalid_class, das::CompilationError::invalid_class_local, das::CompilationError::invalid_class_tuple, das::CompilationError::invalid_class_variant,
    das::CompilationError::invalid_clone_smart_pointer_type, das::CompilationError::invalid_comprehension_element_type, das::CompilationError::invalid_continue, das::CompilationError::invalid_debug_argument_count,
    das::CompilationError::invalid_debug_comment_type, das::CompilationError::invalid_delete_size_type, das::CompilationError::invalid_delete_super_self_type, das::CompilationError::invalid_empty_name,
    das::CompilationError::invalid_enumeration, das::CompilationError::invalid_enumeration_name, das::CompilationError::invalid_enumerator, das::CompilationError::invalid_enumerator_name, das::CompilationError::invalid_enumerator_type,
    das::CompilationError::invalid_erase_argument_count, das::CompilationError::invalid_expression, das::CompilationError::invalid_field_name, das::CompilationError::invalid_field_syntax,
    das::CompilationError::invalid_field_type, das::CompilationError::invalid_finally_in_generator_if, das::CompilationError::invalid_find_argument_count, das::CompilationError::invalid_for_iterator_count,
    das::CompilationError::invalid_for_iterator_tuple, das::CompilationError::invalid_function_argument, das::CompilationError::invalid_function_argument_count, das::CompilationError::invalid_function_argument_type,
    das::CompilationError::invalid_function_argument_type_block, das::CompilationError::invalid_function_name, das::CompilationError::invalid_function_options, das::CompilationError::invalid_function_result,
    das::CompilationError::invalid_function_result_discarded, das::CompilationError::invalid_function_result_type, das::CompilationError::invalid_function_type, das::CompilationError::invalid_generator,
    das::CompilationError::invalid_generator_argument_count, das::CompilationError::invalid_generator_argument_type, das::CompilationError::invalid_generator_result_type, das::CompilationError::invalid_global,
    das::CompilationError::invalid_global_init_options, das::CompilationError::invalid_global_init_type, das::CompilationError::invalid_global_options, das::CompilationError::invalid_global_self_init,
    das::CompilationError::invalid_global_shared, das::CompilationError::invalid_global_type, das::CompilationError::invalid_global_type_shared, das::CompilationError::invalid_handle_index_type,
    das::CompilationError::invalid_handle_safe_index_type, das::CompilationError::invalid_if_condition_type, das::CompilationError::invalid_index_type, das::CompilationError::invalid_insert_argument_count,
    das::CompilationError::invalid_invoke_argument_count, das::CompilationError::invalid_invoke_argument_type, das::CompilationError::invalid_invoke_method_syntax, das::CompilationError::invalid_invoke_target_type,
    das::CompilationError::invalid_is, das::CompilationError::invalid_is_expression, das::CompilationError::invalid_iteration_source_type, das::CompilationError::invalid_key_exists_argument_count,
    das::CompilationError::invalid_label_type, das::CompilationError::invalid_local_in_scope, das::CompilationError::invalid_local_in_scope_finally, das::CompilationError::invalid_local_init,
    das::CompilationError::invalid_local_init_block, das::CompilationError::invalid_local_init_constructor, das::CompilationError::invalid_local_init_type, das::CompilationError::invalid_local_tuple_expansion,
    das::CompilationError::invalid_local_type, das::CompilationError::invalid_macro_context, das::CompilationError::invalid_macro_read, das::CompilationError::invalid_macro_tag,
    das::CompilationError::invalid_macro_type, das::CompilationError::invalid_memzero_argument, das::CompilationError::invalid_memzero_argument_count, das::CompilationError::invalid_memzero_argument_type,
    das::CompilationError::invalid_module_name, das::CompilationError::invalid_new_class_syntax, das::CompilationError::invalid_new_initializer_result_type, das::CompilationError::invalid_new_initializer_type,
    das::CompilationError::invalid_new_type, das::CompilationError::invalid_null_coalescing_type, das::CompilationError::invalid_op3_expression, das::CompilationError::invalid_pointer_arithmetic,
    das::CompilationError::invalid_quote_argument_count, das::CompilationError::invalid_result, das::CompilationError::invalid_result_type, das::CompilationError::invalid_return_semantics,
    das::CompilationError::invalid_safe_as, das::CompilationError::invalid_safe_dereference_type, das::CompilationError::invalid_safe_field_type, das::CompilationError::invalid_static_assert_argument_count,
    das::CompilationError::invalid_static_assert_comment_type, das::CompilationError::invalid_static_assert_condition_type, das::CompilationError::invalid_static_if_condition, das::CompilationError::invalid_storage_type_op,
    das::CompilationError::invalid_string_builder_argument, das::CompilationError::invalid_structure, das::CompilationError::invalid_structure_annotation, das::CompilationError::invalid_structure_array,
    das::CompilationError::invalid_structure_block_pipe, das::CompilationError::invalid_structure_field_init, das::CompilationError::invalid_structure_field_type, das::CompilationError::invalid_structure_initializer_required,
    das::CompilationError::invalid_structure_local, das::CompilationError::invalid_structure_name, das::CompilationError::invalid_structure_template, das::CompilationError::invalid_structure_tuple,
    das::CompilationError::invalid_structure_type, das::CompilationError::invalid_structure_variant, das::CompilationError::invalid_super_call, das::CompilationError::invalid_swizzle_mask,
    das::CompilationError::invalid_swizzle_type, das::CompilationError::invalid_table, das::CompilationError::invalid_table_argument_type, das::CompilationError::invalid_table_expression,
    das::CompilationError::invalid_table_index_type, das::CompilationError::invalid_table_key_type, das::CompilationError::invalid_table_safe_index_type, das::CompilationError::invalid_table_type,
    das::CompilationError::invalid_tuple, das::CompilationError::invalid_tuple_argument_type, das::CompilationError::invalid_tuple_block, das::CompilationError::invalid_tuple_key,
    das::CompilationError::invalid_tuple_key_type, das::CompilationError::invalid_tuple_type, das::CompilationError::invalid_tuple_variant, das::CompilationError::invalid_type,
    das::CompilationError::invalid_type_dimension, das::CompilationError::invalid_type_expression, das::CompilationError::invalid_typeinfo, das::CompilationError::invalid_typeinfo_annotation,
    das::CompilationError::invalid_typeinfo_annotation_argument_type, das::CompilationError::invalid_typeinfo_dim, das::CompilationError::invalid_typeinfo_dim_table, das::CompilationError::invalid_typeinfo_dim_table_type,
    das::CompilationError::invalid_typeinfo_function, das::CompilationError::invalid_typeinfo_has_field_type, das::CompilationError::invalid_typeinfo_mangled_subexpression, das::CompilationError::invalid_typeinfo_module_subexpression,
    das::CompilationError::invalid_typeinfo_offsetof_type, das::CompilationError::invalid_typeinfo_struct_get_annotation_argument_type, das::CompilationError::invalid_typeinfo_struct_has_annotation_argument_type, das::CompilationError::invalid_typeinfo_struct_has_annotation_type,
    das::CompilationError::invalid_typeinfo_struct_modulename, das::CompilationError::invalid_typeinfo_struct_name, das::CompilationError::invalid_typeinfo_variant_index_type, das::CompilationError::invalid_variable_name,
    das::CompilationError::invalid_variable_private, das::CompilationError::invalid_variant, das::CompilationError::invalid_variant_array, das::CompilationError::invalid_variant_block,
    das::CompilationError::invalid_variant_initializer_count, das::CompilationError::invalid_variant_tuple, das::CompilationError::invalid_variant_type, das::CompilationError::invalid_variant_unique,
    das::CompilationError::invalid_while_condition_type, das::CompilationError::invalid_with_array_type, das::CompilationError::invalid_with_type, das::CompilationError::invalid_yield,
    das::CompilationError::invalid_yield_in_block, das::CompilationError::missing_annotation, das::CompilationError::missing_assume_type, das::CompilationError::missing_bitfield_init,
    das::CompilationError::missing_block_argument_init, das::CompilationError::missing_block_result, das::CompilationError::missing_enumeration_zero, das::CompilationError::missing_finalizer,
    das::CompilationError::missing_for_iterator, das::CompilationError::missing_function_body, das::CompilationError::missing_function_name, das::CompilationError::missing_function_result,
    das::CompilationError::missing_global, das::CompilationError::missing_global_init, das::CompilationError::missing_global_shared_init, das::CompilationError::missing_local,
    das::CompilationError::missing_local_block_init, das::CompilationError::missing_local_init, das::CompilationError::missing_local_reference_init, das::CompilationError::missing_new_default_initializer,
    das::CompilationError::missing_result, das::CompilationError::missing_structure_field, das::CompilationError::missing_typeinfo_subexpression, das::CompilationError::mismatching_array_dimension,
    das::CompilationError::mismatching_array_element_type, das::CompilationError::mismatching_block_argument_type, das::CompilationError::mismatching_clone_type, das::CompilationError::mismatching_function_argument,
    das::CompilationError::mismatching_function_argument_count, das::CompilationError::mismatching_numeric_type, das::CompilationError::mismatching_result_type, das::CompilationError::mismatching_structure_dimension,
    das::CompilationError::mismatching_tuple_argument_count, das::CompilationError::mismatching_tuple_field_names, das::CompilationError::mismatching_type, das::CompilationError::mismatching_variant_dimension,
    das::CompilationError::exceeds_argument, das::CompilationError::exceeds_array_index, das::CompilationError::exceeds_call_depth, das::CompilationError::exceeds_expression_recursion,
    das::CompilationError::exceeds_function_argument, das::CompilationError::exceeds_infer_passes, das::CompilationError::exceeds_local, das::CompilationError::exceeds_new_argument,
    das::CompilationError::exceeds_structure, das::CompilationError::exceeds_tuple_index, das::CompilationError::exceeds_type, das::CompilationError::exceeds_type_alias,
    das::CompilationError::exceeds_typeinfo_sizeof, das::CompilationError::exceeds_constant_range, das::CompilationError::ambiguous_annotation, das::CompilationError::ambiguous_bitfield, das::CompilationError::ambiguous_call_macro,
    das::CompilationError::ambiguous_enumeration, das::CompilationError::ambiguous_field, das::CompilationError::ambiguous_field_lookup, das::CompilationError::ambiguous_finalizer,
    das::CompilationError::ambiguous_function, das::CompilationError::ambiguous_macro, das::CompilationError::ambiguous_structure, das::CompilationError::ambiguous_super_call,
    das::CompilationError::ambiguous_super_constructor, das::CompilationError::ambiguous_type, das::CompilationError::ambiguous_type_alias, das::CompilationError::ambiguous_typeinfo_macro,
    das::CompilationError::ambiguous_variable, das::CompilationError::already_declared_assume_alias, das::CompilationError::already_declared_block_argument, das::CompilationError::already_declared_function,
    das::CompilationError::already_declared_label, das::CompilationError::already_declared_local_variable, das::CompilationError::already_declared_structure_field_init, das::CompilationError::already_declared_table,
    das::CompilationError::already_declared_tuple_field, das::CompilationError::already_declared_variable, das::CompilationError::already_declared_with_shadow, das::CompilationError::lookup_annotation_field,
    das::CompilationError::lookup_argument_type, das::CompilationError::lookup_bitfield, das::CompilationError::lookup_block_argument_type, das::CompilationError::lookup_block_result_type,
    das::CompilationError::lookup_cast_type, das::CompilationError::lookup_constructor, das::CompilationError::lookup_expression_type, das::CompilationError::lookup_field,
    das::CompilationError::lookup_field_type, das::CompilationError::lookup_function, das::CompilationError::lookup_function_address_type, das::CompilationError::lookup_function_argument_type,
    das::CompilationError::lookup_function_mangled_name, das::CompilationError::lookup_function_result_type, das::CompilationError::lookup_function_type, das::CompilationError::lookup_generator_type,
    das::CompilationError::lookup_global_type, das::CompilationError::lookup_is_expression_type, das::CompilationError::lookup_label, das::CompilationError::lookup_local_type,
    das::CompilationError::lookup_macro, das::CompilationError::lookup_method, das::CompilationError::lookup_new_type, das::CompilationError::lookup_safe_field_type,
    das::CompilationError::lookup_structure_field, das::CompilationError::lookup_structure_field_type, das::CompilationError::lookup_super_class, das::CompilationError::lookup_super_constructor,
    das::CompilationError::lookup_super_finalizer, das::CompilationError::lookup_super_method, das::CompilationError::lookup_tuple_field, das::CompilationError::lookup_type,
    das::CompilationError::lookup_typeinfo_annotation, das::CompilationError::lookup_typeinfo_annotation_argument, das::CompilationError::lookup_typeinfo_macro, das::CompilationError::lookup_typeinfo_offsetof_type,
    das::CompilationError::lookup_typeinfo_type, das::CompilationError::lookup_variable, das::CompilationError::lookup_variant_field, das::CompilationError::lookup_variant_type,
    das::CompilationError::cant_access_private_field, das::CompilationError::cant_access_private_structure, das::CompilationError::cant_address_function, das::CompilationError::cant_address_template_function,
    das::CompilationError::cant_annotation_field, das::CompilationError::cant_apply_op, das::CompilationError::cant_argument, das::CompilationError::cant_argument_structure,
    das::CompilationError::cant_array_element, das::CompilationError::cant_ascend, das::CompilationError::cant_assign_op, das::CompilationError::cant_block,
    das::CompilationError::cant_capture_variable, das::CompilationError::cant_clone, das::CompilationError::cant_clone_type, das::CompilationError::cant_copy,
    das::CompilationError::cant_create_structure_annotation, das::CompilationError::cant_delete, das::CompilationError::cant_delete_local, das::CompilationError::cant_delete_smart_pointer,
    das::CompilationError::cant_delete_super, das::CompilationError::cant_dereference, das::CompilationError::cant_expression, das::CompilationError::cant_field_class,
    das::CompilationError::cant_finalize_block_annotation, das::CompilationError::cant_finalize_function_annotation, das::CompilationError::cant_finalize_structure_annotation, das::CompilationError::cant_function,
    das::CompilationError::cant_get_field, das::CompilationError::cant_get_field_pointer, das::CompilationError::cant_global, das::CompilationError::cant_index,
    das::CompilationError::cant_index_key, das::CompilationError::cant_index_pointer, das::CompilationError::cant_index_table, das::CompilationError::cant_initialize_array_element,
    das::CompilationError::cant_initialize_private_field, das::CompilationError::cant_initialize_structure_field, das::CompilationError::cant_initialize_variant_field, das::CompilationError::cant_iterate_iterator,
    das::CompilationError::cant_local, das::CompilationError::cant_move, das::CompilationError::cant_pointer, das::CompilationError::cant_result,
    das::CompilationError::cant_safe_get_field, das::CompilationError::cant_safe_index, das::CompilationError::cant_safe_index_table, das::CompilationError::cant_structure_field,
    das::CompilationError::cant_take_pointer, das::CompilationError::cant_tuple, das::CompilationError::cant_type, das::CompilationError::cant_variant_field,
    das::CompilationError::cant_write, das::CompilationError::unsafe_address, das::CompilationError::unsafe_argument, das::CompilationError::unsafe_array_safe_index,
    das::CompilationError::unsafe_capture_variable, das::CompilationError::unsafe_cast, das::CompilationError::unsafe_class, das::CompilationError::unsafe_class_initializer,
    das::CompilationError::unsafe_class_local, das::CompilationError::unsafe_class_stack_construction, das::CompilationError::unsafe_delete, das::CompilationError::unsafe_delete_pointer,
    das::CompilationError::unsafe_fixed_array_safe_index, das::CompilationError::unsafe_function, das::CompilationError::unsafe_function_call, das::CompilationError::unsafe_global,
    das::CompilationError::unsafe_global_pointer, das::CompilationError::unsafe_local, das::CompilationError::unsafe_local_class, das::CompilationError::unsafe_local_in_scope_required,
    das::CompilationError::unsafe_local_reference, das::CompilationError::unsafe_local_type, das::CompilationError::unsafe_move, das::CompilationError::unsafe_operator,
    das::CompilationError::unsafe_pointer_index, das::CompilationError::unsafe_pointer_safe_index, das::CompilationError::unsafe_return_block, das::CompilationError::unsafe_return_function,
    das::CompilationError::unsafe_return_reference, das::CompilationError::unsafe_return_smart_pointer, das::CompilationError::unsafe_smart_pointer, das::CompilationError::unsafe_structure_field,
    das::CompilationError::unsafe_structure_uninitialized, das::CompilationError::unsafe_structure_visibility, das::CompilationError::unsafe_table_index, das::CompilationError::unsafe_table_safe_index,
    das::CompilationError::unsafe_variant_field, das::CompilationError::unsafe_variant_safe_as, das::CompilationError::recursion_argument, das::CompilationError::recursion_assume_alias,
    das::CompilationError::recursion_function, das::CompilationError::recursion_function_argument, das::CompilationError::recursion_global, das::CompilationError::recursion_structure,
    das::CompilationError::recursion_type_alias, das::CompilationError::runtime_annotation_transform, das::CompilationError::runtime_call_macro, das::CompilationError::runtime_expression,
    das::CompilationError::runtime_macro_exception, das::CompilationError::runtime_macro_infer, das::CompilationError::runtime_macro_performance, das::CompilationError::runtime_macro_style,
    das::CompilationError::runtime_structure_annotation, das::CompilationError::runtime_typeinfo_macro, das::CompilationError::not_resolved_yet_argument_type, das::CompilationError::not_resolved_yet_array_dimension,
    das::CompilationError::not_resolved_yet_array_type, das::CompilationError::not_resolved_yet_bitfield_type, das::CompilationError::not_resolved_yet_block, das::CompilationError::not_resolved_yet_block_argument,
    das::CompilationError::not_resolved_yet_block_argument_init, das::CompilationError::not_resolved_yet_class_type, das::CompilationError::not_resolved_yet_comprehension_type, das::CompilationError::not_resolved_yet_delete_size_type,
    das::CompilationError::not_resolved_yet_dereference_type, das::CompilationError::not_resolved_yet_enumerator, das::CompilationError::not_resolved_yet_expression_type, das::CompilationError::not_resolved_yet_function,
    das::CompilationError::not_resolved_yet_function_block, das::CompilationError::not_resolved_yet_generator_block, das::CompilationError::not_resolved_yet_index_type, das::CompilationError::not_resolved_yet_invoke_argument_type,
    das::CompilationError::not_resolved_yet_is_expression_type, das::CompilationError::not_resolved_yet_label_type, das::CompilationError::not_resolved_yet_lambda_block, das::CompilationError::not_resolved_yet_local_type,
    das::CompilationError::not_resolved_yet_method_call, das::CompilationError::not_resolved_yet_new_type, das::CompilationError::not_resolved_yet_null_coalescing_type, das::CompilationError::not_resolved_yet_structure,
    das::CompilationError::not_resolved_yet_structure_field, das::CompilationError::not_resolved_yet_structure_type, das::CompilationError::not_resolved_yet_table_type, das::CompilationError::not_resolved_yet_tuple_type,
    das::CompilationError::not_resolved_yet_type, das::CompilationError::not_resolved_yet_type_alias, das::CompilationError::not_resolved_yet_typeinfo, das::CompilationError::not_resolved_yet_typeinfo_alignof,
    das::CompilationError::not_resolved_yet_typeinfo_dim, das::CompilationError::not_resolved_yet_typeinfo_dim_table, das::CompilationError::not_resolved_yet_typeinfo_sizeof, das::CompilationError::concept_failed,
    das::CompilationError::condition_must_be_bool, das::CompilationError::not_expecting_result, das::CompilationError::static_assert_failed, das::CompilationError::invalid_options,
    das::CompilationError::mismatching_runtime_type, das::CompilationError::mismatching_runtime_type_hash, das::CompilationError::exceeds_array, das::CompilationError::exceeds_global,
    das::CompilationError::exceeds_runtime_buffer, das::CompilationError::cant_hash_block, das::CompilationError::cant_hash_context, das::CompilationError::cant_hash_iterator,
    das::CompilationError::cant_serialize_block, das::CompilationError::cant_serialize_iterator, das::CompilationError::cant_serialize_null_pointer, das::CompilationError::cant_serialize_pointer,
    das::CompilationError::cant_serialize_table, das::CompilationError::runtime_function, das::CompilationError::runtime_function_annotation, das::CompilationError::runtime_global,
    das::CompilationError::runtime_macro, das::CompilationError::internal_annotation, das::CompilationError::internal_array, das::CompilationError::internal_array_type,
    das::CompilationError::internal_block, das::CompilationError::internal_block_missing_return_type, das::CompilationError::internal_class, das::CompilationError::internal_enumeration,
    das::CompilationError::internal_expression, das::CompilationError::internal_field, das::CompilationError::internal_function, das::CompilationError::internal_function_annotation,
    das::CompilationError::internal_function_changed, das::CompilationError::internal_function_name, das::CompilationError::internal_function_not_resolved_yet, das::CompilationError::internal_function_refresh,
    das::CompilationError::internal_generator, das::CompilationError::internal_generator_finalizer, das::CompilationError::internal_generator_finalizer_multiple, das::CompilationError::internal_generator_finalizer_name,
    das::CompilationError::internal_generator_function_name, das::CompilationError::internal_generator_structure_name, das::CompilationError::internal_global, das::CompilationError::internal_label,
    das::CompilationError::internal_lambda_finalizer_name, das::CompilationError::internal_lambda_function_name, das::CompilationError::internal_lambda_in_scope_conversion, das::CompilationError::internal_lambda_structure_name,
    das::CompilationError::internal_macro, das::CompilationError::internal_name, das::CompilationError::internal_options, das::CompilationError::internal_pod_analysis_infer,
    das::CompilationError::internal_relocate_infer, das::CompilationError::internal_structure, das::CompilationError::internal_structure_block, das::CompilationError::internal_table,
    das::CompilationError::internal_tuple, das::CompilationError::internal_tuple_type, das::CompilationError::internal_type, das::CompilationError::internal_type_alias,
    das::CompilationError::internal_typeinfo_macro, das::CompilationError::internal_variable, das::CompilationError::missing_aot, das::CompilationError::syntax_error,
    das::CompilationError::function_not_found, das::CompilationError::function_already_declared, das::CompilationError::invalid_return_type, das::CompilationError::invalid_initialization_type,
    das::CompilationError::invalid_private, das::CompilationError::invalid_static, das::CompilationError::aot_side_effects, das::CompilationError::cant_pipe,
    das::CompilationError::enumeration_value_already_declared, das::CompilationError::invalid_escape_sequence, das::CompilationError::invalid_option, das::CompilationError::module_already_has_a_name,
    das::CompilationError::type_alias_already_declared, das::CompilationError::unsupported_read_macro
        };
        for (uint32_t i = 0; i < sizeof(enumArray)/sizeof(enumArray[0]); ++i)
            addI(enumArrayName[i], int64_t(enumArray[i]), das::LineInfo());
    }
};
namespace das {
    template <>
    struct typeFactory< das::CompilationError > {
        static TypeDeclPtr make(const ModuleLibrary & library ) {
            return library.makeEnumType("CompilationError");
        }
    };
}

das::FileAccessPtr get_file_access( char * pak );//link time resolved dependencies
das::Context * get_context ( int stackSize=0 );//link time resolved dependencies

namespace das {
    template <>
    struct das_default_vector_size<AnnotationArgumentList> {
        static __forceinline uint32_t size( const AnnotationArgumentList & value ) {
            return uint32_t(value.size());
        }
    };


    enum RttiIdx {
        RttiBool = 0,
        RttiInt32 = 1,
        RttiUint32 = 2,
        RttiInt64 = 3,
        RttiUint64 = 4,
        RttiFloat = 5,
        RttiDouble = 6,
        RttiString = 7,
        RttiAny = 8,
    };

    template <>
    struct typeFactory<RttiValue> {
        static TypeDeclPtr make(const ModuleLibrary & library ) {
            auto vtype = new TypeDecl(Type::tVariant);
            vtype->alias = "RttiValue";
            vtype->aotAlias = false;
            vtype->addVariant("tBool",   typeFactory<RttiValue::NthType<RttiBool>>::make(library));
            vtype->addVariant("tInt",    typeFactory<RttiValue::NthType<RttiInt32>>::make(library));
            vtype->addVariant("tUInt",   typeFactory<RttiValue::NthType<RttiUint32>>::make(library));
            vtype->addVariant("tInt64",  typeFactory<RttiValue::NthType<RttiInt64>>::make(library));
            vtype->addVariant("tUInt64", typeFactory<RttiValue::NthType<RttiUint64>>::make(library));
            vtype->addVariant("tFloat",  typeFactory<RttiValue::NthType<RttiFloat>>::make(library));
            vtype->addVariant("tDouble", typeFactory<RttiValue::NthType<RttiDouble>>::make(library));
            vtype->addVariant("tString", typeFactory<RttiValue::NthType<RttiString>>::make(library));
            vtype->addVariant("nothing", typeFactory<RttiValue::NthType<RttiAny>>::make(library));
            // optional validation
            DAS_ASSERT(sizeof(RttiValue) == vtype->getSizeOf());
            DAS_ASSERT(alignof(RttiValue) == vtype->getAlignOf());
            return vtype;
        }
    };

    TypeDeclPtr makeModuleFlags() {
        auto ft = new TypeDecl(Type::tBitfield);
        ft->alias = "ModuleFlags";
        ft->argNames = {
            "builtIn", "promoted", "isPublic", "isModule", "isSolidContext",
            "fromExtraDependency", "doNotAllowUnsafe",
            "wasParsedNameless", "visibleEverywhere", "allowPodInscope"
        };
        return ft;
    }

    struct ModuleAnnotation : ManagedStructureAnnotation<Module,false> {
        ModuleAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Module", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(cppClassName)>("cppClassName");
            addField<DAS_BIND_MANAGED_FIELD(fileName)>("fileName");
            addFieldEx ( "moduleFlags", "moduleFlags", offsetof(Module, moduleFlags), makeModuleFlags() );
        }
    };

    struct AstModuleGroupAnnotation : ManagedStructureAnnotation<ModuleGroup, true, true> {
        AstModuleGroupAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("ModuleGroup", ml) {
        }
    };

    struct AstSerializerAnnotation : ManagedStructureAnnotation<AstSerializerState, false, false> {
        AstSerializerAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("AstSerializer", ml) {
        }
    };


    struct FileAccessAnnotation : ManagedStructureAnnotation<FileAccess,false,true> {
        FileAccessAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("FileAccess", ml) {
        }
    };

    struct ErrorAnnotation : ManagedStructureAnnotation<Error> {
        ErrorAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Error", ml) {
            addField<DAS_BIND_MANAGED_FIELD(what)>("what");
            addField<DAS_BIND_MANAGED_FIELD(extra)>("extra");
            addField<DAS_BIND_MANAGED_FIELD(fixme)>("fixme");
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
            addField<DAS_BIND_MANAGED_FIELD(cerr)>("cerr");
        }
    };

    struct FileInfoAnnotation : ManagedStructureAnnotation<FileInfo,false> {
        FileInfoAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("FileInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            // addProperty<DAS_BIND_MANAGED_PROP(getSource)>("source");
            // addField<DAS_BIND_MANAGED_FIELD(sourceLength)>("sourceLength");
            addField<DAS_BIND_MANAGED_FIELD(tabSize)>("tabSize");
        }
    };

    TypeDeclPtr makeContextCategoryFlags() {
        auto ft = new TypeDecl(Type::tBitfield);
        ft->alias = "context_category_flags";
        ft->argNames = { "dead", "debug_context", "thread_clone", "job_clone", "opengl",
            "debugger_tick", "debugger_attached", "macro_context", "folding_context", "audio" };
        return ft;
    }

    template <>
    struct typeFactory<ContextCategory> {
        static TypeDeclPtr make(const ModuleLibrary &) {
            return makeContextCategoryFlags();
        }
    };

    struct ContextAnnotation : ManagedStructureAnnotation<Context,false> {
        ContextAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Context", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addFieldEx("category", "category", offsetof(Context, category), makeContextCategoryFlags());
            addField<DAS_BIND_MANAGED_FIELD(breakOnException)>("breakOnException");
            addField<DAS_BIND_MANAGED_FIELD(alwaysErrorOnException)>("alwaysErrorOnException");
            addField<DAS_BIND_MANAGED_FIELD(alwaysStackWalkOnException)>("alwaysStackWalkOnException");
            addField<DAS_BIND_MANAGED_FIELD(exception)>("exception");
            addField<DAS_BIND_MANAGED_FIELD(last_exception)>("last_exception");
            addField<DAS_BIND_MANAGED_FIELD(exceptionAt)>("exceptionAt");
            addField<DAS_BIND_MANAGED_FIELD(contextMutex)>("contextMutex");
            addProperty<DAS_BIND_MANAGED_PROP(getTotalFunctions)>("totalFunctions",
                "getTotalFunctions");
            addProperty<DAS_BIND_MANAGED_PROP(getTotalVariables)>("totalVariables",
                "getTotalVariables");
            addProperty<DAS_BIND_MANAGED_PROP(getCodeAllocatorId)>("getCodeAllocatorId",
                "getCodeAllocatorId");
        }
        virtual void walk ( das::DataWalker & walker, void * data ) override {
            if ( !walker.reading ) {
                if ( sizeof(intptr_t)==4 ) {
                    uint32_t T = uint32_t(intptr_t(data));
                    walker.UInt(T);
                } else {
                    uint64_t T = uint64_t(intptr_t(data));
                    walker.UInt64(T);
                }
            }
        }
    };

    TypeDeclPtr makeSimFunctionFlags() {
        auto ft = new TypeDecl(Type::tBitfield);
        ft->alias = "SimFunctionFlags";
        ft->argNames = { "aot", "fastcall", "builtin", "jit", "unsafe", "cmres", "pinvoke" };
        return ft;
    }

    struct SimFunctionAnnotation : ManagedStructureAnnotation<SimFunction,false> {
        SimFunctionAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("SimFunction", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(mangledName)>("mangledName");
            addField<DAS_BIND_MANAGED_FIELD(debugInfo)>("debugInfo");
            addField<DAS_BIND_MANAGED_FIELD(stackSize)>("stackSize");
            addField<DAS_BIND_MANAGED_FIELD(mangledNameHash)>("mangledNameHash");
            addProperty<DAS_BIND_MANAGED_PROP(getLineInfo)>("lineInfo","getLineInfo");
            addFieldEx ( "flags", "flags", offsetof(SimFunction, flags), makeSimFunctionFlags() );
        }
    };

    struct LineInfoAnnotation : ManagedStructureAnnotation<LineInfo,false> {
        LineInfoAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("LineInfo", ml) {
            this->addField<DAS_BIND_MANAGED_FIELD(fileInfo)>("fileInfo");
            this->addField<DAS_BIND_MANAGED_FIELD(column)>("column");
            this->addField<DAS_BIND_MANAGED_FIELD(line)>("line");
            this->addField<DAS_BIND_MANAGED_FIELD(last_column)>("last_column");
            this->addField<DAS_BIND_MANAGED_FIELD(last_line)>("last_line");
        }
        virtual bool isLocal() const override { return true; }
    };

    TypeDeclPtr makeProgramFlags() {
        auto ft = new TypeDecl(Type::tBitfield);
        ft->alias = "ProgramFlags";
        ft->argNames = { "failToCompile", "_unsafe", "isCompiling", "isSimulating",
            "isCompilingMacros", "needMacroModule", "promoteToBuiltin",
            "isDependency", "macroException"
        };
        return ft;
    }

    struct ProgramAnnotation : ManagedStructureAnnotation <Program,false,true> {
        ProgramAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Program", ml) {
            addField<DAS_BIND_MANAGED_FIELD(thisModuleName)>("thisModuleName");
            addField<DAS_BIND_MANAGED_FIELD(thisNamespace)>("thisNamespace");
            addField<DAS_BIND_MANAGED_FIELD(totalFunctions)>("totalFunctions");
            addField<DAS_BIND_MANAGED_FIELD(totalVariables)>("totalVariables");
            addField<DAS_BIND_MANAGED_FIELD(globalStringHeapSize)>("globalStringHeapSize");
            addField<DAS_BIND_MANAGED_FIELD(initSemanticHashWithDep)>("initSemanticHashWithDep");
            addFieldEx ( "flags", "flags", offsetof(Program, flags), makeProgramFlags() );
            addProperty<DAS_BIND_MANAGED_PROP(getThisModule)>("getThisModule");
            addProperty<DAS_BIND_MANAGED_PROP(getDebugger)>("getDebugger");
            addField<DAS_BIND_MANAGED_FIELD(errors)>("errors");
            addField<DAS_BIND_MANAGED_FIELD(options)>("_options","options");
            addField<DAS_BIND_MANAGED_FIELD(policies)>("policies","policies");
        }
    };

    struct AnnotationArgumentAnnotation : ManagedStructureAnnotation <AnnotationArgument,false> {
        AnnotationArgumentAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("AnnotationArgument", ml) {
            addFieldEx ( "basicType", "type", offsetof(AnnotationArgument, type), makeType<Type>(ml) );
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(sValue)>("sValue");
            addField<DAS_BIND_MANAGED_FIELD(bValue)>("bValue");
            addField<DAS_BIND_MANAGED_FIELD(iValue)>("iValue");
            addField<DAS_BIND_MANAGED_FIELD(fValue)>("fValue");
            addField<DAS_BIND_MANAGED_FIELD(at)>("at");
        }
    };

    TypeDeclPtr makeAnnotationDeclarationFlags() {
        auto ft = new TypeDecl(Type::tBitfield);
        ft->alias = "AnnotationDeclarationFlags";
        ft->argNames = { "inherited" };
        return ft;
    }

    struct AnnotationDeclarationAnnotation : ManagedStructureAnnotation <AnnotationDeclaration,true> {
        AnnotationDeclarationAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("AnnotationDeclaration", ml) {
                addField<DAS_BIND_MANAGED_FIELD(annotation)>("annotation");
                addField<DAS_BIND_MANAGED_FIELD(arguments)>("arguments");
                addField<DAS_BIND_MANAGED_FIELD(at)>("at");
                addFieldEx ( "flags", "flags", offsetof(AnnotationDeclaration, flags), makeAnnotationDeclarationFlags() );
                // TODO: function?
                // addProperty<DAS_BIND_MANAGED_PROP(getMangledName)>("getMangledName","getMangledName");
        }
    };

    struct TypeAnnotationAnnotation : ManagedStructureAnnotation <TypeAnnotation,false> {
        TypeAnnotationAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("TypeAnnotation", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(cppName)>("cppName");
            addField<DAS_BIND_MANAGED_FIELD(module)>("_module","module");
            addProperty<DAS_BIND_MANAGED_PROP(isYetAnotherVectorTemplate)>("is_any_vector","isYetAnotherVectorTemplate");
            addProperty<DAS_BIND_MANAGED_PROP(canMove)>("canMove");
            addProperty<DAS_BIND_MANAGED_PROP(canCopy)>("canCopy");
            addProperty<DAS_BIND_MANAGED_PROP(canClone)>("canClone");
            addProperty<DAS_BIND_MANAGED_PROP(isPod)>("isPod");
            addProperty<DAS_BIND_MANAGED_PROP(isRawPod)>("isRawPod");
            addProperty<DAS_BIND_MANAGED_PROP(isRefType)>("isRefType");
            addProperty<DAS_BIND_MANAGED_PROP(hasNonTrivialCtor)>("hasNonTrivialCtor");
            addProperty<DAS_BIND_MANAGED_PROP(hasNonTrivialDtor)>("hasNonTrivialDtor");
            addProperty<DAS_BIND_MANAGED_PROP(hasNonTrivialCopy)>("hasNonTrivialCopy");
            addProperty<DAS_BIND_MANAGED_PROP(canBePlacedInContainer)>("canBePlacedInContainer");
            addProperty<DAS_BIND_MANAGED_PROP(isLocal)>("isLocal");
            addProperty<DAS_BIND_MANAGED_PROP(canNew)>("canNew");
            addProperty<DAS_BIND_MANAGED_PROP(canDelete)>("canDelete");
            addProperty<DAS_BIND_MANAGED_PROP(needDelete)>("needDelete");
            addProperty<DAS_BIND_MANAGED_PROP(canDeletePtr)>("canDeletePtr");
            addProperty<DAS_BIND_MANAGED_PROP(isIterable)>("isIterable");
            addProperty<DAS_BIND_MANAGED_PROP(isShareable)>("isShareable");
            addProperty<DAS_BIND_MANAGED_PROP(isSmart)>("isSmart");
            addProperty<DAS_BIND_MANAGED_PROP(avoidNullPtr)>("avoidNullPtr");
            addProperty<DAS_BIND_MANAGED_PROP(getSizeOf)>("sizeOf", "getSizeOf");
            addProperty<DAS_BIND_MANAGED_PROP(getAlignOf)>("alignOf", "getAlignOf");
            from("Annotation");
        }
    };

    struct BasicStructureAnnotationAnnotation : ManagedStructureAnnotation <BasicStructureAnnotation,false> {
        BasicStructureAnnotationAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("BasicStructureAnnotation", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(cppName)>("cppName");
            addProperty<DAS_BIND_MANAGED_PROP(fieldCount)>("fieldCount");
            from("TypeAnnotation");
        }
    };


    struct AnnotationAnnotation : ManagedStructureAnnotation <Annotation,false> {
        AnnotationAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("Annotation", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(cppName)>("cppName");
            addField<DAS_BIND_MANAGED_FIELD(module)>("_module", "module");
            addProperty<DAS_BIND_MANAGED_PROP(rtti_isHandledTypeAnnotation)>("isTypeAnnotation",
                "rtti_isHandledTypeAnnotation");
            addProperty<DAS_BIND_MANAGED_PROP(rtti_isBasicStructureAnnotation)>("isBasicStructureAnnotation",
                "rtti_isBasicStructureAnnotation");
            addProperty<DAS_BIND_MANAGED_PROP(rtti_isStructureAnnotation)>("isStructureAnnotation",
                "rtti_isStructureAnnotation");
            addProperty<DAS_BIND_MANAGED_PROP(rtti_isStructureTypeAnnotation)>("isStructureTypeAnnotation",
                "rtti_isStructureTypeAnnotation");
            addProperty<DAS_BIND_MANAGED_PROP(rtti_isFunctionAnnotation)>("isFunctionAnnotation",
                "rtti_isFunctionAnnotation");
            addProperty<DAS_BIND_MANAGED_PROP(rtti_isEnumerationAnnotation)>("isEnumerationAnnotation",
                "rtti_isEnumerationAnnotation");
        }
    };

    struct EnumValueInfoAnnotation : ManagedStructureAnnotation <EnumValueInfo,false> {
        EnumValueInfoAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("EnumValueInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(value)>("value");
        }
    };

    template <typename ST>
    struct SimNode_DebugInfoAtField : SimNode_At {
        using TT = ST;
        DAS_PTR_NODE;
        SimNode_DebugInfoAtField ( const LineInfo & at, SimNode * rv, SimNode * idx, uint32_t ofs )
            : SimNode_At(at, rv, idx, 0, ofs, 0, "type<DebugInfo>.field[index]") {}
        __forceinline char * compute ( Context & context ) {
            DAS_PROFILE_NODE
            auto pValue = (ST *) value->evalPtr(context);
            uint32_t idx = cast<uint32_t>::to(index->eval(context));
            if ( idx >= pValue->count ) {
                context.throw_error_at(debugInfo,"field index out of range, %u of %u", idx, pValue->count);
                return nullptr;
            } else {
                return ((char *)(pValue->fields[idx])) + offset;
            }
        }
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP_TT(DebugInfoAtField);
            V_SUB(value);
            V_SUB(index);
            V_ARG(stride);
            V_ARG(offset);
            V_ARG(range);
            V_END();
        }
    };

    template <typename VT, typename ST>
    struct DebugInfoIterator : PointerDimIterator {
        DebugInfoIterator  ( ST * ar, LineInfo * at )
            : PointerDimIterator((char **)ar->fields, ar->count, sizeof(DebugInfoIterator<VT,ST>), at) {}
    };

    template <typename VT, typename ST>
    struct DebugInfoAnnotation : ManagedStructureAnnotation <ST,false> {
        DebugInfoAnnotation(const string & st, ModuleLibrary & ml)
            : ManagedStructureAnnotation<ST,false> (st,ml) {
            using ManagedType = ST;
            this->template addField<DAS_BIND_MANAGED_FIELD(count)>("count");
        }
        virtual bool isIndexable ( const TypeDeclPtr & indexType ) const override {
            return indexType->isIndex();
        }
        virtual TypeDeclPtr makeIndexType ( ExpressionPtr, ExpressionPtr ) const override {
            return new TypeDecl(*fieldType);
        }
        virtual SimNode * simulateGetAt ( Context & context, const LineInfo & at, const TypeDeclPtr &,
                                         ExpressionPtr rv, ExpressionPtr idx, uint32_t ofs ) const override {
            return context.code->makeNode<SimNode_DebugInfoAtField<ST>>(at,
                                                                        simulateExpression(context, rv),
                                                                        simulateExpression(context, idx),
                                                                        ofs);
        }
        virtual bool isIterable ( ) const override {
            return true;
        }
        virtual TypeDeclPtr makeIteratorType ( ExpressionPtr ) const override {
            return new TypeDecl(*fieldType);
        }
        virtual SimNode * simulateGetIterator ( Context & context, const LineInfo & at, ExpressionPtr src ) const override {
            auto rv = simulateExpression(context, src);
            return context.code->makeNode<SimNode_AnyIterator<ST,DebugInfoIterator<VT,ST>>>(at, rv);
        }
        virtual void gc_collect ( gc_root * target, gc_root * from ) override {
            ManagedStructureAnnotation<ST,false>::gc_collect(target, from);
            if ( fieldType ) fieldType->gc_collect(target, from);
        }
        virtual void visitTypeDecls ( const function<void(TypeDecl *)> & callback ) override {
            ManagedStructureAnnotation<ST,false>::visitTypeDecls(callback);
            if ( fieldType ) callback(fieldType);
        }
        TypeDeclPtr fieldType = nullptr;
    };

    template <typename ST, typename VT>
    Sequence debugInfoIterator ( ST * st, Context * context, LineInfoArg * at ) {
        using StructIterator = DebugInfoIterator<VT,ST>;
        char * iter = context->allocateIterator(sizeof(StructIterator), "debug info iterator", at);
        new (iter) StructIterator(st, at);
        return { (Iterator *) iter };
    }

    struct EnumInfoAnnotation : DebugInfoAnnotation<EnumValueInfo,EnumInfo> {
        EnumInfoAnnotation(ModuleLibrary & ml) : DebugInfoAnnotation ("EnumInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(fields)>("fields");
            addField<DAS_BIND_MANAGED_FIELD(module_name)>("module_name");
            addField<DAS_BIND_MANAGED_FIELD(hash)>("hash");
            addField<DAS_BIND_MANAGED_FIELD(flags)>("flags");
            fieldType = makeType<EnumValueInfo>(*mlib);
            fieldType->ref = true;
        }
    };

    TSequence<EnumValueInfo&> each_EnumInfo ( EnumInfo & st, Context * context, LineInfoArg * at ) {
        return debugInfoIterator<EnumInfo,EnumValueInfo>(&st, context, at);
    }

    TSequence<const EnumValueInfo&> each_const_EnumInfo ( const EnumInfo & st, Context * context, LineInfoArg * at ) {
        return debugInfoIterator<EnumInfo,EnumValueInfo>((EnumInfo *)&st, context, at);
    }


    TypeDeclPtr makeStructInfoFlags() {
        auto ft = new TypeDecl(Type::tBitfield);
        ft->alias = "StructInfoFlags";
        ft->argNames = { "_class", "_lambda", "heapGC", "stringHeapGC" };
        return ft;
    }

    struct StructInfoAnnotation : DebugInfoAnnotation<VarInfo,StructInfo> {
        StructInfoAnnotation(ModuleLibrary & ml) : DebugInfoAnnotation ("StructInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(module_name)>("module_name");
            addField<DAS_BIND_MANAGED_FIELD(firstGcField)>("firstGcField");
            addFieldEx ( "flags", "flags", offsetof(StructInfo, flags), makeStructInfoFlags());
            addField<DAS_BIND_MANAGED_FIELD(size)>("size");
            addField<DAS_BIND_MANAGED_FIELD(init_mnh)>("init_mnh");
            addField<DAS_BIND_MANAGED_FIELD(hash)>("hash");
        }
        void init () {
            fieldType = makeType<VarInfo>(*mlib);
            fieldType->ref = true;
            addField<DAS_BIND_MANAGED_FIELD(fields)>("fields");
        }
    };

    TSequence<VarInfo&> each_StructInfo ( StructInfo & st, Context * context, LineInfoArg * at ) {
        return debugInfoIterator<StructInfo,VarInfo>(&st, context, at);
    }

    TSequence<const VarInfo&> each_const_StructInfo ( const StructInfo & st, Context * context, LineInfoArg * at ) {
        return debugInfoIterator<StructInfo,VarInfo>((StructInfo *)&st, context, at);
    }

    TypeDeclPtr makeTypeInfoFlags() {
        auto ft = new TypeDecl(Type::tBitfield);
        ft->alias = "TypeInfoFlags";
        ft->argNames = { "ref", "refType", "canCopy", "isPod", "isRawPod", "isConst", "isTemp", "isImplicit",
            "refValue", "hasInitValue", "isSmartPtr", "isSmartPtrNative", "isHandled",
            "heapGC", "stringHeapGC", "isPrivate" };
        return ft;
    }

    template <typename TT>
    struct ManagedTypeInfoAnnotation : ManagedStructureAnnotation <TT,false> {
        ManagedTypeInfoAnnotation ( const string & st, ModuleLibrary & ml )
            : ManagedStructureAnnotation<TT,false> (st, ml) {
            using ManagedType = TT;
            this->addFieldEx ( "basicType", "type", offsetof(TypeInfo, type), makeType<Type>(ml) );
            this->template addProperty<DAS_BIND_MANAGED_PROP(getEnumType)>("enumType", "getEnumType");
            this->template addField<DAS_BIND_MANAGED_FIELD(dimSize)>("dimSize");
            this->template addField<DAS_BIND_MANAGED_FIELD(argCount)>("argCount");
            this->addFieldEx ( "flags", "flags", offsetof(TT, flags), makeTypeInfoFlags());
            this->template addField<DAS_BIND_MANAGED_FIELD(size)>("size");
            this->template addField<DAS_BIND_MANAGED_FIELD(hash)>("hash");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isRef)>("isRef");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isRefType)>("isRefType");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isRefValue)>("isRefValue");
            this->template addProperty<DAS_BIND_MANAGED_PROP(canCopy)>("canCopy");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isPod)>("isPod");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isRawPod)>("isRawPod");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isConst)>("isConst");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isTemp)>("isTemp");
            this->template addProperty<DAS_BIND_MANAGED_PROP(isImplicit)>("isImplicit");
            this->template addProperty<DAS_BIND_MANAGED_PROP(getAnnotation)>("annotation","getAnnotation");
            this->template addField<DAS_BIND_MANAGED_FIELD(argNames)>("argNames");
        }
        void init() {
            // this needs to be initialized separately
            // reason is recursive type
            using ManagedType = TT;
            this->template addProperty<DAS_BIND_MANAGED_PROP(getStructType)>("structType", "getStructType");
            this->template addField<DAS_BIND_MANAGED_FIELD(firstType)>("firstType");
            this->template addField<DAS_BIND_MANAGED_FIELD(secondType)>("secondType");
            this->template addField<DAS_BIND_MANAGED_FIELD(argTypes)>("argTypes");
        }
    };

    struct TypeInfoAnnotation : ManagedTypeInfoAnnotation <TypeInfo> {
        TypeInfoAnnotation(ModuleLibrary & ml) : ManagedTypeInfoAnnotation ("TypeInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(type)>("_type","type");
            addField<DAS_BIND_MANAGED_FIELD(dim)>("dim");
            // addField<DAS_BIND_MANAGED_FIELD(annotation_or_name)>("annotation_or_name");
            addProperty<DAS_BIND_MANAGED_PROP(getAnnotation)>("annotation_or_name", "getAnnotation");
        }
    };

    struct VarInfoAnnotation : ManagedTypeInfoAnnotation <VarInfo> {
        VarInfoAnnotation(ModuleLibrary & ml) : ManagedTypeInfoAnnotation ("VarInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(offset)>("offset");
            addField<DAS_BIND_MANAGED_FIELD(nextGcField)>("nextGcField");
            addFieldEx ( "annotation_arguments", "annotation_arguments",
                        offsetof(VarInfo, annotation_arguments), makeType<const AnnotationArguments *>(ml) );
            // default values
            addField<DAS_BIND_MANAGED_FIELD(sValue)>("sValue");
            addField<DAS_BIND_MANAGED_FIELD(value)>("value");
            // from
            from("TypeInfo");
        }
    };

    TypeDeclPtr makeLocalVariableInfoFlagsFlags() {
        auto ft = new TypeDecl(Type::tBitfield);
        ft->alias = "LocalVariableInfoFlags";
        ft->argNames = { "cmres" };
        return ft;
    }


    struct LocalVariableInfoAnnotation : ManagedTypeInfoAnnotation<LocalVariableInfo> {
        LocalVariableInfoAnnotation(ModuleLibrary & ml) : ManagedTypeInfoAnnotation ("LocalVariableInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(stackTop)>("stackTop");
            addField<DAS_BIND_MANAGED_FIELD(visibility)>("visibility");
            addFieldEx ( "localFlags", "localFlags", offsetof(LocalVariableInfo, localFlags), makeLocalVariableInfoFlagsFlags());
            from("TypeInfo");
        }
    };

    struct FuncInfoAnnotation : DebugInfoAnnotation<VarInfo,FuncInfo> {
        FuncInfoAnnotation(ModuleLibrary & ml) : DebugInfoAnnotation ("FuncInfo", ml) {
            addField<DAS_BIND_MANAGED_FIELD(name)>("name");
            addField<DAS_BIND_MANAGED_FIELD(cppName)>("cppName");
            addField<DAS_BIND_MANAGED_FIELD(fields)>("fields");
            addField<DAS_BIND_MANAGED_FIELD(stackSize)>("stackSize");
            addField<DAS_BIND_MANAGED_FIELD(result)>("result");
            addField<DAS_BIND_MANAGED_FIELD(locals)>("locals");
            addField<DAS_BIND_MANAGED_FIELD(globals)>("globals");
            addField<DAS_BIND_MANAGED_FIELD(hash)>("hash");
            addField<DAS_BIND_MANAGED_FIELD(flags)>("flags");
            addField<DAS_BIND_MANAGED_FIELD(localCount)>("localCount");
            addField<DAS_BIND_MANAGED_FIELD(globalCount)>("globalCount");
            fieldType = makeType<VarInfo>(*mlib);
            fieldType->ref = true;
        }
    };

    TSequence<VarInfo&> each_FuncInfo ( FuncInfo & st, Context * context, LineInfoArg * at ) {
        return debugInfoIterator<FuncInfo,VarInfo>(&st, context, at);
    }

    TSequence<const VarInfo&> each_const_FuncInfo ( const FuncInfo & st, Context * context, LineInfoArg * at ) {
        return debugInfoIterator<FuncInfo,VarInfo>((FuncInfo *)&st, context, at);
    }

    struct CodeOfPoliciesAnnotation : ManagedStructureAnnotation<CodeOfPolicies,false,false> {
        CodeOfPoliciesAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CodeOfPolicies", ml) {
        // aot
            addField<DAS_BIND_MANAGED_FIELD(aot)>("aot");
            addField<DAS_BIND_MANAGED_FIELD(aot_lib)>("aot_lib");
            addField<DAS_BIND_MANAGED_FIELD(paranoid_validation)>("paranoid_validation");
            addField<DAS_BIND_MANAGED_FIELD(validate_ast)>("validate_ast");
            addField<DAS_BIND_MANAGED_FIELD(cross_platform)>("cross_platform");
            addField<DAS_BIND_MANAGED_FIELD(standalone_context)>("standalone_context");
            addField<DAS_BIND_MANAGED_FIELD(aot_module)>("aot_module");
            addField<DAS_BIND_MANAGED_FIELD(aot_macros)>("aot_macros");
            addField<DAS_BIND_MANAGED_FIELD(aot_result)>("aot_result");
            addField<DAS_BIND_MANAGED_FIELD(completion)>("completion");
            addField<DAS_BIND_MANAGED_FIELD(lint_check)>("lint_check");
            addField<DAS_BIND_MANAGED_FIELD(no_init_check)>("no_init_check");
            addField<DAS_BIND_MANAGED_FIELD(export_all)>("export_all");
            addField<DAS_BIND_MANAGED_FIELD(serialize_main_module)>("serialize_main_module");
            addField<DAS_BIND_MANAGED_FIELD(keep_alive)>("keep_alive");
            addField<DAS_BIND_MANAGED_FIELD(very_safe_context)>("very_safe_context");
        // reporting
            addField<DAS_BIND_MANAGED_FIELD(always_report_candidates_threshold)>("always_report_candidates_threshold");
        // infer passes
            addField<DAS_BIND_MANAGED_FIELD(max_infer_passes)>("max_infer_passes");
            addField<DAS_BIND_MANAGED_FIELD(max_call_depth)>("max_call_depth");
        // memory
            addField<DAS_BIND_MANAGED_FIELD(stack)>("stack");
            addField<DAS_BIND_MANAGED_FIELD(intern_strings)>("intern_strings");
            addField<DAS_BIND_MANAGED_FIELD(persistent_heap)>("persistent_heap");
            addField<DAS_BIND_MANAGED_FIELD(multiple_contexts)>("multiple_contexts");
            addField<DAS_BIND_MANAGED_FIELD(heap_size_hint)>("heap_size_hint");
            addField<DAS_BIND_MANAGED_FIELD(string_heap_size_hint)>("string_heap_size_hint");
            addField<DAS_BIND_MANAGED_FIELD(solid_context)>("solid_context");
            addField<DAS_BIND_MANAGED_FIELD(macro_context_persistent_heap)>("macro_context_persistent_heap");
            addField<DAS_BIND_MANAGED_FIELD(macro_context_collect)>("macro_context_collect");
            addField<DAS_BIND_MANAGED_FIELD(max_static_variables_size)>("max_static_variables_size");
            addField<DAS_BIND_MANAGED_FIELD(max_heap_allocated)>("max_heap_allocated");
            addField<DAS_BIND_MANAGED_FIELD(max_string_heap_allocated)>("max_string_heap_allocated");
            addField<DAS_BIND_MANAGED_FIELD(track_allocations)>("track_allocations");
        // rtti
            addField<DAS_BIND_MANAGED_FIELD(rtti)>("rtti");
        // language
            addField<DAS_BIND_MANAGED_FIELD(unsafe_table_lookup)>("unsafe_table_lookup");
            addField<DAS_BIND_MANAGED_FIELD(relaxed_pointer_const)>("relaxed_pointer_const");
            addField<DAS_BIND_MANAGED_FIELD(version_2_syntax)>("version_2_syntax");
            addField<DAS_BIND_MANAGED_FIELD(gen2_make_syntax)>("gen2_make_syntax");
            addField<DAS_BIND_MANAGED_FIELD(relaxed_assign)>("relaxed_assign");
            addField<DAS_BIND_MANAGED_FIELD(no_unsafe)>("no_unsafe");
            addField<DAS_BIND_MANAGED_FIELD(local_ref_is_unsafe)>("local_ref_is_unsafe");
            addField<DAS_BIND_MANAGED_FIELD(no_global_variables)>("no_global_variables");
            addField<DAS_BIND_MANAGED_FIELD(no_global_variables_at_all)>("no_global_variables_at_all");
            addField<DAS_BIND_MANAGED_FIELD(no_global_heap)>("no_global_heap");
            addField<DAS_BIND_MANAGED_FIELD(only_fast_aot)>("only_fast_aot");
            addField<DAS_BIND_MANAGED_FIELD(aot_order_side_effects)>("aot_order_side_effects");
            addField<DAS_BIND_MANAGED_FIELD(no_unused_function_arguments)>("no_unused_function_arguments");
            addField<DAS_BIND_MANAGED_FIELD(no_unused_block_arguments)>("no_unused_block_arguments");
            addField<DAS_BIND_MANAGED_FIELD(allow_block_variable_shadowing)>("allow_block_variable_shadowing");
            addField<DAS_BIND_MANAGED_FIELD(allow_local_variable_shadowing)>("allow_local_variable_shadowing");
            addField<DAS_BIND_MANAGED_FIELD(allow_shared_lambda)>("allow_shared_lambda");
            addField<DAS_BIND_MANAGED_FIELD(ignore_shared_modules)>("ignore_shared_modules");
            addField<DAS_BIND_MANAGED_FIELD(default_module_public)>("default_module_public");
            addField<DAS_BIND_MANAGED_FIELD(no_deprecated)>("no_deprecated");
            addField<DAS_BIND_MANAGED_FIELD(no_aliasing)>("no_aliasing");
            addField<DAS_BIND_MANAGED_FIELD(strict_smart_pointers)>("strict_smart_pointers");
            addField<DAS_BIND_MANAGED_FIELD(no_init)>("no_init");
            addField<DAS_BIND_MANAGED_FIELD(strict_unsafe_delete)>("strict_unsafe_delete");
            addField<DAS_BIND_MANAGED_FIELD(no_members_functions_in_struct)>("no_members_functions_in_struct");
            addField<DAS_BIND_MANAGED_FIELD(no_local_class_members)>("no_local_class_members");
            addField<DAS_BIND_MANAGED_FIELD(report_invisible_functions)>("report_invisible_functions");
            addField<DAS_BIND_MANAGED_FIELD(report_private_functions)>("report_private_functions");
            addField<DAS_BIND_MANAGED_FIELD(strict_properties)>("strict_properties");
        // environment
            addField<DAS_BIND_MANAGED_FIELD(no_optimizations)>("no_optimizations");
            addField<DAS_BIND_MANAGED_FIELD(no_infer_time_folding)>("no_infer_time_folding");
            addField<DAS_BIND_MANAGED_FIELD(fail_on_no_aot)>("fail_on_no_aot");
            addField<DAS_BIND_MANAGED_FIELD(fail_on_lack_of_aot_export)>("fail_on_lack_of_aot_export");
            addField<DAS_BIND_MANAGED_FIELD(log_compile_time)>("log_compile_time");
            addField<DAS_BIND_MANAGED_FIELD(log_total_compile_time)>("log_total_compile_time");
            addField<DAS_BIND_MANAGED_FIELD(log_module_compile_time)>("log_module_compile_time");
            addField<DAS_BIND_MANAGED_FIELD(no_fast_call)>("no_fast_call");
            addField<DAS_BIND_MANAGED_FIELD(scoped_stack_allocator)>("scoped_stack_allocator");
            addField<DAS_BIND_MANAGED_FIELD(force_inscope_pod)>("force_inscope_pod");
            addField<DAS_BIND_MANAGED_FIELD(log_inscope_pod)>("log_inscope_pod");
            addField<DAS_BIND_MANAGED_FIELD(force_escape_free)>("force_escape_free");
            addField<DAS_BIND_MANAGED_FIELD(force_allocate_on_stack)>("force_allocate_on_stack");
            addField<DAS_BIND_MANAGED_FIELD(log_escape_analysis)>("log_escape_analysis");
        // debugger
            addField<DAS_BIND_MANAGED_FIELD(debugger)>("debugger");
            addField<DAS_BIND_MANAGED_FIELD(debug_infer_flag)>("debug_infer_flag");
        // profiler
            addField<DAS_BIND_MANAGED_FIELD(profiler)>("profiler");
        // threadlock context
            addField<DAS_BIND_MANAGED_FIELD(threadlock_context)>("threadlock_context");
        // jit
            addField<DAS_BIND_MANAGED_FIELD(jit_enabled)>("jit_enabled");
            addField<DAS_BIND_MANAGED_FIELD(jit_jit_all_functions)>("jit_jit_all_functions");
            addField<DAS_BIND_MANAGED_FIELD(jit_debug_info)>("jit_debug_info");
            addField<DAS_BIND_MANAGED_FIELD(jit_opt_level)>("jit_opt_level");
            addField<DAS_BIND_MANAGED_FIELD(jit_size_level)>("jit_size_level");
            addField<DAS_BIND_MANAGED_FIELD(jit_dll_mode)>("jit_dll_mode");
            addField<DAS_BIND_MANAGED_FIELD(jit_exe_mode)>("jit_exe_mode");
            addField<DAS_BIND_MANAGED_FIELD(jit_emit_prologue)>("emit_prologue");
            addField<DAS_BIND_MANAGED_FIELD(jit_output_path)>("jit_output_path");
            addField<DAS_BIND_MANAGED_FIELD(jit_path_to_shared_lib)>("jit_path_to_shared_lib");
            addField<DAS_BIND_MANAGED_FIELD(jit_path_to_linker)>("jit_path_to_linker");
        }
        virtual bool isLocal() const override { return true; }
    };

    vector<pair<string,Type>> getCodeOfPolicyOptions() {
        vector<pair<string,Type>> options;
        Module dummyMod;
        ModuleLibrary dummy(&dummyMod);
        auto cop = new CodeOfPoliciesAnnotation(dummy);
        for ( auto & f : cop->fields ) {
            if ( f.second.decl->isWorkhorseType() ) {
                auto bT = f.second.decl->baseType;
                switch ( bT ) {
                    case Type::tUInt:   bT = Type::tInt; break;
                    default: break;
                }
                options.push_back({f.first,bT});
            }
        }
        return options;
    }


    struct DebugInfoHelperAnnotation : ManagedStructureAnnotation<DebugInfoHelper> {
        DebugInfoHelperAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation<DebugInfoHelper> ("DebugInfoHelper", ml) {
            addField<DAS_BIND_MANAGED_FIELD(rtti)>("rtti");
        }
    };

    template <typename TT>
    int32_t rtti_getDim ( const TT & ti, int32_t _index, Context * context, LineInfoArg * at ) {
        uint32_t index = _index;
        if ( ti.dimSize==0 ) {
            context->throw_error_at(at, "type is not an array");
        }
        if ( index>=ti.dimSize ) {
            context->throw_error_at(at, "dim index out of range, %u of %u", index, uint32_t(ti.dimSize));
        }
        return ti.dim[index];
    }

    int32_t rtti_getDimTypeInfo ( const TypeInfo & ti, int32_t index, Context * context, LineInfoArg * at ) {
        return rtti_getDim(ti, index, context, at);
    }

    int32_t rtti_getDimVarInfo ( const VarInfo & ti, int32_t index, Context * context, LineInfoArg * at ) {
        return rtti_getDim(ti, index, context, at);
    }

    int32_t rtti_contextTotalFunctions ( Context & context ) {
        return context.getTotalFunctions();
    }

    int32_t rtti_contextTotalVariables ( Context & context ) {
        return context.getTotalVariables();
    }

    vec4f rtti_contextFunctionInfo ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        Context * ctx = cast<Context *>::to(args[0]);
        int32_t tf = ctx->getTotalFunctions();
        int32_t index = cast<int32_t>::to(args[1]);
        if ( index<0 || index>=tf ) {
            context.throw_error_at(call->debugInfo, "function index out of range, %i of %i", index, tf);
        }
        FuncInfo * fi = ctx->getFunction(index)->debugInfo;
        return cast<FuncInfo *>::from(fi);
    }

    vec4f rtti_contextVariableInfo ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        Context * ctx = cast<Context *>::to(args[0]);
        int32_t tf = ctx->getTotalVariables();
        int32_t index = cast<int32_t>::to(args[1]);
        if ( index<0 || index>=tf ) {
            context.throw_error_at(call->debugInfo, "variable index out of range, %i of %i", index, tf);
        }
        return cast<VarInfo *>::from(ctx->getVariableInfo(index));
    }

    void rtti_builtin_simulate ( const smart_ptr<Program> & program,
            const TBlock<void,bool,smart_ptr_raw<Context>,string> & block, Context * context, LineInfoArg * lineinfo ) {
        TextWriter issues;
        auto ctx = get_context(program->getContextStackSize());
        ctx->addRef();
        bool failed = !program->simulate(*ctx, issues);
        if ( failed ) {
            for ( auto & err : program->errors ) {
                issues << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
            }
            string istr = issues.str();
            das_invoke<void>::invoke<bool,smart_ptr_raw<Context>,const string &>(context,lineinfo,block,false,nullptr,istr);
        } else {
            das_invoke<void>::invoke<bool,smart_ptr_raw<Context>,const string &>(context,lineinfo,block,true,ctx,"");
        }
        ctx->delRef();
    }

    void rtti_builtin_compile ( char * modName, char * str, const CodeOfPolicies & cop,
            const TBlock<void,bool,smart_ptr<Program>,const string> & block, Context * context, LineInfoArg * at ) {
        return rtti_builtin_compile_ex(modName, str, cop, true, block, context, at);
    }

    void rtti_builtin_compile_ex ( char * modName, char * str, const CodeOfPolicies & cop, bool exportAll,
            const TBlock<void,bool,smart_ptr<Program>,const string> & block, Context * context, LineInfoArg * at ) {
        str = str ? str : ((char *)"");
        TextWriter issues;
        uint32_t str_len = stringLengthSafe(*context, str);
        auto access = make_smart<FileAccess>();
        auto fileInfo = make_unique<TextFileInfo>((char *) str, uint32_t(str_len), false);
        access->setFileInfo(modName, das::move(fileInfo));
        ModuleGroup dummyLibGroup;
        auto program = parseDaScript(modName, "", access, issues, dummyLibGroup, exportAll, false, cop);
        if ( program ) {
            if (program->failed()) {
                for (auto & err : program->errors) {
                    issues << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
                }
                string istr = issues.str();
                vec4f args[3] = {
                    cast<bool>::from(false),
                    cast<smart_ptr<Program>>::from(program),
                    cast<string *>::from(&istr)
                };
                context->invoke(block, args, nullptr, at);
            } else {
                string istr = issues.str();
                vec4f args[3] = {
                    cast<bool>::from(true),
                    cast<smart_ptr<Program>>::from(program),
                    cast<string *>::from(&istr)
                };
                context->invoke(block, args, nullptr, at);
            }
        } else {
            context->throw_error_at(at, "rtti_compile internal error, something went wrong");
        }
    }

    Module * rtti_get_this_module ( smart_ptr_raw<Program> program ) {
        return program->thisModule.get();
    }

    Module * rtti_get_builtin_module ( const char * name ) {
        return Module::require(name);
    }

    bool rtti_has_module ( const char * name ) {
        return Module::require(name) != nullptr;
    }

    void rtti_builtin_program_for_each_module ( smart_ptr_raw<Program> program, const TBlock<void,Module *> & block, Context * context, LineInfoArg * at ) {
        program->library.foreach([&](Module * pm) -> bool {
            vec4f args[1] = { cast<Module *>::from(pm) };
            context->invoke(block, args, nullptr, at);
            return true;
        }, "*");
    }

    void rtti_builtin_program_for_each_registered_module ( const TBlock<void,Module *> & block, Context * context, LineInfoArg * at ) {
        Module::foreach([&](Module * pm) -> bool {
            vec4f args[1] = { cast<Module *>::from(pm) };
            context->invoke(block, args, nullptr, at);
            return true;
        });
    }

    void rtti_builtin_module_for_each_dependency ( Module * module, const TBlock<void,Module *,bool> & block, Context * context, LineInfoArg * at ) {
        for ( auto it : module->requireModule ) {
            vec4f args[2] = { cast<Module *>::from(it.first), cast<bool>::from(it.second) };
            context->invoke(block, args, nullptr, at);
        }
    }

    void rtti_builtin_module_for_each_enumeration ( Module * module, const TBlock<void,const EnumInfo> & block, Context * context, LineInfoArg * at ) {
        DebugInfoHelper helper;
        helper.rtti = true;
        module->enumerations.foreach([&](auto penum){
            EnumInfo * info = helper.makeEnumDebugInfo(*penum);
            vec4f args[1] = {
                cast<EnumInfo *>::from(info)
            };
            context->invoke(block, args, nullptr, at);
        });
    }

    RttiValue rtti_builtin_argument_value(const AnnotationArgument & info, Context * context, LineInfoArg * at ) {
        const auto align = sizeof(vec4f) - sizeof(int32_t);
        switch (info.type) {
        case Type::tBool:   return RttiValue::create<bool, RttiBool>(info.bValue, align);
        case Type::tInt:    return RttiValue::create<int32_t, RttiInt32>(info.iValue, align);
        case Type::tFloat:  return RttiValue::create<float, RttiFloat>(info.fValue, align);
        case Type::tString: return RttiValue::create<char*, RttiString>(context->allocateString(info.sValue, at), align);
        default: DAS_ASSERT(false); // I guess unreachable?
        }
        return RttiValue{};
    }


    RttiValue rtti_builtin_variable_value(const VarInfo & info) {
        RttiValue def {};
        def.index = RttiAny;
        if (info.dimSize == 0 && (info.flags & TypeInfo::flag_hasInitValue)!=0 ) {
            const auto align = sizeof(vec4f) - sizeof(int32_t);
            switch (info.type) {
            case Type::tBool:   def.index = RttiBool; break;
            case Type::tInt:    def.index = RttiInt32; break;
            case Type::tBitfield:
            case Type::tUInt:   def.index = RttiUint32; break;
            case Type::tInt64:  def.index = RttiInt64; break;
            case Type::tBitfield64:
            case Type::tUInt64: def.index = RttiUint64; break;
            case Type::tFloat:  def.index = RttiFloat; break;
            case Type::tDouble: def.index = RttiDouble; break;
            case Type::tString: def.index = RttiString; break;
            case Type::tFloat2:     case Type::tFloat3: case Type::tFloat4:
            case Type::tRange:      case Type::tURange:
            case Type::tRange64:    case Type::tURange64:
            case Type::tInt2:       case Type::tInt3:   case Type::tInt4:   case Type::tInt8:   case Type::tInt16:
            case Type::tUInt2:      case Type::tUInt3:  case Type::tUInt4:  case Type::tUInt8:  case Type::tUInt16:
            case Type::tBitfield8:  case Type::tBitfield16:
            case Type::tEnumeration:    case Type::tEnumeration8:   case Type::tEnumeration16:  case Type::tEnumeration64:
            case Type::tPointer:    case Type::tStructure:  case Type::tHandle:
            case Type::tArray:      case Type::tTable:      case Type::tTuple:  case Type::tVariant:
            case Type::tFunction:   case Type::tLambda:     case Type::tBlock:  case Type::tIterator:
                def.index = RttiAny;
                break;
            default:
                DAS_VERIFYF(false,"unsupported type %i in rtti_builtin_variable_value for %s. i guess new Type::type has been added",
                    int(info.type), info.name ? info.name : "???");
            }
            /*
             * Due to alignment we can't simply copy value.
             */
            if (def.index != RttiAny) {
                if (def.index == RttiString) {
                    def.set<char*, RttiString>(info.sValue, align);
                } else {
                    auto prev = def.index;
                    def.set<vec4f, RttiAny>(info.value, align);
                    def.index = prev;
                }
            }
        }
        return def;
    }

    void rtti_builtin_module_for_each_structure ( Module * module, const TBlock<void,const StructInfo> & block, Context * context, LineInfoArg * at ) {
        DebugInfoHelper helper;
        helper.rtti = true;
        module->structures.foreach([&](auto structPtr){
            if ( structPtr->isTemplate ) return;
            StructInfo * info = helper.makeStructureDebugInfo(*structPtr);
            vec4f args[1] = {
                cast<StructInfo *>::from(info)
            };
            context->invoke(block, args, nullptr, at);
        });
    }

    void rtti_builtin_structure_for_each_annotation ( const StructInfo & info, const Block & block, Context * context, LineInfoArg * at ) {
        if ( info.annotation_list ) {
            auto al = (const AnnotationList *) info.annotation_list;
            for ( const auto & adp : *al ) {
                vec4f args[2] = {
                    cast<Annotation *>::from(adp->annotation),
                    cast<AnnotationArgumentList *>::from(&adp->arguments)
                };
                context->invoke(block, args, nullptr, at);
            }
        }
    }

    void rtti_builtin_module_for_each_function ( Module * module, const TBlock<void,const FuncInfo> & block, Context * context, LineInfoArg * at ) {
        DebugInfoHelper helper;
        helper.rtti = true;
        module->functions.foreach([&](auto funcPtr){
            if ( funcPtr->isTemplate ) return;
            FuncInfo * info = helper.makeFunctionDebugInfo(*funcPtr);
            vec4f args[1] = {
                cast<FuncInfo *>::from(info)
            };
            context->invoke(block, args, nullptr, at);
        });
    }

    void rtti_builtin_module_for_each_generic ( Module * module, const TBlock<void,const FuncInfo> & block, Context * context, LineInfoArg * at ) {
        DebugInfoHelper helper;
        helper.rtti = true;
        module->generics.foreach([&](auto funcPtr){
            FuncInfo * info = helper.makeFunctionDebugInfo(*funcPtr);
            vec4f args[1] = {
                cast<FuncInfo *>::from(info)
            };
            context->invoke(block, args, nullptr, at);
        });
    }

    void rtti_builtin_module_for_each_global ( Module * module, const TBlock<void,const VarInfo> & block, Context * context, LineInfoArg * at ) {
        DebugInfoHelper helper;
        helper.rtti = true;
        module->globals.foreach([&](auto var){
            VarInfo * info = helper.makeVariableDebugInfo(*var);
            vec4f args[1] = {
                cast<VarInfo *>::from(info)
            };
            context->invoke(block, args, nullptr, at);
        });
    }

    void rtti_builtin_module_for_each_annotation ( Module * module, const TBlock<void,const Annotation> & block, Context * context, LineInfoArg * at ) {
        for ( auto & [key, annotationPtr] : module->handleTypes ) {
            vec4f args[1] = {
                cast<Annotation*>::from(annotationPtr)
            };
            context->invoke(block, args, nullptr, at);
        }
    }

    void rtti_builtin_basic_struct_for_each_parent ( const BasicStructureAnnotation & ann, const TBlock<void,Annotation *> & block, Context * context, LineInfoArg * at ) {
        for ( auto & it : ann.parents ) {
            vec4f args[1] = { cast<Annotation *>::from(it) };
            context->invoke(block, args, nullptr, at);
        }
    }

    void rtti_builtin_basic_struct_for_each_field ( const BasicStructureAnnotation & ann,
        const TBlock<void,char *,char*,const TypeInfo,uint32_t> & block, Context * context, LineInfoArg * at ) {
        DebugInfoHelper helper;
        helper.rtti = true;
        for ( auto & it : ann.fields ) {
            const auto & fld = it.second;
            TypeInfo * info = helper.makeTypeInfo(nullptr, fld.decl);
            vec4f args[4] = {
                cast<char *>::from(it.first.c_str()),
                cast<char *>::from(fld.cppName.c_str()),
                cast<TypeInfo *>::from(info),
                cast<uint32_t>::from(fld.offset)
            };
            context->invoke(block, args, nullptr, at);
        }
    }


    char * rtti_get_das_type_name(Type tt, Context * context, LineInfoArg * at) {
        string str = das_to_string(tt);
        return context->allocateString(str, at);
    }

    int rtti_add_annotation_argument(AnnotationArgumentList& list, const char* name) {
        list.emplace_back(name != nullptr ? name : "", 0);
        return (int)list.size() - 1;
    }

    bool rtti_add_file_access_root ( smart_ptr<FileAccess> access, const char * mod, const char * path ) {
        if ( !mod ) return false;
        if ( !path ) return false;
        return access->addFsRoot(mod, path);
    }

    void rtti_add_extra_module ( smart_ptr_raw<FileAccess> access, const char * modName, const char * modFile, Context * context, LineInfoArg * at ) {
        if ( !modName ) context->throw_error_at(at, "expecting module name");
        if ( !modFile ) context->throw_error_at(at, "expecting module file path");
        access->addExtraModule(modName, modFile);
    }


#if !DAS_NO_FILEIO

    void rtti_builtin_compile_file ( char * modName, smart_ptr<FileAccess> access, ModuleGroup* module_group, const CodeOfPolicies & cop,
            const TBlock<void,bool,smart_ptr<Program>,const string> & block, Context * context, LineInfoArg * at ) {
        TextWriter issues;
        if ( !access ) access = make_smart<FsFileAccess>();
        auto program = compileDaScript(modName, access, issues, *module_group, cop);
        if ( program ) {
            if (program->failed()) {
                for (auto & err : program->errors) {
                    issues << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
                }
                string istr = issues.str();
                vec4f args[3] = {
                    cast<bool>::from(false),
                    cast<smart_ptr<Program>>::from(program),
                    cast<string *>::from(&istr)
                };
                context->invoke(block, args, nullptr, at);
            } else {
                string istr = issues.str();
                vec4f args[3] = {
                    cast<bool>::from(true),
                    cast<smart_ptr<Program>>::from(program),
                    cast<string *>::from(&istr)
                };
                daScriptEnvironment::getBound()->g_Program = program;
                context->invoke(block, args, nullptr, at);
                daScriptEnvironment::getBound()->g_Program.reset();
            }
        } else {
            context->throw_error_at(at, "rtti_compile internal error, something went wrong");
        }
    }

    smart_ptr<FileAccess> makeFileAccess( char * pak, Context *, LineInfoArg * ) {
        return get_file_access(pak);
    }

    bool introduceFile ( smart_ptr_raw<FileAccess> access, char * fname, char * str, Context * context, LineInfoArg * at ) {
        if ( !fname ) context->throw_error_at(at, "expecting file name");
        const char * safeStr = str ? str : "";
        uint32_t str_len = stringLength(*context, safeStr);
        auto fileInfo = make_unique<TextFileInfo>(safeStr, str_len, false);
        return access->setFileInfo(fname, das::move(fileInfo)) != nullptr;
    }

#else
    smart_ptr<FileAccess> makeFileAccess( char *, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "not supported with DAS_NO_FILEIO");
        return nullptr;
    }

    void rtti_builtin_compile_file(  char *, smart_ptr<FileAccess>, ModuleGroup*, const CodeOfPolicies & cop,
            const TBlock<void, bool, smart_ptr<Program>, const string> &, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "not supported with DAS_NO_FILEIO");
    }

    bool introduceFile ( smart_ptr_raw<FileAccess>, char *, char *, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "not supported with DAS_NO_FILEIO");
        return false;
    }
#endif

    struct SimNode_RttiGetTypeDecl : SimNode_CallBase {
        DAS_PTR_NODE;
        SimNode_RttiGetTypeDecl ( const LineInfo & at, ExpressionPtr d )
            : SimNode_CallBase(at,"") {
            typeExpr = d->type;
        }
        virtual SimNode * visit ( SimVisitor & vis ) override {
            V_BEGIN();
            V_OP(RttiGetTypeDecl);
            V_ARG(typeExpr->getMangledName().c_str());
            V_END();
        }
        __forceinline char * compute(Context &) {
            DAS_PROFILE_NODE
            return (char *) typeExpr;
        }
        TypeDecl *  typeExpr;   // requires RTTI
    };

    struct RttiTypeInfoMacro : TypeInfoMacro {
        RttiTypeInfoMacro() : TypeInfoMacro("rtti_typeinfo") {}
        virtual TypeDeclPtr getAstType ( ModuleLibrary & lib, ExpressionPtr, string & ) override {
            return typeFactory<const TypeInfo>::make(lib);
        }
        virtual SimNode * simluate ( Context * context, ExpressionPtr expr, string & ) override {
            auto exprTypeInfo = static_cast<ExprTypeInfo*>(expr);
            TypeInfo * typeInfo = context->thisHelper->makeTypeInfo(nullptr, exprTypeInfo->typeexpr);
            return context->code->makeNode<SimNode_TypeInfo>(expr->at, typeInfo);
        }
        virtual bool aotNeedTypeInfo ( ExpressionPtr ) const override {
            return true;
        }
    };

    LineInfo getCurrentLineInfo( LineInfoArg * lineInfo ) {
        return lineInfo ? *lineInfo : LineInfo();
    }

    char * builtin_print_data ( const void * data, const TypeInfo * typeInfo, Bitfield flags, Context * context, LineInfoArg * at ) {
        TextWriter ssw;
        ssw << debug_value(data, (TypeInfo *)typeInfo, PrintFlags(uint32_t(flags)));
        return context->allocateString(ssw.str(), at);
    }

    char * builtin_print_data_v ( float4 data, const TypeInfo * typeInfo, Bitfield flags, Context * context, LineInfoArg * at ) {
        TextWriter ssw;
        ssw << debug_value(vec4f(data), (TypeInfo *)typeInfo, PrintFlags(uint32_t(flags)));
        return context->allocateString(ssw.str(), at);
    }

    char * builtin_json_sprint_at ( const void * addr, const TypeInfo & typeInfo, bool humanReadable, Context * context, LineInfoArg * at ) {
        auto s = debug_json_value((void *)addr, (TypeInfo *)&typeInfo, humanReadable);
        return context->allocateString(s, at);
    }

    bool builtin_json_sscan_at ( char * json, void * addr, const TypeInfo & typeInfo, Context * context, LineInfoArg * at ) {
        if ( !json || !addr ) return false;
        return debug_json_scan(*context, (char *)addr, (TypeInfo *)&typeInfo,
                               json, uint32_t(strlen(json)), at);
    }

    char * builtin_debug_type ( const TypeInfo * typeInfo, Context * context, LineInfoArg * at ) {
        if ( !typeInfo ) return nullptr;
        auto dt = debug_type(typeInfo);
        return context->allocateString(dt, at);
    }

    char * builtin_debug_line ( const LineInfo & at, bool fully, Context * context, LineInfoArg * lineInfo ) {
        auto dt = at.describe(fully);
        return context->allocateString(dt, lineInfo);
    }

    char * builtin_get_typeinfo_mangled_name ( const TypeInfo * typeInfo, Context * context, LineInfoArg * at ) {
        if ( !typeInfo ) return nullptr;
        auto dt = getTypeInfoMangledName((TypeInfo*)typeInfo);
        return context->allocateString(dt, at);
    }

    const FuncInfo * builtin_get_function_info_by_mnh ( Context &, Func fun ) {
        if ( fun.PTR ) {
            return fun.PTR->debugInfo;
        } else {
            return nullptr;
        }
    }

    void builtin_expected_errors ( ProgramPtr prog, const TBlock<void,CompilationError,int> & block, Context * context, LineInfoArg * lineInfo ) {
        for ( auto er : prog->expectErrors ) {
            das_invoke<void>::invoke<CompilationError,int>(context,lineInfo,block,er.first,er.second);
        }
    }

    void builtin_list_require ( ProgramPtr prog, const TBlock<void,Module *,TTemporary<const char *>,TTemporary<const char *>,bool,const LineInfo &> & block,
            Context * context, LineInfoArg * lineInfo ) {
        for ( auto & allR : prog->allRequireDecl ) {
            das_invoke<void>::invoke<Module *,const char *,const char *,bool,const LineInfo &>(context,lineInfo,block,
                get<0>(allR),get<1>(allR).c_str(),get<2>(allR).c_str(),get<3>(allR),get<4>(allR));
        }
    }


    Func builtin_SimFunction_by_MNH ( Context & context, uint64_t MNH ) {
        Func fn;
        fn.PTR = context.fnByMangledName(MNH);
        return fn;
    }

    LineInfo rtti_get_line_info ( int depth, Context * context, LineInfoArg * at ) {
    #if DAS_ENABLE_STACK_WALK
        char * sp = context->stack.ap();
        const LineInfo * lineAt = at;
        while (  sp < context->stack.top() ) {
            Prologue * pp = (Prologue *) sp;
            Block * block = nullptr;
            FuncInfo * info = nullptr;
            // char * SP = sp;
            if ( pp->info ) {
                intptr_t iblock = intptr_t(pp->block);
                if ( iblock & 1 ) {
                    block = (Block *) (iblock & ~1);
                    info = block->info;
                    // SP = context->stack.bottom() + block->stackOffset;
                } else {
                    info = pp->info;
                }
            }
            lineAt = info ? pp->line : pp->functionLine;
            sp += info ? info->stackSize : pp->stackSize;
            depth --;
            if ( depth==0 ) return lineAt ? *lineAt : LineInfo();
        }
        return LineInfo();
    #else
        return LineInfo();
    #endif
    }

    Func builtin_getFunctionByMnh ( uint64_t MNH, Context * context ) {
        return Func(context->fnByMangledName(MNH));
    }

    Func builtin_getFunctionByMnh_inContext ( uint64_t MNH, Context & context ) {
        return Func(context.fnByMangledName(MNH));
    }


    uint64_t builtin_getFunctionMnh ( Func func, Context * ) {
        return func.PTR ? func.PTR->mangledNameHash : 0;
    }

    void lockThisContext ( const TBlock<void> & block, Context * context, LineInfoArg * lineInfo ) {
        if ( !context->contextMutex ) context->throw_error_at(lineInfo,"threadlock_context is not set");
        context->threadlock_context([&](){
            context->invoke(block, nullptr, nullptr, lineInfo);
        });
    }

    void lockAnyContext ( Context & ctx, const TBlock<void> & block, Context * context, LineInfoArg * lineInfo ) {
        if ( !context->contextMutex ) context->throw_error_at(lineInfo,"threadlock_context is not set");
        ctx.threadlock_context([&](){
            context->invoke(block, nullptr, nullptr, lineInfo);
        });
    }

    void lockAnyMutex ( recursive_mutex & rm, const TBlock<void> & block, Context * context, LineInfoArg * lineInfo ) {
        lock_guard<recursive_mutex> guard(rm);
        context->invoke(block, nullptr, nullptr, lineInfo);
    }

    uint64_t das_get_SimFunction_by_MNH ( uint64_t MNH, Context & context ) {
        return (uint64_t) context.fnByMangledName(MNH);
    }

    template <typename KeyType>
    int32_t tableFindValue ( Table * tab, vec4f _key, int32_t valueTypeSize, Context * context ) {
        auto key = cast<KeyType>::to(_key);
        TableHash<KeyType> thh(context,valueTypeSize);
        auto hfn = hash_function(*context, key);
        // TODO Phase 4: surface as `long_rtti_get_value_pointer` and panic from int form on >INT_MAX.
        return (int32_t) thh.find(*tab, key, hfn);
    }

    int32_t rtti_getTablePtr ( void * _table, vec4f key, Type baseType, int valueTypeSize, Context * context, LineInfoArg * at ) {
        Table * tab = (Table *) _table;
        switch ( baseType ) {
            case Type::tBool:           return tableFindValue<bool>        (tab,key,valueTypeSize,context);
            case Type::tInt8:           return tableFindValue<int8_t>      (tab,key,valueTypeSize,context);
            case Type::tUInt8:          return tableFindValue<uint8_t>     (tab,key,valueTypeSize,context);
            case Type::tInt16:          return tableFindValue<int16_t>     (tab,key,valueTypeSize,context);
            case Type::tUInt16:         return tableFindValue<uint16_t>    (tab,key,valueTypeSize,context);
            case Type::tInt64:          return tableFindValue<int64_t>     (tab,key,valueTypeSize,context);
            case Type::tUInt64:         return tableFindValue<uint64_t>    (tab,key,valueTypeSize,context);
            case Type::tEnumeration:    return tableFindValue<int32_t>     (tab,key,valueTypeSize,context);
            case Type::tEnumeration8:   return tableFindValue<int8_t>      (tab,key,valueTypeSize,context);
            case Type::tEnumeration16:  return tableFindValue<int16_t>     (tab,key,valueTypeSize,context);
            case Type::tEnumeration64:  return tableFindValue<int64_t>     (tab,key,valueTypeSize,context);
            case Type::tBitfield:       return tableFindValue<Bitfield>    (tab,key,valueTypeSize,context);
            case Type::tBitfield8:      return tableFindValue<uint8_t>     (tab,key,valueTypeSize,context);
            case Type::tBitfield16:     return tableFindValue<uint16_t>    (tab,key,valueTypeSize,context);
            case Type::tBitfield64:     return tableFindValue<uint64_t>    (tab,key,valueTypeSize,context);
            case Type::tInt:            return tableFindValue<int32_t>     (tab,key,valueTypeSize,context);
            case Type::tInt2:           return tableFindValue<int2>        (tab,key,valueTypeSize,context);
            case Type::tInt3:           return tableFindValue<int3>        (tab,key,valueTypeSize,context);
            case Type::tInt4:           return tableFindValue<int4>        (tab,key,valueTypeSize,context);
            case Type::tUInt:           return tableFindValue<uint32_t>    (tab,key,valueTypeSize,context);
            case Type::tUInt2:          return tableFindValue<uint2>       (tab,key,valueTypeSize,context);
            case Type::tUInt3:          return tableFindValue<uint3>       (tab,key,valueTypeSize,context);
            case Type::tUInt4:          return tableFindValue<uint4>       (tab,key,valueTypeSize,context);
            case Type::tFloat:          return tableFindValue<float>       (tab,key,valueTypeSize,context);
            case Type::tFloat2:         return tableFindValue<float2>      (tab,key,valueTypeSize,context);
            case Type::tFloat3:         return tableFindValue<float3>      (tab,key,valueTypeSize,context);
            case Type::tFloat4:         return tableFindValue<float4>      (tab,key,valueTypeSize,context);
            case Type::tRange:          return tableFindValue<range>       (tab,key,valueTypeSize,context);
            case Type::tURange:         return tableFindValue<urange>      (tab,key,valueTypeSize,context);
            case Type::tRange64:        return tableFindValue<range64>     (tab,key,valueTypeSize,context);
            case Type::tURange64:       return tableFindValue<urange64>    (tab,key,valueTypeSize,context);
            case Type::tString:         return tableFindValue<char *>      (tab,key,valueTypeSize,context);
            case Type::tPointer:        return tableFindValue<void *>      (tab,key,valueTypeSize,context);
            default:
                context->throw_error_at(at,"rtti.getTablePtr: unsupported type '%s'", das_to_string(baseType).c_str());
        }
    }

    char * rtti_get_source_line ( FileInfo * info, uint32_t line, Context * context, LineInfoArg * at ) {
        if ( !info ) return nullptr;
        const char * begin = nullptr;
        uint32_t len = 0;
        if ( !info->getLine(line, begin, len) ) return nullptr;
        return context->allocateString(begin, len, at);
    }

    // C-style scanner; no std::regex, no allocation. Matches all of:
    //   // nolint:CODE                          -> suppresses CODE
    //   // nolint:CODE1,CODE2,...               -> comma-separated list
    //   // nolint:CODE1 nolint:CODE2            -> repeated directives, space-separated
    //   // free-form prose nolint:CODE more     -> directive anywhere after the `//`
    // After the `//`, we walk the rest of the line looking for `nolint:` occurrences
    // and parse a comma-list of codes after each one. Any match wins.
    bool rtti_is_nolint_suppressed ( FileInfo * info, uint32_t line, const char * code, Context * /*context*/, LineInfoArg * /*at*/ ) {
        if ( !info || !code || !*code ) return false;
        const char * begin = nullptr;
        uint32_t len = 0;
        if ( !info->getLine(line, begin, len) ) return false;
        const char * end = begin + len;
        // locate "//"
        const char * p = begin;
        while ( p + 1 < end && !(p[0] == '/' && p[1] == '/') ) p++;
        if ( p + 1 >= end ) return false;
        p += 2;
        static const char NOLINT[] = "nolint:";
        constexpr uint32_t NOLINT_LEN = sizeof(NOLINT) - 1;
        const uint32_t codeLen = uint32_t(strlen(code));
        // walk the rest of the line; each `nolint:` opens a comma-list of codes.
        while ( p + NOLINT_LEN <= end ) {
            if ( memcmp(p, NOLINT, NOLINT_LEN) != 0 ) { p++; continue; }
            p += NOLINT_LEN;
            // comma-list parse
            while ( p < end ) {
                while ( p < end && (*p == ' ' || *p == '\t') ) p++;
                const char * tokStart = p;
                while ( p < end && *p != ',' && *p != ' ' && *p != '\t' ) p++;
                uint32_t tokLen = uint32_t(p - tokStart);
                if ( tokLen == codeLen && memcmp(tokStart, code, codeLen) == 0 ) return true;
                while ( p < end && (*p == ' ' || *p == '\t') ) p++;
                if ( p < end && *p == ',' ) { p++; continue; }
                break;
            }
        }
        return false;
    }

    class Module_Rtti : public Module {
    public:
        template <typename RecAnn>
        void addRecAnnotation ( ModuleLibrary & lib ) {
            auto rec = new RecAnn(lib);
            addAnnotation(rec);
            initRecAnnotation(rec, lib);
        }
        Module_Rtti() : Module("rtti_core") {
            DAS_PROFILE_SECTION("Module_Rtti");
            ModuleLibrary lib(this);
            lib.addBuiltInModule();
            // flags
            addAlias(makeProgramFlags());
            addAlias(makeContextCategoryFlags());
            addAlias(makeTypeInfoFlags());
            addAlias(makeStructInfoFlags());
            addAlias(makeModuleFlags());
            addAlias(makeAnnotationDeclarationFlags());
            addAlias(makeSimFunctionFlags());
            addAlias(makeLocalVariableInfoFlagsFlags());
            // CodeOfPolicies
            addAnnotation(new CodeOfPoliciesAnnotation(lib));
            addCtorAndUsing<CodeOfPolicies>(*this,lib,"CodeOfPolicies","CodeOfPolicies");
            // enums
            addEnumeration(new EnumerationCompilationError());
            // type annotations
            addAnnotation(new FileInfoAnnotation(lib));
            addAnnotation(new LineInfoAnnotation(lib));
                addCtor<LineInfo>(*this,lib,"LineInfo","LineInfo");
                addCtor<LineInfo,FileInfo *,int,int,int,int>(*this,lib,"LineInfo","LineInfo");
            addAnnotation(new DummyTypeAnnotation("recursive_mutex","recursive_mutex",sizeof(recursive_mutex),alignof(recursive_mutex)));
            addUsing<recursive_mutex>(*this, lib, "das::recursive_mutex");
            addAnnotation(new ContextAnnotation(lib));
            addAnnotation(new ErrorAnnotation(lib));
            addAnnotation(new FileAccessAnnotation(lib));
            addAnnotation(new ModuleAnnotation(lib));
            addAnnotation(new AstModuleGroupAnnotation(lib));
            addAnnotation(new AstSerializerAnnotation(lib));
            addEnumeration(new EnumerationType());
            addAnnotation(new AnnotationArgumentAnnotation(lib));
            addVectorAnnotation<AnnotationArguments>(this,lib,"AnnotationArguments");
            addVectorAnnotation<AnnotationArgumentList>(this,lib,"AnnotationArgumentList");
            addAnnotation(new ProgramAnnotation(lib));
            addAnnotation(new AnnotationAnnotation(lib));
            addAnnotation(new AnnotationDeclarationAnnotation(lib));
            addVectorAnnotation<AnnotationList>(this,lib,"AnnotationList");
            addAnnotation(new TypeAnnotationAnnotation(lib));
            addAnnotation(new BasicStructureAnnotationAnnotation(lib));
            addAnnotation(new EnumValueInfoAnnotation(lib));
            addAnnotation(new EnumInfoAnnotation(lib));
            addEnumeration(new EnumerationRefMatters());
            addEnumeration(new EnumerationConstMatters());
            addEnumeration(new EnumerationTemporaryMatters());
            auto sia = new StructInfoAnnotation(lib);              // this is type forward decl
            addAnnotation(sia);
            addRecAnnotation<TypeInfoAnnotation>(lib);
            addRecAnnotation<VarInfoAnnotation>(lib);
            addRecAnnotation<LocalVariableInfoAnnotation>(lib);
            initRecAnnotation(sia, lib);
            addAnnotation(new FuncInfoAnnotation(lib));
            addAnnotation(new SimFunctionAnnotation(lib));
            // DebugInfoHelper
            addAnnotation(new DebugInfoHelperAnnotation(lib));
            // RttiValue
            addAlias(typeFactory<RttiValue>::make(lib));
            // func info flags
            addConstant<uint32_t>(*this, "FUNCINFO_INIT", uint32_t(FuncInfo::flag_init));
            addConstant<uint32_t>(*this, "FUNCINFO_BUILTIN", uint32_t(FuncInfo::flag_builtin));
            addConstant<uint32_t>(*this, "FUNCINFO_PRIVATE", uint32_t(FuncInfo::flag_private));
            addConstant<uint32_t>(*this, "FUNCINFO_SHUTDOWN", uint32_t(FuncInfo::flag_shutdown));
            addConstant<uint32_t>(*this, "FUNCINFO_LATE_INIT", uint32_t(FuncInfo::flag_late_init));
            // macros
            addTypeInfoMacro(new RttiTypeInfoMacro());
            // ctors
            addUsing<ModuleGroup>(*this, lib, "ModuleGroup");
            // functions
            //      all the stuff is only resolved after debug info is built
            //      hence SideEffects::modifyExternal is essential for it to not be optimized out
            addExtern<DAS_BIND_FUN(rtti_getDimTypeInfo)>(*this, lib, "get_dim",
                SideEffects::modifyExternal, "rtti_getDimTypeInfo")
                    ->args({"typeinfo","index","context","at"});
            addExtern<DAS_BIND_FUN(rtti_getDimVarInfo)>(*this, lib, "get_dim",
                SideEffects::modifyExternal, "rtti_getDimVarInfo")
                    ->args({"typeinfo","index","context","at"});
            addExtern<DAS_BIND_FUN(rtti_contextTotalFunctions)>(*this, lib, "get_total_functions",
                SideEffects::modifyExternal, "rtti_contextTotalFunctions")
                    ->arg("context");
            addExtern<DAS_BIND_FUN(rtti_contextTotalVariables)>(*this, lib, "get_total_variables",
                SideEffects::modifyExternal, "rtti_contextTotalVariables")
                    ->arg("context");
            addInterop<rtti_contextFunctionInfo,const FuncInfo &,vec4f,int32_t>(*this, lib, "get_function_info",
                SideEffects::modifyExternal, "rtti_contextFunctionInfo")
                    ->args({"context","index"});
            addInterop<rtti_contextVariableInfo,const VarInfo &,vec4f,int32_t>(*this, lib, "get_variable_info",
                SideEffects::modifyExternal, "rtti_contextVariableInfo")
                    ->args({"context","index"});
            addExtern<DAS_BIND_FUN(rtti_get_this_module)>(*this, lib, "get_this_module",
                SideEffects::modifyExternal, "rtti_get_this_module")
                    ->arg("program");
            addExtern<DAS_BIND_FUN(rtti_get_builtin_module)>(*this, lib, "get_module",
                SideEffects::modifyExternal, "rtti_get_builtin_module")
                    ->arg("name");
            addExtern<DAS_BIND_FUN(rtti_has_module)>(*this, lib, "has_module",
                SideEffects::modifyExternal, "rtti_has_module")
                    ->arg("name");
            addExtern<DAS_BIND_FUN(rtti_builtin_compile)>(*this, lib, "compile",
                SideEffects::modifyExternal, "rtti_builtin_compile")
                    ->args({"module_name","codeText","codeOfPolicies","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_compile_ex)>(*this, lib, "compile",
                SideEffects::modifyExternal, "rtti_builtin_compile_ex")
                    ->args({"module_name","codeText","codeOfPolicies","exportAll","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_compile_file)>(*this, lib, "compile_file",
                SideEffects::modifyExternal, "rtti_builtin_compile_file")
                    ->args({"module_name","fileAccess","moduleGroup","codeOfPolicies","block","context","line"});
            addExtern<DAS_BIND_FUN(builtin_expected_errors)>(*this, lib, "for_each_expected_error",
                SideEffects::modifyExternal, "builtin_expected_errors")
                    ->args({"program","block","context","line"});
            addExtern<DAS_BIND_FUN(builtin_list_require)>(*this, lib, "for_each_require_declaration",
                SideEffects::modifyExternal, "builtin_list_require")
                    ->args({"program","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_simulate)>(*this, lib, "simulate",
                SideEffects::modifyExternal, "rtti_builtin_simulate")
                    ->args({"program","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_create_ast_serializer)>(*this, lib, "create_ast_serializer",
                SideEffects::modifyExternal, "rtti_create_ast_serializer");
            addExtern<DAS_BIND_FUN(rtti_create_ast_deserializer)>(*this, lib, "create_ast_deserializer",
                SideEffects::modifyExternal, "rtti_create_ast_deserializer")
                    ->args({"data"});
            addExtern<DAS_BIND_FUN(rtti_delete_ast_serializer)>(*this, lib, "delete_ast_serializer",
                SideEffects::modifyExternal, "rtti_delete_ast_serializer")
                    ->args({"serializer"});
            addExtern<DAS_BIND_FUN(rtti_ast_serializer_serialize_program)>(*this, lib, "serialize_program",
                SideEffects::modifyExternal, "rtti_ast_serializer_serialize_program")
                    ->args({"serializer","program"});
            addExtern<DAS_BIND_FUN(rtti_ast_serializer_deserialize_program)>(*this, lib, "deserialize_program",
                SideEffects::modifyExternal, "rtti_ast_serializer_deserialize_program")
                    ->args({"serializer","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_ast_serializer_get_data)>(*this, lib, "ast_serializer_get_data",
                SideEffects::modifyExternal, "rtti_ast_serializer_get_data")
                    ->args({"serializer","block","context","line"});
            addExtern<DAS_BIND_FUN(makeFileAccess)>(*this, lib, "make_file_access",
                SideEffects::modifyExternal, "makeFileAccess")
                    ->args({"project","context","at"});
            addExtern<DAS_BIND_FUN(introduceFile)>(*this, lib, "set_file_source",
                SideEffects::modifyExternal, "introduceFile")
                    ->args({"access","fileName","text","context","line"});
            addExtern<DAS_BIND_FUN(rtti_add_file_access_root)>(*this, lib, "add_file_access_root",
                SideEffects::modifyExternal, "rtti_add_file_access_root")
                    ->args({"access","mod","path"});
            addExtern<DAS_BIND_FUN(rtti_add_extra_module)>(*this, lib, "add_extra_module",
                SideEffects::modifyExternal, "rtti_add_extra_module")
                    ->args({"access","modName","modFile","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_program_for_each_module)>(*this, lib, "program_for_each_module",
                SideEffects::modifyExternal, "rtti_builtin_program_for_each_module")
                    ->args({"program","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_dependency)>(*this, lib, "module_for_each_dependency",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_dependency")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_program_for_each_registered_module)>(*this, lib, "program_for_each_registered_module",
                SideEffects::modifyExternal, "rtti_builtin_program_for_each_registered_module")
                    ->args({"block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_structure)>(*this, lib, "module_for_each_structure",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_structure")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_variable_value),SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "get_variable_value",
                SideEffects::modifyExternal, "rtti_builtin_variable_value")
                    ->arg("varInfo");
            addExtern<DAS_BIND_FUN(rtti_builtin_argument_value),SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "get_annotation_argument_value",
                SideEffects::modifyExternal, "rtti_builtin_argument_value")
                    ->args({"info","context","at"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_enumeration)>(*this, lib, "module_for_each_enumeration",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_enumeration")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_function)>(*this, lib, "module_for_each_function",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_function")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_generic)>(*this, lib, "module_for_each_generic",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_generic")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_global)>(*this, lib, "module_for_each_global",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_global")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_module_for_each_annotation)>(*this, lib, "module_for_each_annotation",
                SideEffects::modifyExternal, "rtti_builtin_module_for_each_annotation")
                    ->args({"module","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_structure_for_each_annotation)>(*this, lib, "rtti_builtin_structure_for_each_annotation",
                SideEffects::modifyExternal, "rtti_builtin_structure_for_each_annotation")
                    ->args({"struct","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_basic_struct_for_each_field)>(*this, lib, "basic_struct_for_each_field",
                SideEffects::invokeAndAccessExternal, "rtti_builtin_basic_struct_for_each_field")
                    ->args({"annotation","block","context","line"});
            addExtern<DAS_BIND_FUN(rtti_builtin_basic_struct_for_each_parent)>(*this, lib, "basic_struct_for_each_parent",
                SideEffects::invokeAndAccessExternal, "rtti_builtin_basic_struct_for_each_parent")
                    ->args({"annotation","block","context","line"});
            addExtern<DAS_BIND_FUN(isSameType)>(*this, lib, "builtin_is_same_type",
                SideEffects::modifyExternal, "isSameType")
                    ->args({"a","b","refMatters","cosntMatters","tempMatters","topLevel"});
            addExtern<DAS_BIND_FUN(getTypeSize)>(*this, lib, "get_type_size",
                SideEffects::none, "getTypeSize")
                    ->args({"type"});
            addExtern<DAS_BIND_FUN(getTypeAlign)>(*this, lib, "get_type_align",
                SideEffects::none, "getTypeAlign")
                    ->args({"type"});
            addExtern<DAS_BIND_FUN(getTupleFieldOffset)>(*this, lib, "get_tuple_field_offset",
                SideEffects::none, "getTupleFieldOffset")
                    ->args({"type", "index"});
            addExtern<DAS_BIND_FUN(getVariantFieldOffset)>(*this, lib, "get_variant_field_offset",
                SideEffects::none, "getVariantFieldOffset")
                    ->args({"type", "index"});
            addExtern<DAS_BIND_FUN(isCompatibleCast)>(*this, lib, "is_compatible_cast",
                SideEffects::modifyExternal, "isCompatibleCast")
                    ->args({"from","to"});
            addExtern<DAS_BIND_FUN(rtti_get_das_type_name)>(*this, lib,  "get_das_type_name",
                SideEffects::none, "rtti_get_das_type_name")
                    ->args({"type","context","at"});
            addExtern<DAS_BIND_FUN(rtti_add_annotation_argument)>(*this, lib,  "add_annotation_argument",
                SideEffects::none, "rtti_add_annotation_argument")
                    ->args({"annotation","name"});
            // data printer
            addExtern<DAS_BIND_FUN(builtin_print_data)>(*this, lib, "sprint_data",
                SideEffects::none, "builtin_print_data")
                    ->args({"data","type","flags","context","at"});
            addExtern<DAS_BIND_FUN(builtin_print_data_v)>(*this, lib, "sprint_data",
                SideEffects::none, "builtin_print_data_v")
                    ->args({"data","type","flags","context","at"});
            // sprint_json_at / sscan_json_at — addr+TypeInfo entry points.
            // Unlike sprint_json / sscan_json (which use any+SimNode_CallBase::types[]
            // and require a typed value expression at the call site), these take an
            // explicit (addr, ti) pair. Use when you have only a raw address
            // (e.g. from `unsafe(addr(g))`) plus a TypeInfo pointer from
            // `typeinfo rtti_typeinfo(type<T>)`. No per-call-site daslang code emit.
            // SideEffects::modifyExternal matches sprint_json (module_builtin_runtime.cpp).
            // sprint_json_at reads memory THROUGH addr — tagging this as `none` would let
            // the optimizer CSE/hoist calls across mutations of *addr, producing stale JSON.
            // The flag is intentionally pessimistic ("modify" is a superset of "access");
            // the read-only semantics are correct in code but invisible to the typer.
            addExtern<DAS_BIND_FUN(builtin_json_sprint_at)>(*this, lib, "sprint_json_at",
                SideEffects::modifyExternal, "builtin_json_sprint_at")
                    ->args({"addr","type","humanReadable","context","at"});
            addExtern<DAS_BIND_FUN(builtin_json_sscan_at)>(*this, lib, "sscan_json_at",
                SideEffects::modifyArgumentAndExternal, "builtin_json_sscan_at")
                    ->args({"json","addr","type","context","at"});
            // debug typeinfo
            addExtern<DAS_BIND_FUN(builtin_debug_type)>(*this, lib, "describe",
                SideEffects::none, "builtin_debug_type")
                    ->args({"type","context","at"});
            auto dl = addExtern<DAS_BIND_FUN(builtin_debug_line)>(*this, lib, "describe",
                SideEffects::none, "builtin_debug_line")
                    ->args({"lineinfo","fully","context","at"});
            dl->arguments[1]->init = new ExprConstBool(false);
            addExtern<DAS_BIND_FUN(builtin_get_typeinfo_mangled_name)>(*this, lib, "get_mangled_name",
                SideEffects::none, "builtin_get_typeinfo_mangled_name")
                    ->args({"type","context","at"});
            // function mnh lookup
            addExtern<DAS_BIND_FUN(builtin_get_function_info_by_mnh)>(*this, lib, "get_function_info",
                SideEffects::none, "builtin_get_function_info_by_mnh")
                    ->args({"context","function"});
            addExtern<DAS_BIND_FUN(builtin_SimFunction_by_MNH)>(*this, lib, "get_function_by_mnh",
                SideEffects::none, "builtin_SimFunction_by_MNH")
                    ->args({"context","MNH"});
            // current line info
            addExtern<DAS_BIND_FUN(getCurrentLineInfo), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
                "get_line_info", SideEffects::none, "getCurrentLineInfo")->arg("line");
            addExtern<DAS_BIND_FUN(rtti_get_line_info), SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "get_line_info",
                SideEffects::modifyExternal, "rtti_get_line_info")
                    ->args({"depth","context","line"});
            // this context
            addExtern<DAS_BIND_FUN(thisContext), SimNode_ExtFuncCallRef>(*this, lib,  "this_context",
                SideEffects::accessExternal, "thisContext")->arg("context");
            // fn-mnh
            addExtern<DAS_BIND_FUN(builtin_getFunctionByMnh)>(*this, lib, "get_function_by_mangled_name_hash",
                SideEffects::none, "builtin_getFunctionByMnh")
                    ->args({"src","context"});
            addExtern<DAS_BIND_FUN(builtin_getFunctionByMnh_inContext)>(*this, lib, "get_function_by_mangled_name_hash",
                SideEffects::none, "builtin_getFunctionByMnh_inContext")
                    ->args({"src","context"});
            addExtern<DAS_BIND_FUN(builtin_getFunctionMnh)>(*this, lib, "get_function_mangled_name_hash",
                SideEffects::none, "builtin_getFunctionMnh")
                    ->args({"src","context"});
            // mutex & lock
            addExtern<DAS_BIND_FUN(lockThisContext)>(*this, lib, "lock_this_context",
                SideEffects::worstDefault, "lockThisContext")
                    ->args({"block","context","line"});
            addExtern<DAS_BIND_FUN(lockAnyContext)>(*this, lib, "lock_context",
                SideEffects::worstDefault, "lockAnyContext")
                    ->args({"lock_context","block","context","line"});
            addExtern<DAS_BIND_FUN(lockAnyMutex)>(*this, lib,  "lock_mutex",
                SideEffects::worstDefault, "lockAnyMutex")
                    ->args({"mutex","block","context","line"});
            // in context
            addExtern<DAS_BIND_FUN(das_get_SimFunction_by_MNH)>(*this, lib, "get_function_address",
                SideEffects::none, "das_get_SimFunction_by_MNH")
                    ->args({"MNH","at"});
            // table key index
            addExtern<DAS_BIND_FUN(rtti_getTablePtr)>(*this, lib, "get_table_key_index",
                SideEffects::none, "rtti_getTablePtr")
                    ->args({"table","key","baseType","valueTypeSize","context","at"});
            // 'each' iterators for jit
            addExtern<DAS_BIND_FUN(each_FuncInfo),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
                SideEffects::modifyArgument, "each_FuncInfo")
                    ->args({"info","context","at"});
            addExtern<DAS_BIND_FUN(each_const_FuncInfo),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
                SideEffects::none, "each_const_FuncInfo")
                    ->args({"info","context","at"});
            addExtern<DAS_BIND_FUN(each_StructInfo),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
                SideEffects::modifyArgument, "each_StructInfo")
                    ->args({"info","context","at"});
            addExtern<DAS_BIND_FUN(each_const_StructInfo),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
                SideEffects::none, "each_const_StructInfo")
                    ->args({"info","context","at"});
            addExtern<DAS_BIND_FUN(each_EnumInfo),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
                SideEffects::modifyArgument, "each_EnumInfo")
                    ->args({"info","context","at"});
            addExtern<DAS_BIND_FUN(each_const_EnumInfo),SimNode_ExtFuncCallAndCopyOrMove,explicitConstArgFn>(*this, lib, "each",
                SideEffects::none, "each_const_EnumInfo")
                    ->args({"info","context","at"});
            addExtern<DAS_BIND_FUN(rtti_get_source_line)>(*this, lib, "rtti_get_source_line",
                SideEffects::accessExternal, "rtti_get_source_line")
                    ->args({"info","line","context","at"});
            addExtern<DAS_BIND_FUN(rtti_is_nolint_suppressed)>(*this, lib, "rtti_is_nolint_suppressed",
                SideEffects::accessExternal, "rtti_is_nolint_suppressed")
                    ->args({"info","line","code","context","at"});
            // lets make sure its all aot ready
            verifyAotReady();
        }
        virtual ModuleAotType aotRequire ( TextWriter & tw ) const override {
            tw << "#include \"daScript/simulate/aot_builtin_rtti.h\"\n";
            tw << "#include \"daScript/ast/ast.h\"\n";
            tw << "#include \"daScript/ast/ast_handle.h\"\n";
            return ModuleAotType::hybrid;
        }
    };
}

REGISTER_MODULE_IN_NAMESPACE(Module_Rtti,das);
