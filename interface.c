// interface.c - User interface implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "interface.h"
#include "screen.h"
#include "config.h"
#include "database.h"

// Unicode small numbers
const wchar_t UC_NUMS[10] = {
    L'\u2080', L'\u2081', L'\u2082', L'\u2083', L'\u2084',
    L'\u2085', L'\u2086', L'\u2087', L'\u2088', L'\u2089'
};

// Top Bar implementation
TopBar* top_bar_new(Date date) {
    TopBar *bar = calloc(1, sizeof(TopBar));
    window_init(&bar->window, 1, 0, 0, 0);
    bar->date = date;
    return bar;
}

void top_bar_draw(TopBar *bar) {
    window_draw_bar(&bar->window, g_colors.bar);
    
    // Draw title
    console_set_color(g_colors.bar);
    console_move_cursor(0, 1);
    console_write_string(L"cliptic:");
    
    // Draw date
    char date_str[64];
    date_to_long_string(bar->date, date_str, sizeof(date_str));
    wchar_t wdate[64];
    mbstowcs(wdate, date_str, 64);
    console_move_cursor(0, 11);
    console_write_string(wdate);
}

void top_bar_free(TopBar *bar) {
    free(bar);
}

// Bottom Bar implementation
BottomBar* bottom_bar_new(void) {
    int lines, cols;
    screen_get_size(&lines, &cols);
    
    BottomBar *bar = calloc(1, sizeof(BottomBar));
    window_init(&bar->window, 1, 0, lines - 1, 0);
    return bar;
}

void bottom_bar_draw(BottomBar *bar) {
    window_draw_bar(&bar->window, g_colors.bar);
    
    // Draw controls
    console_set_color(g_colors.bar);
    int lines, cols;
    screen_get_size(&lines, &cols);
    console_move_cursor(lines - 1, cols - 42);
    console_write_string(L"^S save | ^R reveal | ^E reset | ^G check");
}

void bottom_bar_mode(BottomBar *bar, GameMode mode) {
    console_set_color(mode == MODE_INSERT ? g_colors.mode_I : g_colors.mode_N);
    console_move_cursor(bar->window.line, 0);
    
    wchar_t mode_str[10];
    swprintf(mode_str, 10, L"%-8s", mode == MODE_INSERT ? L"INSERT" : L"NORMAL");
    console_write_string(mode_str);
}

void bottom_bar_unsaved(BottomBar *bar, bool unsaved) {
    console_set_color(g_colors.bar);
    console_move_cursor(bar->window.line, 9);
    console_write_string(unsaved ? L"| +" : L"   ");
}

void bottom_bar_free(BottomBar *bar) {
    free(bar);
}

// Logo implementation
Logo* logo_new(int line, const wchar_t *text) {
    Logo *logo = calloc(1, sizeof(Logo));
    window_init(&logo->window, 1, wcslen(text), line, -1);
    logo->text = text;
    return logo;
}

void logo_draw(Logo *logo) {
    window_draw_grid(&logo->window, g_colors.logo_grid);
    console_set_color(g_colors.logo_text);
    console_move_cursor(logo->window.line, logo->window.col);
    console_write_string(logo->text);
}

void logo_free(Logo *logo) {
    free(logo);
}

// MenuBox implementation
MenuBox* menu_box_new(int y, const char *title) {
    int lines, cols;
    screen_get_size(&lines, &cols);
    
    MenuBox *box = calloc(1, sizeof(MenuBox));
    box->logo = logo_new((lines - 15) / 2 + 1, L"CLIptic");
    
    window_init(&box->window, y, box->logo->window.x + 4, 
                (lines - y) / 2, -1);
    
    box->top_bar = top_bar_new(date_today());
    box->bottom_bar = bottom_bar_new();
    box->title = title;
    box->draw_bars = true;
    
    return box;
}

void menu_box_draw(MenuBox *box) {
    window_draw(&box->window, g_colors.box);
    
    if (box->draw_bars) {
        top_bar_draw(box->top_bar);
        bottom_bar_draw(box->bottom_bar);
    }
    
    logo_draw(box->logo);
    
    if (box->title) {
        console_set_color(g_colors.title);
        window_add_str_centered(&box->window, 4, box->title);
    }
}

void menu_box_free(MenuBox *box) {
    if (box->top_bar) top_bar_free(box->top_bar);
    if (box->bottom_bar) bottom_bar_free(box->bottom_bar);
    if (box->logo) logo_free(box->logo);
    free(box);
}

