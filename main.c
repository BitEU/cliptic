// main.c - Entry point for Windows Cliptic
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cliptic.h"
#include "terminal.h"
#include "config.h"
#include "database.h"
#include "screen.h"

int main(int argc, char *argv[]) {
    // Initialize Windows console
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    // Parse command line arguments
    if (argc > 1) {
        return terminal_parse_args(argc, argv);
    }
    
    // Setup screen and config
    config_default_set();
    screen_setup();
    config_custom_set();
    
    // Register cleanup
    atexit(terminal_cleanup);
    
    // Show main menu
    return menu_main_show();
}