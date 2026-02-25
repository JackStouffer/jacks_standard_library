#!/bin/sh
#
# Regenerate the embedded template variables in generate_hash_map.h
# by running bin/embed on the .txt template files.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
EMBED="$ROOT_DIR/bin/embed"
HEADER="$SCRIPT_DIR/generate_hash_map.h"

if [ ! -x "$EMBED" ]; then
    echo "error: $EMBED not found or not executable" >&2
    exit 1
fi

# extract_var_block: run embed and extract just the variable declaration block
# (from "static JSLImmutableMemory ..." through ");"), indented with 4 spaces.
extract_var_block() {
    local var_name="$1"
    local txt_file="$2"

    "$EMBED" --text --var-name="$var_name" "$SCRIPT_DIR/$txt_file" \
        | sed -n '/^static JSLImmutableMemory/,/^);$/p' \
        | awk '/^static / || /^\);$/ { print "    " $0; next } { print "        " $0 }'
}

# replace_var_block: replace the variable declaration in the header file.
# Finds the line starting with "    static JSLImmutableMemory <var_name>"
# through the next "    );" and replaces it with the new content.
replace_var_block() {
    local var_name="$1"
    local txt_file="$2"

    local block_file
    block_file=$(mktemp)
    extract_var_block "$var_name" "$txt_file" > "$block_file"

    local out_file
    out_file=$(mktemp)

    awk -v var="$var_name" -v blockfile="$block_file" '
    BEGIN { replacing = 0 }
    $0 ~ "^    static JSLImmutableMemory " var " = JSL_CSTR_INITIALIZER" {
        replacing = 1
        while ((getline line < blockfile) > 0) print line
        close(blockfile)
        next
    }
    replacing && /^    \);/ {
        replacing = 0
        next
    }
    !replacing { print }
    ' "$HEADER" > "$out_file"

    mv "$out_file" "$HEADER"
    rm -f "$block_file"
}

replace_var_block fixed_header_template  fixed_hash_map_header.txt
replace_var_block fixed_source_template  fixed_hash_map_source.txt
replace_var_block dynamic_header_template dynamic_hash_map_header.txt
replace_var_block dynamic_source_template dynamic_hash_map_source.txt
