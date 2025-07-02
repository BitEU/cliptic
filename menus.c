// menus.c - Menu implementations
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "menus.h"
#include "interface.h"
#include "database.h"
#include "screen.h"
#include "game.h"
#include "config.h"

// Main menu
int menu_main_show(void) {
    const char *options[] = {
        "Play Today",
        "Select Date",
        "This Week",
        "Recent Puzzles",
        "High Scores",
        "Quit"
    };
    
    Menu *menu = menu_new(options, 6, "Main Menu");
    
    int choice;
    while ((choice = menu_choose_option(menu)) >= 0) {
        switch (choice) {
            case 0: // Play Today
                {
                    Date today = date_today();
                    Game *game = game_new(today);
                    if (game) {
                        game_play(game);
                        game_free(game);
                    }
                }
                break;
            case 1: // Select Date
                menu_select_date_show();
                break;
            case 2: // This Week
                menu_this_week_show();
                break;
            case 3: // Recent Puzzles
                menu_recent_puzzles_show();
                break;
            case 4: // High Scores
                menu_high_scores_show();
                break;
            case 5: // Quit
                menu_free(menu);
                return 0;
        }
        
        // Redraw menu after returning
        screen_clear();
        menu_box_draw(&menu->menu_box);
    }
    
    menu_free(menu);
    return 0;
}

// Date selector menu
typedef struct {
    Menu *menu;
    StatWindow *stat_win;
    Date date;
    int cursor; // 0=day, 1=month, 2=year
} DateSelectorMenu;

static void date_selector_draw(Selector *sel) {
    DateSelectorMenu *dsm = (DateSelectorMenu*)sel->user_data;
    
    // Draw date components
    console_set_color(g_colors.menu_inactive);
    
    char date_str[32];
    snprintf(date_str, sizeof(date_str), "%02d / %02d / %04d",
             dsm->date.day, dsm->date.month, dsm->date.year);
    
    console_move_cursor(sel->window.line, sel->window.col);
    
    // Highlight active component
    for (int i = 0; i < 3; i++) {
        if (i == dsm->cursor) {
            console_set_color(g_colors.menu_active);
        } else {
            console_set_color(g_colors.menu_inactive);
        }
        
        switch (i) {
            case 0: // Day
                console_write_string(L"  ");
                console_write_char(date_str[0]);
                console_write_char(date_str[1]);
                console_write_string(L"  ");
                break;
            case 1: // Month
                console_write_string(L"  ");
                console_write_char(date_str[5]);
                console_write_char(date_str[6]);
                console_write_string(L"  ");
                break;
            case 2: // Year
                console_write_string(L" ");
                console_write_char(date_str[10]);
                console_write_char(date_str[11]);
                console_write_char(date_str[12]);
                console_write_char(date_str[13]);
                console_write_string(L" ");
                break;
        }
        
        if (i < 2) {
            console_set_color(g_colors.menu_inactive);
            console_write_string(L" / ");
        }
    }
    
    // Update stat window
    stat_window_show(dsm->stat_win, dsm->date);
}

int menu_select_date_show(void) {
    DateSelectorMenu dsm;
    dsm.date = date_today();
    dsm.cursor = 0;
    
    // Create custom menu
    const char *dummy[] = {""};
    dsm.menu = menu_new(dummy, 1, "Select Date");
    dsm.menu->height = 7;
    
    // Override selector drawing
    dsm.menu->selector->tick = date_selector_draw;
    dsm.menu->selector->user_data = &dsm;
    
    // Create stat window
    dsm.stat_win = stat_window_new(dsm.menu->menu_box.window.line + dsm.menu->height);
    
    menu_box_draw(&dsm.menu->menu_box);
    
    bool running = true;
    while (running) {
        date_selector_draw(dsm.menu->selector);
        
        int key = console_get_key();
        switch (key) {
            case 'h':
            case 260: // Left
                dsm.cursor = (dsm.cursor - 1 + 3) % 3;
                break;
            case 'l':
            case 261: // Right
                dsm.cursor = (dsm.cursor + 1) % 3;
                break;
            case 'j':
            case 258: // Down
                switch (dsm.cursor) {
                    case 0: dsm.date.day--; break;
                    case 1: dsm.date.month--; break;
                    case 2: dsm.date.year--; break;
                }
                break;
            case 'k':
            case 259: // Up
                switch (dsm.cursor) {
                    case 0: dsm.date.day++; break;
                    case 1: dsm.date.month++; break;
                    case 2: dsm.date.year++; break;
                }
                break;
            case 10: // Enter
                {
                    Game *game = game_new(dsm.date);
                    if (game) {
                        game_play(game);
                        game_free(game);
                    }
                }
                break;
            case 'q':
            case 3: // Ctrl+C
                running = false;
                break;
        }
        
        // Validate date
        if (!date_valid(dsm.date)) {
            dsm.date = date_today();
        }
    }
    
    stat_window_free(dsm.stat_win);
    menu_free(dsm.menu);
    
    return 0;
}

