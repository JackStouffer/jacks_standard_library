#ifndef TEST_ALLOCATOR_LIBC_H
#define TEST_ALLOCATOR_LIBC_H

void test_libc_init(void);
void test_libc_init_null(void);
void test_libc_allocate_basic(void);
void test_libc_allocate_zeroed(void);
void test_libc_allocate_alignment(void);
void test_libc_allocate_invalid_sizes_return_null(void);
void test_libc_allocate_multiple_are_distinct(void);
void test_libc_free_single(void);
void test_libc_free_null_returns_false(void);
void test_libc_free_all(void);
void test_libc_free_all_null_returns_false(void);
void test_libc_free_all_uninitialized_returns_false(void);
void test_libc_free_middle_of_list(void);
void test_libc_free_all_then_reuse(void);
void test_libc_reallocate_null_behaves_like_allocate(void);
void test_libc_reallocate_preserves_data(void);
void test_libc_reallocate_shrink_preserves_data(void);
void test_libc_reallocate_aligned_preserves_alignment(void);
void test_libc_reallocate_invalid_size_returns_null(void);
void test_libc_typed_allocate_macro(void);
void test_libc_allocator_interface_basic(void);
void test_libc_allocator_interface_realloc(void);
void test_libc_create_child_basic(void);
void test_libc_create_child_parent_survives_realloc(void);
void test_libc_create_child_nested(void);

#endif
