#include "command.h"
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "parser.h"
#include <readline/readline.h>
#include <readline/history.h>
pid_t child_pid = -1;
char *input;

void handle_sigint(int sig) {
	// shell_print("child: %d \n", child_pid);
    if (child_pid > 0) {
        // Forward SIGINT to the child process
        kill(child_pid, SIGINT);
		return;
    }

    rl_replace_line("", 0);
    rl_crlf(); // newline
    rl_on_new_line(); // move to new line
    rl_redisplay();
}


int main() {

	char *history = NULL;
	shell_print("wellcome to my shell \n");
	signal(SIGINT, handle_sigint);

    while ((input = readline(GENERIC_PSI)) != NULL) {
        if (*input) {
            add_history(input);  // Enable up/down arrow history
			parse_call(input, strlen(input));
			// shell_print("%s", input);
        }
        free(input);
    }

	return 0;
}

