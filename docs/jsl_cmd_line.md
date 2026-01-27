# API Documentation

## Macros

- (none)

## Types

- [`JSLCmdLineArgs`](#type-typedef-jslcmdlineargs)

## Functions

- [`jsl_cmd_line_args_init`](#function-jsl_cmd_line_args_init)
- [`jsl_cmd_line_args_parse`](#function-jsl_cmd_line_args_parse)
- [`jsl_cmd_line_args_parse_wide`](#function-jsl_cmd_line_args_parse_wide)
- [`jsl_cmd_line_args_has_short_flag`](#function-jsl_cmd_line_args_has_short_flag)
- [`jsl_cmd_line_args_has_flag`](#function-jsl_cmd_line_args_has_flag)
- [`jsl_cmd_line_args_has_command`](#function-jsl_cmd_line_args_has_command)
- [`jsl_cmd_line_args_pop_arg_list`](#function-jsl_cmd_line_args_pop_arg_list)
- [`jsl_cmd_line_args_pop_flag_with_value`](#function-jsl_cmd_line_args_pop_flag_with_value)

## File: src/jsl_cmd_line.h

TODO: docs

### License

Copyright (c) 2026 Jack Stouffer

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the “Software”),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

<a id="type-typedef-jslcmdlineargs"></a>
### Typedef: `JSLCmdLineArgs`

TODO: docs

State container struct

```c
typedef struct JSL__CmdLineArgs JSLCmdLineArgs;
```


*Defined at*: `src/jsl_cmd_line.h:70`

---

<a id="function-jsl_cmd_line_args_init"></a>
### Function: `jsl_cmd_line_args_init`

Initialize an instance of the command line parser container.

```c
int jsl_cmd_line_args_init(JSLCmdLineArgs *args, JSLAllocatorInterface *allocator);
```


*Defined at*: `src/jsl_cmd_line.h:75`

---

<a id="function-jsl_cmd_line_args_parse"></a>
### Function: `jsl_cmd_line_args_parse`

Parse the given command line arguments that are in the POSIX style.

The inputs must represent valid UTF-8. 

This functions parses and stores the flags, arguments, and commands
from the user input for easy querying with the other functions in this
module. If `out_error` is not NULL it will be set to a
user-friendly message on failure.

TODO: docs

#### Returns

false if the arena is out of memory or the passed in strings
were not valid utf-8.

```c
int jsl_cmd_line_args_parse(JSLCmdLineArgs *args, int argc, char **argv, JSLFatPtr *out_error);
```


*Defined at*: `src/jsl_cmd_line.h:91`

---

<a id="function-jsl_cmd_line_args_parse_wide"></a>
### Function: `jsl_cmd_line_args_parse_wide`

Parse the given command line arguments that are in the POSIX style.

The inputs must represent valid UTF-16. 

This functions parses and stores the flags, arguments, and commands
from the user input for easy querying with the other functions in this
module. If `out_error` is not NULL it will be set to a
user-friendly message on failure.

TODO: docs

#### Returns

false if the arena is out of memory or the passed in strings
were not valid utf-16.

```c
int jsl_cmd_line_args_parse_wide(JSLCmdLineArgs *args, int argc, int **argv, JSLFatPtr *out_error);
```


*Defined at*: `src/jsl_cmd_line.h:112`

---

<a id="function-jsl_cmd_line_args_has_short_flag"></a>
### Function: `jsl_cmd_line_args_has_short_flag`

Checks if the user passed in the given short flag.

Example:

```
./command -f -v
```

```
jsl_cmd_line_args_has_short_flag(cmd, 'f'); // true
jsl_cmd_line_args_has_short_flag(cmd, 'v'); // true
jsl_cmd_line_args_has_short_flag(cmd, 'g'); // false
```
TODO: docs

```c
int jsl_cmd_line_args_has_short_flag(JSLCmdLineArgs *args, int flag);
```


*Defined at*: `src/jsl_cmd_line.h:135`

---

<a id="function-jsl_cmd_line_args_has_flag"></a>
### Function: `jsl_cmd_line_args_has_flag`

Checks if the user passed in the given flag with no value.

Example:

```
./command --long-flag --arg=my_value --arg2
```

```
jsl_cmd_line_args_has_flag(cmd, JSL_FATPTR_EXPRESSION("long-flag")); // true
jsl_cmd_line_args_has_flag(cmd, JSL_FATPTR_EXPRESSION("arg")); // false
jsl_cmd_line_args_has_flag(cmd, JSL_FATPTR_EXPRESSION("arg2")); // true
```
TODO: docs

```c
int jsl_cmd_line_args_has_flag(JSLCmdLineArgs *args, JSLFatPtr flag);
```


*Defined at*: `src/jsl_cmd_line.h:153`

---

<a id="function-jsl_cmd_line_args_has_command"></a>
### Function: `jsl_cmd_line_args_has_command`

Checks if the user passed in the given command.

Example:

```
./command -v clean build 
```

```
jsl_cmd_line_args_has_command(cmd, JSL_FATPTR_EXPRESSION("clean")); // true
jsl_cmd_line_args_has_command(cmd, JSL_FATPTR_EXPRESSION("build")); // true
jsl_cmd_line_args_has_command(cmd, JSL_FATPTR_EXPRESSION("restart")); // false
```
TODO: docs

```c
int jsl_cmd_line_args_has_command(JSLCmdLineArgs *args, JSLFatPtr flag);
```


*Defined at*: `src/jsl_cmd_line.h:171`

---

<a id="function-jsl_cmd_line_args_pop_arg_list"></a>
### Function: `jsl_cmd_line_args_pop_arg_list`

If the user passed in a argument list, pop one off returning true if
a value was successfully gotten.

Example:

```
./command file1 file2 file3 
```

```
JSLFatPtr arg;

jsl_cmd_line_args_pop_arg_list(cmd, &arg); // arg == file1
jsl_cmd_line_args_pop_arg_list(cmd, &arg); // arg == file2
jsl_cmd_line_args_pop_arg_list(cmd, &arg); // arg == file3
jsl_cmd_line_args_pop_arg_list(cmd, &arg); // returns false
```
TODO: docs

#### Warning

This function assumes that there are no commands that can be returned
with `jsl_cmd_line_args_has_command`. There's no syntactic distinction between a single
command and a argument list.

```c
int jsl_cmd_line_args_pop_arg_list(JSLCmdLineArgs *args, JSLFatPtr *out_value);
```


*Defined at*: `src/jsl_cmd_line.h:197`

---

<a id="function-jsl_cmd_line_args_pop_flag_with_value"></a>
### Function: `jsl_cmd_line_args_pop_flag_with_value`

If the user passed in multiple instances of flag with a value, this function
will grab one for each call. If there are no more flag instances this function
returns false.

Example:

```
./command --ignore=foo --ignore=bar
```

```
JSLFatPtr arg;

jsl_cmd_line_args_pop_flag_with_value(cmd, JSL_FATPTR_EXPRESSION("ignore"), &arg); // arg == foo
jsl_cmd_line_args_pop_flag_with_value(cmd, JSL_FATPTR_EXPRESSION("ignore"), &arg); // arg == bar
jsl_cmd_line_args_pop_flag_with_value(cmd, JSL_FATPTR_EXPRESSION("ignore"), &arg); // returns false
```
TODO: docs

#### Warning

Argument ordering is not guaranteed

```c
int jsl_cmd_line_args_pop_flag_with_value(JSLCmdLineArgs *args, JSLFatPtr flag, JSLFatPtr *out_value);
```


*Defined at*: `src/jsl_cmd_line.h:221`

---

