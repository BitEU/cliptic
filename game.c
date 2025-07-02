// game.c - Game logic implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>
#include "game.h"
#include "screen.h"
#include "config.h"
#include "menus.h"

// Game implementation
Game* game_new(Date date) {
    Game *game = calloc(1, sizeof(Game));
    game->date = date;
    
    // Initialize state
    game->state = state_new(date);
    
    // Initialize board
    game->board.puzzle = puzzle_new(date);
    if (!game->board.puzzle) {
        state_free(game->state);
        free(game);
        return NULL;
    }
    
    game->board.grid = grid_new(game->board.puzzle->size.y, 
                               game->board.puzzle->size.x, 1, -1);
    
    // Create cluebox
    game->board.cluebox = calloc(1, sizeof(Window));
    int lines, cols;
    screen_get_size(&lines, &cols);
    window_init(game->board.cluebox, lines - game->board.grid->window.y - 2, 
                0, game->board.grid->window.y + 1, 0);
    
    // Create cursor
    game->board.cursor = calloc(1, sizeof(Cursor));
    game->board.cursor->grid = game->board.grid;
    
    // Initialize timer
    game->timer.time = game->state->time;
    game->timer.running = false;
    game->timer.callback = NULL;
    
    // Create UI elements
    game->top_bar = top_bar_new(date);
    game->bottom_bar = bottom_bar_new();
    
    // Set initial values
    game->mode = MODE_NORMAL;
    game->continue_game = true;
    game->unsaved = false;
    
    // Link cells to clues
    for (int i = 0; i < game->board.puzzle->clue_count; i++) {
        Clue *clue = game->board.puzzle->clues[i];
        clue->cells = calloc(clue->length, sizeof(Cell*));
        for (int j = 0; j < clue->length; j++) {
            clue->cells[j] = grid_get_cell(game->board.grid, 
                                          clue->coords[j].y, 
                                          clue->coords[j].x);
        }
    }
    
    return game;
}

void game_free(Game *game) {
    if (!game) return;
    
    state_free(game->state);
    puzzle_free(game->board.puzzle);
    grid_free(game->board.grid);
    free(game->board.cluebox);
    free(game->board.cursor);
    top_bar_free(game->top_bar);
    bottom_bar_free(game->bottom_bar);
    free(game);
}

// Timer thread function
static unsigned __stdcall timer_thread(void *arg) {
    Timer *timer = (Timer*)arg;
    
    while (timer->running) {
        Sleep(1000);
        if (timer->running) {
            timer->time++;
            if (timer->callback) timer->callback();
        }
    }
    
    return 0;
}

void timer_start(Timer *timer) {
    timer->running = true;
    _beginthreadex(NULL, 0, timer_thread, timer, 0, NULL);
}

void timer_stop(Timer *timer) {
    timer->running = false;
}

void timer_reset(Timer *timer) {
    timer->running = false;
    timer->time = 0;
}

// Board functions
void board_setup(Board *board, GameState *state) {
    // Draw grid
    grid_draw(board->grid);
    
    // Add indices
    for (int i = 0; i < board->puzzle->clue_count; i++) {
        Clue *clue = board->puzzle->clues[i];
        if (clue->index > 0) {
            Cell *cell = grid_get_cell(board->grid, clue->start.y, clue->start.x);
            cell_set_number(cell, clue->index, false);
        }
    }
    
    // Add blocks
    for (int i = 0; i < board->puzzle->block_count; i++) {
        Cell *cell = grid_get_cell(board->grid, 
                                  board->puzzle->blocks[i].y,
                                  board->puzzle->blocks[i].x);
        cell_set_block(cell);
    }
    
    // Load saved state if exists
    if (state && state->exists && state->chars_json) {
        // Parse JSON and restore cell states
        // This would need a JSON parser implementation
    }
    
    // Set initial clue
    if (!board->current_clue) {
        board->current_clue = puzzle_get_first_clue(board->puzzle);
        board->dir = board->current_clue->dir;
        cursor_set(board->cursor, board->current_clue->start.y, 
                                 board->current_clue->start.x);
    }
    
    board_update(board);
}

