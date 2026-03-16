#ifndef TEST_ALLOCATOR_POOL_H
#define TEST_ALLOCATOR_POOL_H

void test_pool_init_sets_counts_and_lists(void);
void test_pool_init2_sets_counts(void);
void test_pool_init_too_small_has_no_allocations(void);
void test_pool_allocate_zeroed_and_alignment_small(void);
void test_pool_allocate_exhaustion_updates_counts(void);
void test_pool_free_invalid_and_double_free(void);
void test_pool_alignment_medium_alloc(void);
void test_pool_alignment_large_alloc(void);
void test_pool_free_middle_node(void);
void test_pool_free_interior_pointer(void);
void test_pool_free_wrong_pool(void);
void test_pool_free_after_free_all(void);
void test_pool_free_sentinel_corruption(void);

#endif