// This week menu
int menu_this_week_show(void) {
    // Get last 7 days
    Date dates[7];
    const char *options[7];
    char option_strs[7][64];
    
    for (int i = 0; i < 7; i++) {
        dates[i] = date_add_days(date_today(), -i);
        
        // Get day name
        struct tm tm = {0};
        tm.tm_year = dates[i].year - 1900;
        tm.tm_mon = dates[i].month - 1;
        tm.tm_mday = dates[i].day;
        mktime(&tm);
        
        char day_name[32];
        strftime(day_name, sizeof(day_name), "%A", &tm);
        
        // Check if completed
        GameState *state = state_new(dates[i]);
        snprintf(option_strs[i], sizeof(option_strs[i]), "%-9s [%c]",
                day_name, state->done ? 'X' : ' ');
        state_free(state);
        
        options[i] = option_strs[i];
    }
    
    Menu *menu = menu_new(options, 7, "This Week");
    
    // Add stat window
    StatWindow *stat_win = stat_window_new(menu->menu_box.window.line + menu->height);
    
    menu->selector->tick = (void(*)(Selector*))stat_window_show;
    menu->selector->user_data = stat_win;
    
    int choice;
    while ((choice = menu_choose_option(menu)) >= 0) {
        Game *game = game_new(dates[choice]);
        if (game) {
            game_play(game);
            game_free(game);
        }
        
        // Refresh menu
        screen_clear();
        menu_box_draw(&menu->menu_box);
    }
    
        // Show score details
        char score_info[256];
        snprintf(score_info, sizeof(score_info),
                "Time: %s\nCompleted: %04d-%02d-%02d",
                entries[choice].time,
                entries[choice].date_done.year,
                entries[choice].date_done.month,
                entries[choice].date_done.day);
        
        window_clear(&stat_win->window);
        window_draw(&stat_win->window, g_colors.box);
        window_wrap_str(&stat_win->window, 1, score_info);
    }
    
    stat_window_free(stat_win);
    menu_free(menu);
    free(options);
    free(option_strs);
    scores_free_list(entries, count);
    
    return 0;
}

// Recent puzzles menu
int menu_recent_puzzles_show(void) {
    RecentEntry *entries;
    int count = recents_get_list(&entries, 10);
    
    if (count == 0) {
        // No recent puzzles
        interface_yes_no_menu("No recent puzzles", NULL);
        return 0;
    }
    
    // Build options
    const char **options = calloc(count, sizeof(char*));
    char (*option_strs)[64] = calloc(count, 64);
    
    for (int i = 0; i < count; i++) {
        date_to_long_string(entries[i].date, option_strs[i], 64);
        options[i] = option_strs[i];
    }
    
    Menu *menu = menu_new(options, count, "Recently Played");
    
    // Add stat window
    StatWindow *stat_win = stat_window_new(menu->menu_box.window.line + menu->height);
    
    int choice;
    while ((choice = menu_choose_option(menu)) >= 0) {
        stat_window_show(stat_win, entries[choice].date);
        
        Game *game = game_new(entries[choice].date);
        if (game) {
            game_play(game);
            game_free(game);
        }
        
        // Refresh menu
        screen_clear();
        menu_box_draw(&menu->menu_box);
    }
    
    stat_window_free(stat_win);
    menu_free(menu);
    free(options);
    free(option_strs);
    recents_free_list(entries, count);
    
    return 0;
}

// High scores menu
int menu_high_scores_show(void) {
    ScoreEntry *entries;
    int count = scores_get_list(&entries, 10);
    
    if (count == 0) {
        interface_yes_no_menu("No high scores yet", NULL);
        return 0;
    }
    
    // Build options
    const char **options = calloc(count, sizeof(char*));
    char (*option_strs)[64] = calloc(count, 64);
    
    for (int i = 0; i < count; i++) {
        date_to_long_string(entries[i].date, option_strs[i], 64);
        options[i] = option_strs[i];
    }
    
    Menu *menu = menu_new(options, count, "High Scores");
    
    // Add stat window showing times
    StatWindow *stat_win = stat_window_new(menu->menu_box.window.line + menu->height);
    
    int choice;
    while ((choice = menu_choose_option(menu)) >= 0) {
        // Show score details
        char score_info[256];
        snprintf(score_info, sizeof(score_info),
                "Time: %s\nCompleted: %04d-%02d-%02d",
                entries[choice].time,
                entries[choice].date_done.year,
                entries[choice].date_done.month,
                entries[choice].date_done.day);
        
    stat_window_free(stat_win);
    menu_free(menu);
    
    return 0;
}

// Pause menu
void menu_pause_show(Game *game) {
    const char *options[] = {"Continue", "Exit Game"};
    Menu *menu = menu_new(options, 2, "Paused");
    menu->menu_box.draw_bars = false;
    
    int choice = menu_choose_option(menu);
    menu_free(menu);
    
    if (choice == 0) {
        game_unpause(game);
    } else {
        game_exit(game);
    }
}

// Puzzle complete menu
void menu_puzzle_complete_show(void) {
    const char *options[] = {"Exit", "Quit"};
    Menu *menu = menu_new(options, 2, "Puzzle Complete!");
    menu->menu_box.draw_bars = false;
    
    int choice = menu_choose_option(menu);
    menu_free(menu);
    
    if (choice == 1) {
        exit(0);
    }
    
    screen_clear();
}

// Reset progress menu
void menu_reset_progress_show(Game *game) {
    timer_stop(&game->timer);
    
    bool reset = interface_yes_no_menu("Reset puzzle progress?", NULL);
    
    if (reset) {
        game_reset(game);
    }
    
    game_unpause(game);
}