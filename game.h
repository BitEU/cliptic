// game.h - Game logic and structures
#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "cliptic.h"
#include "puzzle.h"
#include "windows.h"
#include "interface.h"
#include "database.h"

// Forward declarations
typedef struct Game Game;
typedef struct Board Board;
typedef struct Timer Timer;
typedef struct Cursor Cursor;

// Timer structure
struct Timer {
    int time;
    bool running;
    TopBar *bar;
    void (*callback)(void);
};

// Cursor structure
struct Cursor {
    Grid *grid;
    Position pos;
};

// Board structure
struct Board {
    Puzzle *puzzle;
    Grid *grid;
    Window *cluebox;
    Cursor *cursor;
    Clue *current_clue;
    Direction dir;
};

// Game structure
struct Game {
    Date date;
    GameState *state;
    Board board;
    Timer timer;
    TopBar *top_bar;
    BottomBar *bottom_bar;
    GameMode mode;
    bool continue_game;
    bool unsaved;
};

// Game functions
Game* game_new(Date date);
void game_free(Game *game);
void game_play(Game *game);
void game_pause(Game *game);
void game_unpause(Game *game);
void game_save(Game *game);
void game_reset(Game *game);
void game_reveal(Game *game);
void game_exit(Game *game);
void game_redraw(Game *game);
char* game_generate_state_json(Game *game);

// Board functions
void board_setup(Board *board, GameState *state);
void board_update(Board *board);
void board_redraw(Board *board);
void board_move(Board *board, int y, int x);
void board_insert_char(Board *board, char ch, bool advance);
void board_delete_char(Board *board, bool advance);
void board_next_clue(Board *board, int n);
void board_prev_clue(Board *board, int n);
void board_to_start(Board *board);
void board_to_end(Board *board);
void board_swap_direction(Board *board);
void board_goto_clue(Board *board, int n);
void board_goto_cell(Board *board, int n);
void board_clear_clue(Board *board);
void board_reveal_clue(Board *board);
void board_clear_all(Board *board);

// Timer functions
void timer_start(Timer *timer);
void timer_stop(Timer *timer);
void timer_reset(Timer *timer);

// Cursor functions
void cursor_set(Cursor *cursor, int y, int x);
void cursor_move(Cursor *cursor, int y, int x);
void cursor_reset(Cursor *cursor);

// Controller functions
void game_handle_input(Game *game, int key);

#endif // GAME_H