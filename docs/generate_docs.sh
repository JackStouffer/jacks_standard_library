#! /bin/bash

~/Documents/code/c_doc_gen/doc_gen \
    --ignore "ASAN*" \
    --ignore "bool" \
    --ignore "int64_t" \
    --ignore "JSL__*" \
    --ignore "jsl__*" \
    ~/Documents/code/jacks_standard_library/src/jsl_core.h > docs/jsl_core.md &

~/Documents/code/c_doc_gen/doc_gen \
    --ignore "ASAN*" \
    --ignore "bool" \
    --ignore "int64_t" \
    --ignore "JSL__*" \
    --ignore "jsl__*" \
    ~/Documents/code/jacks_standard_library/src/jsl_allocator.h > docs/jsl_allocator.md &

~/Documents/code/c_doc_gen/doc_gen \
    --ignore "ASAN*" \
    --ignore "bool" \
    --ignore "int64_t" \
    --ignore "JSL__*" \
    --ignore "jsl__*" \
    ~/Documents/code/jacks_standard_library/src/jsl_allocator_arena.h > docs/jsl_allocator_arena.md &

~/Documents/code/c_doc_gen/doc_gen \
    --ignore "ASAN*" \
    --ignore "bool" \
    --ignore "int64_t" \
    --ignore "JSL__*" \
    --ignore "jsl__*" \
    ~/Documents/code/jacks_standard_library/src/jsl_allocator_infinite_arena.h > docs/jsl_allocator_infinite_arena.md &

~/Documents/code/c_doc_gen/doc_gen \
    --ignore "ASAN*" \
    --ignore "bool" \
    --ignore "int64_t" \
    --ignore "JSL__*" \
    --ignore "jsl__*" \
    ~/Documents/code/jacks_standard_library/src/jsl_allocator_pool.h > docs/jsl_allocator_pool.md &

~/Documents/code/c_doc_gen/doc_gen \
    --ignore "ASAN*" \
    --ignore "bool" \
    --ignore "int64_t" \
    --ignore "JSL__*" \
    --ignore "jsl__*" \
    ~/Documents/code/jacks_standard_library/src/jsl_cmd_line.h > docs/jsl_cmd_line.md &

~/Documents/code/c_doc_gen/doc_gen \
    --ignore "ASAN*" \
    --ignore "bool" \
    --ignore "int64_t" \
    --ignore "JSL__*" \
    --ignore "jsl__*" \
    ~/Documents/code/jacks_standard_library/src/jsl_os.h > docs/jsl_os.md &

~/Documents/code/c_doc_gen/doc_gen \
    --ignore "ASAN*" \
    --ignore "bool" \
    --ignore "int64_t" \
    --ignore "JSL__*" \
    --ignore "jsl__*" \
    ~/Documents/code/jacks_standard_library/src/jsl_str_set.h > docs/jsl_str_set.md &

~/Documents/code/c_doc_gen/doc_gen \
    --ignore "ASAN*" \
    --ignore "bool" \
    --ignore "int64_t" \
    --ignore "JSL__*" \
    --ignore "jsl__*" \
    ~/Documents/code/jacks_standard_library/src/jsl_str_to_str_map.h > docs/jsl_str_to_str_map.md &

~/Documents/code/c_doc_gen/doc_gen \
    --ignore "ASAN*" \
    --ignore "bool" \
    --ignore "int64_t" \
    --ignore "JSL__*" \
    --ignore "jsl__*" \
    ~/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h > docs/jsl_str_to_str_multimap.md &

~/Documents/code/c_doc_gen/doc_gen \
    --ignore "ASAN*" \
    --ignore "bool" \
    --ignore "int64_t" \
    --ignore "JSL__*" \
    --ignore "jsl__*" \
    ~/Documents/code/jacks_standard_library/src/jsl_string_builder.h > docs/jsl_string_builder.md &

wait
