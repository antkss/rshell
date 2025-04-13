#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>

mode_t get_perm(const char *filename);
extern char input_buf[ARG_MAX];
extern int input_len;
extern int cursor;
struct winsize get_terminal_size();
void enable_raw_mode();
void disable_raw_mode();
void move_cursor_right(int index);
void move_cursor_left(int index);
void clear_line();
mode_t get_perm(const char *filename);
void clearln();
