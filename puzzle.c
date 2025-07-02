// puzzle.c - Puzzle implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <sys/stat.h>
#include "puzzle.h"
#include "config.h"
#include "game.h"  // For game_generate_state_json

// If curl is not available, define minimal stubs
#ifdef HAVE_CURL
#include <curl/curl.h>
#else
// Minimal CURL definitions for compilation without the library
typedef void CURL;
typedef int CURLcode;
#define CURLE_OK 0
#define CURLOPT_URL 10002
#define CURLOPT_WRITEFUNCTION 20011
#define CURLOPT_WRITEDATA 10001
#define CURLOPT_SSL_VERIFYPEER 64

void curl_global_init(int flags) {}
void curl_global_cleanup(void) {}
CURL *curl_easy_init(void) { return NULL; }
void curl_easy_cleanup(CURL *curl) {}
CURLcode curl_easy_setopt(CURL *curl, int option, ...) { return CURLE_OK; }
CURLcode curl_easy_perform(CURL *curl) { return CURLE_OK; }
#endif

// For now, we'll implement a simple JSON parser or use stubs
// In production, you would use a library like cJSON

#define PUZZLE_URL "https://data.puzzlexperts.com/puzzleapp-v3/data.php"
#define PUZZLE_PSID "100000160"
#define CACHE_PATH "%USERPROFILE%\\.cache\\cliptic"

// Memory callback for CURL
struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

// Fetch puzzle data from server
char* fetch_puzzle_data(Date date) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {0};
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if (!curl) return NULL;
    
    // Build URL with parameters
    char url[512];
    snprintf(url, sizeof(url), "%s?date=%04d-%02d-%02d&psid=%s",
             PUZZLE_URL, date.year, date.month, date.day, PUZZLE_PSID);
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    
    if (res != CURLE_OK) {
        if (chunk.memory) free(chunk.memory);
        return NULL;
    }
    
    return chunk.memory;
}

// Cache puzzle data
bool cache_puzzle_data(Date date, const char *data) {
    char cache_dir[MAX_PATH];
    char cache_file[MAX_PATH];
    
    ExpandEnvironmentStringsA(CACHE_PATH, cache_dir, MAX_PATH);
    _mkdir(cache_dir);
    
    snprintf(cache_file, sizeof(cache_file), "%s\\%04d-%02d-%02d",
             cache_dir, date.year, date.month, date.day);
    
    FILE *fp = fopen(cache_file, "w");
    if (!fp) return false;
    
    fprintf(fp, "%s", data);
    fclose(fp);
    return true;
}

// Load cached puzzle
char* load_cached_puzzle(Date date) {
    char cache_file[MAX_PATH];
    char temp[MAX_PATH];
    
    ExpandEnvironmentStringsA(CACHE_PATH, temp, MAX_PATH);
    snprintf(cache_file, sizeof(cache_file), "%s\\%04d-%02d-%02d",
             temp, date.year, date.month, date.day);
    
    struct stat st;
    if (stat(cache_file, &st) != 0) return NULL;
    
    FILE *fp = fopen(cache_file, "r");
    if (!fp) return NULL;
    
    char *data = malloc(st.st_size + 1);
    if (!data) {
        fclose(fp);
        return NULL;
    }
    
    fread(data, 1, st.st_size, fp);
    data[st.st_size] = '\0';
    fclose(fp);
    
    return data;
}

// Clue implementation
Clue* clue_new(const char *answer, const char *hint, Direction dir, Position start) {
    Clue *clue = calloc(1, sizeof(Clue));
    
    clue->length = strlen(answer);
    clue->answer = strdup(answer);
    clue->dir = dir;
    clue->start = start;
    clue->done = false;
    
    // Parse hint - add length if not present
    if (strstr(hint, "(") && strstr(hint, ")")) {
        clue->hint = strdup(hint);
    } else {
        clue->hint = malloc(strlen(hint) + 10);
        sprintf(clue->hint, "%s (%d)", hint, clue->length);
    }
    
    // Map coordinates
    clue->coords = calloc(clue->length, sizeof(Position));
    for (int i = 0; i < clue->length; i++) {
        if (dir == DIR_ACROSS) {
            clue->coords[i].y = start.y;
            clue->coords[i].x = start.x + i;
        } else {
            clue->coords[i].y = start.y + i;
            clue->coords[i].x = start.x;
        }
    }
    
    return clue;
}

