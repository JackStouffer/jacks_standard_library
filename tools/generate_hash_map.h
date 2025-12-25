/**
 * TODO: docs
 * 
 * ## License
 *
 * YOUR LICENSE HERE 
 */


#ifndef GENERATE_HASH_MAP_H_INCLUDED
    #define GENERATE_HASH_MAP_H_INCLUDED

    #include <stdint.h>
    #include <stddef.h>
    #include <inttypes.h>
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
        #include <stdbool.h>
    #endif

    #include "../src/jsl_core.h"

    /* Versioning to catch mismatches across deps */
    #ifndef GENERATE_HASH_MAP_VERSION
        #define GENERATE_HASH_MAP_VERSION 0x010000  /* 1.0.0 */
    #else
        #if GENERATE_HASH_MAP_VERSION != 0x010000
            #error "generate_hash_map.h version mismatch across includes"
        #endif
    #endif

    #ifndef GENERATE_HASH_MAP_DEF
        #define GENERATE_HASH_MAP_DEF
    #endif

    #ifdef __cplusplus
    extern "C" {
    #endif

    typedef enum {
        IMPL_ERROR,
        IMPL_FIXED,
        IMPL_DYNAMIC
    } HashMapImplementation;

    GENERATE_HASH_MAP_DEF void write_hash_map_header(
        JSLArena* arena,
        JSLStringBuilder* builder,
        HashMapImplementation impl,
        JSLFatPtr hash_map_name,
        JSLFatPtr function_prefix,
        JSLFatPtr key_type_name,
        JSLFatPtr value_type_name,
        JSLFatPtr hash_function_name,
        JSLFatPtr* include_header_array,
        int32_t include_header_count
    );

    GENERATE_HASH_MAP_DEF void write_hash_map_source(
        JSLArena* arena,
        JSLStringBuilder* builder,
        HashMapImplementation impl,
        JSLFatPtr hash_map_name,
        JSLFatPtr function_prefix,
        JSLFatPtr key_type_name,
        JSLFatPtr value_type_name,
        JSLFatPtr hash_function_name,
        JSLFatPtr* include_header_array,
        int32_t include_header_count
    );
    
    #ifdef __cplusplus
    }
    #endif
    
#endif /* GENERATE_HASH_MAP_H_INCLUDED */

