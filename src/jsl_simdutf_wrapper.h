/**
 * C ABI for the simdutf wrapper functions implemented in jsl_simdutf_wrapper.cpp.
 * These declarations allow C code to call the simdutf library.
 */

#ifndef JSL_SIMDUTF_WRAPPER
#define JSL_SIMDUTF_WRAPPER

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if !defined(__cplusplus)
  #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #include <uchar.h>
  #else
    typedef uint16_t char16_t;
  #endif
#endif

#ifndef SIMDUTF_ATOMIC_REF
  #define SIMDUTF_ATOMIC_REF 0
#endif

/* base64_options (mirrors simdutf values) */
#define SIMDUTF_BASE64_OPTION_DEFAULT 0ULL
#define SIMDUTF_BASE64_OPTION_URL 1ULL
#define SIMDUTF_BASE64_OPTION_REVERSE_PADDING 2ULL
#define SIMDUTF_BASE64_OPTION_DEFAULT_NO_PADDING                                   \
  (SIMDUTF_BASE64_OPTION_DEFAULT | SIMDUTF_BASE64_OPTION_REVERSE_PADDING)
#define SIMDUTF_BASE64_OPTION_URL_WITH_PADDING                                     \
  (SIMDUTF_BASE64_OPTION_URL | SIMDUTF_BASE64_OPTION_REVERSE_PADDING)
#define SIMDUTF_BASE64_OPTION_DEFAULT_ACCEPT_GARBAGE 4ULL
#define SIMDUTF_BASE64_OPTION_URL_ACCEPT_GARBAGE 5ULL
#define SIMDUTF_BASE64_OPTION_DEFAULT_OR_URL 8ULL
#define SIMDUTF_BASE64_OPTION_DEFAULT_OR_URL_ACCEPT_GARBAGE 12ULL

/* last_chunk_handling_options (mirrors simdutf values) */
#define SIMDUTF_LAST_CHUNK_LOOSE 0ULL
#define SIMDUTF_LAST_CHUNK_STRICT 1ULL
#define SIMDUTF_LAST_CHUNK_STOP_BEFORE_PARTIAL 2ULL
#define SIMDUTF_LAST_CHUNK_ONLY_FULL_CHUNKS 3ULL

typedef enum simdutf_error_code {
  SIMDUTF_ERROR_SUCCESS = 0,
  SIMDUTF_ERROR_HEADER_BITS,
  SIMDUTF_ERROR_TOO_SHORT,
  SIMDUTF_ERROR_TOO_LONG,
  SIMDUTF_ERROR_OVERLONG,
  SIMDUTF_ERROR_TOO_LARGE,
  SIMDUTF_ERROR_SURROGATE,
  SIMDUTF_ERROR_INVALID_BASE64_CHARACTER,
  SIMDUTF_ERROR_BASE64_INPUT_REMAINDER,
  SIMDUTF_ERROR_BASE64_EXTRA_BITS,
  SIMDUTF_ERROR_OUTPUT_BUFFER_TOO_SMALL,
  SIMDUTF_ERROR_OTHER
} simdutf_error_code;

typedef struct simdutf_result {
  simdutf_error_code error;
  size_t count;
} simdutf_result;