void clue_free(Clue *clue) {
    if (clue) {
        free(clue->answer);
        free(clue->hint);
        free(clue->coords);
        free(clue);
    }
}

void clue_activate(Clue *clue) {
    if (clue->cells[0]) {
        cell_set_number(clue->cells[0], clue->index, true);
    }
    for (int i = 0; i < clue->length; i++) {
        if (clue->cells[i]) {
            cell_underline(clue->cells[i]);
        }
    }
}

void clue_deactivate(Clue *clue) {
    if (clue->cells[0]) {
        cell_set_number(clue->cells[0], clue->index, false);
    }
    for (int i = 0; i < clue->length; i++) {
        if (clue->cells[i]) {
            cell_write(clue->cells[i], clue->cells[i]->buffer);
        }
    }
    if (g_config.auto_mark) {
        clue_check(clue);
    }
}

bool clue_has_cell(Clue *clue, int y, int x) {
    for (int i = 0; i < clue->length; i++) {
        if (clue->coords[i].y == y && clue->coords[i].x == x) {
            return true;
        }
    }
    return false;
}

void clue_check(Clue *clue) {
    if (clue_is_full(clue)) {
        bool correct = true;
        for (int i = 0; i < clue->length; i++) {
            if (clue->cells[i]->buffer != clue->answer[i]) {
                correct = false;
                break;
            }
        }
        
        if (correct) {
            // Mark correct
            for (int i = 0; i < clue->length; i++) {
                cell_color(clue->cells[i], g_colors.correct);
                clue->cells[i]->locked = true;
            }
            clue->done = true;
        } else {
            // Mark incorrect
            for (int i = 0; i < clue->length; i++) {
                cell_color(clue->cells[i], g_colors.incorrect);
            }
        }
    }
}

bool clue_is_full(Clue *clue) {
    for (int i = 0; i < clue->length; i++) {
        if (clue->cells[i]->buffer == ' ') {
            return false;
        }
    }
    return true;
}

void clue_clear(Clue *clue) {
    for (int i = 0; i < clue->length; i++) {
        cell_write(clue->cells[i], ' ');
    }
}

void clue_reveal(Clue *clue) {
    for (int i = 0; i < clue->length; i++) {
        cell_write(clue->cells[i], clue->answer[i]);
    }
    // Mark as correct
    for (int i = 0; i < clue->length; i++) {
        cell_color(clue->cells[i], g_colors.correct);
        clue->cells[i]->locked = true;
    }
    clue->done = true;
}

const char* clue_get_meta(Clue *clue) {
    static char meta[32];
    snprintf(meta, sizeof(meta), "%d %s", 
             clue->index, 
             clue->dir == DIR_ACROSS ? "across" : "down");
    return meta;
}

// Puzzle implementation
Puzzle* puzzle_new(Date date) {
    Puzzle *puzzle = calloc(1, sizeof(Puzzle));
    
    // Try to load from cache first
    char *data = load_cached_puzzle(date);
    if (!data) {
        // Fetch from server
        data = fetch_puzzle_data(date);
        if (!data) {
            free(puzzle);
            return NULL;
        }
        // Cache for future use
        cache_puzzle_data(date, data);
    }
    
    // Parse the data
    if (!parse_puzzle_data(data, puzzle)) {
        free(data);
        free(puzzle);
        return NULL;
    }
    
    free(data);
    
    // Index clues
    puzzle_index_clues(puzzle);
    
    // Map clues
    puzzle_map_clues(puzzle);
    
    // Find blocks
    puzzle_find_blocks(puzzle);
    
    // Chain clues
    puzzle_chain_clues(puzzle);
    
    return puzzle;
}

void puzzle_free(Puzzle *puzzle) {
    if (!puzzle) return;
    
    // Free clues
    for (int i = 0; i < puzzle->clue_count; i++) {
        clue_free(puzzle->clues[i]);
    }
    free(puzzle->clues);
    
    // Free maps
    if (puzzle->map_chars) {
        for (int y = 0; y < puzzle->size.y; y++) {
            for (int x = 0; x < puzzle->size.x; x++) {
                free(puzzle->map_chars[y][x]);
            }
            free(puzzle->map_chars[y]);
        }
        free(puzzle->map_chars);
    }
    
    if (puzzle->map_index) {
        for (int d = 0; d < 2; d++) {
            for (int y = 0; y < puzzle->size.y; y++) {
                free(puzzle->map_index[d][y]);
            }
            free(puzzle->map_index[d]);
        }
        free(puzzle->map_index);
    }
    
    free(puzzle->indices);
    free(puzzle->blocks);
    free(puzzle);
}

