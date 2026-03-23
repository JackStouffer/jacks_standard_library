#ifndef TEST_STRING_BUILDER_H
#define TEST_STRING_BUILDER_H

void test_jsl_string_builder_init(void);
void test_jsl_string_builder_init_invalid_arguments(void);
void test_jsl_string_builder_append(void);
void test_jsl_string_builder_append_edge_cases(void);
void test_jsl_string_builder_get_string(void);
void test_jsl_string_builder_delete(void);
void test_jsl_string_builder_clear(void);
void test_jsl_string_builder_output_sink(void);
void test_jsl_string_builder_format_via_sink(void);
void test_jsl_string_builder_format_invalid_builder(void);
void test_jsl_string_builder_free_null_and_uninitialized(void);
void test_jsl_string_builder_free_invalid_sentinel_noop(void);
void test_jsl_string_builder_free_empty_builder(void);
void test_jsl_string_builder_free_and_reinit(void);

#endif
