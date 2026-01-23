#include <stdint.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_allocator.h"
#include "jsl_allocator_pool.h"

#define JSL__POOL_PRIVATE_SENTINEL (uint64_t) 659574655145560084UL
#define JSL__ITEM_PRIVATE_SENTINEL (uint64_t) 2471501631370269720UL

#ifdef JSL_DEBUG

    static JSL__FORCE_INLINE void jsl__pool_debug_memset_old_memory(
        void* allocation, int64_t num_bytes
    )
    {
        int32_t* fake_array = (int32_t*) allocation;
        int64_t fake_array_len = num_bytes / (int64_t) sizeof(int32_t);
        for (int64_t i = 0; i < fake_array_len; ++i)
        {
            fake_array[i] = 0xfeefee;
        }

        int64_t trailing_bytes = num_bytes - (fake_array_len * (int64_t) sizeof(int32_t));
        if (trailing_bytes > 0)
        {
            const uint32_t pattern = 0x00feefee;
            const uint8_t* pattern_bytes = (const uint8_t*) &pattern;
            uint8_t* trailing = (uint8_t*) (fake_array + fake_array_len);
            for (int64_t i = 0; i < trailing_bytes; ++i)
            {
                trailing[i] = pattern_bytes[i];
            }
        }
    }

#endif

JSL_DEF void jsl_pool_init(
    JSLPoolAllocator* pool,
    void* memory,
    int64_t length,
    int64_t allocation_size
)
{
    JSLFatPtr mem = {memory, length};
    jsl_pool_init2(
        pool, mem, allocation_size
    );
}

JSL_DEF void jsl_pool_init2(
    JSLPoolAllocator* pool,
    JSLFatPtr memory,
    int64_t allocation_size
)
{
    if (pool == NULL || memory.data == NULL || memory.length < 0 || allocation_size < 1)
        return;

    JSL_MEMSET(pool, 0, sizeof(struct JSL__PoolAllocator));
    pool->memory_start = (uintptr_t) memory.data;
    pool->memory_end = pool->memory_start + (uintptr_t) memory.length;

    
    #if JSL_IS_WEB_ASSEMBLY

        // WASM's memory is in a VM so it doesn't really make sense to do these
        // page and cache line alignment things. Who knows how the memory is
        // actually mapped.
        const int32_t alignment = 8;

    #else

        int32_t alignment = 8;
        if (allocation_size >= JSL_KILOBYTES(2))
            alignment = JSL_KILOBYTES(4);
        else if (allocation_size > 64)
            alignment = 64;

    #endif

    JSLFatPtr memory_cursor = memory;
    uintptr_t memory_end = (uintptr_t) memory.data + (uintptr_t) memory.length;

    const int64_t stopping_point = allocation_size + (int64_t) sizeof(struct JSL__PoolAllocatorHeader);
    while (memory_cursor.length >= stopping_point)
    {
        uintptr_t chunk_pointer = (uintptr_t) memory_cursor.data;

        // Make sure there's enough room the header before aligning
        chunk_pointer += sizeof(struct JSL__PoolAllocatorHeader); 
        chunk_pointer = jsl_align_ptr_upwards_uintptr(chunk_pointer, alignment);

        struct JSL__PoolAllocatorHeader* chunk_header_ptr = (void*) (
            chunk_pointer
            - sizeof(struct JSL__PoolAllocatorHeader)
        );

        chunk_header_ptr->sentinel = JSL__ITEM_PRIVATE_SENTINEL;
        chunk_header_ptr->allocation = (void*) chunk_pointer;
        chunk_header_ptr->prev_next = NULL;
        chunk_header_ptr->next = pool->free_list;
        pool->free_list = (void*) chunk_header_ptr;

        uintptr_t new_memory_cursor = (uintptr_t) chunk_pointer + (uintptr_t) allocation_size;

        if (new_memory_cursor <= memory_end)
        {
            memory_cursor.length -= (uint8_t*) new_memory_cursor - memory_cursor.data;
            memory_cursor.data = (uint8_t*) new_memory_cursor;
            ++pool->chunk_count;
        }
        else
        {
            break;
        }
    }

    pool->allocation_size = allocation_size;
    pool->sentinel = JSL__POOL_PRIVATE_SENTINEL;
}

