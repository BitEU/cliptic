// database.c - Database management implementation (using SQLite)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <time.h>
#include "database.h"

// If sqlite3.h is not available, we'll need to either:
// 1. Download it from https://www.sqlite.org/download.html
// 2. Or define minimal stubs for compilation
#ifdef HAVE_SQLITE3
#include "sqlite3.h"
#else
// Minimal SQLite3 definitions for compilation without the library
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
#define SQLITE_OK 0
#define SQLITE_ROW 100
#define SQLITE_DONE 101
#define SQLITE_STATIC ((void(*)(void*))0)
#define SQLITE_TRANSIENT ((void(*)(void*))-1)

// Function prototypes (these would be implemented by the actual SQLite library)
int sqlite3_open(const char *filename, sqlite3 **ppDb) { return 0; }
int sqlite3_close(sqlite3 *db) { return 0; }
int sqlite3_exec(sqlite3 *db, const char *sql, void *callback, void *arg, char **errmsg) { return 0; }
int sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail) { return 0; }
int sqlite3_bind_text(sqlite3_stmt *stmt, int idx, const char *text, int n, void(*)(void*)) { return 0; }
int sqlite3_bind_int(sqlite3_stmt *stmt, int idx, int value) { return 0; }
int sqlite3_step(sqlite3_stmt *stmt) { return SQLITE_DONE; }
int sqlite3_finalize(sqlite3_stmt *stmt) { return 0; }
int sqlite3_column_int(sqlite3_stmt *stmt, int iCol) { return 0; }
const unsigned char *sqlite3_column_text(sqlite3_stmt *stmt, int iCol) { return NULL; }
#endif

static sqlite3 *db = NULL;

// SQL statements
static const char *SQL_CREATE_STATES = 
    "CREATE TABLE IF NOT EXISTS states ("
    "date DATE PRIMARY KEY, "
    "time INT, "
    "chars TEXT, "
    "n_done INT, "
    "n_tot INT, "
    "reveals INT, "
    "done INT)";

static const char *SQL_CREATE_RECENTS = 
    "CREATE TABLE IF NOT EXISTS recents ("
    "date DATE PRIMARY KEY, "
    "play_date DATE, "
    "play_time TIME)";

static const char *SQL_CREATE_SCORES = 
    "CREATE TABLE IF NOT EXISTS scores ("
    "date DATE, "
    "date_done DATE, "
    "time TEXT, "
    "reveals INT)";

bool db_init(void) {
    char db_dir[MAX_PATH];
    char db_file[MAX_PATH];
    
    // Expand environment variables
    ExpandEnvironmentStringsA(DB_DIR_PATH, db_dir, MAX_PATH);
    ExpandEnvironmentStringsA(DB_FILE_PATH, db_file, MAX_PATH);
    
    // Create directory if it doesn't exist
    _mkdir(db_dir);
    
    // Open database
    if (sqlite3_open(db_file, &db) != SQLITE_OK) {
        return false;
    }
    
    // Create tables
    sqlite3_exec(db, SQL_CREATE_STATES, NULL, NULL, NULL);
    sqlite3_exec(db, SQL_CREATE_RECENTS, NULL, NULL, NULL);
    sqlite3_exec(db, SQL_CREATE_SCORES, NULL, NULL, NULL);
    
    return true;
}

void db_close(void) {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

// State functions
GameState* state_new(Date date) {
    GameState *state = calloc(1, sizeof(GameState));
    state->date = date;
    state->exists = state_load(state);
    return state;
}

void state_free(GameState *state) {
    if (state) {
        if (state->chars_json) free(state->chars_json);
        free(state);
    }
}

bool state_exists(Date date) {
    char date_str[11];
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", date.year, date.month, date.day);
    
    sqlite3_stmt *stmt;
    const char *sql = "SELECT COUNT(*) FROM states WHERE date = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, date_str, -1, SQLITE_STATIC);
    
    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }
    
    sqlite3_finalize(stmt);
    return exists;
}