#ifdef GENERATE_HASH_MAP_IMPLEMENTATION

    /**
     * TODO: Documentation: talk about
     *  - must use arena with lifetime greater than the hash_map
     *  - flat hash map with open addressing
     *  - uses PRNG hash, so protected against hash flooding
     *  - large init bucket size because rehashing is expensive
     *  - aggressive growth rate with .5 load factor
     *  - pow 2 bucket size
     *  - large memory usage
     *  - doesn't give up when runs out of memory so you can use a separate arena
     *  - generational ids
     *  - Give warning about composite keys and zero initialization, garbage memory in the padding
     */


    //  static bool function_prefix##_expand(JSL_HASHMAP_TYPE_NAME(name)* hash_map)
    // {
    //     JSL_DEBUG_ASSERT(hash_map != NULL);
    //     JSL_DEBUG_ASSERT(hash_map->arena != NULL);
    //     JSL_DEBUG_ASSERT(hash_map->slots_array != NULL);
    //     JSL_DEBUG_ASSERT(hash_map->is_set_flags_array != NULL);

    //     bool success;

    //     JSL_HASHMAP_ITEM_TYPE_NAME(name)* old_slots_array = hash_map->slots_array;
    //     int64_t old_slots_array_length = hash_map->slots_array_length;

    //     uint32_t* old_is_set_flags_array = hash_map->is_set_flags_array;
    //     int64_t old_is_set_flags_array_length = hash_map->is_set_flags_array_length;

    //     int64_t new_slots_array_length = jsl__hashmap_expand_size(old_slots_array_length);
    //     JSL_HASHMAP_ITEM_TYPE_NAME(name)* new_slots_array = (JSL_HASHMAP_ITEM_TYPE_NAME(name)*) jsl_arena_allocate(
    //         hash_map->arena, sizeof(JSL_HASHMAP_ITEM_TYPE_NAME(name)) * new_slots_array_length, false
    //     ).data;

    //     int64_t new_is_set_flags_array_length = new_slots_array_length >> 5L;
    //     uint32_t* new_is_set_flags_array = (uint32_t*) jsl_arena_allocate(
    //         hash_map->arena, sizeof(uint32_t) * new_is_set_flags_array_length, true
    //     ).data;

    //     if (new_slots_array != NULL && new_is_set_flags_array != NULL)
    //     {
    //         hash_map->item_count = 0;
    //         hash_map->slots_array = new_slots_array;
    //         hash_map->slots_array_length = new_slots_array_length;
    //         hash_map->is_set_flags_array = new_is_set_flags_array;
    //         hash_map->is_set_flags_array_length = new_is_set_flags_array_length;

    //         int64_t slot_index = 0;
    //         for (
    //             int64_t is_set_flags_index = 0;
    //             is_set_flags_index < old_is_set_flags_array_length;
    //             is_set_flags_index++
    //         )
    //         {
    //             for (uint32_t current_bit = 0; current_bit < 32; current_bit++)
    //             {
    //                 uint32_t bitflag = JSL_MAKE_BITFLAG(current_bit);
    //                 if (JSL_IS_BITFLAG_SET(old_is_set_flags_array[is_set_flags_index], bitflag))
    //                 {
    //                     function_prefix##_insert(hash_map, old_slots_array[slot_index].key, old_slots_array[slot_index].value);
    //                 }
    //                 ++slot_index;
    //             }
    //         }

    //         success = true;
    //     }
    //     else
    //     {
    //         success = false;
    //     }

    //     return success;
    // }

    static JSLFatPtr hash_map_name_key = JSL_FATPTR_INITIALIZER("hash_map_name");
    static JSLFatPtr key_type_name_key = JSL_FATPTR_INITIALIZER("key_type_name");
    static JSLFatPtr value_type_name_key = JSL_FATPTR_INITIALIZER("value_type_name");
    static JSLFatPtr function_prefix_key = JSL_FATPTR_INITIALIZER("function_prefix");
    static JSLFatPtr hash_function_key = JSL_FATPTR_INITIALIZER("hash_function");

    static JSLFatPtr int32_t_str = JSL_FATPTR_INITIALIZER("int32_t");
    static JSLFatPtr int_str = JSL_FATPTR_INITIALIZER("int");
    static JSLFatPtr unsigned_str = JSL_FATPTR_INITIALIZER("unsigned");
    static JSLFatPtr unsigned_int_str = JSL_FATPTR_INITIALIZER("unsigned int");
    static JSLFatPtr uint32_t_str = JSL_FATPTR_INITIALIZER("uint32_t");
    static JSLFatPtr int64_t_str = JSL_FATPTR_INITIALIZER("int64_t");
    static JSLFatPtr long_str = JSL_FATPTR_INITIALIZER("long");
    static JSLFatPtr long_int_str = JSL_FATPTR_INITIALIZER("long int");
    static JSLFatPtr long_long_str = JSL_FATPTR_INITIALIZER("long long");
    static JSLFatPtr long_long_int_str = JSL_FATPTR_INITIALIZER("long long int");
    static JSLFatPtr uint64_t_str = JSL_FATPTR_INITIALIZER("uint64_t");
    static JSLFatPtr unsigned_long_str = JSL_FATPTR_INITIALIZER("unsigned long");
    static JSLFatPtr unsigned_long_long_str = JSL_FATPTR_INITIALIZER("unsigned long long");
    static JSLFatPtr unsigned_long_long_int_str = JSL_FATPTR_INITIALIZER("unsigned long long int");

    static uint64_t rand_u64(void)
    {
        uint64_t value = 0;

        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);

        return value;
    }

    static void render_template(
        JSLStringBuilder* str_builder,
        JSLFatPtr template,
        JSLStrToStrMap* variables
    )
    {
        static JSLFatPtr open_param = JSL_FATPTR_INITIALIZER("{{");
        static JSLFatPtr close_param = JSL_FATPTR_INITIALIZER("}}");
        JSLFatPtr template_reader = template;
        
        while (template_reader.length > 0)
        {
            int64_t index_of_open = jsl_fatptr_substring_search(template_reader, open_param);

            // No more variables, write everything
            if (index_of_open == -1)
            {
                jsl_string_builder_insert_fatptr(str_builder, template_reader);
                break;
            }

            if (index_of_open > 0)
            {
                JSLFatPtr slice = jsl_fatptr_slice(template_reader, 0, index_of_open);
                jsl_string_builder_insert_fatptr(str_builder, slice);
            }

            JSL_FATPTR_ADVANCE(template_reader, index_of_open + open_param.length);

            int64_t index_of_close = jsl_fatptr_substring_search(template_reader, close_param);

            // Improperly closed template param, write everything including the open marker
            if (index_of_close == -1)
            {
                jsl_string_builder_insert_fatptr(str_builder, open_param);
                jsl_string_builder_insert_fatptr(str_builder, template_reader);
                break;
            }

            JSLFatPtr var_name = jsl_fatptr_slice(template_reader, 0, index_of_close);
            jsl_fatptr_strip_whitespace(&var_name);

            JSLFatPtr var_value;
            if (jsl_str_to_str_map_get(variables, var_name, &var_value))
            {
                jsl_string_builder_insert_fatptr(str_builder, var_value);
            }

            JSL_FATPTR_ADVANCE(template_reader, index_of_close + close_param.length);
        }
    }

    /**
     * Generates the header file data for your hash map. This file includes all the typedefs
     * and function signatures for this hash map.
     *
     * The generated header file includes "jacks_hash_map.h", and it's assumed to be in the
     * same directory as where this header file will live.
     *
     * If your type needs a custom hash function, it must have the function signature
     * `uint64_t my_hash_function(void* data, int64_t length, uint64_t seed);`.
     *
     * @param arena Arena allocator used for memory allocation. The arena must have
     *              sufficient space (at least 512KB recommended) to hold the generated
     *              header content.
     * @param hash_map_name The name of the hash map type (e.g., "StringIntHashMap").
     *                      This will be used as the main type name in the generated code.
     * @param function_prefix The prefix for all generated function names (e.g., "string_int_map").
     *                        Functions will be named like: {function_prefix}_init, {function_prefix}_insert, etc.
     * @param key_type_name The C type name for hash map keys (e.g., "int", "MyStruct").
     * @param value_type_name The C type name for hash map values (e.g., "int", "MyData*", "float").
     * @param hash_function_name Name of your custom hash function if you have one, NULL otherwise.
     * @param include_header_count Number of additional header files to include in the generated header.
     * @param include_header_count Number of additional header files to include in the generated header.

    * @return JSLFatPtr containing the generated header file content
    *
    * @warning Ensure the arena has sufficient space (minimum 512KB recommended) to avoid
    *          allocation failures during header generation.
    */
    GENERATE_HASH_MAP_DEF void write_hash_map_header(
        JSLArena* arena,
        JSLStringBuilder* builder,
        HashMapImplementation impl,
        JSLFatPtr hash_map_name,
        JSLFatPtr function_prefix,
        JSLFatPtr key_type_name,
        JSLFatPtr value_type_name,
        JSLFatPtr hash_function_name,
        JSLFatPtr* include_header_array,
        int32_t include_header_count
    )
    {
        (void) impl;
        (void) hash_function_name;

        jsl_string_builder_insert_fatptr(
            builder,
            JSL_FATPTR_EXPRESSION("// DEFAULT INCLUDED HEADERS\n")
        );

        jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("#pragma once\n\n"));
        jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("#include <stdint.h>\n"));
        jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("#include \"jsl_hash_map_common.h\"\n\n"));

        jsl_string_builder_insert_fatptr(
            builder,
            JSL_FATPTR_EXPRESSION("// USER INCLUDED HEADERS\n")
        );

        for (int32_t i = 0; i < include_header_count; ++i)
        {
            jsl_string_builder_format(builder, JSL_FATPTR_EXPRESSION("#include \"%y\"\n"), include_header_array[i]);
        }

        jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("\n"));
        
        jsl_string_builder_format(
            builder,
            JSL_FATPTR_EXPRESSION("#define PRIVATE_SENTINEL_%y %" PRIu64 "U \n"),
            hash_map_name,
            rand_u64()
        );

        jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("\n"));

        JSLStrToStrMap map;
        jsl_str_to_str_map_init(&map, arena, 0x123456789);

        jsl_str_to_str_map_insert(
            &map,
            hash_map_name_key,
            JSL_STRING_LIFETIME_STATIC,
            hash_map_name,
            JSL_STRING_LIFETIME_STATIC
        );
        jsl_str_to_str_map_insert(
            &map,
            key_type_name_key,
            JSL_STRING_LIFETIME_STATIC,
            key_type_name,
            JSL_STRING_LIFETIME_STATIC
        );
        jsl_str_to_str_map_insert(
            &map,
            value_type_name_key,
            JSL_STRING_LIFETIME_STATIC,
            value_type_name,
            JSL_STRING_LIFETIME_STATIC
        );
        jsl_str_to_str_map_insert(
            &map,
            function_prefix_key,
            JSL_STRING_LIFETIME_STATIC,
            function_prefix,
            JSL_STRING_LIFETIME_STATIC
        );

        render_template(builder, header_template, &map);
    }

    GENERATE_HASH_MAP_DEF void write_hash_map_source(
        JSLArena* arena,
        JSLStringBuilder* builder,
        HashMapImplementation impl,
        JSLFatPtr hash_map_name,
        JSLFatPtr function_prefix,
        JSLFatPtr key_type_name,
        JSLFatPtr value_type_name,
        JSLFatPtr hash_function_name,
        JSLFatPtr* include_header_array,
        int32_t include_header_count
    )
    {
        (void) impl;

        jsl_string_builder_insert_fatptr(
            builder,
            JSL_FATPTR_EXPRESSION("// DEFAULT INCLUDED HEADERS\n")
        );

        jsl_string_builder_insert_fatptr(
            builder,
            JSL_FATPTR_EXPRESSION("#include <stddef.h>\n")
        );
        jsl_string_builder_insert_fatptr(
            builder,
            JSL_FATPTR_EXPRESSION("#include <stdint.h>\n")
        );
        jsl_string_builder_insert_fatptr(
            builder,
            JSL_FATPTR_EXPRESSION("#include \"jsl_core.h\"\n")
        );
        jsl_string_builder_insert_fatptr(
            builder,
            JSL_FATPTR_EXPRESSION("#include \"jsl_hash_map_common.h\"\n\n")
        );

        jsl_string_builder_insert_fatptr(
            builder,
            JSL_FATPTR_EXPRESSION("// USER INCLUDED HEADERS\n")
        );

        for (int32_t i = 0; i < include_header_count; ++i)
        {
            jsl_string_builder_format(builder, JSL_FATPTR_EXPRESSION("#include \"%y\"\n"), include_header_array[i]);
        }

        jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("\n"));


        JSLStrToStrMap map;
        jsl_str_to_str_map_init(&map, arena, 0x123456789);

        jsl_str_to_str_map_insert(
            &map,
            hash_map_name_key,
            JSL_STRING_LIFETIME_STATIC,
            hash_map_name,
            JSL_STRING_LIFETIME_STATIC
        );
        jsl_str_to_str_map_insert(
            &map,
            key_type_name_key,
            JSL_STRING_LIFETIME_STATIC,
            key_type_name,
            JSL_STRING_LIFETIME_STATIC
        );
        jsl_str_to_str_map_insert(
            &map,
            value_type_name_key,
            JSL_STRING_LIFETIME_STATIC,
            value_type_name,
            JSL_STRING_LIFETIME_STATIC
        );
        jsl_str_to_str_map_insert(
            &map,
            function_prefix_key,
            JSL_STRING_LIFETIME_STATIC,
            function_prefix,
            JSL_STRING_LIFETIME_STATIC
        );

        // hash and find slot
        {
            uint8_t hash_function_call_buffer[4098];
            JSLArena hash_function_scratch_arena = JSL_ARENA_FROM_STACK(hash_function_call_buffer);

            JSLFatPtr resolved_hash_function_call;
            if (hash_function_name.data != NULL && hash_function_name.length > 0)
            {
                resolved_hash_function_call = jsl_format(
                    &hash_function_scratch_arena,
                    JSL_FATPTR_EXPRESSION("uint64_t hash = %y(&key, sizeof(%y), hash_map->seed)"),
                    hash_function_name,
                    key_type_name
                );
            }
            else if (
                jsl_fatptr_memory_compare(key_type_name, int32_t_str)
                || jsl_fatptr_memory_compare(key_type_name, int_str)
                || jsl_fatptr_memory_compare(key_type_name, unsigned_str)
                || jsl_fatptr_memory_compare(key_type_name, unsigned_int_str)
                || jsl_fatptr_memory_compare(key_type_name, uint32_t_str)
                || jsl_fatptr_memory_compare(key_type_name, int64_t_str)
                || jsl_fatptr_memory_compare(key_type_name, long_str)
                || jsl_fatptr_memory_compare(key_type_name, uint64_t_str)
                || jsl_fatptr_memory_compare(key_type_name, unsigned_long_str)
                || jsl_fatptr_memory_compare(key_type_name, long_int_str)
                || jsl_fatptr_memory_compare(key_type_name, long_long_str)
                || jsl_fatptr_memory_compare(key_type_name, long_long_int_str)
                || jsl_fatptr_memory_compare(key_type_name, unsigned_long_long_str)
                || jsl_fatptr_memory_compare(key_type_name, unsigned_long_long_int_str)
                || key_type_name.data[key_type_name.length - 1] == '*'
            )
            {
                resolved_hash_function_call = jsl_format(
                    &hash_function_scratch_arena,
                    JSL_FATPTR_EXPRESSION("*out_hash = murmur3_fmix_u64((uint64_t) key, hash_map->seed)"),
                    key_type_name
                );
            }
            else
            {
                resolved_hash_function_call = jsl_format(
                    &hash_function_scratch_arena,
                    JSL_FATPTR_EXPRESSION("*out_hash = jsl__rapidhash_withSeed(&key, sizeof(%y), hash_map->seed)"),
                    key_type_name
                );
            }

            jsl_str_to_str_map_insert(
                &map,
                hash_function_key,
                JSL_STRING_LIFETIME_STATIC,
                resolved_hash_function_call,
                JSL_STRING_LIFETIME_STATIC
            );
        }

        render_template(builder, source_template, &map);
    }

#endif /* GENERATE_HASH_MAP_IMPLEMENTATION */
