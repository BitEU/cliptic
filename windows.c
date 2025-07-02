// windows.c - Window management implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "windows.h"
#include "screen.h"
#include "config.h"

// Window functions
void window_init(Window *win, int y, int x, int line, int col) {
    int lines, cols;
    screen_get_size(&lines, &cols);
    
    // Handle negative dimensions (relative to screen size)
    win->y = (y <= 0) ? y + lines : y;
    win->x = (x <= 0) ? x + cols : x;
    
    // Handle centering
    win->centered_y = (line == -1);
    win->centered_x = (col == -1);
    
    win->line = win->centered_y ? (lines - win->y) / 2 : line;
    win->col = win->centered_x ? (cols - win->x) / 2 : col;
}

void window_draw(Window *win, int color_pair) {
    console_set_color(color_pair);
    
    for (int i = 0; i < win->y; i++) {
        console_move_cursor(win->line + i, win->col);
        
        if (i == 0) {
            // Top border
            console_write_char(UC_TL);
            for (int j = 0; j < win->x - 2; j++) {
                console_write_char(UC_HL);
            }
            console_write_char(UC_TR);
        } else if (i == win->y - 1) {
            // Bottom border
            console_write_char(UC_BL);
            for (int j = 0; j < win->x - 2; j++) {
                console_write_char(UC_HL);
            }
            console_write_char(UC_BR);
        } else {
            // Side borders
            console_write_char(UC_VL);
            for (int j = 0; j < win->x - 2; j++) {
                console_write_char(L' ');
            }
            console_write_char(UC_VL);
        }
    }
}

void window_draw_bar(Window *win, int bg_color) {
    console_set_color(bg_color);
    
    console_move_cursor(win->line, win->col);
    for (int i = 0; i < win->x; i++) {
        console_write_char(L' ');
    }
}

void window_clear(Window *win) {
    for (int i = 0; i < win->y; i++) {
        console_move_cursor(win->line + i, win->col);
        for (int j = 0; j < win->x; j++) {
            console_write_char(L' ');
        }
    }
}

void window_add_str(Window *win, int y, int x, const char *str) {
    console_move_cursor(win->line + y, win->col + x);
    
    wchar_t wstr[256];
    mbstowcs(wstr, str, 256);
    console_write_string(wstr);
}

void window_add_str_centered(Window *win, int y, const char *str) {
    int x = (win->x - strlen(str)) / 2;
    window_add_str(win, y, x, str);
}

void window_wrap_str(Window *win, int start_line, const char *str) {
    int max_width = win->x - 4;
    int len = strlen(str);
    int line = start_line;
    int pos = 0;
    
    while (pos < len && line < win->y - 1) {
        int line_end = pos + max_width;
        if (line_end > len) line_end = len;
        
        // Find last space before line_end
        int space_pos = line_end;
        while (space_pos > pos && str[space_pos] != ' ') {
            space_pos--;
        }
        
        if (space_pos > pos) {
            line_end = space_pos;
        }
        
        // Print the line
        console_move_cursor(win->line + line, win->col + 2);
        for (int i = pos; i < line_end; i++) {
            console_write_char(str[i]);
        }
        
        pos = line_end;
        if (pos < len && str[pos] == ' ') pos++;
        line++;
    }
}

void window_move(Window *win, int line, int col) {
    int lines, cols;
    screen_get_size(&lines, &cols);
    
    if (win->centered_y) {
        win->line = (lines - win->y) / 2;
    } else if (line >= 0) {
        win->line = line;
    }
    
    if (win->centered_x) {
        win->col = (cols - win->x) / 2;
    } else if (col >= 0) {
        win->col = col;
    }
}

// Grid functions
Grid* grid_new(int y, int x, int line, int col) {
    Grid *grid = calloc(1, sizeof(Grid));
    grid->sq.y = y;
    grid->sq.x = x;
    
    // Convert grid squares to window dimensions
    window_init(&grid->window, (2 * y) + 1, (4 * x) + 1, line, col);
    
    // Allocate cells
    grid->cells = calloc(y, sizeof(Cell**));
    for (int i = 0; i < y; i++) {
        grid->cells[i] = calloc(x, sizeof(Cell*));
        for (int j = 0; j < x; j++) {
            grid->cells[i][j] = calloc(1, sizeof(Cell));
            grid->cells[i][j]->sq.y = i;
            grid->cells[i][j]->sq.x = j;
            grid->cells[i][j]->grid = grid;
            grid->cells[i][j]->pos.y = (2 * i) + 1;
            grid->cells[i][j]->pos.x = (4 * j) + 2;
            grid->cells[i][j]->buffer = ' ';
        }
    }
    
    return grid;
}

