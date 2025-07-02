// utils.c - Utility functions implementation
#include <stdio.h>
#include <time.h>
#include "cliptic.h"

// Position functions
Position pos_make(int y, int x) {
    Position pos = {y, x};
    return pos;
}

int pos_wrap(int val, int min, int max) {
    if (val > max) return min;
    if (val < min) return max;
    return val;
}

Direction pos_change_dir(Direction dir) {
    return (dir == DIR_ACROSS) ? DIR_DOWN : DIR_ACROSS;
}

// Date functions
Date date_today(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    
    Date date = {
        .year = tm->tm_year + 1900,
        .month = tm->tm_mon + 1,
        .day = tm->tm_mday
    };
    
    return date;
}

Date date_add_days(Date date, int days) {
    struct tm tm = {0};
    tm.tm_year = date.year - 1900;
    tm.tm_mon = date.month - 1;
    tm.tm_mday = date.day + days;
    
    mktime(&tm);
    
    Date result = {
        .year = tm.tm_year + 1900,
        .month = tm.tm_mon + 1,
        .day = tm.tm_mday
    };
    
    return result;
}

bool date_valid(Date date) {
    if (date.month < 1 || date.month > 12) return false;
    if (date.day < 1 || date.day > 31) return false;
    if (date.year < 1900 || date.year > 2100) return false;
    
    // Check specific month limits
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Leap year check
    if (date.month == 2) {
        bool is_leap = (date.year % 4 == 0 && date.year % 100 != 0) || (date.year % 400 == 0);
        if (is_leap) {
            days_in_month[1] = 29;
        }
    }
    
    if (date.day > days_in_month[date.month - 1]) return false;
    
    // Check if date is not in the future
    Date today = date_today();
    if (date.year > today.year) return false;
    if (date.year == today.year && date.month > today.month) return false;
    if (date.year == today.year && date.month == today.month && date.day > today.day) return false;
    
    // Check if date is not too far in the past (9 months)
    Date earliest = date_add_days(today, -270);
    if (date.year < earliest.year) return false;
    if (date.year == earliest.year && date.month < earliest.month) return false;
    if (date.year == earliest.year && date.month == earliest.month && date.day < earliest.day) return false;
    
    return true;
}

void date_to_string(Date date, char *buffer, size_t size) {
    snprintf(buffer, size, "%04d-%02d-%02d", date.year, date.month, date.day);
}

void date_to_long_string(Date date, char *buffer, size_t size) {
    struct tm tm = {0};
    tm.tm_year = date.year - 1900;
    tm.tm_mon = date.month - 1;
    tm.tm_mday = date.day;
    mktime(&tm);
    
    strftime(buffer, size, "%A %b %d %Y", &tm);
}