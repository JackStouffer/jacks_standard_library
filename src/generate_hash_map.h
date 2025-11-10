#pragma once

#include <stdint.h>

#include "jacks_standard_library.h"

typedef enum {
    IMPL_ERROR,
    IMPL_STATIC,
    IMPL_DYNAMIC
} HashMapImplementation;

void write_hash_map_header(
    HashMapImplementation impl,
    JSLStringBuilder* builder,
    JSLFatPtr hash_map_name,
    JSLFatPtr function_prefix,
    JSLFatPtr key_type_name,
    JSLFatPtr value_type_name,
    JSLFatPtr hash_function_name,
    JSLFatPtr* include_header_array,
    int32_t include_header_count
);

void write_hash_map_source(
    HashMapImplementation impl,
    JSLStringBuilder* builder,
    JSLFatPtr hash_map_name,
    JSLFatPtr function_prefix,
    JSLFatPtr key_type_name,
    JSLFatPtr value_type_name,
    JSLFatPtr hash_function_name,
    JSLFatPtr* include_header_array,
    int32_t include_header_count
);