void board_update(Board *board) {
    cursor_reset(board->cursor);
    // Refresh would be called here if needed
}

void board_redraw(Board *board) {
    grid_draw(board->grid);
    
    // Redraw all cells
    for (int y = 0; y < board->grid->sq.y; y++) {
        for (int x = 0; x < board->grid->sq.x; x++) {
            Cell *cell = grid_get_cell(board->grid, y, x);
            if (cell->buffer != ' ' && !cell->blocked) {
                cell->locked = false;
                cell_write(cell, cell->buffer);
            }
        }
    }
    
    if (g_config.auto_mark) {
        puzzle_check_all(board->puzzle);
    }
    
    if (board->current_clue) {
        clue_activate(board->current_clue);
    }
}

void board_move(Board *board, int y, int x) {
    cursor_move(board->cursor, y, x);
    
    Cell *cell = grid_get_cell(board->grid, board->cursor->pos.y, board->cursor->pos.x);
    if (cell && cell->blocked) {
        board_move(board, y, x); // Continue moving
    } else if (!clue_has_cell(board->current_clue, board->cursor->pos.y, board->cursor->pos.x)) {
        // Switch to clue at new position
        Clue *new_clue = puzzle_get_clue(board->puzzle, 
                                        board->cursor->pos.y, 
                                        board->cursor->pos.x, 
                                        board->dir);
        if (new_clue) {
            clue_deactivate(board->current_clue);
            board->current_clue = new_clue;
            board->dir = new_clue->dir;
            clue_activate(board->current_clue);
            
            // Update cluebox
            window_draw(board->cluebox, g_colors.cluebox);
            console_set_color(board->current_clue->done ? g_colors.correct : g_colors.meta);
            window_add_str(board->cluebox, 0, 2, clue_get_meta(board->current_clue));
            window_wrap_str(board->cluebox, 1, board->current_clue->hint);
        }
    }
}

void board_insert_char(Board *board, char ch, bool advance) {
    Cell *cell = grid_get_cell(board->grid, board->cursor->pos.y, board->cursor->pos.x);
    if (cell) {
        cell_write(cell, toupper(ch));
        
        if (advance) {
            // Check if we're at the end of the clue
            bool at_end = true;
            for (int i = 0; i < board->current_clue->length - 1; i++) {
                if (board->current_clue->cells[i] == cell) {
                    at_end = false;
                    break;
                }
            }
            
            if (at_end && g_config.auto_advance) {
                board_next_clue(board, 1);
            } else {
                board_move(board, 
                          board->dir == DIR_DOWN ? 1 : 0,
                          board->dir == DIR_ACROSS ? 1 : 0);
            }
        }
        
        if (g_config.auto_mark) {
            clue_check(board->current_clue);
        }
    }
}

void board_delete_char(Board *board, bool advance) {
    Cell *cell = grid_get_cell(board->grid, board->cursor->pos.y, board->cursor->pos.x);
    if (cell) {
        cell_write(cell, ' ');
        cell_underline(cell);
        
        if (advance) {
            // Check if we're at the start
            if (board->current_clue->cells[0] != cell) {
                board_move(board, 
                          board->dir == DIR_DOWN ? -1 : 0,
                          board->dir == DIR_ACROSS ? -1 : 0);
            }
        }
    }
}

void board_next_clue(Board *board, int n) {
    for (int i = 0; i < n; i++) {
        clue_deactivate(board->current_clue);
        board->current_clue = board->current_clue->next;
        board->dir = board->current_clue->dir;
        cursor_set(board->cursor, board->current_clue->start.y, board->current_clue->start.x);
        clue_activate(board->current_clue);
    }
    
    // Skip completed clues if not all done
    if (board->current_clue->done && !puzzle_is_complete(board->puzzle)) {
        board_next_clue(board, 1);
    }
}

