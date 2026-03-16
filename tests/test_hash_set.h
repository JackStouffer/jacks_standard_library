#ifndef TEST_HASH_SET_H
#define TEST_HASH_SET_H

void test_jsl_str_set_init_success(void);
void test_jsl_str_set_init_invalid_arguments(void);
void test_jsl_str_set_insert_and_has(void);
void test_jsl_str_set_respects_lifetime_rules(void);
void test_jsl_str_set_iterator_covers_all_values(void);
void test_jsl_str_set_iterator_invalidated_on_mutation(void);
void test_jsl_str_set_delete(void);
void test_jsl_str_set_clear(void);
void test_jsl_str_set_handles_empty_and_binary_values(void);
void test_jsl_str_set_intersection_basic(void);
void test_jsl_str_set_intersection_with_empty_sets(void);
void test_jsl_str_set_union_collects_all_unique_values(void);
void test_jsl_str_set_union_with_empty_sets(void);
void test_jsl_str_set_difference_basic(void);
void test_jsl_str_set_difference_with_empty_sets(void);
void test_jsl_str_set_set_operations_invalid_parameters(void);
void test_jsl_str_set_rehash_preserves_entries(void);
void test_jsl_str_set_rejects_invalid_parameters(void);

#endif
