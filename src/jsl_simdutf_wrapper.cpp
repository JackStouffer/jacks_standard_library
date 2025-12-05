/**
 * C ABI wrappers around the public simdutf API so C callers can link against
 * the library without C++ name mangling.
 */

#include "simdutf.h"

#include <cstdint>

extern "C" {

typedef enum simdutf_error_code {
  SIMDUTF_ERROR_SUCCESS = simdutf::SUCCESS,
  SIMDUTF_ERROR_HEADER_BITS = simdutf::HEADER_BITS,
  SIMDUTF_ERROR_TOO_SHORT = simdutf::TOO_SHORT,
  SIMDUTF_ERROR_TOO_LONG = simdutf::TOO_LONG,
  SIMDUTF_ERROR_OVERLONG = simdutf::OVERLONG,
  SIMDUTF_ERROR_TOO_LARGE = simdutf::TOO_LARGE,
  SIMDUTF_ERROR_SURROGATE = simdutf::SURROGATE,
  SIMDUTF_ERROR_INVALID_BASE64_CHARACTER = simdutf::INVALID_BASE64_CHARACTER,
  SIMDUTF_ERROR_BASE64_INPUT_REMAINDER = simdutf::BASE64_INPUT_REMAINDER,
  SIMDUTF_ERROR_BASE64_EXTRA_BITS = simdutf::BASE64_EXTRA_BITS,
  SIMDUTF_ERROR_OUTPUT_BUFFER_TOO_SMALL = simdutf::OUTPUT_BUFFER_TOO_SMALL,
  SIMDUTF_ERROR_OTHER = simdutf::OTHER
} simdutf_error_code;

typedef struct simdutf_result {
  simdutf_error_code error;
  size_t count;
} simdutf_result;

} // extern "C"

namespace {

simdutf_result to_c_result(simdutf::result r) {
  return simdutf_result{static_cast<simdutf_error_code>(r.error), r.count};
}

simdutf::base64_options to_base64_options(uint64_t options) {
  return static_cast<simdutf::base64_options>(options);
}

simdutf::last_chunk_handling_options
to_last_chunk_options(uint64_t options) {
  return static_cast<simdutf::last_chunk_handling_options>(options);
}

} // namespace

