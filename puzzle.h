// database.h - Database management
#ifndef DATABASE_H
#define DATABASE_H

#include <stdbool.h>
#include "cliptic.h"

// Forward declaration
typedef struct Game Game;

// Database paths
#define DB_DIR_PATH "%USERPROFILE%\\.config\\cliptic\\db"
#define DB_FILE_PATH "%USERPROFILE%\\.config\\cliptic\\db\\cliptic.db"

// State structure
typedef struct {
    Date date;
    int time;
    char *chars_json;
    int n_done;
    int n_tot;
    int reveals;
    bool done;
    bool exists;
} GameState;

// Recent puzzle entry
typedef struct {
    Date date;
    Date play_date;
    char play_time[9]; // HH:MM:SS
} RecentEntry;

// Score entry
typedef struct {
    Date date;
    Date date_done;
    char time[9]; // HH:MM:SS
    int reveals;
} ScoreEntry;

// Database functions
bool db_init(void);
void db_close(void);

// State functions
GameState* state_new(Date date);
void state_free(GameState *state);
bool state_exists(Date date);
bool state_save(GameState *state, Game *game);
bool state_load(GameState *state);
bool state_delete(Date date);
void state_get_stats_string(Date date, char *buffer, size_t size);

// Recent puzzles functions
bool recents_add(Date date);
int recents_get_list(RecentEntry **entries, int limit);
void recents_free_list(RecentEntry *entries, int count);

// Scores functions
bool scores_add(Game *game);
int scores_get_list(ScoreEntry **entries, int limit);
void scores_free_list(ScoreEntry *entries, int count);

// Reset functions
bool db_reset_all(void);
bool db_reset_states(void);
bool db_reset_scores(void);
bool db_reset_recents(void);

#endif // DATABASE_H