void board_prev_clue(Board *board, int n) {
    for (int i = 0; i < n; i++) {
        // If at start of clue, go to previous
        if (board->current_clue->cells[0] == 
            grid_get_cell(board->grid, board->cursor->pos.y, board->cursor->pos.x)) {
            clue_deactivate(board->current_clue);
            board->current_clue = board->current_clue->prev;
            board->dir = board->current_clue->dir;
            cursor_set(board->cursor, 
                      board->current_clue->coords[board->current_clue->length-1].y,
                      board->current_clue->coords[board->current_clue->length-1].x);
            clue_activate(board->current_clue);
        } else {
            board_to_start(board);
        }
    }
    
    // Skip completed clues
    if (board->current_clue->done && !puzzle_is_complete(board->puzzle)) {
        board_prev_clue(board, 1);
    }
}

void board_to_start(Board *board) {
    cursor_set(board->cursor, board->current_clue->start.y, board->current_clue->start.x);
}

void board_to_end(Board *board) {
    cursor_set(board->cursor, 
              board->current_clue->coords[board->current_clue->length-1].y,
              board->current_clue->coords[board->current_clue->length-1].x);
}

void board_swap_direction(Board *board) {
    Direction new_dir = (board->dir == DIR_ACROSS) ? DIR_DOWN : DIR_ACROSS;
    Clue *new_clue = puzzle_get_clue(board->puzzle, 
                                    board->cursor->pos.y, 
                                    board->cursor->pos.x, 
                                    new_dir);
    if (new_clue) {
        clue_deactivate(board->current_clue);
        board->current_clue = new_clue;
        board->dir = new_dir;
        clue_activate(board->current_clue);
    }
}

void board_goto_clue(Board *board, int n) {
    Clue *clue = puzzle_get_clue_by_index(board->puzzle, n, board->dir);
    if (clue) {
        clue_deactivate(board->current_clue);
        board->current_clue = clue;
        board->dir = clue->dir;
        cursor_set(board->cursor, clue->start.y, clue->start.x);
        clue_activate(board->current_clue);
    }
}

void board_goto_cell(Board *board, int n) {
    if (n > 0 && n <= board->current_clue->length) {
        cursor_set(board->cursor, 
                  board->current_clue->coords[n-1].y,
                  board->current_clue->coords[n-1].x);
    }
}

void board_clear_clue(Board *board) {
    clue_clear(board->current_clue);
    clue_activate(board->current_clue);
}

void board_reveal_clue(Board *board) {
    clue_reveal(board->current_clue);
    board_next_clue(board, 1);
}

void board_clear_all(Board *board) {
    for (int y = 0; y < board->grid->sq.y; y++) {
        for (int x = 0; x < board->grid->sq.x; x++) {
            Cell *cell = grid_get_cell(board->grid, y, x);
            if (!cell->blocked) {
                cell_clear(cell);
            }
        }
    }
    
    for (int i = 0; i < board->puzzle->clue_count; i++) {
        board->puzzle->clues[i]->done = false;
    }
}

// Cursor functions
void cursor_set(Cursor *cursor, int y, int x) {
    cursor->pos.y = y;
    cursor->pos.x = x;
}

void cursor_move(Cursor *cursor, int y, int x) {
    cursor->pos.y += y;
    cursor->pos.x += x;
    
    // Wrap around
    while (cursor->pos.x < 0) cursor->pos.x += cursor->grid->sq.x;
    while (cursor->pos.y < 0) cursor->pos.y += cursor->grid->sq.y;
    while (cursor->pos.x >= cursor->grid->sq.x) cursor->pos.x -= cursor->grid->sq.x;
    while (cursor->pos.y >= cursor->grid->sq.y) cursor->pos.y -= cursor->grid->sq.y;
}

void cursor_reset(Cursor *cursor) {
    Cell *cell = grid_get_cell(cursor->grid, cursor->pos.y, cursor->pos.x);
    if (cell) {
        cell_focus(cell, 0, 0);
        console_set_cursor_visible(true);
    }
}

