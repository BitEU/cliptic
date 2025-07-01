// screen.c - Screen management implementation
#include <stdio.h>
#include <conio.h>
#include "screen.h"
#include "cliptic.h"
#include "interface.h"

static HANDLE hConsoleOut;
static HANDLE hConsoleIn;
static WORD originalAttributes;
static CONSOLE_SCREEN_BUFFER_INFO csbi;

// Color pair mappings
static struct {
    WORD attributes;
} colorPairs[32];

void screen_setup(void) {
    console_init();
    console_set_colors();
    screen_redraw(NULL);
}

void console_init(void) {
    // Get console handles
    hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
    
    // Save original attributes
    GetConsoleScreenBufferInfo(hConsoleOut, &csbi);
    originalAttributes = csbi.wAttributes;
    
    // Set console mode
    DWORD mode;
    GetConsoleMode(hConsoleIn, &mode);
    mode &= ~ENABLE_ECHO_INPUT;
    mode &= ~ENABLE_LINE_INPUT;
    mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    SetConsoleMode(hConsoleIn, mode);
    
    // Enable virtual terminal processing for output
    GetConsoleMode(hConsoleOut, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsoleOut, mode);
    
    // Hide cursor by default
    console_set_cursor_visible(false);
}

void console_set_colors(void) {
    // Initialize color pairs like ncurses
    for (int i = 1; i <= 8; i++) {
        // Foreground colors on default background
        color_init_pair(i, i, -1);
        // Black on colored background
        color_init_pair(i + 8, 0, i);
    }
}

void color_init_pair(int pair, int fg, int bg) {
    if (pair < 0 || pair >= 32) return;
    
    WORD attributes = 0;
    
    // Map foreground colors
    if (fg >= 0) {
        switch (fg) {
            case 0: break; // Black
            case 1: attributes |= FOREGROUND_RED; break;
            case 2: attributes |= FOREGROUND_GREEN; break;
            case 3: attributes |= FOREGROUND_RED | FOREGROUND_GREEN; break; // Yellow
            case 4: attributes |= FOREGROUND_BLUE; break;
            case 5: attributes |= FOREGROUND_RED | FOREGROUND_BLUE; break; // Magenta
            case 6: attributes |= FOREGROUND_GREEN | FOREGROUND_BLUE; break; // Cyan
            case 7: attributes |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break; // White
            default: attributes |= originalAttributes & 0x0F; break;
        }
        if (fg >= 8) attributes |= FOREGROUND_INTENSITY;
    } else {
        attributes |= originalAttributes & 0x0F;
    }
    
    // Map background colors
    if (bg >= 0) {
        switch (bg) {
            case 0: break; // Black
            case 1: attributes |= BACKGROUND_RED; break;
            case 2: attributes |= BACKGROUND_GREEN; break;
            case 3: attributes |= BACKGROUND_RED | BACKGROUND_GREEN; break; // Yellow
            case 4: attributes |= BACKGROUND_BLUE; break;
            case 5: attributes |= BACKGROUND_RED | BACKGROUND_BLUE; break; // Magenta
            case 6: attributes |= BACKGROUND_GREEN | BACKGROUND_BLUE; break; // Cyan
            case 7: attributes |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE; break; // White
            default: attributes |= originalAttributes & 0xF0; break;
        }
        if (bg >= 8) attributes |= BACKGROUND_INTENSITY;
    } else {
        attributes |= originalAttributes & 0xF0;
    }
    
    colorPairs[pair].attributes = attributes;
}

WORD color_pair_to_attributes(int pair) {
    if (pair < 0 || pair >= 32) return originalAttributes;
    return colorPairs[pair].attributes;
}

void console_set_color(int color_pair) {
    SetConsoleTextAttribute(hConsoleOut, color_pair_to_attributes(color_pair));
}

void console_move_cursor(int y, int x) {
    COORD coord = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(hConsoleOut, coord);
}

void console_set_cursor_visible(bool visible) {
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(hConsoleOut, &cci);
    cci.bVisible = visible;
    SetConsoleCursorInfo(hConsoleOut, &cci);
}

void console_write_char(wchar_t ch) {
    DWORD written;
    WriteConsoleW(hConsoleOut, &ch, 1, &written, NULL);
}

void console_write_string(const wchar_t *str) {
    DWORD written;
    WriteConsoleW(hConsoleOut, str, wcslen(str), &written, NULL);
}

void console_write_string_at(int y, int x, const wchar_t *str) {
    console_move_cursor(y, x);
    console_write_string(str);
}

void screen_clear(void) {
    COORD topLeft = {0, 0};
    DWORD written;
    GetConsoleScreenBufferInfo(hConsoleOut, &csbi);
    FillConsoleOutputCharacterW(hConsoleOut, L' ', csbi.dwSize.X * csbi.dwSize.Y, topLeft, &written);
    FillConsoleOutputAttribute(hConsoleOut, originalAttributes, csbi.dwSize.X * csbi.dwSize.Y, topLeft, &written);
    SetConsoleCursorPosition(hConsoleOut, topLeft);
}

bool screen_too_small(void) {
    int lines, cols;
    screen_get_size(&lines, &cols);
    return lines < GRID_MIN_HEIGHT || cols < GRID_MIN_WIDTH;
}

void screen_get_size(int *lines, int *cols) {
    GetConsoleScreenBufferInfo(hConsoleOut, &csbi);
    *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *lines = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void screen_redraw(void (*callback)(void)) {
    if (screen_too_small()) {
        interface_resizer_show();
    }
    screen_clear();
    if (callback) callback();
}

int console_get_key(void) {
    INPUT_RECORD inputRecord;
    DWORD events;
    
    while (1) {
        ReadConsoleInput(hConsoleIn, &inputRecord, 1, &events);
        
        if (inputRecord.EventType == KEY_EVENT && inputRecord.Event.KeyEvent.bKeyDown) {
            WORD vk = inputRecord.Event.KeyEvent.wVirtualKeyCode;
            CHAR ch = inputRecord.Event.KeyEvent.uChar.AsciiChar;
            DWORD state = inputRecord.Event.KeyEvent.dwControlKeyState;
            
            // Handle special keys
            if (vk == VK_UP) return 259;
            if (vk == VK_DOWN) return 258;
            if (vk == VK_LEFT) return 260;
            if (vk == VK_RIGHT) return 261;
            if (vk == VK_BACK) return 127;
            if (vk == VK_RETURN) return 10;
            if (vk == VK_ESCAPE) return 27;
            if (vk == VK_TAB) return 9;
            
            // Handle Ctrl+key combinations
            if (state & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
                if (ch >= 1 && ch <= 26) return ch;
            }
            
            // Regular characters
            if (ch > 0) return ch;
        }
        
        // Handle window resize
        if (inputRecord.EventType == WINDOW_BUFFER_SIZE_EVENT) {
            return -1; // Special code for resize
        }
    }
}

int console_get_key_timeout(int timeout_ms) {
    DWORD start = GetTickCount();
    
    while (GetTickCount() - start < timeout_ms) {
        if (_kbhit()) {
            return console_get_key();
        }
        Sleep(10);
    }
    
    return -1; // Timeout
}