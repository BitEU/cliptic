// terminal.c - Terminal command handling implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "terminal.h"
#include "cliptic.h"
#include "screen.h"
#include "config.h"
#include "database.h"
#include "game.h"

int terminal_parse_args(int argc, char *argv[]) {
    if (strcmp(argv[1], "today") == 0 || strcmp(argv[1], "-t") == 0) {
        int offset = 0;
        if (argc > 2) {
            offset = atoi(argv[2]);
        }
        return terminal_cmd_today(offset);
    }
    else if (strcmp(argv[1], "reset") == 0 || strcmp(argv[1], "-r") == 0) {
        if (argc > 2) {
            return terminal_cmd_reset(argv[2]);
        } else {
            printf("Usage: cliptic reset [all|states|scores|recents]\n");
            return 1;
        }
    }
    else {
        printf("Unknown command: %s\n", argv[1]);
        printf("Usage: cliptic [today [-n]|reset <what>]\n");
        return 1;
    }
}

int terminal_cmd_today(int offset) {
    // Setup screen and config
    config_default_set();
    screen_setup();
    config_custom_set();
    
    // Register cleanup
    atexit(terminal_cleanup);
    
    // Initialize database
    if (!db_init()) {
        printf("Failed to initialize database\n");
        return 1;
    }
    
    // Play today's puzzle (with offset)
    Date date = date_add_days(date_today(), offset);
    Game *game = game_new(date);
    game_play(game);
    game_free(game);
    
    return 0;
}

int terminal_cmd_reset(const char *what) {
    printf("cliptic: Reset %s\n", what);
    printf("Are you sure? This cannot be undone! [Y/n]\n");
    
    char response[10];
    if (fgets(response, sizeof(response), stdin) == NULL) {
        printf("Wise choice\n");
        return 0;
    }
    
    if (response[0] != 'Y') {
        printf("Wise choice\n");
        return 0;
    }
    
    // Initialize database
    if (!db_init()) {
        printf("Failed to initialize database\n");
        return 1;
    }
    
    bool success = false;
    if (strcmp(what, "all") == 0) {
        success = db_reset_all();
    } else if (strcmp(what, "states") == 0) {
        success = db_reset_states();
    } else if (strcmp(what, "scores") == 0) {
        success = db_reset_scores();
    } else if (strcmp(what, "recents") == 0) {
        success = db_reset_recents();
    } else {
        printf("Unknown option: %s\n", what);
        printf("Valid options: all, states, scores, recents\n");
        db_close();
        return 1;
    }
    
    if (success) {
        printf("Reset successful\n");
    } else {
        printf("Reset failed\n");
    }
    
    db_close();
    return success ? 0 : 1;
}

void terminal_cleanup(void) {
    // Clean up database
    db_close();
    
    // Clear screen and show message
    screen_clear();
    printf("Thanks for playing!\n");
}