// terminal.h - Terminal command handling
#ifndef TERMINAL_H
#define TERMINAL_H

// Terminal functions
int terminal_parse_args(int argc, char *argv[]);
void terminal_cleanup(void);

// Command functions
int terminal_cmd_today(int offset);
int terminal_cmd_reset(const char *what);

#endif // TERMINAL_H