Here’s a practical proposal for a small-but-capable C terminal styling/printing library. The goals:

* Simple “print styled text” for 90% of use cases
* Zero heap allocations by default
* Works with: 16-color, 256-color (0–255), and truecolor (24-bit)
* Supports: fg/bg, bold/dim/italic/underline/double-underline, blink, inverse, hidden, strikethrough, etc.
* Lets users either:

  * print immediately (convenience), or
  * build output into a buffer (fast, testable), or
  * use a streaming writer callback (no stdio dependency)

Below is an API shape that’s hard to misuse, easy to extend, and doesn’t lock you into one output strategy.

---

## 1) Public header sketch

### Core types

```c
// termsty.h
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Which color mode to emit (what sequences we generate).
typedef enum termsty_mode {
    TERMSTY_MODE_AUTO = 0,   // detect (TTY + env hints) or fall back
    TERMSTY_MODE_ANSI16,     // 3/4-bit
    TERMSTY_MODE_ANSI256,    // 8-bit 0..255
    TERMSTY_MODE_TRUECOLOR,  // 24-bit
    TERMSTY_MODE_NONE        // emit no escapes (plain text)
} termsty_mode_t;

// How we represent a color in the style.
typedef enum termsty_color_kind {
    TERMSTY_COLOR_DEFAULT = 0,
    TERMSTY_COLOR_ANSI16,      // 0..15
    TERMSTY_COLOR_ANSI256,     // 0..255
    TERMSTY_COLOR_RGB          // 24-bit
} termsty_color_kind_t;

typedef struct termsty_color {
    termsty_color_kind_t kind;
    union {
        uint8_t ansi16;   // 0..15
        uint8_t ansi256;  // 0..255
        struct { uint8_t r, g, b; } rgb;
    } v;
} termsty_color_t;

// Attributes are a bitset, easy to extend.
typedef uint32_t termsty_attr_t;
enum {
    TERMSTY_BOLD          = 1u << 0,
    TERMSTY_DIM           = 1u << 1,
    TERMSTY_ITALIC        = 1u << 2,
    TERMSTY_UNDERLINE     = 1u << 3,
    TERMSTY_DUNDERLINE    = 1u << 4, // double underline (if supported)
    TERMSTY_BLINK         = 1u << 5,
    TERMSTY_RBLINK        = 1u << 6, // rapid blink (rare)
    TERMSTY_INVERSE       = 1u << 7,
    TERMSTY_HIDDEN        = 1u << 8,
    TERMSTY_STRIKE        = 1u << 9,
    // reserve bits for future attrs
};

typedef struct termsty_style {
    termsty_color_t fg;
    termsty_color_t bg;
    termsty_attr_t  add;   // attrs to enable
    termsty_attr_t  rem;   // attrs to disable (useful for diffing)
} termsty_style_t;

// Output abstraction: write bytes somewhere.
typedef size_t (*termsty_write_fn)(void* user, const char* data, size_t len);

typedef struct termsty_writer {
    termsty_write_fn write;
    void* user;
    termsty_mode_t mode;     // what to emit
    bool strip_escapes;      // force plain text (overrides mode)
} termsty_writer_t;
```

### Convenience constructors

```c
// Colors
static inline termsty_color_t termsty_default(void) {
    termsty_color_t c; c.kind = TERMSTY_COLOR_DEFAULT; return c;
}
static inline termsty_color_t termsty_ansi16(uint8_t idx0_15) {
    termsty_color_t c; c.kind = TERMSTY_COLOR_ANSI16; c.v.ansi16 = idx0_15; return c;
}
static inline termsty_color_t termsty_ansi256(uint8_t idx0_255) {
    termsty_color_t c; c.kind = TERMSTY_COLOR_ANSI256; c.v.ansi256 = idx0_255; return c;
}
static inline termsty_color_t termsty_rgb(uint8_t r, uint8_t g, uint8_t b) {
    termsty_color_t c; c.kind = TERMSTY_COLOR_RGB; c.v.rgb = (typeof(c.v.rgb)){r,g,b}; return c;
}

// Styles
static inline termsty_style_t termsty_style(termsty_color_t fg, termsty_color_t bg, termsty_attr_t add) {
    termsty_style_t s;
    s.fg = fg; s.bg = bg; s.add = add; s.rem = 0;
    return s;
}
static inline termsty_style_t termsty_plain(void) {
    return termsty_style(termsty_default(), termsty_default(), 0);
}
```

### Writer creation helpers (stdio optional)