// Resizer implementation
void interface_resizer_show(void) {
    MenuBox *resizer = menu_box_new(8, "Screen too small");
    
    while (screen_too_small()) {
        screen_clear();
        
        // Recalculate position
        int lines, cols;
        screen_get_size(&lines, &cols);
        resizer->window.line = (lines - 8) / 2;
        resizer->logo->window.line = resizer->window.line + 1;
        
        menu_box_draw(resizer);
        
        // Draw message
        console_set_color(g_colors.box);
        window_wrap_str(&resizer->window, 5, 
                       "Screen too small. Increase screen size to run cliptic.");
        
        // Check for exit
        int c = console_get_key();
        if (c == 'q' || c == 3) { // q or Ctrl+C
            exit(0);
        }
    }
    
    menu_box_free(resizer);
}

// Selector implementation
Selector* selector_new(const char **options, int count, int x, int line) {
    Selector *sel = calloc(1, sizeof(Selector));
    window_init(&sel->window, count, x, line, -1);
    sel->options = options;
    sel->option_count = count;
    sel->cursor = 0;
    sel->running = true;
    sel->tick = NULL;
    sel->user_data = NULL;
    return sel;
}

void selector_draw(Selector *sel) {
    console_set_cursor_visible(false);
    
    for (int i = 0; i < sel->option_count; i++) {
        console_move_cursor(sel->window.line + i, sel->window.col);
        
        if (i == sel->cursor) {
            console_set_color(g_colors.menu_active);
        } else {
            console_set_color(g_colors.menu_inactive);
        }
        
        // Center the option text
        int padding = (sel->window.x - strlen(sel->options[i])) / 2;
        for (int j = 0; j < padding; j++) {
            console_write_char(L' ');
        }
        
        wchar_t woption[256];
        mbstowcs(woption, sel->options[i], 256);
        console_write_string(woption);
        
        for (int j = strlen(sel->options[i]) + padding; j < sel->window.x; j++) {
            console_write_char(L' ');
        }
    }
    
    if (sel->tick) {
        sel->tick(sel);
    }
}

int selector_run(Selector *sel) {
    while (sel->running) {
        selector_draw(sel);
        
        int key = console_get_key();
        switch (key) {
            case 'j':
            case 258: // Down arrow
                sel->cursor = (sel->cursor + 1) % sel->option_count;
                break;
            case 'k':
            case 259: // Up arrow
                sel->cursor = (sel->cursor - 1 + sel->option_count) % sel->option_count;
                break;
            case 10: // Enter
                return sel->cursor;
            case 'q':
            case 3: // Ctrl+C
                return -1;
            case -1: // Window resize
                screen_redraw(NULL);
                break;
        }
    }
    
    return -1;
}

void selector_free(Selector *sel) {
    free(sel);
}

// Menu implementation
Menu* menu_new(const char **options, int count, const char *title) {
    Menu *menu = calloc(1, sizeof(Menu));
    MenuBox *box = menu_box_new(count + 6, title);
    menu->menu_box = *box;
    menu->height = count + 6;
    
    menu->selector = selector_new(options, count, 
                                 menu->menu_box.logo->window.x,
                                 menu->menu_box.window.line + 5);
    
    menu->enter_callback = NULL;
    menu->back_callback = NULL;
    menu->user_data = NULL;
    
    free(box);  // We copied the structure, so free the allocated one
    
    return menu;
}

int menu_choose_option(Menu *menu) {
    menu_box_draw(&menu->menu_box);
    int choice = selector_run(menu->selector);
    
    if (choice >= 0 && menu->enter_callback) {
        menu->enter_callback(menu);
    } else if (choice < 0 && menu->back_callback) {
        menu->back_callback(menu);
    }
    
    return choice;
}

void menu_free(Menu *menu) {
    if (menu->selector) selector_free(menu->selector);
    menu_box_free(&menu->menu_box);
    free(menu);
}

// Stat Window implementation
StatWindow* stat_window_new(int line) {
    StatWindow *win = calloc(1, sizeof(StatWindow));
    window_init(&win->window, 5, 33, line, -1);
    return win;
}

void stat_window_show(StatWindow *win, Date date) {
    window_draw(&win->window, g_colors.box);
    console_set_color(g_colors.stats);
    
    char stats[256];
    state_get_stats_string(date, stats, sizeof(stats));
    
    // Convert and display stats line by line
    char *line = strtok(stats, "\n");
    int y = 1;
    while (line != NULL && y < 4) {
        console_move_cursor(win->window.line + y, win->window.col + 8);
        
        wchar_t wline[128];
        mbstowcs(wline, line, 128);
        console_write_string(wline);
        
        line = strtok(NULL, "\n");
        y++;
    }
}

void stat_window_free(StatWindow *win) {
    free(win);
}

// Yes/No Menu
bool interface_yes_no_menu(const char *title, void (*yes_callback)(void)) {
    const char *options[] = {"Yes", "No"};
    Menu *menu = menu_new(options, 2, title);
    
    int choice = menu_choose_option(menu);
    menu_free(menu);
    
    if (choice == 0) {
        if (yes_callback) yes_callback();
        return true;
    }
    
    return false;
}