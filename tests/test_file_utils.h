#ifndef TEST_FILE_UTILS_H
#define TEST_FILE_UTILS_H

void test_jsl_load_file_contents(void);
void test_jsl_get_file_size(void);
void test_jsl_load_file_contents_buffer(void);
void test_jsl_format_file_formats_and_writes_output(void);
void test_jsl_format_file_accepts_empty_format(void);
void test_jsl_format_file_null_out_parameter(void);
void test_jsl_format_file_null_format_pointer(void);
void test_jsl_format_file_negative_length(void);
void test_jsl_format_file_write_failure(void);
void test_jsl_make_directory_bad_parameters(void);
void test_jsl_make_directory_creates_directory(void);
void test_jsl_make_directory_already_exists(void);
void test_jsl_make_directory_parent_not_found(void);
void test_jsl_make_directory_path_too_long(void);
void test_jsl_get_file_type_regular_file(void);
void test_jsl_get_file_type_directory(void);
void test_jsl_get_file_type_bad_parameters(void);
void test_jsl_get_file_type_nonexistent(void);
void test_jsl_get_file_type_symlink(void);

#endif