JSL_DEF void* jsl_pool_allocate(JSLPoolAllocator* pool, bool zeroed)
{
    if (pool == NULL || pool->sentinel != JSL__POOL_PRIVATE_SENTINEL)
        return NULL;

    struct JSL__PoolAllocatorHeader* current = pool->free_list;

    if (current != NULL)
    {
        struct JSL__PoolAllocatorHeader* free_list_next = current->next;
       
        current->next = pool->checked_out;
        current->prev_next = &pool->checked_out;
        if (current->next != NULL)
            current->next->prev_next = &current->next;

        pool->checked_out = current;
        pool->free_list = free_list_next;

        if (zeroed)
            JSL_MEMSET(current->allocation, 0, (size_t) pool->allocation_size);

        return current->allocation;
    }

    return NULL;
}

bool jsl_pool_free(JSLPoolAllocator* pool, void* allocation)
{
    if (pool == NULL || pool->sentinel != JSL__POOL_PRIVATE_SENTINEL || allocation == NULL)
        return false;

    uintptr_t allocation_addr = (uintptr_t) allocation;

    const bool memory_in_bounds = (
        allocation_addr >= pool->memory_start
        && allocation_addr < pool->memory_end
    );
    if (!memory_in_bounds)
        return false;

    uintptr_t header_addr = allocation_addr - sizeof(struct JSL__PoolAllocatorHeader);
    const bool good_alignment = (header_addr % (uintptr_t) _Alignof(struct JSL__PoolAllocatorHeader)) == 0;
    if (!good_alignment)
        return false;

    struct JSL__PoolAllocatorHeader* header = (void*) header_addr;

    if (header->sentinel != JSL__ITEM_PRIVATE_SENTINEL || header->allocation != allocation || header->prev_next == NULL)
        return false;

    *(header->prev_next) = header->next;
    if (header->next != NULL)
        header->next->prev_next = header->prev_next;

    header->prev_next = NULL;
    header->next = pool->free_list;
    pool->free_list = header;
    
    #ifdef JSL_DEBUG
        jsl__pool_debug_memset_old_memory(header->allocation, pool->allocation_size);
    #endif

    return true;

}


JSL_DEF void jsl_pool_free_all(JSLPoolAllocator* pool)
{
    if (pool == NULL || pool->sentinel != JSL__POOL_PRIVATE_SENTINEL)
        return;

    struct JSL__PoolAllocatorHeader* current = pool->checked_out;

    while (current != NULL)
    {
        struct JSL__PoolAllocatorHeader* next = current->next;
        current->prev_next = NULL;
        current->next = pool->free_list;
        pool->free_list = current;

        #ifdef JSL_DEBUG
            jsl__pool_debug_memset_old_memory(current->allocation, pool->allocation_size);
        #endif

        current = next;
    }

    pool->checked_out = NULL;
}

int64_t jsl_pool_free_allocation_count(
    JSLPoolAllocator* pool
)
{
    if (pool == NULL || pool->sentinel != JSL__POOL_PRIVATE_SENTINEL)
        return -1;

    int64_t res = 0;
    struct JSL__PoolAllocatorHeader* current = pool->free_list;

    while (current != NULL)
    {
        ++res;
        current = current->next;
    }
    
    return res;
}

int64_t jsl_pool_total_allocation_count(
    JSLPoolAllocator* pool
)
{
    if (pool == NULL || pool->sentinel != JSL__POOL_PRIVATE_SENTINEL)
        return -1;

    int64_t res = 0;
    struct JSL__PoolAllocatorHeader* current = pool->free_list;

    while (current != NULL)
    {
        ++res;
        current = current->next;
    }

    current = pool->checked_out;

    while (current != NULL)
    {
        ++res;
        current = current->next;
    }
    
    return res;
}

#undef JSL__POOL_PRIVATE_SENTINEL
#undef JSL__ITEM_PRIVATE_SENTINEL