#ifdef __cplusplus
extern "C" {
#endif

bool simdutf_validate_utf8(const char *buf, size_t len);
simdutf_result simdutf_validate_utf8_with_errors(const char *buf, size_t len);

size_t simdutf_convert_utf8_to_utf16(const char *input, size_t length,
                                     char16_t *utf16_output);
size_t simdutf_convert_utf8_to_utf16le(const char *input, size_t length,
                                       char16_t *utf16_output);
size_t simdutf_convert_utf8_to_utf16be(const char *input, size_t length,
                                       char16_t *utf16_output);
simdutf_result simdutf_convert_utf8_to_utf16_with_errors(const char *input,
                                                         size_t length,
                                                         char16_t *utf16_output);
simdutf_result simdutf_convert_utf8_to_utf16le_with_errors(
    const char *input, size_t length, char16_t *utf16_output);
simdutf_result simdutf_convert_utf8_to_utf16be_with_errors(
    const char *input, size_t length, char16_t *utf16_output);

bool simdutf_validate_utf16(const char16_t *buf, size_t len);
void simdutf_to_well_formed_utf16be(const char16_t *input, size_t len,
                                    char16_t *output);
void simdutf_to_well_formed_utf16le(const char16_t *input, size_t len,
                                    char16_t *output);
void simdutf_to_well_formed_utf16(const char16_t *input, size_t len,
                                  char16_t *output);
bool simdutf_validate_utf16le(const char16_t *buf, size_t len);
#if SIMDUTF_ATOMIC_REF
simdutf_result simdutf_atomic_base64_to_binary_safe(
    const char *input, size_t length, char *output, size_t *outlen,
    uint64_t options, uint64_t last_chunk_handling_options,
    bool decode_up_to_bad_char);
simdutf_result simdutf_atomic_base64_to_binary_safe_u16(
    const char16_t *input, size_t length, char *output, size_t *outlen,
    uint64_t options, uint64_t last_chunk_handling_options,
    bool decode_up_to_bad_char);
#endif

bool simdutf_validate_utf16be(const char16_t *buf, size_t len);
simdutf_result simdutf_validate_utf16_with_errors(const char16_t *buf,
                                                  size_t len);
simdutf_result simdutf_validate_utf16le_with_errors(const char16_t *buf,
                                                    size_t len);
simdutf_result simdutf_validate_utf16be_with_errors(const char16_t *buf,
                                                    size_t len);

size_t simdutf_convert_valid_utf8_to_utf16(const char *input, size_t length,
                                           char16_t *utf16_buffer);
size_t simdutf_convert_valid_utf8_to_utf16le(const char *input, size_t length,
                                             char16_t *utf16_buffer);
size_t simdutf_convert_valid_utf8_to_utf16be(const char *input, size_t length,
                                             char16_t *utf16_buffer);

size_t simdutf_convert_utf16_to_utf8(const char16_t *buf, size_t len,
                                     char *utf8_buffer);
size_t simdutf_convert_utf16_to_utf8_safe(const char16_t *buf, size_t len,
                                          char *utf8_output, size_t utf8_len);
size_t simdutf_convert_utf16le_to_utf8(const char16_t *buf, size_t len,
                                       char *utf8_buffer);
size_t simdutf_convert_utf16be_to_utf8(const char16_t *buf, size_t len,
                                       char *utf8_buffer);
simdutf_result simdutf_convert_utf16_to_utf8_with_errors(const char16_t *buf,
                                                         size_t len,
                                                         char *utf8_buffer);
simdutf_result simdutf_convert_utf16le_to_utf8_with_errors(
    const char16_t *buf, size_t len, char *utf8_buffer);
simdutf_result simdutf_convert_utf16be_to_utf8_with_errors(
    const char16_t *buf, size_t len, char *utf8_buffer);

size_t simdutf_convert_valid_utf16_to_utf8(const char16_t *buf, size_t len,
                                           char *utf8_buffer);
size_t simdutf_convert_valid_utf16le_to_utf8(const char16_t *buf, size_t len,
                                             char *utf8_buffer);
size_t simdutf_convert_valid_utf16be_to_utf8(const char16_t *buf, size_t len,
                                             char *utf8_buffer);

void simdutf_change_endianness_utf16(const char16_t *input, size_t length,
                                     char16_t *output);
size_t simdutf_count_utf16(const char16_t *input, size_t length);
size_t simdutf_count_utf16le(const char16_t *input, size_t length);
size_t simdutf_count_utf16be(const char16_t *input, size_t length);

size_t simdutf_utf8_length_from_utf16(const char16_t *input, size_t length);
simdutf_result
simdutf_utf8_length_from_utf16_with_replacement(const char16_t *input,
                                                size_t length);
size_t simdutf_utf8_length_from_utf16le(const char16_t *input, size_t length);
size_t simdutf_utf8_length_from_utf16be(const char16_t *input, size_t length);
size_t simdutf_utf16_length_from_utf8(const char *input, size_t length);
simdutf_result simdutf_utf8_length_from_utf16le_with_replacement(
    const char16_t *input, size_t length);
simdutf_result simdutf_utf8_length_from_utf16be_with_replacement(
    const char16_t *input, size_t length);

size_t simdutf_base64_length_from_binary(size_t length, uint64_t options);
size_t simdutf_base64_length_from_binary_with_lines(size_t length,
                                                    uint64_t options,
                                                    size_t line_length);

const char *simdutf_find_char(const char *start, const char *end,
                              char character);
const char16_t *simdutf_find_char16(const char16_t *start,
                                    const char16_t *end, char16_t character);

size_t simdutf_maximal_binary_length_from_base64(const char *input,
                                                 size_t length);
simdutf_result simdutf_base64_to_binary(const char *input, size_t length,
                                        char *output, uint64_t options,
                                        uint64_t last_chunk_handling_options);
size_t simdutf_maximal_binary_length_from_base64_u16(const char16_t *input,
                                                     size_t length);
simdutf_result simdutf_base64_to_binary_u16(
    const char16_t *input, size_t length, char *output, uint64_t options,
    uint64_t last_chunk_handling_options);

bool simdutf_base64_ignorable(char input, uint64_t options);
bool simdutf_base64_ignorable_u16(char16_t input, uint64_t options);
bool simdutf_base64_valid(char input, uint64_t options);
bool simdutf_base64_valid_u16(char16_t input, uint64_t options);
bool simdutf_base64_valid_or_padding(char input, uint64_t options);
bool simdutf_base64_valid_or_padding_u16(char16_t input, uint64_t options);

simdutf_result simdutf_base64_to_binary_safe(
    const char *input, size_t length, char *output, size_t *outlen,
    uint64_t options, uint64_t last_chunk_handling_options,
    bool decode_up_to_bad_char);
simdutf_result simdutf_base64_to_binary_safe_u16(
    const char16_t *input, size_t length, char *output, size_t *outlen,
    uint64_t options, uint64_t last_chunk_handling_options,
    bool decode_up_to_bad_char);

#if SIMDUTF_ATOMIC_REF
size_t simdutf_atomic_binary_to_base64(const char *input, size_t length,
                                       char *output, uint64_t options);
#endif

size_t simdutf_binary_to_base64(const char *input, size_t length, char *output,
                                uint64_t options);
size_t simdutf_binary_to_base64_with_lines(const char *input, size_t length,
                                           char *output, size_t line_length,
                                           uint64_t options);

const void *simdutf_builtin_implementation(void);

size_t simdutf_trim_partial_utf8(const char *input, size_t length);
size_t simdutf_trim_partial_utf16be(const char16_t *input, size_t length);
size_t simdutf_trim_partial_utf16le(const char16_t *input, size_t length);
size_t simdutf_trim_partial_utf16(const char16_t *input, size_t length);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* JSL_SIMDUTF_WRAPPER */
