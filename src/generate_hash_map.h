#pragma once

#include <stdint.h>

#include "jacks_standard_library.h"

void write_hash_map_header(
    JSLStringBuilder* builder,
    JSLFatPtr hash_map_name,
    JSLFatPtr function_prefix,
    JSLFatPtr key_type_name,
    JSLFatPtr value_type_name,
    JSLFatPtr hash_function_name,
    int32_t include_header_count,
    ...
);

void write_hash_map_source(
    JSLStringBuilder* builder,
    JSLFatPtr hash_map_name,
    JSLFatPtr function_prefix,
    JSLFatPtr key_type_name,
    JSLFatPtr value_type_name,
    JSLFatPtr hash_function_name,
    int32_t include_header_count,
    ...
);
