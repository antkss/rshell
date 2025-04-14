#include "command.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "terminal.h"
#include "parser.h"
#define MAX_HISTORY 0x1000

char input_buf[ARG_MAX];
int input_len = 0;
int line_cnt = 0;
char *command_history[MAX_HISTORY];
unsigned int history_cnt = 0;

void handle_deletion() {
    input_buf[cursor] = '\0';
	char *next_buf = &input_buf[cursor + 1];
	// shell_print("next_buf %s ", next_buf);
	strcat(input_buf, next_buf);
	// printf(" | input len: %d\n", input_len);
	// printf(" | cursor: %d\n", cursor);
	delete_at_cursor();
	// r@🍎 | lmaodark |
	//    start       end
	//  the cursor variable from the start is 0
	//  and at the end of the buffer is actually the buffer len, the real position is 0 from the end of the buffer to the next slot
}


void insert_char(char ch, char *buf) {
	char *part_1 = buf;
	char *part_2 = &buf[cursor];
	int len_part2 = input_len - cursor;
	
	if (len_part2 < 0) {
		shell_print("len_part2 error cursor: %d, input_len: %d \n", cursor, input_len);
		return;
	}
	if (len_part2 >= 0) {
		char *tmp = malloc(len_part2 + 0x10);
		strncpy(tmp, part_2, len_part2);
		buf[cursor] = ch;
		buf[cursor + 1] = 0;
		strcat(buf, tmp);
		free(tmp);
		shell_print("\033[1@");
		write(STDOUT_FILENO, &ch, 1);
	} else {
		input_buf[input_len] = ch;
		write(STDOUT_FILENO, &ch, 1);
	}
	cursor++;
	input_len++;
	input_buf[input_len] = '\0';
}


int main() {

	char *history = NULL;
	unsigned int history_len = 0;
	shell_print("wellcome to my shell \n");
	set_raw_mode(1);
	signal(SIGINT, SIG_IGN);
	while(1) {
		// shell_print("new buffer\n");
		print_sign(GENERIC_PSI);
		cursor = 0;
		input_len = 0;
		int history_idx = 0;
		int counter = history_cnt;

		while (1) {
			char c;
			read(STDIN_FILENO, &c, 1);
			// printf("byte: %d\n", (int)c);
			if (c == '\x1b') { // Escape sequence
				char seq[2];
				if (read(STDIN_FILENO, &seq[0], 1) == 0) continue;
				if (read(STDIN_FILENO, &seq[1], 1) == 0) continue;
				if (seq[0] == '[') {
					if (seq[1] == 'A') {
						if (history_cnt > 0) {
							counter--;
							clear_chars(input_len);
							history_idx = counter % history_cnt;
							history = command_history[history_idx];
							cursor = input_len = strlen(history);
							shell_print("%s", history);
							// shell_print("idx: %d", history_idx);
							strncpy(input_buf, history, input_len);

						}
					} else if (seq[1] == 'B') {
						if (history_cnt > 0) {
							counter++;
							clear_chars(input_len);
							history_idx = counter % history_cnt;
							history = command_history[history_idx];
							cursor = input_len = strlen(history);
							shell_print("%s", history);
							// shell_print("idx: %d", history_idx);
							strncpy(input_buf, history, input_len);

						} 
					} else if (seq[1] == 'C') { // move cusor to right
						// shell_print("right");
						if (cursor < input_len) {
							cursor++;
							write(STDOUT_FILENO, "\033[1C", 4);
						}
						// shell_print("cursor: %d \n", cursor);
					} else if (seq[1] == 'D') {
						// shell_print("left");
						if (cursor > 0) {
							cursor--;
							write(STDOUT_FILENO, "\033[1D", 4);
						}
					}
				}
			} else if (c == 127 || c == 8) {  // Backspace
				if (input_len > 0 && cursor > 0) {
					input_len--;
					cursor--;
					handle_deletion();
				}
			} else if (c == 5) {

			} else if (c == 4) {
				if (input_len == 0) {
					shell_print("\nctrl D");
					set_raw_mode(0);
					_exit(1);
				}
			} else if (c == '\r' || c == '\n') {
				// input_buf[input_len] = '\0';
				shell_print("\n");
				break;
			} else if (c == 3) {
				input_buf[input_len] = '\0';
				input_len = 0;
				cursor = 0;
				shell_print("\n");
				print_sign(GENERIC_PSI);
				continue;
			} else if (c == '\t') {
				continue;
			} else {
				if (input_len < sizeof(input_buf) - 1) {
					insert_char(c, input_buf);
				}
			}
		}
		parse_call(input_buf, input_len);
	}
	return 0;
}