extern "C" {

bool simdutf_validate_utf8(const char *buf, size_t len) {
  return simdutf::validate_utf8(buf, len);
}

simdutf_result simdutf_validate_utf8_with_errors(const char *buf, size_t len) {
  return to_c_result(simdutf::validate_utf8_with_errors(buf, len));
}

size_t simdutf_convert_utf8_to_utf16(const char *input, size_t length,
                                     char16_t *utf16_output) {
  return simdutf::convert_utf8_to_utf16(input, length, utf16_output);
}

size_t simdutf_convert_utf8_to_utf16le(const char *input, size_t length,
                                       char16_t *utf16_output) {
  return simdutf::convert_utf8_to_utf16le(input, length, utf16_output);
}

size_t simdutf_convert_utf8_to_utf16be(const char *input, size_t length,
                                       char16_t *utf16_output) {
  return simdutf::convert_utf8_to_utf16be(input, length, utf16_output);
}

simdutf_result simdutf_convert_utf8_to_utf16_with_errors(
    const char *input, size_t length, char16_t *utf16_output) {
  return to_c_result(
      simdutf::convert_utf8_to_utf16_with_errors(input, length, utf16_output));
}

simdutf_result simdutf_convert_utf8_to_utf16le_with_errors(
    const char *input, size_t length, char16_t *utf16_output) {
  return to_c_result(simdutf::convert_utf8_to_utf16le_with_errors(
      input, length, utf16_output));
}

simdutf_result simdutf_convert_utf8_to_utf16be_with_errors(
    const char *input, size_t length, char16_t *utf16_output) {
  return to_c_result(simdutf::convert_utf8_to_utf16be_with_errors(
      input, length, utf16_output));
}

bool simdutf_validate_utf16(const char16_t *buf, size_t len) {
  return simdutf::validate_utf16(buf, len);
}

void simdutf_to_well_formed_utf16be(const char16_t *input, size_t len,
                                    char16_t *output) {
  simdutf::to_well_formed_utf16be(input, len, output);
}

void simdutf_to_well_formed_utf16le(const char16_t *input, size_t len,
                                    char16_t *output) {
  simdutf::to_well_formed_utf16le(input, len, output);
}

void simdutf_to_well_formed_utf16(const char16_t *input, size_t len,
                                  char16_t *output) {
  simdutf::to_well_formed_utf16(input, len, output);
}

bool simdutf_validate_utf16le(const char16_t *buf, size_t len) {
  return simdutf::validate_utf16le(buf, len);
}

#if SIMDUTF_ATOMIC_REF
simdutf_result simdutf_atomic_base64_to_binary_safe(
    const char *input, size_t length, char *output, size_t *outlen,
    uint64_t options, uint64_t last_chunk_handling_options,
    bool decode_up_to_bad_char) {
  if (outlen == nullptr) {
    return simdutf_result{SIMDUTF_ERROR_OTHER, 0};
  }
  size_t outlen_value = *outlen;
  simdutf::result r = simdutf::atomic_base64_to_binary_safe(
      input, length, output, outlen_value, to_base64_options(options),
      to_last_chunk_options(last_chunk_handling_options),
      decode_up_to_bad_char);
  *outlen = outlen_value;
  return to_c_result(r);
}

simdutf_result simdutf_atomic_base64_to_binary_safe_u16(
    const char16_t *input, size_t length, char *output, size_t *outlen,
    uint64_t options, uint64_t last_chunk_handling_options,
    bool decode_up_to_bad_char) {
  if (outlen == nullptr) {
    return simdutf_result{SIMDUTF_ERROR_OTHER, 0};
  }
  size_t outlen_value = *outlen;
  simdutf::result r = simdutf::atomic_base64_to_binary_safe(
      input, length, output, outlen_value, to_base64_options(options),
      to_last_chunk_options(last_chunk_handling_options),
      decode_up_to_bad_char);
  *outlen = outlen_value;
  return to_c_result(r);
}
#endif // SIMDUTF_ATOMIC_REF

bool simdutf_validate_utf16be(const char16_t *buf, size_t len) {
  return simdutf::validate_utf16be(buf, len);
}

simdutf_result simdutf_validate_utf16_with_errors(const char16_t *buf,
                                                  size_t len) {
  return to_c_result(simdutf::validate_utf16_with_errors(buf, len));
}

simdutf_result simdutf_validate_utf16le_with_errors(const char16_t *buf,
                                                    size_t len) {
  return to_c_result(simdutf::validate_utf16le_with_errors(buf, len));
}

simdutf_result simdutf_validate_utf16be_with_errors(const char16_t *buf,
                                                    size_t len) {
  return to_c_result(simdutf::validate_utf16be_with_errors(buf, len));
}

size_t simdutf_convert_valid_utf8_to_utf16(const char *input, size_t length,
                                           char16_t *utf16_buffer) {
  return simdutf::convert_valid_utf8_to_utf16(input, length, utf16_buffer);
}

size_t simdutf_convert_valid_utf8_to_utf16le(const char *input, size_t length,
                                             char16_t *utf16_buffer) {
  return simdutf::convert_valid_utf8_to_utf16le(input, length, utf16_buffer);
}

size_t simdutf_convert_valid_utf8_to_utf16be(const char *input, size_t length,
                                             char16_t *utf16_buffer) {
  return simdutf::convert_valid_utf8_to_utf16be(input, length, utf16_buffer);
}

size_t simdutf_convert_utf16_to_utf8(const char16_t *buf, size_t len,
                                     char *utf8_buffer) {
  return simdutf::convert_utf16_to_utf8(buf, len, utf8_buffer);
}

size_t simdutf_convert_utf16_to_utf8_safe(const char16_t *buf, size_t len,
                                          char *utf8_output, size_t utf8_len) {
  return simdutf::convert_utf16_to_utf8_safe(buf, len, utf8_output, utf8_len);
}

size_t simdutf_convert_utf16le_to_utf8(const char16_t *buf, size_t len,
                                       char *utf8_buffer) {
  return simdutf::convert_utf16le_to_utf8(buf, len, utf8_buffer);
}

size_t simdutf_convert_utf16be_to_utf8(const char16_t *buf, size_t len,
                                       char *utf8_buffer) {
  return simdutf::convert_utf16be_to_utf8(buf, len, utf8_buffer);
}

simdutf_result simdutf_convert_utf16_to_utf8_with_errors(const char16_t *buf,
                                                         size_t len,
                                                         char *utf8_buffer) {
  return to_c_result(
      simdutf::convert_utf16_to_utf8_with_errors(buf, len, utf8_buffer));
}

simdutf_result simdutf_convert_utf16le_to_utf8_with_errors(
    const char16_t *buf, size_t len, char *utf8_buffer) {
  return to_c_result(
      simdutf::convert_utf16le_to_utf8_with_errors(buf, len, utf8_buffer));
}

simdutf_result simdutf_convert_utf16be_to_utf8_with_errors(
    const char16_t *buf, size_t len, char *utf8_buffer) {
  return to_c_result(
      simdutf::convert_utf16be_to_utf8_with_errors(buf, len, utf8_buffer));
}

size_t simdutf_convert_valid_utf16_to_utf8(const char16_t *buf, size_t len,
                                           char *utf8_buffer) {
  return simdutf::convert_valid_utf16_to_utf8(buf, len, utf8_buffer);
}

size_t simdutf_convert_valid_utf16le_to_utf8(const char16_t *buf, size_t len,
                                             char *utf8_buffer) {
  return simdutf::convert_valid_utf16le_to_utf8(buf, len, utf8_buffer);
}

size_t simdutf_convert_valid_utf16be_to_utf8(const char16_t *buf, size_t len,
                                             char *utf8_buffer) {
  return simdutf::convert_valid_utf16be_to_utf8(buf, len, utf8_buffer);
}

void simdutf_change_endianness_utf16(const char16_t *input, size_t length,
                                     char16_t *output) {
  simdutf::change_endianness_utf16(input, length, output);
}

size_t simdutf_count_utf16(const char16_t *input, size_t length) {
  return simdutf::count_utf16(input, length);
}

size_t simdutf_count_utf16le(const char16_t *input, size_t length) {
  return simdutf::count_utf16le(input, length);
}

size_t simdutf_count_utf16be(const char16_t *input, size_t length) {
  return simdutf::count_utf16be(input, length);
}

size_t simdutf_utf8_length_from_utf16(const char16_t *input, size_t length) {
  return simdutf::utf8_length_from_utf16(input, length);
}

simdutf_result
simdutf_utf8_length_from_utf16_with_replacement(const char16_t *input,
                                                size_t length) {
  return to_c_result(
      simdutf::utf8_length_from_utf16_with_replacement(input, length));
}

size_t simdutf_utf8_length_from_utf16le(const char16_t *input, size_t length) {
  return simdutf::utf8_length_from_utf16le(input, length);
}

size_t simdutf_utf8_length_from_utf16be(const char16_t *input, size_t length) {
  return simdutf::utf8_length_from_utf16be(input, length);
}

size_t simdutf_utf16_length_from_utf8(const char *input, size_t length) {
  return simdutf::utf16_length_from_utf8(input, length);
}

simdutf_result simdutf_utf8_length_from_utf16le_with_replacement(
    const char16_t *input, size_t length) {
  return to_c_result(
      simdutf::utf8_length_from_utf16le_with_replacement(input, length));
}

simdutf_result simdutf_utf8_length_from_utf16be_with_replacement(
    const char16_t *input, size_t length) {
  return to_c_result(
      simdutf::utf8_length_from_utf16be_with_replacement(input, length));
}

size_t simdutf_base64_length_from_binary(size_t length, uint64_t options) {
  return simdutf::base64_length_from_binary(length,
                                            to_base64_options(options));
}

size_t simdutf_base64_length_from_binary_with_lines(size_t length,
                                                    uint64_t options,
                                                    size_t line_length) {
  return simdutf::base64_length_from_binary_with_lines(
      length, to_base64_options(options), line_length);
}

const char *simdutf_find_char(const char *start, const char *end,
                              char character) {
  return simdutf::find(start, end, character);
}

const char16_t *simdutf_find_char16(const char16_t *start, const char16_t *end,
                                    char16_t character) {
  return simdutf::find(start, end, character);
}

size_t simdutf_maximal_binary_length_from_base64(const char *input,
                                                 size_t length) {
  return simdutf::maximal_binary_length_from_base64(input, length);
}

simdutf_result simdutf_base64_to_binary(const char *input, size_t length,
                                        char *output, uint64_t options,
                                        uint64_t last_chunk_handling_options) {
  return to_c_result(simdutf::base64_to_binary(
      input, length, output, to_base64_options(options),
      to_last_chunk_options(last_chunk_handling_options)));
}

size_t simdutf_maximal_binary_length_from_base64_u16(const char16_t *input,
                                                     size_t length) {
  return simdutf::maximal_binary_length_from_base64(input, length);
}

simdutf_result simdutf_base64_to_binary_u16(
    const char16_t *input, size_t length, char *output, uint64_t options,
    uint64_t last_chunk_handling_options) {
  return to_c_result(simdutf::base64_to_binary(
      input, length, output, to_base64_options(options),
      to_last_chunk_options(last_chunk_handling_options)));
}

bool simdutf_base64_ignorable(char input, uint64_t options) {
  return simdutf::base64_ignorable(input, to_base64_options(options));
}

bool simdutf_base64_ignorable_u16(char16_t input, uint64_t options) {
  return simdutf::base64_ignorable(input, to_base64_options(options));
}

bool simdutf_base64_valid(char input, uint64_t options) {
  return simdutf::base64_valid(input, to_base64_options(options));
}

bool simdutf_base64_valid_u16(char16_t input, uint64_t options) {
  return simdutf::base64_valid(input, to_base64_options(options));
}

bool simdutf_base64_valid_or_padding(char input, uint64_t options) {
  return simdutf::base64_valid_or_padding(input, to_base64_options(options));
}

bool simdutf_base64_valid_or_padding_u16(char16_t input, uint64_t options) {
  return simdutf::base64_valid_or_padding(input, to_base64_options(options));
}

simdutf_result simdutf_base64_to_binary_safe(
    const char *input, size_t length, char *output, size_t *outlen,
    uint64_t options, uint64_t last_chunk_handling_options,
    bool decode_up_to_bad_char) {
  if (outlen == nullptr) {
    return simdutf_result{SIMDUTF_ERROR_OTHER, 0};
  }
  size_t outlen_value = *outlen;
  simdutf::result r = simdutf::base64_to_binary_safe(
      input, length, output, outlen_value, to_base64_options(options),
      to_last_chunk_options(last_chunk_handling_options),
      decode_up_to_bad_char);
  *outlen = outlen_value;
  return to_c_result(r);
}

simdutf_result simdutf_base64_to_binary_safe_u16(
    const char16_t *input, size_t length, char *output, size_t *outlen,
    uint64_t options, uint64_t last_chunk_handling_options,
    bool decode_up_to_bad_char) {
  if (outlen == nullptr) {
    return simdutf_result{SIMDUTF_ERROR_OTHER, 0};
  }
  size_t outlen_value = *outlen;
  simdutf::result r = simdutf::base64_to_binary_safe(
      input, length, output, outlen_value, to_base64_options(options),
      to_last_chunk_options(last_chunk_handling_options),
      decode_up_to_bad_char);
  *outlen = outlen_value;
  return to_c_result(r);
}

#if SIMDUTF_ATOMIC_REF
size_t simdutf_atomic_binary_to_base64(const char *input, size_t length,
                                       char *output, uint64_t options) {
  return simdutf::atomic_binary_to_base64(
      input, length, output, to_base64_options(options));
}
#endif // SIMDUTF_ATOMIC_REF

size_t simdutf_binary_to_base64(const char *input, size_t length, char *output,
                                uint64_t options) {
  return simdutf::binary_to_base64(input, length, output,
                                   to_base64_options(options));
}

size_t simdutf_binary_to_base64_with_lines(const char *input, size_t length,
                                           char *output, size_t line_length,
                                           uint64_t options) {
  return simdutf::binary_to_base64_with_lines(
      input, length, output, line_length, to_base64_options(options));
}

size_t simdutf_trim_partial_utf8(const char *input, size_t length) {
  return simdutf::trim_partial_utf8(input, length);
}

size_t simdutf_trim_partial_utf16be(const char16_t *input, size_t length) {
  return simdutf::trim_partial_utf16be(input, length);
}

size_t simdutf_trim_partial_utf16le(const char16_t *input, size_t length) {
  return simdutf::trim_partial_utf16le(input, length);
}

size_t simdutf_trim_partial_utf16(const char16_t *input, size_t length) {
  return simdutf::trim_partial_utf16(input, length);
}

} // extern "C"
