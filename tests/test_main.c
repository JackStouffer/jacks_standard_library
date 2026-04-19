/**
 * Copyright (c) 2026 Jack Stouffer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define _CRT_SECURE_NO_WARNINGS

// jsl os.c requires this
#if !defined(_WIN32) && !defined(__wasm__)
    #define _GNU_SOURCE
    #define _XOPEN_SOURCE 700
#endif

#include <assert.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "jsl/core.h"
#include "jsl/allocator.h"
#include "jsl/allocator_arena.h"
#include "jsl/allocator_infinite_arena.h"

#include "minctest.h"
#include "test_core.h"
#include "test_allocator_arena.h"
#include "test_allocator_libc.h"
#include "test_allocator_pool.h"
#include "test_array.h"
#include "test_cmd_line.h"
#include "test_file_utils.h"
#include "test_format.h"
#include "test_hash_map.h"
#include "test_hash_set.h"
#include "test_intrinsics.h"
#include "test_str_to_str_multimap.h"
#include "test_string_builder.h"

size_t ltests = 0;
size_t lfails = 0;

JSLInfiniteArena global_arena;

int main(void)
{
    // Windows programs that crash can lose all of the terminal output.
    // Set the buffer to zero to auto flush on output.
    #if JSL_IS_WINDOWS
        setvbuf(stdout, NULL, _IONBF, 0);
    #endif

    srand((unsigned) time(NULL));

    jsl_infinite_arena_init(&global_arena);

    //
    //              Test Core
    //

    RUN_TEST_FUNCTION("Test jsl_cstr_to_memory", test_jsl_from_cstr);
    RUN_TEST_FUNCTION("Test jsl_cstr_memory_copy", test_jsl_cstr_memory_copy);
    RUN_TEST_FUNCTION("Test jsl_memory_compare", test_jsl_memory_compare);
    RUN_TEST_FUNCTION("Test jsl_slice", test_jsl_slice);
    RUN_TEST_FUNCTION("Test jsl_total_write_length", test_jsl_total_write_length);
    RUN_TEST_FUNCTION("Test jsl_auto_slice", test_jsl_auto_slice);
    RUN_TEST_FUNCTION("Test jsl_auto_slice_arena_reallocate", test_jsl_auto_slice_arena_reallocate);
    RUN_TEST_FUNCTION("Test jsl_strip_whitespace_left", test_jsl_strip_whitespace_left);
    RUN_TEST_FUNCTION("Test jsl_strip_whitespace_right", test_jsl_strip_whitespace_right);
    RUN_TEST_FUNCTION("Test jsl_strip_whitespace", test_jsl_strip_whitespace);
    RUN_TEST_FUNCTION("Test jsl_index_of", test_jsl_index_of);
    RUN_TEST_FUNCTION("Test jsl_index_of_reverse", test_jsl_index_of_reverse);
    RUN_TEST_FUNCTION("Test jsl_to_lowercase_ascii", test_jsl_to_lowercase_ascii);
    RUN_TEST_FUNCTION("Test jsl_memory_to_i32", test_jsl_memory_to_i32);
    RUN_TEST_FUNCTION("Test jsl_memory_to_u32", test_jsl_memory_to_u32);
    RUN_TEST_FUNCTION("Test jsl_memory_to_u16", test_jsl_memory_to_u16);
    RUN_TEST_FUNCTION("Test jsl_substring_search", test_jsl_substring_search);
    RUN_TEST_FUNCTION("Test jsl_starts_with", test_jsl_starts_with);
    RUN_TEST_FUNCTION("Test jsl_ends_with", test_jsl_ends_with);
    RUN_TEST_FUNCTION("Test jsl_compare_ascii_insensitive", test_jsl_compare_ascii_insensitive);
    RUN_TEST_FUNCTION("Test jsl_count", test_jsl_count);
    RUN_TEST_FUNCTION("Test jsl_memory_to_cstr", test_jsl_to_cstr);
    RUN_TEST_FUNCTION("Test jsl_get_file_extension", test_jsl_get_file_extension);

    // 
    //              Test Allocator Arena
    // 

    RUN_TEST_FUNCTION("Test arena init sets pointers", test_arena_init_sets_pointers);
    RUN_TEST_FUNCTION("Test arena init2 sets pointers", test_arena_init2_sets_pointers);
    RUN_TEST_FUNCTION("Test arena allocate zeroed and alignment", test_arena_allocate_zeroed_and_alignment);
    RUN_TEST_FUNCTION("Test arena allocate invalid sizes", test_arena_allocate_invalid_sizes_return_null);
    RUN_TEST_FUNCTION("Test arena allocate out of memory", test_arena_allocate_out_of_memory_returns_null);
    RUN_TEST_FUNCTION("Test arena realloc null behaves like alloc", test_arena_reallocate_null_behaves_like_allocate);
    RUN_TEST_FUNCTION("Test arena realloc in place", test_arena_reallocate_in_place_when_last);
    RUN_TEST_FUNCTION("Test arena realloc not last", test_arena_reallocate_not_last_allocates_new);
    RUN_TEST_FUNCTION("Test arena realloc invalid pointer", test_arena_reallocate_invalid_pointer_returns_null);
    RUN_TEST_FUNCTION("Test arena reset reuses memory", test_arena_reset_reuses_memory);
    RUN_TEST_FUNCTION("Test arena save/restore point", test_arena_save_restore_point_rewinds);
    RUN_TEST_FUNCTION("Test arena allocator interface", test_arena_allocator_interface_basic);
    RUN_TEST_FUNCTION("Test arena create child basic", test_arena_create_child_basic);
    RUN_TEST_FUNCTION("Test arena create child parent survives realloc", test_arena_create_child_parent_survives_realloc);
    RUN_TEST_FUNCTION("Test arena create child nested", test_arena_create_child_nested);
    RUN_TEST_FUNCTION("Test arena typed macros", test_arena_typed_macros);
    RUN_TEST_FUNCTION("Test arena from stack macro", test_arena_from_stack_macro);

    // 
    //              Test Allocator Infinite Arena
    // 

    RUN_TEST_FUNCTION("Test infinite arena init sets pointers", test_infinite_arena_init);
    RUN_TEST_FUNCTION("Test infinite arena allocate zeroed and alignment", test_infinite_arena_allocate_zeroed_and_alignment);
    RUN_TEST_FUNCTION("Test infinite arena allocate invalid sizes", test_infinite_arena_allocate_invalid_sizes_return_null);
    RUN_TEST_FUNCTION("Test infinite arena allocate distinct blocks", test_infinite_arena_allocate_multiple_are_distinct);
    RUN_TEST_FUNCTION("Test infinite arena realloc null behaves like alloc", test_infinite_arena_reallocate_null_behaves_like_allocate);
    RUN_TEST_FUNCTION("Test infinite arena realloc aligned null behaves like alloc", test_infinite_arena_reallocate_aligned_null_behaves_like_allocate);
    RUN_TEST_FUNCTION("Test infinite arena realloc in place", test_infinite_arena_reallocate_in_place_when_last);
    RUN_TEST_FUNCTION("Test infinite arena realloc not last", test_infinite_arena_reallocate_not_last_allocates_new);
    RUN_TEST_FUNCTION("Test infinite arena realloc aligned in place", test_infinite_arena_reallocate_aligned_in_place_when_last_and_fits);
    RUN_TEST_FUNCTION("Test infinite arena realloc aligned not last", test_infinite_arena_reallocate_aligned_not_last_allocates_new);
    RUN_TEST_FUNCTION("Test infinite arena realloc aligned mismatch", test_infinite_arena_reallocate_aligned_alignment_mismatch_allocates_new);
    RUN_TEST_FUNCTION("Test infinite arena reset reuses memory", test_infinite_arena_reset_reuses_memory);
    RUN_TEST_FUNCTION("Test infinite arena save/restore point", test_infinite_arena_save_restore_point);
    RUN_TEST_FUNCTION("Test infinite arena allocator interface", test_infinite_arena_allocator_interface_basic);
    RUN_TEST_FUNCTION("Test infinite arena create child basic", test_infinite_arena_create_child_basic);
    RUN_TEST_FUNCTION("Test infinite arena create child parent survives realloc", test_infinite_arena_create_child_parent_survives_realloc);
    RUN_TEST_FUNCTION("Test infinite arena create child nested", test_infinite_arena_create_child_nested);

    // 
    //              Test Allocator Libc
    // 

    RUN_TEST_FUNCTION("Test libc allocator init", test_libc_init);
    RUN_TEST_FUNCTION("Test libc allocator init null", test_libc_init_null);
    RUN_TEST_FUNCTION("Test libc allocate basic", test_libc_allocate_basic);
    RUN_TEST_FUNCTION("Test libc allocate zeroed", test_libc_allocate_zeroed);
    RUN_TEST_FUNCTION("Test libc allocate alignment", test_libc_allocate_alignment);
    RUN_TEST_FUNCTION("Test libc allocate invalid sizes", test_libc_allocate_invalid_sizes_return_null);
    RUN_TEST_FUNCTION("Test libc allocate distinct blocks", test_libc_allocate_multiple_are_distinct);
    RUN_TEST_FUNCTION("Test libc free single", test_libc_free_single);
    RUN_TEST_FUNCTION("Test libc free null returns false", test_libc_free_null_returns_false);
    RUN_TEST_FUNCTION("Test libc free all", test_libc_free_all);
    RUN_TEST_FUNCTION("Test libc free all null returns false", test_libc_free_all_null_returns_false);
    RUN_TEST_FUNCTION("Test libc free all uninitialized returns false", test_libc_free_all_uninitialized_returns_false);
    RUN_TEST_FUNCTION("Test libc free middle of list", test_libc_free_middle_of_list);
    RUN_TEST_FUNCTION("Test libc free all then reuse", test_libc_free_all_then_reuse);
    RUN_TEST_FUNCTION("Test libc realloc null behaves like alloc", test_libc_reallocate_null_behaves_like_allocate);
    RUN_TEST_FUNCTION("Test libc realloc preserves data", test_libc_reallocate_preserves_data);
    RUN_TEST_FUNCTION("Test libc realloc shrink preserves data", test_libc_reallocate_shrink_preserves_data);
    RUN_TEST_FUNCTION("Test libc realloc aligned preserves alignment", test_libc_reallocate_aligned_preserves_alignment);
    RUN_TEST_FUNCTION("Test libc realloc invalid size", test_libc_reallocate_invalid_size_returns_null);
    RUN_TEST_FUNCTION("Test libc typed allocate macro", test_libc_typed_allocate_macro);
    RUN_TEST_FUNCTION("Test libc allocator interface basic", test_libc_allocator_interface_basic);
    RUN_TEST_FUNCTION("Test libc allocator interface realloc", test_libc_allocator_interface_realloc);
    RUN_TEST_FUNCTION("Test libc create child basic", test_libc_create_child_basic);
    RUN_TEST_FUNCTION("Test libc create child parent survives realloc", test_libc_create_child_parent_survives_realloc);
    RUN_TEST_FUNCTION("Test libc create child nested", test_libc_create_child_nested);

    //
    //              Test Allocator Pool
    //

    RUN_TEST_FUNCTION("Test pool init sets counts and lists", test_pool_init_sets_counts_and_lists);
    RUN_TEST_FUNCTION("Test pool init2 sets counts", test_pool_init2_sets_counts);
    RUN_TEST_FUNCTION("Test pool init too small has no allocations", test_pool_init_too_small_has_no_allocations);
    RUN_TEST_FUNCTION("Test pool allocate zeroed and alignment small", test_pool_allocate_zeroed_and_alignment_small);
    RUN_TEST_FUNCTION("Test pool allocate exhaustion updates counts", test_pool_allocate_exhaustion_updates_counts);
    RUN_TEST_FUNCTION("Test pool free invalid and double free", test_pool_free_invalid_and_double_free);
    RUN_TEST_FUNCTION("Test pool alignment medium alloc", test_pool_alignment_medium_alloc);
    RUN_TEST_FUNCTION("Test pool alignment large alloc", test_pool_alignment_large_alloc);
    RUN_TEST_FUNCTION("Test pool free middle node", test_pool_free_middle_node);
    RUN_TEST_FUNCTION("Test pool free interior pointer", test_pool_free_interior_pointer);
    RUN_TEST_FUNCTION("Test pool free wrong pool", test_pool_free_wrong_pool);
    RUN_TEST_FUNCTION("Test pool free after free all", test_pool_free_after_free_all);
    RUN_TEST_FUNCTION("Test pool free sentinel corruption", test_pool_free_sentinel_corruption);

    //
    //              Test Intrinsics
    //

    RUN_TEST_FUNCTION("Test count trailing zeros u32", test_jsl__count_trailing_zeros_u32);
    RUN_TEST_FUNCTION("Test count trailing zeros u64", test_jsl__count_trailing_zeros_u64);
    RUN_TEST_FUNCTION("Test count leading zeros u32", test_jsl__count_leading_zeros_u32);
    RUN_TEST_FUNCTION("Test count leading zeros u64", test_jsl__count_leading_zeros_u64);
    RUN_TEST_FUNCTION("Test find first set u32", test_jsl__find_first_set_u32);
    RUN_TEST_FUNCTION("Test find first set u64", test_jsl__find_first_set_u64);
    RUN_TEST_FUNCTION("Test population count u32", test_jsl__population_count_u32);
    RUN_TEST_FUNCTION("Test population count u64", test_jsl__population_count_u64);
    RUN_TEST_FUNCTION("Test next power of two u32", test_jsl_next_power_of_two_u32);
    RUN_TEST_FUNCTION("Test next power of two u64", test_jsl_next_power_of_two_u64);
    RUN_TEST_FUNCTION("Test previous power of two u32", test_jsl_previous_power_of_two_u32);
    RUN_TEST_FUNCTION("Test previous power of two u64", test_jsl_previous_power_of_two_u64);

    // 
    //              Test Format
    // 

    RUN_TEST_FUNCTION("Test format ints", test_integers);
    RUN_TEST_FUNCTION("Test format floating point", test_floating_point);
    RUN_TEST_FUNCTION("Test format length capture", test_n);
    RUN_TEST_FUNCTION("Test format hex floats", test_hex_floats);
    RUN_TEST_FUNCTION("Test format pointer", test_pointer);
    RUN_TEST_FUNCTION("Test format fat pointer", test_memory_format);
    RUN_TEST_FUNCTION("Test format quote modifier", test_quote_modifier);
    RUN_TEST_FUNCTION("Test format non-standard", test_nonstandard);
    RUN_TEST_FUNCTION("Test format separators", test_separators);

    // 
    //              Test File Utils
    // 

    RUN_TEST_FUNCTION("Test jsl_load_file_contents", test_jsl_load_file_contents);
    RUN_TEST_FUNCTION("Test jsl_get_file_size", test_jsl_get_file_size);
    RUN_TEST_FUNCTION("Test jsl_load_file_contents_buffer", test_jsl_load_file_contents_buffer);

    RUN_TEST_FUNCTION("Test jsl_format_to_c_file formats and writes output", test_jsl_format_file_formats_and_writes_output);
    RUN_TEST_FUNCTION("Test jsl_format_to_c_file accepts empty format", test_jsl_format_file_accepts_empty_format);
    RUN_TEST_FUNCTION("Test jsl_format_to_c_file null out parameter", test_jsl_format_file_null_out_parameter);
    RUN_TEST_FUNCTION("Test jsl_format_to_c_file null format pointer", test_jsl_format_file_null_format_pointer);
    RUN_TEST_FUNCTION("Test jsl_format_to_c_file negative length", test_jsl_format_file_negative_length);
    RUN_TEST_FUNCTION("Test jsl_format_to_c_file write failure", test_jsl_format_file_write_failure);

    RUN_TEST_FUNCTION("Test jsl_make_directory bad parameters", test_jsl_make_directory_bad_parameters);
    RUN_TEST_FUNCTION("Test jsl_make_directory creates directory", test_jsl_make_directory_creates_directory);
    RUN_TEST_FUNCTION("Test jsl_make_directory already exists", test_jsl_make_directory_already_exists);
    RUN_TEST_FUNCTION("Test jsl_make_directory parent not found", test_jsl_make_directory_parent_not_found);
    RUN_TEST_FUNCTION("Test jsl_make_directory path too long", test_jsl_make_directory_path_too_long);

    RUN_TEST_FUNCTION("Test jsl_get_file_type regular file", test_jsl_get_file_type_regular_file);
    RUN_TEST_FUNCTION("Test jsl_get_file_type directory", test_jsl_get_file_type_directory);
    RUN_TEST_FUNCTION("Test jsl_get_file_type bad parameters", test_jsl_get_file_type_bad_parameters);
    RUN_TEST_FUNCTION("Test jsl_get_file_type nonexistent", test_jsl_get_file_type_nonexistent);
    RUN_TEST_FUNCTION("Test jsl_get_file_type symlink", test_jsl_get_file_type_symlink);

    RUN_TEST_FUNCTION("Test jsl_delete_file bad parameters", test_jsl_delete_file_bad_parameters);
    RUN_TEST_FUNCTION("Test jsl_delete_file success", test_jsl_delete_file_success);
    RUN_TEST_FUNCTION("Test jsl_delete_file not found", test_jsl_delete_file_not_found);
    RUN_TEST_FUNCTION("Test jsl_delete_file is directory", test_jsl_delete_file_is_directory);
    RUN_TEST_FUNCTION("Test jsl_delete_file symlink", test_jsl_delete_file_symlink);
    RUN_TEST_FUNCTION("Test jsl_delete_file path too long", test_jsl_delete_file_path_too_long);

    RUN_TEST_FUNCTION("Test jsl_delete_directory bad parameters", test_jsl_delete_directory_bad_parameters);
    RUN_TEST_FUNCTION("Test jsl_delete_directory not found", test_jsl_delete_directory_not_found);
    RUN_TEST_FUNCTION("Test jsl_delete_directory not a directory", test_jsl_delete_directory_not_a_directory);
    RUN_TEST_FUNCTION("Test jsl_delete_directory empty", test_jsl_delete_directory_empty);
    RUN_TEST_FUNCTION("Test jsl_delete_directory with files", test_jsl_delete_directory_with_files);
    RUN_TEST_FUNCTION("Test jsl_delete_directory nested", test_jsl_delete_directory_nested);
    RUN_TEST_FUNCTION("Test jsl_delete_directory path too long", test_jsl_delete_directory_path_too_long);

    RUN_TEST_FUNCTION("Test jsl_directory_iterator bad parameters", test_jsl_directory_iterator_bad_parameters);
    RUN_TEST_FUNCTION("Test jsl_directory_iterator not found", test_jsl_directory_iterator_not_found);
    RUN_TEST_FUNCTION("Test jsl_directory_iterator not a directory", test_jsl_directory_iterator_not_a_directory);
    RUN_TEST_FUNCTION("Test jsl_directory_iterator empty", test_jsl_directory_iterator_empty);
    RUN_TEST_FUNCTION("Test jsl_directory_iterator flat", test_jsl_directory_iterator_flat);
    RUN_TEST_FUNCTION("Test jsl_directory_iterator nested", test_jsl_directory_iterator_nested);
    RUN_TEST_FUNCTION("Test jsl_directory_iterator relative paths", test_jsl_directory_iterator_relative_paths);
    RUN_TEST_FUNCTION("Test jsl_directory_iterator close early", test_jsl_directory_iterator_close_early);
    RUN_TEST_FUNCTION("Test jsl_directory_iterator symlink default", test_jsl_directory_iterator_symlink_default);
    RUN_TEST_FUNCTION("Test jsl_directory_iterator symlink follow", test_jsl_directory_iterator_symlink_follow);
    RUN_TEST_FUNCTION("Test jsl_directory_iterator path too long", test_jsl_directory_iterator_path_too_long);
    RUN_TEST_FUNCTION("Test jsl_directory_iterator skips dot and dotdot", test_jsl_directory_iterator_skips_dot_and_dotdot);

    RUN_TEST_FUNCTION("Test jsl_copy_file bad parameters", test_jsl_copy_file_bad_parameters);
    RUN_TEST_FUNCTION("Test jsl_copy_file path too long", test_jsl_copy_file_path_too_long);
    RUN_TEST_FUNCTION("Test jsl_copy_file source not found", test_jsl_copy_file_source_not_found);
    RUN_TEST_FUNCTION("Test jsl_copy_file source is directory", test_jsl_copy_file_source_is_directory);
    RUN_TEST_FUNCTION("Test jsl_copy_file success", test_jsl_copy_file_success);
    RUN_TEST_FUNCTION("Test jsl_copy_file overwrites existing", test_jsl_copy_file_overwrites_existing);
    RUN_TEST_FUNCTION("Test jsl_copy_file dest parent not found", test_jsl_copy_file_dest_parent_not_found);

    RUN_TEST_FUNCTION("Test jsl_copy_directory bad parameters", test_jsl_copy_directory_bad_parameters);
    RUN_TEST_FUNCTION("Test jsl_copy_directory path too long", test_jsl_copy_directory_path_too_long);
    RUN_TEST_FUNCTION("Test jsl_copy_directory source not found", test_jsl_copy_directory_source_not_found);
    RUN_TEST_FUNCTION("Test jsl_copy_directory source not a directory", test_jsl_copy_directory_source_not_a_directory);
    RUN_TEST_FUNCTION("Test jsl_copy_directory dest already exists", test_jsl_copy_directory_dest_already_exists);
    RUN_TEST_FUNCTION("Test jsl_copy_directory empty", test_jsl_copy_directory_empty);
    RUN_TEST_FUNCTION("Test jsl_copy_directory with files", test_jsl_copy_directory_with_files);
    RUN_TEST_FUNCTION("Test jsl_copy_directory nested", test_jsl_copy_directory_nested);
    RUN_TEST_FUNCTION("Test jsl_rename_file bad parameters", test_jsl_rename_file_bad_parameters);
    RUN_TEST_FUNCTION("Test jsl_rename_file path too long", test_jsl_rename_file_path_too_long);
    RUN_TEST_FUNCTION("Test jsl_rename_file source not found", test_jsl_rename_file_source_not_found);
    RUN_TEST_FUNCTION("Test jsl_rename_file renames file", test_jsl_rename_file_renames_file);
    RUN_TEST_FUNCTION("Test jsl_rename_file renames directory", test_jsl_rename_file_renames_directory);
    RUN_TEST_FUNCTION("Test jsl_rename_file overwrites existing file", test_jsl_rename_file_overwrites_existing_file);
    RUN_TEST_FUNCTION("Test jsl_rename_file renames symlink", test_jsl_rename_file_renames_symlink);

    //
    //              Test Array
    //

    RUN_TEST_FUNCTION("Test dynamic array init success", test_dynamic_array_init_success);
    RUN_TEST_FUNCTION("Test dynamic array init invalid args", test_dynamic_array_init_invalid_args);
    RUN_TEST_FUNCTION("Test dynamic array insert", test_dynamic_array_insert_appends_and_grows);
    RUN_TEST_FUNCTION("Test dynamic array insert at", test_dynamic_array_insert_at_inserts_and_shifts);
    RUN_TEST_FUNCTION("Test dynamic array delete at", test_dynamic_array_delete_at_removes_and_shifts);
    RUN_TEST_FUNCTION("Test dynamic array clear", test_dynamic_array_clear_resets_length);
    RUN_TEST_FUNCTION("Test dynamic array sentinel checks", test_dynamic_array_checks_sentinel);
    // 
    //              Test Fixed Hash Map
    // 

    RUN_TEST_FUNCTION("Test fixed hashmap insert", test_fixed_insert);
    RUN_TEST_FUNCTION("Test fixed hashmap get", test_fixed_get);
    RUN_TEST_FUNCTION("Test fixed hashmap iterator", test_fixed_iterator);
    RUN_TEST_FUNCTION("Test fixed hashmap delete", test_fixed_delete);
    RUN_TEST_FUNCTION("Test fixed hashmap struct key padding", test_fixed_struct_key_padding);
    RUN_TEST_FUNCTION("Test fixed int32 to str insert overwrites", test_fixed_int32_to_str_insert_overwrites);
    RUN_TEST_FUNCTION("Test fixed str to int32 insert overwrites", test_fixed_str_to_int32_insert_overwrites);
    RUN_TEST_FUNCTION("Test fixed int32 to str lifetime", test_fixed_int32_to_str_lifetime);
    RUN_TEST_FUNCTION("Test fixed str to int32 lifetime", test_fixed_str_to_int32_lifetime);
    RUN_TEST_FUNCTION("Test fixed int32 to str free", test_fixed_int32_to_str_free);
    RUN_TEST_FUNCTION("Test fixed str to int32 free", test_fixed_str_to_int32_free);
    RUN_TEST_FUNCTION("Test fixed int32 to str overwrite frees old", test_fixed_int32_to_str_overwrite_frees_old);

    // 
    //              Test String to String Hash Map
    // 

    RUN_TEST_FUNCTION("Test str to str map init success", test_jsl_str_to_str_map_init_success);
    RUN_TEST_FUNCTION("Test str to str map init invalid args", test_jsl_str_to_str_map_init_invalid_arguments);
    RUN_TEST_FUNCTION("Test str to str map item count and has key", test_jsl_str_to_str_map_item_count_and_has_key);
    RUN_TEST_FUNCTION("Test str to str map get", test_jsl_str_to_str_map_get);
    RUN_TEST_FUNCTION("Test str to str map insert", test_jsl_str_to_str_map_insert_overwrites_value);
    RUN_TEST_FUNCTION("Test str to str map transient lifetime copies", test_jsl_str_to_str_map_transient_lifetime_copies_data);
    RUN_TEST_FUNCTION("Test str to str map static lifetime keeps pointer", test_jsl_str_to_str_map_fixed_lifetime_uses_original_pointers);
    RUN_TEST_FUNCTION("Test str to str map empty and binary strings", test_jsl_str_to_str_map_handles_empty_and_binary_strings);
    RUN_TEST_FUNCTION("Test str to str map iterator", test_jsl_str_to_str_map_iterator_covers_all_pairs);
    RUN_TEST_FUNCTION("Test str to str map iterator invalid after mutation", test_jsl_str_to_str_map_iterator_invalidated_on_mutation);
    RUN_TEST_FUNCTION("Test str to str map delete", test_jsl_str_to_str_map_delete);
    RUN_TEST_FUNCTION("Test str to str map clear", test_jsl_str_to_str_map_clear);
    RUN_TEST_FUNCTION("Test str to str map rehash", test_jsl_str_to_str_map_rehash);
    RUN_TEST_FUNCTION("Test str to str map invalid inserts", test_jsl_str_to_str_map_invalid_inserts);

    // 
    //              Test String to String Multimap
    // 

    RUN_TEST_FUNCTION("init success", test_jsl_str_to_str_multimap_init_success);
    RUN_TEST_FUNCTION("init invalid args", test_jsl_str_to_str_multimap_init_invalid_arguments);
    RUN_TEST_FUNCTION("insert and value count", test_jsl_str_to_str_multimap_insert_and_get_value_count);
    RUN_TEST_FUNCTION("duplicate values", test_jsl_str_to_str_multimap_duplicate_values_allowed);
    RUN_TEST_FUNCTION("transient lifetime copies data", test_jsl_str_to_str_multimap_transient_lifetime_copies);
    RUN_TEST_FUNCTION("static lifetime uses original pointers", test_jsl_str_to_str_multimap_static_lifetime_no_copy);
    RUN_TEST_FUNCTION("key/value iterator covers all", test_jsl_str_to_str_multimap_key_value_iterator_covers_all_pairs);
    RUN_TEST_FUNCTION("get-key iterator filters", test_jsl_str_to_str_multimap_get_key_iterator_filters_by_key);
    RUN_TEST_FUNCTION("empty strings and binary values", test_jsl_str_to_str_multimap_handles_empty_and_binary_values);
    RUN_TEST_FUNCTION("delete value behavior", test_jsl_str_to_str_multimap_delete_value);
    RUN_TEST_FUNCTION("delete value removes empty key", test_jsl_str_to_str_multimap_delete_value_removes_empty_key);
    RUN_TEST_FUNCTION("delete key behavior", test_jsl_str_to_str_multimap_delete_key);
    RUN_TEST_FUNCTION("clear and reuse", test_jsl_str_to_str_multimap_clear);
    RUN_TEST_FUNCTION("stress test", test_stress_test);

    // 
    //              Test Hash Set
    // 

    RUN_TEST_FUNCTION("String set init success", test_jsl_str_set_init_success);
    RUN_TEST_FUNCTION("String set init invalid args", test_jsl_str_set_init_invalid_arguments);
    RUN_TEST_FUNCTION("String set insert and has", test_jsl_str_set_insert_and_has);
    RUN_TEST_FUNCTION("String Set lifetime rules", test_jsl_str_set_respects_lifetime_rules);
    RUN_TEST_FUNCTION("String Set iterator covers all", test_jsl_str_set_iterator_covers_all_values);
    RUN_TEST_FUNCTION("String Set iterator invalidation", test_jsl_str_set_iterator_invalidated_on_mutation);
    RUN_TEST_FUNCTION("String Set delete behavior", test_jsl_str_set_delete);
    RUN_TEST_FUNCTION("String Set clear behavior", test_jsl_str_set_clear);
    RUN_TEST_FUNCTION("String Set empty and binary values", test_jsl_str_set_handles_empty_and_binary_values);
    RUN_TEST_FUNCTION("String Set intersection basic cases", test_jsl_str_set_intersection_basic);
    RUN_TEST_FUNCTION("String Set intersection empty sets", test_jsl_str_set_intersection_with_empty_sets);
    RUN_TEST_FUNCTION("String Set union collects uniques", test_jsl_str_set_union_collects_all_unique_values);
    RUN_TEST_FUNCTION("String Set union with empty sets", test_jsl_str_set_union_with_empty_sets);
    RUN_TEST_FUNCTION("String Set difference basic cases", test_jsl_str_set_difference_basic);
    RUN_TEST_FUNCTION("String Set difference with empty sets", test_jsl_str_set_difference_with_empty_sets);
    RUN_TEST_FUNCTION("String Set operations invalid parameters", test_jsl_str_set_set_operations_invalid_parameters);
    RUN_TEST_FUNCTION("String Set rehash preserves entries", test_jsl_str_set_rehash_preserves_entries);
    RUN_TEST_FUNCTION("String Set rejects invalid parameters", test_jsl_str_set_rejects_invalid_parameters);

    //
    //              Test String builder
    //

    RUN_TEST_FUNCTION("Test builder init", test_jsl_string_builder_init);
    RUN_TEST_FUNCTION("Test invalid init args", test_jsl_string_builder_init_invalid_arguments);
    RUN_TEST_FUNCTION("Test builder append", test_jsl_string_builder_append);
    RUN_TEST_FUNCTION("Test append edge cases", test_jsl_string_builder_append_edge_cases);
    RUN_TEST_FUNCTION("Test get string", test_jsl_string_builder_get_string);
    RUN_TEST_FUNCTION("Test delete", test_jsl_string_builder_delete);
    RUN_TEST_FUNCTION("Test clear", test_jsl_string_builder_clear);
    RUN_TEST_FUNCTION("Test output sink", test_jsl_string_builder_output_sink);
    RUN_TEST_FUNCTION("Test format via sink", test_jsl_string_builder_format_via_sink);
    RUN_TEST_FUNCTION("Test format invalid builder", test_jsl_string_builder_format_invalid_builder);
    RUN_TEST_FUNCTION("Test free null and uninitialized", test_jsl_string_builder_free_null_and_uninitialized);
    RUN_TEST_FUNCTION("Test free invalid sentinel no-op", test_jsl_string_builder_free_invalid_sentinel_noop);
    RUN_TEST_FUNCTION("Test free empty builder", test_jsl_string_builder_free_empty_builder);
    RUN_TEST_FUNCTION("Test free and reinit", test_jsl_string_builder_free_and_reinit);

    // 
    //              Test Command Line Args
    // 

    RUN_TEST_FUNCTION("Test command line arg short flags grouping", test_short_flags_grouping);
    RUN_TEST_FUNCTION("Test command line arg short flag with equals fails", test_short_flag_equals_is_invalid);
    RUN_TEST_FUNCTION("Test command line arg long flags, commands, and terminator", test_long_flags_and_commands);
    RUN_TEST_FUNCTION("Test command line arg long flag values via equals and space", test_long_values_equals_and_space);
    RUN_TEST_FUNCTION("Test command line arg wide argument parsing", test_wide_parsing);

    // 
    //              Test Command Line Style
    // 

    RUN_TEST_FUNCTION("Test command line color conversions", test_cmd_line_color_conversions);
    RUN_TEST_FUNCTION("Test command line style writes no color", test_cmd_line_write_style_no_color);
    RUN_TEST_FUNCTION("Test command line style write ANSI16", test_cmd_line_write_style_ansi16);
    RUN_TEST_FUNCTION("Test command line style converts to ANSI16", test_cmd_line_write_style_ansi16_converts_color_types);
    RUN_TEST_FUNCTION("Test command line style write ANSI256", test_cmd_line_write_style_ansi256);
    RUN_TEST_FUNCTION("Test command line style write truecolor", test_cmd_line_write_style_truecolor);
    RUN_TEST_FUNCTION("Test command line style/reset invalid", test_cmd_line_write_style_and_reset_invalid);
    RUN_TEST_FUNCTION("Test command line reset ANSI modes", test_cmd_line_write_reset_ansi_modes);


    TEST_RESULTS();
    return lfails != 0;
}
