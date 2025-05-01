#if ALTER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

char PC = '\0';
char *BC = "\b";
char *UP = "\033[A";
int tgetent(char *bp, const char *name) {
    (void)bp;
    (void)name;
    return 1;
}

int tgetnum(const char *id) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if (strcmp(id, "co") == 0) return w.ws_col;
    if (strcmp(id, "li") == 0) return w.ws_row;
    return -1;
}


int tgetflag(const char *id) {
    (void)id;
    return 1;
}

char *tgetstr(const char *id, char **area) {
    if (strcmp(id, "cl") == 0) {
        static char *clear = "\033[H\033[J"; // ANSI clear
        if (area) *area = clear + strlen(clear) + 1;
        return clear;
    }
    return NULL;
}

char *tgoto(const char *cap, int col, int row) {
    static char buf[32];
    (void)cap;
    snprintf(buf, sizeof(buf), "\033[%d;%dH", row + 1, col + 1);
    return buf;
}

int tputs(const char *str, int affcnt, int (*putc_fn)(int)) {
    (void)affcnt;
    while (*str) {
        if (putc_fn(*str++) == EOF) return -1;
    }
    return 0;
}
#endif
