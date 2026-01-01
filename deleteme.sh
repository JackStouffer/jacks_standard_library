./embed.exe --var-name=dynamic_header_template tools/templates/dynamic_hash_map_header.txt > tools/templates/dynamic_hash_map_header.h
./embed.exe --var-name=dynamic_source_template tools/templates/dynamic_hash_map_source.txt > tools/templates/dynamic_hash_map_source.h

clang \
    -DJSL_DEBUG -fno-omit-frame-pointer \
    -fno-optimize-sibling-calls -O0 -glldb \
    -std=c11 -Wall -Wextra -Wconversion -Wsign-conversion \
    -Wshadow -Wconditional-uninitialized -Wcomma -Widiomatic-parentheses \
    -Wpointer-arith -Wassign-enum -Wswitch-enum -Wimplicit-fallthrough \
    -Wnull-dereference -Wmissing-prototypes -Wundef \
    -pedantic -o tests/bin/generate_hash_map.exe \
    -Isrc/ tools/generate_hash_map.c

./tests/bin/generate_hash_map.exe \
    --name DynamicIntToIntMap \
    --function-prefix dynamic_int32_to_int32_map \
    --key-type int32_t \
    --value-type int32_t \
    --dynamic \
    --header \
    --add-header ../tests/hash_maps/dynamic_int32_to_int32_map.h \
    --add-header ../tests/test_hash_map_types.h > tests/hash_maps/dynamic_int32_to_int32_map.h
./tests/bin/generate_hash_map.exe \
    --name DynamicIntToIntMap \
    --function-prefix dynamic_int32_to_int32_map \
    --key-type int32_t \
    --value-type int32_t \
    --dynamic \
    --source \
    --add-header ../tests/hash_maps/dynamic_int32_to_int32_map.h \
    --add-header ../tests/test_hash_map_types.h > tests/hash_maps/dynamic_int32_to_int32_map.c

./tests/bin/generate_hash_map.exe \
    --name DynamicCompositeType3ToCompositeType2Map \
    --function-prefix dynamic_comp3_to_comp2_map \
    --key-type CompositeType3 \
    --value-type CompositeType2 \
    --dynamic \
    --header \
    --add-header ../tests/hash_maps/dynamic_comp3_to_comp2_map.h \
    --add-header ../tests/test_hash_map_types.h > tests/hash_maps/dynamic_comp3_to_comp2_map.h
./tests/bin/generate_hash_map.exe \
    --name DynamicCompositeType3ToCompositeType2Map \
    --function-prefix dynamic_comp3_to_comp2_map \
    --key-type CompositeType3 \
    --value-type CompositeType2 \
    --dynamic \
    --source \
    --add-header ../tests/hash_maps/dynamic_comp3_to_comp2_map.h \
    --add-header ../tests/test_hash_map_type > tests/hash_maps/dynamic_comp3_to_comp2_map.c
