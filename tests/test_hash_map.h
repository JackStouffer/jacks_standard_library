#ifndef TEST_HASH_MAP_H
#define TEST_HASH_MAP_H

void test_fixed_insert(void);
void test_fixed_get(void);
void test_fixed_delete(void);
void test_fixed_iterator(void);
void test_fixed_struct_key_padding(void);
void test_fixed_int32_to_str_insert_overwrites(void);
void test_fixed_str_to_int32_insert_overwrites(void);
void test_fixed_int32_to_str_lifetime(void);
void test_fixed_str_to_int32_lifetime(void);
void test_fixed_int32_to_str_free(void);
void test_fixed_str_to_int32_free(void);
void test_fixed_int32_to_str_overwrite_frees_old(void);

void test_jsl_str_to_str_map_init_success(void);
void test_jsl_str_to_str_map_init_invalid_arguments(void);
void test_jsl_str_to_str_map_item_count_and_has_key(void);
void test_jsl_str_to_str_map_get(void);
void test_jsl_str_to_str_map_insert_overwrites_value(void);
void test_jsl_str_to_str_map_transient_lifetime_copies_data(void);
void test_jsl_str_to_str_map_fixed_lifetime_uses_original_pointers(void);
void test_jsl_str_to_str_map_handles_empty_and_binary_strings(void);
void test_jsl_str_to_str_map_iterator_covers_all_pairs(void);
void test_jsl_str_to_str_map_iterator_invalidated_on_mutation(void);
void test_jsl_str_to_str_map_delete(void);
void test_jsl_str_to_str_map_clear(void);
void test_jsl_str_to_str_map_rehash(void);
void test_jsl_str_to_str_map_invalid_inserts(void);

#endif