```c
// A writer that writes to a FILE* (stdout/stderr), if you want to ship a stdio helper.
struct _IO_FILE; // avoid pulling <stdio.h> in the header if you want

termsty_writer_t termsty_writer_file(void* file /* FILE* */, termsty_mode_t mode);

// A writer that writes into a fixed buffer (no overflow; returns truncated length).
typedef struct termsty_buf {
    char*  data;
    size_t cap;
    size_t len;
} termsty_buf_t;

termsty_writer_t termsty_writer_buf(termsty_buf_t* b, termsty_mode_t mode);

// A raw writer for custom sinks (sockets, ring buffers, logging).
static inline termsty_writer_t termsty_writer(termsty_write_fn fn, void* user, termsty_mode_t mode) {
    termsty_writer_t w = { fn, user, mode, false };
    return w;
}
```

### Capability + mode selection

```c
typedef struct termsty_detect {
    termsty_mode_t mode;    // chosen mode
    bool is_tty;
    bool supports_underline;
    bool supports_strike;
    // optional: more feature flags
} termsty_detect_t;

// If you ship platform-specific detection, keep it in .c.
termsty_detect_t termsty_detect_stdout(void);
termsty_detect_t termsty_detect_stderr(void);

// Force “no escapes” globally for a writer.
void termsty_set_strip(termsty_writer_t* w, bool strip);
```

### Emitting SGR sequences

Key concept: *style diffing* so you don’t spam resets on every token.

```c
// Emits SGR to set absolute style (commonly uses reset + set).
size_t termsty_emit_set(termsty_writer_t* w, termsty_style_t style);

// Emits minimal SGR to transition prev -> next (preferable).
size_t termsty_emit_diff(termsty_writer_t* w, termsty_style_t prev, termsty_style_t next);

// Emit reset (SGR 0)
size_t termsty_emit_reset(termsty_writer_t* w);
```

### High-level print API

A tiny “scoped” pattern that’s easy to read:

```c
// Print raw text
size_t termsty_write(termsty_writer_t* w, const char* s, size_t len);
size_t termsty_puts(termsty_writer_t* w, const char* s); // null-terminated

// Print styled text with auto reset (safe default).
size_t termsty_print(termsty_writer_t* w, termsty_style_t style, const char* s);

// printf variant (optional; requires stdarg).
size_t termsty_printf(termsty_writer_t* w, termsty_style_t style, const char* fmt, ...);

// Scoped style (user controls when to reset).
size_t termsty_push(termsty_writer_t* w, termsty_style_t style);
size_t termsty_pop(termsty_writer_t* w); // typically emits reset or returns to previous
```

To implement push/pop cleanly, you can embed a small stack in the writer (fixed depth, like 8), or make it an optional “context” object.

---

## 2) User-facing usage examples

### Simple “print in a color”

```c
termsty_detect_t det = termsty_detect_stdout();
termsty_writer_t out = termsty_writer_file(stdout, det.mode);

termsty_print(&out,
    termsty_style(termsty_ansi256(33), termsty_default(), TERMSTY_BOLD),
    "Info: connected\n");
```

### Truecolor with underline + background

```c
termsty_style_t s = termsty_style(termsty_rgb(255, 240, 128),
                                  termsty_rgb(32, 32, 32),
                                  TERMSTY_UNDERLINE);

termsty_print(&out, s, "Highlighted\n");
```

### Streaming with diffs (fast for UIs)

```c
termsty_style_t prev = termsty_plain();
termsty_style_t next = termsty_style(termsty_ansi256(196), termsty_default(), TERMSTY_BOLD);

termsty_emit_diff(&out, prev, next);
termsty_puts(&out, "ERROR");
termsty_emit_diff(&out, next, prev);
termsty_puts(&out, ": failed\n");
```

### Build into a buffer for logging/tests

```c
char mem[1024];
termsty_buf_t b = { .data = mem, .cap = sizeof(mem), .len = 0 };
termsty_writer_t w = termsty_writer_buf(&b, TERMSTY_MODE_ANSI256);

termsty_print(&w, termsty_style(termsty_ansi256(82), termsty_default(), 0), "ok");
termsty_emit_reset(&w);

// b.data now contains escapes + text; b.len is used length
```

---

## 3) Escape generation rules (SGR)

You’ll primarily emit CSI SGR:

