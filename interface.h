// interface.h - User interface components
#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdbool.h>
#include "cliptic.h"
#include "windows.h"

// Forward declarations
typedef struct Menu Menu;
typedef struct Selector Selector;

// Interface components
typedef struct {
    Window window;
    Date date;
} TopBar;

typedef struct {
    Window window;
} BottomBar;

typedef struct {
    Window window;
    const wchar_t *text;
} Logo;

typedef struct {
    Window window;
    Logo *logo;
    TopBar *top_bar;
    BottomBar *bottom_bar;
    const char *title;
    bool draw_bars;
} MenuBox;

typedef struct {
    MenuBox menu_box;
} Resizer;

// Selector structure
struct Selector {
    Window window;
    const char **options;
    int option_count;
    int cursor;
    bool running;
    void (*tick)(struct Selector *sel);
    void *user_data;
};

// Menu structure
struct Menu {
    MenuBox menu_box;
    Selector *selector;
    int height;
    void (*enter_callback)(struct Menu *menu);
    void (*back_callback)(struct Menu *menu);
    void *user_data;
};

// Interface functions
void interface_resizer_show(void);
bool interface_yes_no_menu(const char *title, void (*yes_callback)(void));

// Component creation
TopBar* top_bar_new(Date date);
void top_bar_draw(TopBar *bar);
void top_bar_free(TopBar *bar);

BottomBar* bottom_bar_new(void);
void bottom_bar_draw(BottomBar *bar);
void bottom_bar_mode(BottomBar *bar, GameMode mode);
void bottom_bar_unsaved(BottomBar *bar, bool unsaved);
void bottom_bar_free(BottomBar *bar);

Logo* logo_new(int line, const wchar_t *text);
void logo_draw(Logo *logo);
void logo_free(Logo *logo);

MenuBox* menu_box_new(int y, const char *title);
void menu_box_draw(MenuBox *box);
void menu_box_free(MenuBox *box);

Selector* selector_new(const char **options, int count, int x, int line);
void selector_draw(Selector *sel);
int selector_run(Selector *sel);
void selector_free(Selector *sel);

Menu* menu_new(const char **options, int count, const char *title);
int menu_choose_option(Menu *menu);
void menu_free(Menu *menu);

// Stat window
typedef struct {
    Window window;
} StatWindow;

StatWindow* stat_window_new(int line);
void stat_window_show(StatWindow *win, Date date);
void stat_window_free(StatWindow *win);

#endif // INTERFACE_H