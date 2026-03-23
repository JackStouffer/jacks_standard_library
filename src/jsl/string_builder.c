#include <stdint.h>
#include <stddef.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif
#include <string.h>

#include "jsl/core.h"
#include "jsl/string_builder.h"

#define JSL___BUILDER_PRIVATE_SENTINEL 16674017140609501479U

static inline bool jsl_string_builder__ensure_capacity(
    JSLStringBuilder* builder,
    int64_t needed_capacity
)
{
    if (JSL__LIKELY(needed_capacity <= builder->capacity))
        return true;

    bool res = false;
    int64_t target_capacity = jsl_next_power_of_two_i64(needed_capacity);

    void* new_mem = NULL;

    if (builder->data != NULL && builder->capacity > 0)
    {
        new_mem = jsl_allocator_interface_realloc(
            builder->allocator,
            builder->data,
            target_capacity,
            JSL_DEFAULT_ALLOCATION_ALIGNMENT
        );
    }
    else
    {
        new_mem = jsl_allocator_interface_alloc(
            builder->allocator,
            target_capacity,
            JSL_DEFAULT_ALLOCATION_ALIGNMENT,
            false
        );
    }

    if (new_mem != NULL)
    {
        builder->data = (uint8_t*) new_mem;
        builder->capacity = target_capacity;
        res = true;
    }

    return res;
}

bool jsl_string_builder_init(
    JSLStringBuilder* builder,
    JSLAllocatorInterface allocator,
    int64_t initial_capacity
)
{
    bool res = builder != NULL && initial_capacity > -1;

    if (res)
    {
        JSL_MEMSET(builder, 0, sizeof(JSLStringBuilder));
        builder->allocator = allocator;
        builder->sentinel = JSL___BUILDER_PRIVATE_SENTINEL;

        int64_t target_capacity = jsl_next_power_of_two_i64(JSL_MAX(32L, initial_capacity));
        res = jsl_string_builder__ensure_capacity(builder, target_capacity);
    }

    return res;
}

JSLImmutableMemory jsl_string_builder_get_string(
    JSLStringBuilder* builder
)
{
    JSLImmutableMemory res = {builder->data, builder->length};
    return res;
}

bool jsl_string_builder_append(
    JSLStringBuilder* builder,
    JSLImmutableMemory str_data
)
{
    bool res = (
        builder != NULL
        && builder->sentinel == JSL___BUILDER_PRIVATE_SENTINEL
        && jsl_string_builder__ensure_capacity(builder, builder->length + str_data.length)
    );

    if (res)
    {
        JSL_MEMCPY(&builder->data[builder->length], str_data.data, (size_t) str_data.length);
        builder->length += str_data.length;
    }

    return res;
}

bool jsl_string_builder_delete(
    JSLStringBuilder* builder,
    int64_t index,
    int64_t count
)
{
    bool res = (
        builder != NULL
        && builder->sentinel == JSL___BUILDER_PRIVATE_SENTINEL
        && index > -1
        && count > 0
        && index + count <= builder->length
    );

    int64_t items_to_move = res ? builder->length - index - count : -1;

    if (items_to_move > 0)
    {
        size_t move_bytes = (size_t) items_to_move;
        JSL_MEMMOVE(
            builder->data + index,
            builder->data + index + count,
            move_bytes
        );
        builder->length -= count;
    }
    else if (items_to_move == 0)
    {
        builder->length -= count;
    }

    return res;
}

void jsl_string_builder_clear(
    JSLStringBuilder* builder
)
{
    if (
        builder != NULL
        && builder->sentinel == JSL___BUILDER_PRIVATE_SENTINEL
    )
    {
        builder->length = 0;
    }
}

static void jsl__builder_sink_write(
    void* user_data, JSLImmutableMemory data
)
{
    JSLStringBuilder* builder = user_data;
    jsl_string_builder_append(builder, data);
}

JSLOutputSink jsl_string_builder_output_sink(
    JSLStringBuilder* builder
)
{
    JSLOutputSink res;
    res.write_fp = jsl__builder_sink_write;
    res.user_data = builder;
    return res;
}

void jsl_string_builder_free(
    JSLStringBuilder* builder
)
{
    if (
        builder != NULL
        && builder->sentinel == JSL___BUILDER_PRIVATE_SENTINEL
    )
    {
        jsl_allocator_interface_free(
            builder->allocator,
            builder->data
        );
        builder->length = 0;
        builder->capacity = 0;
        builder->sentinel = 0;
    }
}

#undef JSL___BUILDER_PRIVATE_SENTINEL