* Reset: `\x1b[0m`
* Bold: `\x1b[1m` (off is `22m`)
* Dim: `2m` (off `22m`)
* Italic: `3m` (off `23m`)
* Underline: `4m` (off `24m`)
* Double underline: `21m` in many terminals (but note: some use 21 for “bold off”; you’ll want a feature flag / careful behavior)
* Strikethrough: `9m` (off `29m`)
* Inverse: `7m` (off `27m`)
* Hidden: `8m` (off `28m`)
* 256-color FG: `38;5;{n}m`
* 256-color BG: `48;5;{n}m`
* Truecolor FG: `38;2;R;G;Bm`
* Truecolor BG: `48;2;R;G;Bm`
* Default FG/BG: `39m` / `49m`

Implementation detail that matters: for `termsty_emit_diff(prev,next)`, you can either:

1. do a conservative `reset + set next` (simple, correct, a bit spammy), or
2. do minimal changes:

   * turn off attrs present in prev but not in next (use rem bitset)
   * turn on attrs present in next but not in prev
   * update fg/bg only if changed

Both are useful. Provide both functions; most people will use `print()` (reset wrapped), advanced folks use `diff()`.

---

## 4) Color downsampling policy (when mode < requested)

If the user asks for RGB but output mode is ANSI256/ANSI16, you need a deterministic mapping:

* RGB → ANSI256:

  * Use xterm 6×6×6 cube + grayscale ramp mapping (standard)
* RGB → ANSI16:

  * Map to closest of 16 colors (or 8 + bright bit)
* ANSI256 → ANSI16:

  * nearest of 16 (or a standard table)
* ANSI16 → others: trivial

Expose mapping helpers so users can precompute:

```c
uint8_t termsty_rgb_to_ansi256(uint8_t r, uint8_t g, uint8_t b);
uint8_t termsty_rgb_to_ansi16(uint8_t r, uint8_t g, uint8_t b);
```

Then your emitter can do the right thing automatically based on `writer.mode`.

---

## 5) Detection (AUTO mode) without being too clever

AUTO should pick the best mode that won’t produce garbage:

* If not a TTY → `TERMSTY_MODE_NONE` (unless user forces)
* Respect common env toggles:

  * `NO_COLOR` → NONE
  * `TERM=dumb` → NONE
  * `COLORTERM=truecolor|24bit` → TRUECOLOR
  * `TERM` containing `256color` → ANSI256
* On Windows, you can enable VT processing on newer consoles, otherwise fall back to NONE or WinAPI (depending on scope).

Keep detection optional and overridable. Make it easy to do:

```c
termsty_writer_t out = termsty_writer_file(stdout, TERMSTY_MODE_AUTO);
termsty_writer_autoconfigure(&out); // calls detect + applies strip_escapes etc.
```

(You can also fold AUTO handling into writer_file.)

---

## 6) Ergonomics extras that actually help

### Named “semantic” styles (optional)

A small theme object:

```c
typedef struct termsty_theme {
    termsty_style_t info;
    termsty_style_t warn;
    termsty_style_t error;
    termsty_style_t success;
    termsty_style_t heading;
} termsty_theme_t;

termsty_theme_t termsty_theme_default(termsty_mode_t mode);
```

### Safe interpolation helpers

If you ever plan to support user-provided strings that might include `\x1b`, some apps want to strip/escape those:

```c
size_t termsty_write_sanitized(termsty_writer_t* w, const char* s, size_t len, bool allow_esc);
```

---

## 7) “Tell it like it is” design tradeoffs

* **Don’t over-engineer a markup language** (like `{red}text{/}`) in v1. It’s seductive but turns into a parsing mess and encourages mixing style and content.
* **Provide both**:

  * one-shot `termsty_print` (reset wrapped) so users don’t accidentally “leak” styles into later output,
  * plus diff/push/pop for TUIs.
* **Avoid heap allocation** in the core. If you need a stack for push/pop, use a fixed small stack in the writer or a separate `termsty_ctx` struct that the user can allocate however they want.
* **Make output sink abstract** so the library is usable in embedded / logging / unit tests.

---

## 8) Minimal file layout

* `termsty.h` — public API
* `termsty_core.c` — SGR formatting, diffing, mapping
* `termsty_detect.c` — tty/env/windows detection (optional compile)
* `termsty_stdio.c` — FILE* writer (optional)
* `termsty_buf.c` — buffer writer
* `termsty_win.c` — Windows VT enablement (optional)

---

If you want, I can also provide:

* a concrete `termsty_emit_diff()` algorithm (including correct “turn-off” SGR codes),
* the RGB→ANSI256/ANSI16 mapping tables/functions,
* and a small demo program showing performance-friendly “diff printing” for a status UI.