bool state_load(GameState *state) {
    char date_str[11];
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", state->date.year, state->date.month, state->date.day);
    
    sqlite3_stmt *stmt;
    const char *sql = "SELECT time, chars, n_done, n_tot, reveals, done FROM states WHERE date = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, date_str, -1, SQLITE_STATIC);
    
    bool loaded = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        state->time = sqlite3_column_int(stmt, 0);
        const char *chars = (const char*)sqlite3_column_text(stmt, 1);
        if (chars) {
            state->chars_json = strdup(chars);
        }
        state->n_done = sqlite3_column_int(stmt, 2);
        state->n_tot = sqlite3_column_int(stmt, 3);
        state->reveals = sqlite3_column_int(stmt, 4);
        state->done = sqlite3_column_int(stmt, 5) == 1;
        loaded = true;
    }
    
    sqlite3_finalize(stmt);
    return loaded;
}

bool state_save(GameState *state, Game *game) {
    char date_str[11];
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", state->date.year, state->date.month, state->date.day);
    
    // Generate JSON for current state
    char *chars_json = game_generate_state_json(game);
    
    const char *sql;
    if (state->exists) {
        sql = "UPDATE states SET time=?, chars=?, n_done=?, n_tot=?, reveals=?, done=? WHERE date=?";
    } else {
        sql = "INSERT INTO states (time, chars, n_done, n_tot, reveals, done, date) VALUES (?, ?, ?, ?, ?, ?, ?)";
    }
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(chars_json);
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, game->timer.time);
    sqlite3_bind_text(stmt, 2, chars_json, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, puzzle_count_done(game->board.puzzle));
    sqlite3_bind_int(stmt, 4, game->board.puzzle->clue_count);
    sqlite3_bind_int(stmt, 5, state->reveals);
    sqlite3_bind_int(stmt, 6, puzzle_is_complete(game->board.puzzle) ? 1 : 0);
    sqlite3_bind_text(stmt, 7, date_str, -1, SQLITE_STATIC);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    free(chars_json);
    
    if (success) state->exists = true;
    return success;
}

bool state_delete(Date date) {
    char date_str[11];
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", date.year, date.month, date.day);
    
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM states WHERE date = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, date_str, -1, SQLITE_STATIC);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

void state_get_stats_string(Date date, char *buffer, size_t size) {
    GameState *state = state_new(date);
    
    if (state->exists) {
        // Format time as HH:MM:SS
        int hours = state->time / 3600;
        int minutes = (state->time % 3600) / 60;
        int seconds = state->time % 60;
        
        snprintf(buffer, size, 
            "Time  │ %02d:%02d:%02d\n"
            "Clues │ %d/%d\n"
            "Done  │ [%c]",
            hours, minutes, seconds,
            state->n_done, state->n_tot,
            state->done ? L'✓' : ' ');
    } else {
        snprintf(buffer, size, "\n  Not attempted\n");
    }
    
    state_free(state);
}

// Recent puzzles functions
bool recents_add(Date date) {
    char date_str[11];
    char today_str[11];
    char time_str[9];
    
    Date today = date_today();
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", date.year, date.month, date.day);
    snprintf(today_str, sizeof(today_str), "%04d-%02d-%02d", today.year, today.month, today.day);
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    
    // Check if exists
    sqlite3_stmt *stmt;
    const char *check_sql = "SELECT COUNT(*) FROM recents WHERE date = ?";
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, date_str, -1, SQLITE_STATIC);
    
    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }
    sqlite3_finalize(stmt);
    
    // Insert or update
    const char *sql;
    if (exists) {
        sql = "UPDATE recents SET play_date=?, play_time=? WHERE date=?";
    } else {
        sql = "INSERT INTO recents (play_date, play_time, date) VALUES (?, ?, ?)";
    }
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, today_str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, time_str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, date_str, -1, SQLITE_STATIC);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

