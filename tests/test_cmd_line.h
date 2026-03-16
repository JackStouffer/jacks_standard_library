#ifndef TEST_CMD_LINE_H
#define TEST_CMD_LINE_H

void test_short_flags_grouping(void);
void test_short_flag_equals_is_invalid(void);
void test_long_flags_and_commands(void);
void test_long_values_equals_and_space(void);
void test_wide_parsing(void);
void test_cmd_line_color_conversions(void);
void test_cmd_line_write_style_no_color(void);
void test_cmd_line_write_style_ansi16(void);
void test_cmd_line_write_style_ansi16_converts_color_types(void);
void test_cmd_line_write_style_ansi256(void);
void test_cmd_line_write_style_truecolor(void);
void test_cmd_line_write_style_and_reset_invalid(void);
void test_cmd_line_write_reset_ansi_modes(void);

#endif
