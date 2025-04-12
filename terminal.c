#include "terminal.h"
#include <sys/ioctl.h>
#include <termios.h>
#include "command.h"
int cursor = 0;
struct termios orig_termios;
struct winsize get_terminal_size(){
	struct winsize w;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
		perror("ioctl error");
	}
	return w;
}


void enable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;

    raw.c_lflag &= ~(ICANON | ECHO | ISIG); // Turn off canonical mode, echo, and signal handling
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}


void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}
void delete_forward_chars(int n) {
    char buf[16];
    snprintf(buf, sizeof(buf), "\x1b[%dP", n);  // DCH - Delete Character
    write(STDOUT_FILENO, buf, strlen(buf));
}

void clear_line() {
	struct winsize size = get_terminal_size();
	// if (input_len + PSI_LEN > size.ws_col) {
	// 	// printf("ws_col: %u \n", size.ws_col);
	// 	// delete_forward_chars(input_len + PSI_LEN + 1);
	// 	write(STDOUT_FILENO, "\r\033[2K", 5);
	// 	shell_print("%s", GENERIC_PSI);
	// } else {
	write(STDOUT_FILENO, "\r\033[2K", 5); // remove stdout current line
	shell_print("%s", GENERIC_PSI);
	move_cursor_right(0);	
	// }

}
void move_cursor_right(int index) {
    char seq[32];
    snprintf(seq, sizeof(seq), "\033[%dC", index); // C = right
    write(STDOUT_FILENO, seq, strlen(seq));
}
void move_cursor_left(int n) {
    if (n > 0) {
        char seq[16];
        snprintf(seq, sizeof(seq), "\033[%dD", n);  // D = left
        write(STDOUT_FILENO, seq, strlen(seq));
    }
}
