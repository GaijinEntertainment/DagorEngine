# Migration Review Checklist

All test files added from the `examples/test/` (daScriptTest) migration on the `language-testing` branch.
Each file needs manual review to verify correctness, gen2 syntax, and proper dastest integration.

**Legend:**
- `[ ]` — needs review
- `[x]` — reviewed and approved

---

## assert_once/

- [ ] test_assert_once.das

## json/

- [ ] test_sprint_json.das

## language/

### Helpers (review for gen2 correctness)

- [ ] _helper_foo.das
- [ ] _module_a.das
- [ ] _module_b.das

### Tests

- [ ] access_private_from_lambda.das
- [ ] add_property_ext_const.das
- [ ] array.das
- [ ] array_comprehension.das
- [ ] auto_infer.das
- [ ] auto_infer_success.das
- [ ] auto_ref_and_move_ret.das
- [ ] bin_serializer.das
- [ ] block_access_function_arg.das
- [ ] block_args_nested.das
- [ ] block_invoke.das
- [ ] block_variable.das
- [ ] block_vs_local_block.das
- [ ] bool_condition.das
- [ ] cant_access_private_members.das
- [ ] cant_dereference_mix.das
- [ ] cant_derive_from_sealed_class.das
- [ ] cant_get_field.das
- [ ] cant_have_local_variable.das
- [ ] cant_index.das
- [ ] cant_override_sealed.das
- [ ] cant_write_to_constant_value.das
- [ ] capture_as_ref.das
- [ ] cast.das
- [ ] chain_invoke_method.das
- [ ] check_eid.das
- [ ] clone.das
- [ ] clone_temp.das
- [ ] clone_to_move.das
- [ ] comment_eof.das
- [ ] condition_must_be_bool.das
- [ ] const_and_block_folding.das
- [ ] const_ref.das
- [ ] copy_and_move_on_return.das
- [ ] cpp_layout.das
- [ ] ctor.das
- [ ] das_string.das
- [ ] defer.das
- [ ] deref_ptr_fun.das
- [ ] dim.das
- [ ] div_by_zero.das
- [ ] dummy.das
- [ ] duplicate_keys.das
- [ ] dynamic_array.das
- [ ] each_std_vector.das
- [ ] enum.das
- [ ] erase_if.das
- [ ] finally.das
- [ ] for_const_array.das
- [ ] for_continue.das
- [ ] for_loop.das
- [ ] for_single_element.das
- [ ] fully_qualified_generic_name.das
- [ ] func_addr.das
- [ ] function_already_declared.das
- [ ] function_argument_already_declared.das
- [ ] function_not_found_ambiguous.das
- [ ] generators.das
- [ ] global_init_type_mismatch.das
- [ ] global_order.das
- [ ] global_ptr_init.das
- [ ] global_variable_already_declared.das
- [ ] handle.das
- [ ] if_not_null.das
- [ ] ignore_deref.das
- [ ] infer_alias_and_alias_ctor.das
- [ ] infer_alias_argument.das
- [ ] infer_remove_ref_const.das
- [ ] inscope_return_inscope.das
- [ ] int_types.das
- [ ] invalid_argument_count_mix.das
- [ ] invalid_array_type_mix.das
- [ ] invalid_block.das
- [ ] invalid_escape_sequence.das
- [ ] invalid_index_type.das
- [ ] invalid_infer_return_type.das
- [ ] invalid_return_type_mix.das
- [ ] invalid_structure_field_type_ref.das
- [ ] invalid_structure_field_type_void.das
- [ ] invalid_table_type_mix.das
- [ ] invalid_type_ref_in_table_value.das
- [ ] invoke_cmres.das
- [ ] jit_abi.das
- [ ] labels.das
- [ ] lambda_basic.das
- [ ] lambda_capture_modes.das
- [ ] lambda_to_iter.das
- [ ] line_info.das
- [ ] lock_array.das
- [ ] loop_ret.das
- [ ] make_default.das
- [ ] make_handle.das
- [ ] make_local.das
- [ ] make_struct_with_clone.das
- [ ] map_to_a.das
- [ ] memzero.das
- [ ] method_semantic.das
- [ ] mismatching_curly_bracers.das
- [ ] mismatching_parentheses.das
- [ ] mksmart_zero.das
- [ ] module_vis_fail.das
- [ ] move_lambda_local_ref.das
- [ ] move_on_return.das
- [ ] new_and_init.das
- [ ] new_delete.das
- [ ] new_type_infer.das
- [ ] new_with_init.das
- [ ] not_all_paths_return_a_value.das
- [ ] oop.das
- [ ] operator_overload.das
- [ ] override_field.das
- [ ] partial_specialization.das
- [ ] peek_and_modify_string.das
- [ ] ptr_arithmetic.das
- [ ] ptr_index.das
- [ ] reflection.das
- [ ] reserved_names.das
- [ ] return_reference.das
- [ ] rpipe.das
- [ ] run_annotation_side_effects.das
- [ ] safe_index.das
- [ ] safe_operators.das
- [ ] scatter_gather.das
- [ ] set_table.das
- [ ] setand_and_setor_bool.das
- [ ] shifts.das
- [ ] sizeof_reference.das
- [ ] smart_ptr.das
- [ ] smart_ptr_move.das
- [ ] sort.das
- [ ] static_assert_in_infer.das
- [ ] static_if.das
- [ ] stdvec_r2v.das
- [ ] storage_types.das
- [ ] string_builder.das
- [ ] string_ops.das
- [ ] struct.das
- [ ] structure_already_defined.das
- [ ] structure_field_already_declared.das
- [ ] structure_not_found_ambiguous.das
- [ ] table_operations.das
- [ ] test_value_table_key.das
- [ ] to_array.das
- [ ] to_table.das
- [ ] trailing_delimiters.das
- [ ] try_recover.das
- [ ] tuple.das
- [ ] type_loop.das
- [ ] type_not_found.das
- [ ] typeAlias.das
- [ ] typeinfo_annotations.das
- [ ] typename.das
- [ ] types.das
- [ ] unused_argument.das
- [ ] unused_arguments_annotation.das
- [ ] variant.das
- [ ] vec_constructors.das
- [ ] vec_index.das
- [ ] vec_ops.das
- [ ] vec_swizzle.das
- [ ] with_statement.das

## math/

- [ ] mat_ctors.das
- [ ] mat_let_handle.das
- [ ] math_misc.das

## module_tests/

- [ ] test_modules.das

## template/

- [ ] test_template.das

---

## Summary

- **171** files to review (164 language + 3 math + 1 json + 1 module_tests + 1 assert_once + 1 template)
- **8** files not yet in `tests/README.md`
