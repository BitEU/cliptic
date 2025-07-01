// screen.h - Screen management functions
#ifndef SCREEN_H
#define SCREEN_H

#include <windows.h>
#include <stdbool.h>

// Screen functions
void screen_setup(void);
void screen_clear(void);
bool screen_too_small(void);
void screen_redraw(void (*callback)(void));
void screen_get_size(int *lines, int *cols);

// Console functions
void console_init(void);
void console_set_colors(void);
void console_set_cursor_visible(bool visible);
void console_move_cursor(int y, int x);
void console_set_color(int color_pair);
void console_write_char(wchar_t ch);
void console_write_string(const wchar_t *str);
void console_write_string_at(int y, int x, const wchar_t *str);

// Input functions
int console_get_key(void);
int console_get_key_timeout(int timeout_ms);

// Color management
void color_init_pair(int pair, int fg, int bg);
WORD color_pair_to_attributes(int pair);

#endif // SCREEN_H