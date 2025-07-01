// cliptic.h - Main header file
#ifndef CLIPTIC_H
#define CLIPTIC_H

#include <windows.h>
#include <time.h>
#include <stdbool.h>

#define VERSION "0.1.3"
#define GRID_MIN_HEIGHT 36
#define GRID_MIN_WIDTH 61

// Color pairs
typedef enum {
    CP_DEFAULT = 0,
    CP_RED = 1,
    CP_GREEN = 2,
    CP_YELLOW = 3,
    CP_BLUE = 4,
    CP_MAGENTA = 5,
    CP_CYAN = 6,
    CP_WHITE = 7,
    CP_GRAY = 8,
    CP_BRIGHT_RED = 9,
    CP_BRIGHT_GREEN = 10,
    CP_BRIGHT_YELLOW = 11,
    CP_BRIGHT_BLUE = 12,
    CP_BRIGHT_MAGENTA = 13,
    CP_BRIGHT_CYAN = 14,
    CP_BRIGHT_WHITE = 15,
    CP_BLACK_BG = 16
} ColorPair;

// Game modes
typedef enum {
    MODE_NORMAL = 'N',
    MODE_INSERT = 'I'
} GameMode;

// Direction
typedef enum {
    DIR_ACROSS = 'a',
    DIR_DOWN = 'd'
} Direction;

// Position structure
typedef struct {
    int y;
    int x;
} Position;

// Date structure
typedef struct {
    int year;
    int month;
    int day;
} Date;

// Global color settings
typedef struct {
    int box;
    int grid;
    int bar;
    int logo_grid;
    int logo_text;
    int title;
    int stats;
    int active_num;
    int num;
    int block;
    int mode_I;
    int mode_N;
    int correct;
    int incorrect;
    int meta;
    int cluebox;
    int menu_active;
    int menu_inactive;
} ColorSettings;

// Global config settings
typedef struct {
    bool auto_advance;
    bool auto_mark;
    bool auto_save;
} ConfigSettings;

// Menu functions
int menu_main_show(void);
int menu_select_date_show(void);
int menu_this_week_show(void);
int menu_recent_puzzles_show(void);
int menu_high_scores_show(void);

// Utility functions
Position pos_make(int y, int x);
int pos_wrap(int val, int min, int max);
Direction pos_change_dir(Direction dir);
void date_to_string(Date date, char *buffer, size_t size);
void date_to_long_string(Date date, char *buffer, size_t size);
Date date_today(void);
Date date_add_days(Date date, int days);
bool date_valid(Date date);

// Unicode characters
#define UC_HL L'\u2501'     // ─
#define UC_VL L'\u2503'     // │
#define UC_TL L'\u250F'     // ┏
#define UC_TR L'\u2513'     // ┓
#define UC_BL L'\u2517'     // ┗
#define UC_BR L'\u251B'     // ┛
#define UC_TD L'\u2533'     // ┳
#define UC_TU L'\u253B'     // ┻
#define UC_TL_SIDE L'\u2523'// ┣
#define UC_TR_SIDE L'\u252B'// ┫
#define UC_XX L'\u254B'     // ╋
#define UC_TICK L'\u2713'   // ✓
#define UC_BLOCK_L L'\u258C' // ▌
#define UC_BLOCK_M L'\u2588' // █
#define UC_BLOCK_R L'\u2590' // ▐

// Small numbers (subscript)
extern const wchar_t UC_NUMS[10];

#endif // CLIPTIC_H