// Parse puzzle data (simplified - you'll need a proper JSON parser)
bool parse_puzzle_data(const char *data, Puzzle *puzzle) {
    // This is a simplified parser - in reality you'd use a JSON library
    // For now, let's assume we can extract the data somehow
    
    // Extract size
    const char *rows_str = strstr(data, "rows=");
    const char *cols_str = strstr(data, "columns=");
    if (!rows_str || !cols_str) return false;
    
    sscanf(rows_str, "rows=%d", &puzzle->size.y);
    sscanf(cols_str, "columns=%d", &puzzle->size.x);
    
    // Parse clues from JSON
    // This would need a proper JSON parser implementation
    // For now, we'll create some dummy data for testing
    
    // Dummy implementation - replace with actual JSON parsing
    puzzle->clue_count = 10; // Example
    puzzle->clues = calloc(puzzle->clue_count, sizeof(Clue*));
    
    // Create some test clues
    for (int i = 0; i < puzzle->clue_count; i++) {
        char answer[] = "TEST";
        char hint[64];
        snprintf(hint, sizeof(hint), "Test clue %d", i + 1);
        Position start = {i / 5, (i % 5) * 2};
        Direction dir = (i % 2) ? DIR_ACROSS : DIR_DOWN;
        
        puzzle->clues[i] = clue_new(answer, hint, dir, start);
    }
    
    return true;
}

// Add missing static function declarations
static void puzzle_index_clues(Puzzle *puzzle);
static void puzzle_map_clues(Puzzle *puzzle);
static void puzzle_find_blocks(Puzzle *puzzle);
static void puzzle_chain_clues(Puzzle *puzzle);

static void puzzle_index_clues(Puzzle *puzzle) {
    // Find unique starting positions
    Position *starts = calloc(puzzle->clue_count, sizeof(Position));
    int unique_count = 0;
    
    for (int i = 0; i < puzzle->clue_count; i++) {
        bool found = false;
        for (int j = 0; j < unique_count; j++) {
            if (starts[j].y == puzzle->clues[i]->start.y &&
                starts[j].x == puzzle->clues[i]->start.x) {
                found = true;
                break;
            }
        }
        if (!found) {
            starts[unique_count++] = puzzle->clues[i]->start;
        }
    }
    
    // Sort positions
    for (int i = 0; i < unique_count - 1; i++) {
        for (int j = i + 1; j < unique_count; j++) {
            if (starts[i].y > starts[j].y ||
                (starts[i].y == starts[j].y && starts[i].x > starts[j].x)) {
                Position temp = starts[i];
                starts[i] = starts[j];
                starts[j] = temp;
            }
        }
    }
    
    // Assign indices
    puzzle->indices = calloc(unique_count + 1, sizeof(int));
    for (int i = 0; i < unique_count; i++) {
        for (int j = 0; j < puzzle->clue_count; j++) {
            if (puzzle->clues[j]->start.y == starts[i].y &&
                puzzle->clues[j]->start.x == starts[i].x) {
                puzzle->clues[j]->index = i + 1;
            }
        }
    }
    
    free(starts);
}

static void puzzle_map_clues(Puzzle *puzzle) {
    // Allocate maps
    puzzle->map_chars = calloc(puzzle->size.y, sizeof(char**));
    puzzle->map_index = calloc(2, sizeof(Clue***));
    
    for (int y = 0; y < puzzle->size.y; y++) {
        puzzle->map_chars[y] = calloc(puzzle->size.x, sizeof(char*));
        for (int x = 0; x < puzzle->size.x; x++) {
            puzzle->map_chars[y][x] = malloc(2);
            strcpy(puzzle->map_chars[y][x], ".");
        }
    }
    
    for (int d = 0; d < 2; d++) {
        puzzle->map_index[d] = calloc(puzzle->size.y, sizeof(Clue**));
        for (int y = 0; y < puzzle->size.y; y++) {
            puzzle->map_index[d][y] = calloc(puzzle->size.x, sizeof(Clue*));
        }
    }
    
    // Map clues
    for (int i = 0; i < puzzle->clue_count; i++) {
        Clue *clue = puzzle->clues[i];
        int dir_idx = (clue->dir == DIR_ACROSS) ? 0 : 1;
        
        for (int j = 0; j < clue->length; j++) {
            int y = clue->coords[j].y;
            int x = clue->coords[j].x;
            
            puzzle->map_index[dir_idx][y][x] = clue;
            puzzle->map_chars[y][x][0] = clue->answer[j];
        }
    }
}