// Game functions
void game_play(Game *game) {
    if (game->state->done) {
        menu_puzzle_complete_show();
        return;
    }
    
    // Add to recent puzzles
    recents_add(game->date);
    
    // Draw UI
    top_bar_draw(game->top_bar);
    bottom_bar_draw(game->bottom_bar);
    bottom_bar_mode(game->bottom_bar, game->mode);
    
    // Setup board
    board_setup(&game->board, game->state);
    
    // Start timer
    timer_start(&game->timer);
    
    // Main game loop
    while (game->continue_game && !puzzle_is_complete(game->board.puzzle)) {
        int key = console_get_key();
        game_handle_input(game, key);
        board_update(&game->board);
    }
    
    // Stop timer
    timer_stop(&game->timer);
    
    // Save if needed
    if (puzzle_is_complete(game->board.puzzle)) {
        game_save(game);
        scores_add(game);
        menu_puzzle_complete_show();
    } else if (g_config.auto_save) {
        game_save(game);
    }
}

void game_pause(Game *game) {
    timer_stop(&game->timer);
    menu_pause_show(game);
}

void game_unpause(Game *game) {
    timer_start(&game->timer);
    board_redraw(&game->board);
}

void game_save(Game *game) {
    state_save(game->state, game);
    
    // Show "Saved!" message
    console_set_color(g_colors.bar);
    console_move_cursor(game->bottom_bar->window.line, 10);
    console_write_string(L"Saved!");
    Sleep(1000);
    console_move_cursor(game->bottom_bar->window.line, 10);
    console_write_string(L"      ");
    
    game->unsaved = false;
    bottom_bar_unsaved(game->bottom_bar, false);
}

void game_reset(Game *game) {
    if (game->state->exists) {
        state_delete(game->date);
    }
    board_clear_all(&game->board);
    timer_reset(&game->timer);
}

void game_reveal(Game *game) {
    game->state->reveals++;
    board_reveal_clue(&game->board);
}

void game_exit(Game *game) {
    game->continue_game = false;
    screen_clear();
}

void game_redraw(Game *game) {
    game_save(game);
    screen_clear();
    
    // Redraw everything
    top_bar_draw(game->top_bar);
    bottom_bar_draw(game->bottom_bar);
    bottom_bar_mode(game->bottom_bar, game->mode);
    board_redraw(&game->board);
}

// Generate JSON state (simplified)
char* game_generate_state_json(Game *game) {
    // Calculate needed size
    int size = 1024; // Base size
    size += game->board.grid->sq.y * game->board.grid->sq.x * 50; // Per cell
    
    char *json = malloc(size);
    strcpy(json, "[");
    
    bool first = true;
    for (int y = 0; y < game->board.grid->sq.y; y++) {
        for (int x = 0; x < game->board.grid->sq.x; x++) {
            Cell *cell = grid_get_cell(game->board.grid, y, x);
            if (!cell->blocked && cell->buffer != ' ') {
                if (!first) strcat(json, ",");
                
                char cell_json[100];
                snprintf(cell_json, sizeof(cell_json), 
                        "{\"sq\":{\"y\":%d,\"x\":%d},\"char\":\"%c\"}",
                        y, x, cell->buffer);
                strcat(json, cell_json);
                first = false;
            }
        }
    }
    
    strcat(json, "]");
    return json;
}

// Input handling
static int await_number = 0;
static void (*await_callback)(Game*, int) = NULL;

// Forward declarations for static functions
static void game_handle_number_command(Game *game, int key);
static void game_handle_replace(Game *game, int key);
static void game_handle_delete(Game *game, int key);
static void game_handle_change(Game *game, int key);

