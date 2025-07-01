// config.h - Configuration management
#ifndef CONFIG_H
#define CONFIG_H

#include "cliptic.h"

// Global configuration variables
extern ColorSettings g_colors;
extern ConfigSettings g_config;

// Config file paths
#define CONFIG_DIR_PATH "%USERPROFILE%\\.config\\cliptic"
#define CONFIG_FILE_PATH "%USERPROFILE%\\.config\\cliptic\\cliptic.rc"

// Config functions
void config_default_set(void);
void config_custom_set(void);
bool config_file_exists(void);
void config_generate_file(void);
void config_read_file(void);

// Config parsing
void config_parse_line(const char *line);
void config_parse_color(const char *key, int value);
void config_parse_setting(const char *key, int value);

#endif // CONFIG_H