int recents_get_list(RecentEntry **entries, int limit) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT date, play_date, play_time FROM recents "
                     "ORDER BY play_date DESC, play_time DESC LIMIT ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, limit);
    
    // Count results first
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) count++;
    sqlite3_reset(stmt);
    
    if (count == 0) {
        sqlite3_finalize(stmt);
        return 0;
    }
    
    // Allocate and fill entries
    *entries = calloc(count, sizeof(RecentEntry));
    int i = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW && i < count) {
        const char *date_str = (const char*)sqlite3_column_text(stmt, 0);
        const char *play_date_str = (const char*)sqlite3_column_text(stmt, 1);
        const char *play_time_str = (const char*)sqlite3_column_text(stmt, 2);
        
        // Parse dates
        sscanf(date_str, "%d-%d-%d", 
               &(*entries)[i].date.year, &(*entries)[i].date.month, &(*entries)[i].date.day);
        sscanf(play_date_str, "%d-%d-%d", 
               &(*entries)[i].play_date.year, &(*entries)[i].play_date.month, &(*entries)[i].play_date.day);
        strncpy((*entries)[i].play_time, play_time_str, 8);
        
        i++;
    }
    
    sqlite3_finalize(stmt);
    return count;
}

void recents_free_list(RecentEntry *entries, int count) {
    free(entries);
}

// Score functions
bool scores_add(Game *game) {
    char date_str[11];
    char today_str[11];
    char time_str[9];
    
    Date today = date_today();
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", game->date.year, game->date.month, game->date.day);
    snprintf(today_str, sizeof(today_str), "%04d-%02d-%02d", today.year, today.month, today.day);
    
    // Format time
    int hours = game->timer.time / 3600;
    int minutes = (game->timer.time % 3600) / 60;
    int seconds = game->timer.time % 60;
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hours, minutes, seconds);
    
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO scores (date, date_done, time, reveals) VALUES (?, ?, ?, ?)";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, date_str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, today_str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, time_str, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, game->state->reveals);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

int scores_get_list(ScoreEntry **entries, int limit) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT date, date_done, time, reveals FROM scores "
                     "WHERE reveals = 0 ORDER BY time ASC LIMIT ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, limit);
    
    // Count results first
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) count++;
    sqlite3_reset(stmt);
    
    if (count == 0) {
        sqlite3_finalize(stmt);
        return 0;
    }
    
    // Allocate and fill entries
    *entries = calloc(count, sizeof(ScoreEntry));
    int i = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW && i < count) {
        const char *date_str = (const char*)sqlite3_column_text(stmt, 0);
        const char *date_done_str = (const char*)sqlite3_column_text(stmt, 1);
        const char *time_str = (const char*)sqlite3_column_text(stmt, 2);
        
        // Parse dates
        sscanf(date_str, "%d-%d-%d", 
               &(*entries)[i].date.year, &(*entries)[i].date.month, &(*entries)[i].date.day);
        sscanf(date_done_str, "%d-%d-%d", 
               &(*entries)[i].date_done.year, &(*entries)[i].date_done.month, &(*entries)[i].date_done.day);
        strncpy((*entries)[i].time, time_str, 8);
        (*entries)[i].reveals = sqlite3_column_int(stmt, 3);
        
        i++;
    }
    
    sqlite3_finalize(stmt);
    return count;
}

void scores_free_list(ScoreEntry *entries, int count) {
    free(entries);
}

// Reset functions
bool db_reset_all(void) {
    char db_file[MAX_PATH];
    ExpandEnvironmentStringsA(DB_FILE_PATH, db_file, MAX_PATH);
    
    db_close();
    return remove(db_file) == 0;
}

bool db_reset_states(void) {
    return sqlite3_exec(db, "DROP TABLE IF EXISTS states", NULL, NULL, NULL) == SQLITE_OK &&
           sqlite3_exec(db, SQL_CREATE_STATES, NULL, NULL, NULL) == SQLITE_OK;
}

bool db_reset_scores(void) {
    return sqlite3_exec(db, "DROP TABLE IF EXISTS scores", NULL, NULL, NULL) == SQLITE_OK &&
           sqlite3_exec(db, SQL_CREATE_SCORES, NULL, NULL, NULL) == SQLITE_OK;
}

bool db_reset_recents(void) {
    return sqlite3_exec(db, "DROP TABLE IF EXISTS recents", NULL, NULL, NULL) == SQLITE_OK &&
           sqlite3_exec(db, SQL_CREATE_RECENTS, NULL, NULL, NULL) == SQLITE_OK;
}