void game_handle_input(Game *game, int key) {
    // Handle control keys
    if (key >= 1 && key <= 26) {
        switch (key) {
            case 3:  // Ctrl+C
                game_exit(game);
                break;
            case 5:  // Ctrl+E
                menu_reset_progress_show(game);
                break;
            case 7:  // Ctrl+G
                puzzle_check_all(game->board.puzzle);
                break;
            case 12: // Ctrl+L
                game_redraw(game);
                break;
            case 16: // Ctrl+P
                game_pause(game);
                break;
            case 18: // Ctrl+R
                game_reveal(game);
                break;
            case 19: // Ctrl+S
                game_save(game);
                break;
        }
        return;
    }
    
    // Handle arrow keys
    if (key >= 258 && key <= 261) {
        switch (key) {
            case 258: // Down
                board_move(&game->board, 1, 0);
                break;
            case 259: // Up
                board_move(&game->board, -1, 0);
                break;
            case 260: // Left
                board_move(&game->board, 0, -1);
                break;
            case 261: // Right
                board_move(&game->board, 0, 1);
                break;
        }
        return;
    }
    
    // Handle mode-specific input
    if (game->mode == MODE_INSERT) {
        switch (key) {
            case 27: // Escape
                game->mode = MODE_NORMAL;
                bottom_bar_mode(game->bottom_bar, game->mode);
                break;
            case 127: // Backspace
                board_delete_char(&game->board, true);
                game->unsaved = true;
                bottom_bar_unsaved(game->bottom_bar, true);
                break;
            default:
                if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z')) {
                    board_insert_char(&game->board, key, true);
                    game->unsaved = true;
                    bottom_bar_unsaved(game->bottom_bar, true);
                }
                break;
        }
    } else { // MODE_NORMAL
        // Handle number input
        if (key >= '0' && key <= '9') {
            await_number = await_number * 10 + (key - '0');
            await_callback = game_handle_number_command;
            return;
        }
        
        // If we have a pending number, handle it
        if (await_callback) {
            await_callback(game, key);
            await_callback = NULL;
            await_number = 0;
            return;
        }
        
        // Normal mode commands
        switch (key) {
            case 'j':
                board_move(&game->board, 1, 0);
                break;
            case 'k':
                board_move(&game->board, -1, 0);
                break;
            case 'h':
                board_move(&game->board, 0, -1);
                break;
            case 'l':
                board_move(&game->board, 0, 1);
                break;
            case 'i':
                game->mode = MODE_INSERT;
                bottom_bar_mode(game->bottom_bar, game->mode);
                break;
            case 'I':
                board_to_start(&game->board);
                game->mode = MODE_INSERT;
                bottom_bar_mode(game->bottom_bar, game->mode);
                break;
            case 'a':
                board_move(&game->board, 
                          game->board.dir == DIR_DOWN ? 1 : 0,
                          game->board.dir == DIR_ACROSS ? 1 : 0);
                game->mode = MODE_INSERT;
                bottom_bar_mode(game->bottom_bar, game->mode);
                break;
            case 'w':
                board_next_clue(&game->board, 1);
                break;
            case 'b':
                board_prev_clue(&game->board, 1);
                break;
            case 'e':
                board_to_end(&game->board);
                break;
            case 'x':
                board_delete_char(&game->board, false);
                break;
            case 'r':
                await_callback = game_handle_replace;
                break;
            case 'd':
                await_callback = game_handle_delete;
                break;
            case 'c':
                await_callback = game_handle_change;
                break;
            case 9: // Tab
                board_swap_direction(&game->board);
                break;
        }
    }
}

static void game_handle_number_command(Game *game, int key) {
    int n = await_number > 0 ? await_number : 1;
    
    switch (key) {
        case 'g':
            board_goto_clue(&game->board, n);
            break;
        case 'G':
            board_goto_cell(&game->board, n);
            break;
        case 'w':
            board_next_clue(&game->board, n);
            break;
        case 'b':
            board_prev_clue(&game->board, n);
            break;
        case 'j':
            board_move(&game->board, n, 0);
            break;
        case 'k':
            board_move(&game->board, -n, 0);
            break;
        case 'h':
            board_move(&game->board, 0, -n);
            break;
        case 'l':
            board_move(&game->board, 0, n);
            break;
        default:
            // Not a number command, handle normally
            await_number = 0;
            game_handle_input(game, key);
            break;
    }
}

static void game_handle_replace(Game *game, int key) {
    if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z')) {
        board_insert_char(&game->board, key, false);
    }
}

static void game_handle_delete(Game *game, int key) {
    if (key == 'w') {
        board_clear_clue(&game->board);
    }
}

static void game_handle_change(Game *game, int key) {
    game_handle_delete(game, key);
    game->mode = MODE_INSERT;
    bottom_bar_mode(game->bottom_bar, game->mode);
}