void grid_draw(Grid *grid) {
    console_set_color(g_colors.grid);
    
    for (int i = 0; i < grid->window.y; i++) {
        console_move_cursor(grid->window.line + i, grid->window.col);
        
        if (i == 0) {
            // Top border
            console_write_char(UC_TL);
            for (int j = 0; j < grid->sq.x - 1; j++) {
                console_write_char(UC_HL);
                console_write_char(UC_HL);
                console_write_char(UC_HL);
                console_write_char(UC_TD);
            }
            console_write_char(UC_HL);
            console_write_char(UC_HL);
            console_write_char(UC_HL);
            console_write_char(UC_TR);
        } else if (i == grid->window.y - 1) {
            // Bottom border
            console_write_char(UC_BL);
            for (int j = 0; j < grid->sq.x - 1; j++) {
                console_write_char(UC_HL);
                console_write_char(UC_HL);
                console_write_char(UC_HL);
                console_write_char(UC_TU);
            }
            console_write_char(UC_HL);
            console_write_char(UC_HL);
            console_write_char(UC_HL);
            console_write_char(UC_BR);
        } else if (i % 2 == 0) {
            // Inner horizontal lines
            console_write_char(UC_TL_SIDE);
            for (int j = 0; j < grid->sq.x - 1; j++) {
                console_write_char(UC_HL);
                console_write_char(UC_HL);
                console_write_char(UC_HL);
                console_write_char(UC_XX);
            }
            console_write_char(UC_HL);
            console_write_char(UC_HL);
            console_write_char(UC_HL);
            console_write_char(UC_TR_SIDE);
        } else {
            // Cell rows
            for (int j = 0; j <= grid->sq.x; j++) {
                console_write_char(UC_VL);
                if (j < grid->sq.x) {
                    console_write_char(L' ');
                    console_write_char(L' ');
                    console_write_char(L' ');
                }
            }
        }
    }
}

Cell* grid_get_cell(Grid *grid, int y, int x) {
    if (y >= 0 && y < grid->sq.y && x >= 0 && x < grid->sq.x) {
        return grid->cells[y][x];
    }
    return NULL;
}

void grid_free(Grid *grid) {
    if (grid) {
        for (int i = 0; i < grid->sq.y; i++) {
            for (int j = 0; j < grid->sq.x; j++) {
                free(grid->cells[i][j]);
            }
            free(grid->cells[i]);
        }
        free(grid->cells);
        free(grid);
    }
}

// Cell functions
void cell_focus(Cell *cell, int y_offset, int x_offset) {
    console_move_cursor(
        cell->grid->window.line + cell->pos.y + y_offset,
        cell->grid->window.col + cell->pos.x + x_offset
    );
}

void cell_set_number(Cell *cell, int n, bool active) {
    if (!cell->index) cell->index = n;
    
    console_set_color(active ? g_colors.active_num : g_colors.num);
    cell_focus(cell, -1, -1);
    
    // Write small number
    if (n < 10) {
        console_write_char(UC_NUMS[n]);
    } else {
        console_write_char(UC_NUMS[n / 10]);
        console_write_char(UC_NUMS[n % 10]);
    }
    
    console_set_color(g_colors.grid);
}

void cell_set_block(Cell *cell) {
    cell->blocked = true;
    console_set_color(g_colors.block);
    cell_focus(cell, 0, -1);
    console_write_char(UC_BLOCK_L);
    console_write_char(UC_BLOCK_M);
    console_write_char(UC_BLOCK_R);
    console_set_color(g_colors.grid);
}

void cell_write(Cell *cell, char ch) {
    if (!cell->locked) {
        cell->buffer = ch;
        cell_focus(cell, 0, 0);
        console_write_char(ch);
    }
}

void cell_underline(Cell *cell) {
    // Windows console doesn't support underline well, so we'll use a different approach
    // Could use background color or other visual indicator
    cell_write(cell, cell->buffer);
}

void cell_color(Cell *cell, int color_pair) {
    console_set_color(color_pair);
    cell_write(cell, cell->buffer);
    console_set_color(g_colors.grid);
}

void cell_clear(Cell *cell) {
    cell->locked = false;
    cell->blocked = false;
    cell->buffer = ' ';
}