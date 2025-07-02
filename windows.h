// windows.h - Window management
#ifndef WINDOWS_H_CLIPTIC
#define WINDOWS_H_CLIPTIC

#include <stdbool.h>
#include "cliptic.h"

// Window structure
typedef struct {
    int y;      // Height
    int x;      // Width
    int line;   // Top position
    int col;    // Left position
    bool centered_y;
    bool centered_x;
} Window;

// Grid structure
typedef struct {
    Window window;
    Position sq;     // Grid squares (y x x)
    struct Cell ***cells;
} Grid;

// Cell structure
typedef struct Cell {
    Position sq;
    Grid *grid;
    Position pos;    // Absolute position
    int index;       // Cell number (0 if none)
    bool blocked;
    bool locked;
    char buffer;
} Cell;

// Window functions
void window_init(Window *win, int y, int x, int line, int col);
void window_draw(Window *win, int color_pair);
void window_draw_bar(Window *win, int bg_color);
void window_draw_grid(Window *win, int color_pair);
void window_clear(Window *win);
void window_refresh(Window *win);
void window_add_str(Window *win, int y, int x, const char *str);
void window_add_str_centered(Window *win, int y, const char *str);
void window_wrap_str(Window *win, int start_line, const char *str);
void window_move(Window *win, int line, int col);

// Grid functions
Grid* grid_new(int y, int x, int line, int col);
void grid_draw(Grid *grid);
Cell* grid_get_cell(Grid *grid, int y, int x);
void grid_free(Grid *grid);

// Cell functions
void cell_set_number(Cell *cell, int n, bool active);
void cell_set_block(Cell *cell);
void cell_write(Cell *cell, char ch);
void cell_underline(Cell *cell);
void cell_color(Cell *cell, int color_pair);
void cell_clear(Cell *cell);
void cell_focus(Cell *cell, int y_offset, int x_offset);

#endif // WINDOWS_H_CLIPTIC