static void puzzle_find_blocks(Puzzle *puzzle) {
    puzzle->block_count = 0;
    puzzle->blocks = calloc(puzzle->size.y * puzzle->size.x, sizeof(Position));
    
    for (int y = 0; y < puzzle->size.y; y++) {
        for (int x = 0; x < puzzle->size.x; x++) {
            if (puzzle->map_chars[y][x][0] == '.') {
                puzzle->blocks[puzzle->block_count].y = y;
                puzzle->blocks[puzzle->block_count].x = x;
                puzzle->block_count++;
            }
        }
    }
    
    puzzle->blocks = realloc(puzzle->blocks, puzzle->block_count * sizeof(Position));
}

static void puzzle_chain_clues(Puzzle *puzzle) {
    // Sort clues by direction and index
    Clue **across = calloc(puzzle->clue_count, sizeof(Clue*));
    Clue **down = calloc(puzzle->clue_count, sizeof(Clue*));
    int across_count = 0, down_count = 0;
    
    for (int i = 0; i < puzzle->clue_count; i++) {
        if (puzzle->clues[i]->dir == DIR_ACROSS) {
            across[across_count++] = puzzle->clues[i];
        } else {
            down[down_count++] = puzzle->clues[i];
        }
    }
    
    // Chain across clues
    for (int i = 0; i < across_count; i++) {
        if (i < across_count - 1) {
            across[i]->next = across[i + 1];
        } else {
            across[i]->next = down_count > 0 ? down[0] : across[0];
        }
        
        if (i > 0) {
            across[i]->prev = across[i - 1];
        } else {
            across[i]->prev = down_count > 0 ? down[down_count - 1] : across[across_count - 1];
        }
    }
    
    // Chain down clues
    for (int i = 0; i < down_count; i++) {
        if (i < down_count - 1) {
            down[i]->next = down[i + 1];
        } else {
            down[i]->next = across_count > 0 ? across[0] : down[0];
        }
        
        if (i > 0) {
            down[i]->prev = down[i - 1];
        } else {
            down[i]->prev = across_count > 0 ? across[across_count - 1] : down[down_count - 1];
        }
    }
    
    free(across);
    free(down);
}

Clue* puzzle_get_first_clue(Puzzle *puzzle) {
    // Find clue with index 1
    for (int i = 0; i < puzzle->clue_count; i++) {
        if (puzzle->clues[i]->index == 1) {
            return puzzle->clues[i];
        }
    }
    return puzzle->clues[0]; // Fallback
}

Clue* puzzle_get_clue(Puzzle *puzzle, int y, int x, Direction dir) {
    int dir_idx = (dir == DIR_ACROSS) ? 0 : 1;
    int other_idx = 1 - dir_idx;
    
    Clue *clue = puzzle->map_index[dir_idx][y][x];
    if (clue) return clue;
    
    return puzzle->map_index[other_idx][y][x];
}

Clue* puzzle_get_clue_by_index(Puzzle *puzzle, int index, Direction dir) {
    for (int i = 0; i < puzzle->clue_count; i++) {
        if (puzzle->clues[i]->index == index && puzzle->clues[i]->dir == dir) {
            return puzzle->clues[i];
        }
    }
    
    // Try other direction
    Direction other_dir = (dir == DIR_ACROSS) ? DIR_DOWN : DIR_ACROSS;
    for (int i = 0; i < puzzle->clue_count; i++) {
        if (puzzle->clues[i]->index == index && puzzle->clues[i]->dir == other_dir) {
            return puzzle->clues[i];
        }
    }
    
    return NULL;
}

bool puzzle_is_complete(Puzzle *puzzle) {
    for (int i = 0; i < puzzle->clue_count; i++) {
        if (!puzzle->clues[i]->done) {
            return false;
        }
    }
    return true;
}

int puzzle_count_done(Puzzle *puzzle) {
    int count = 0;
    for (int i = 0; i < puzzle->clue_count; i++) {
        if (puzzle->clues[i]->done) {
            count++;
        }
    }
    return count;
}

void puzzle_check_all(Puzzle *puzzle) {
    for (int i = 0; i < puzzle->clue_count; i++) {
        clue_check(puzzle->clues[i]);
    }
}