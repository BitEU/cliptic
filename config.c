// config.c - Configuration management implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <sys/stat.h>
#include "config.h"
#include "interface.h"

// Global configuration variables
ColorSettings g_colors;
ConfigSettings g_config;

void config_default_set(void) {
    // Set default colors
    g_colors.box = 8;
    g_colors.grid = 8;
    g_colors.bar = 16;
    g_colors.logo_grid = 8;
    g_colors.logo_text = 3;
    g_colors.title = 6;
    g_colors.stats = 6;
    g_colors.active_num = 3;
    g_colors.num = 8;
    g_colors.block = 8;
    g_colors.mode_I = 15;
    g_colors.mode_N = 12;
    g_colors.correct = 2;
    g_colors.incorrect = 1;
    g_colors.meta = 3;
    g_colors.cluebox = 8;
    g_colors.menu_active = 15;
    g_colors.menu_inactive = 0;
    
    // Set default config
    g_config.auto_advance = true;
    g_config.auto_mark = true;
    g_config.auto_save = true;
}

void config_custom_set(void) {
    if (config_file_exists()) {
        config_read_file();
    } else {
        // Ask user if they want to generate config file
        if (interface_yes_no_menu("Generate config file?", NULL)) {
            config_generate_file();
        }
    }
}

bool config_file_exists(void) {
    char path[MAX_PATH];
    ExpandEnvironmentStringsA(CONFIG_FILE_PATH, path, MAX_PATH);
    
    struct stat st;
    return stat(path, &st) == 0;
}

void config_generate_file(void) {
    char dir_path[MAX_PATH];
    char file_path[MAX_PATH];
    
    // Expand environment variables
    ExpandEnvironmentStringsA(CONFIG_DIR_PATH, dir_path, MAX_PATH);
    ExpandEnvironmentStringsA(CONFIG_FILE_PATH, file_path, MAX_PATH);
    
    // Create directory if it doesn't exist
    _mkdir(dir_path);
    
    // Write default config file
    FILE *fp = fopen(file_path, "w");
    if (fp == NULL) return;
    
    fprintf(fp, "//Colour Settings\n");
    fprintf(fp, "hi box %d\n", g_colors.box);
    fprintf(fp, "hi grid %d\n", g_colors.grid);
    fprintf(fp, "hi bar %d\n", g_colors.bar);
    fprintf(fp, "hi logo_grid %d\n", g_colors.logo_grid);
    fprintf(fp, "hi logo_text %d\n", g_colors.logo_text);
    fprintf(fp, "hi title %d\n", g_colors.title);
    fprintf(fp, "hi stats %d\n", g_colors.stats);
    fprintf(fp, "hi active_num %d\n", g_colors.active_num);
    fprintf(fp, "hi num %d\n", g_colors.num);
    fprintf(fp, "hi block %d\n", g_colors.block);
    fprintf(fp, "hi I %d\n", g_colors.mode_I);
    fprintf(fp, "hi N %d\n", g_colors.mode_N);
    fprintf(fp, "hi correct %d\n", g_colors.correct);
    fprintf(fp, "hi incorrect %d\n", g_colors.incorrect);
    fprintf(fp, "hi meta %d\n", g_colors.meta);
    fprintf(fp, "hi cluebox %d\n", g_colors.cluebox);
    fprintf(fp, "hi menu_active %d\n", g_colors.menu_active);
    fprintf(fp, "hi menu_inactive %d\n", g_colors.menu_inactive);
    fprintf(fp, "\n");
    
    fprintf(fp, "//Interface Settings\n");
    fprintf(fp, "set auto_advance %d\n", g_config.auto_advance ? 1 : 0);
    fprintf(fp, "set auto_mark %d\n", g_config.auto_mark ? 1 : 0);
    fprintf(fp, "set auto_save %d\n", g_config.auto_save ? 1 : 0);
    
    fclose(fp);
}

void config_read_file(void) {
    char file_path[MAX_PATH];
    ExpandEnvironmentStringsA(CONFIG_FILE_PATH, file_path, MAX_PATH);
    
    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) return;
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        config_parse_line(line);
    }
    
    fclose(fp);
}

void config_parse_line(const char *line) {
    // Skip comments and empty lines
    if (line[0] == '/' || line[0] == '\n' || line[0] == '\r') return;
    
    char cmd[32], key[32];
    int value;
    
    if (sscanf(line, "%s %s %d", cmd, key, &value) == 3) {
        if (strcmp(cmd, "hi") == 0) {
            config_parse_color(key, value);
        } else if (strcmp(cmd, "set") == 0) {
            config_parse_setting(key, value);
        }
    }
}

void config_parse_color(const char *key, int value) {
    if (strcmp(key, "box") == 0) g_colors.box = value;
    else if (strcmp(key, "grid") == 0) g_colors.grid = value;
    else if (strcmp(key, "bar") == 0) g_colors.bar = value;
    else if (strcmp(key, "logo_grid") == 0) g_colors.logo_grid = value;
    else if (strcmp(key, "logo_text") == 0) g_colors.logo_text = value;
    else if (strcmp(key, "title") == 0) g_colors.title = value;
    else if (strcmp(key, "stats") == 0) g_colors.stats = value;
    else if (strcmp(key, "active_num") == 0) g_colors.active_num = value;
    else if (strcmp(key, "num") == 0) g_colors.num = value;
    else if (strcmp(key, "block") == 0) g_colors.block = value;
    else if (strcmp(key, "I") == 0) g_colors.mode_I = value;
    else if (strcmp(key, "N") == 0) g_colors.mode_N = value;
    else if (strcmp(key, "correct") == 0) g_colors.correct = value;
    else if (strcmp(key, "incorrect") == 0) g_colors.incorrect = value;
    else if (strcmp(key, "meta") == 0) g_colors.meta = value;
    else if (strcmp(key, "cluebox") == 0) g_colors.cluebox = value;
    else if (strcmp(key, "menu_active") == 0) g_colors.menu_active = value;
    else if (strcmp(key, "menu_inactive") == 0) g_colors.menu_inactive = value;
}

void config_parse_setting(const char *key, int value) {
    if (strcmp(key, "auto_advance") == 0) g_config.auto_advance = (value == 1);
    else if (strcmp(key, "auto_mark") == 0) g_config.auto_mark = (value == 1);
    else if (strcmp(key, "auto_save") == 0) g_config.auto_save = (value == 1);
}