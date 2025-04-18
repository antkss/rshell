#include "command.h"
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "parser.h"
#include <readline/readline.h>
#include <readline/history.h>
#include "config.h"
#include "complete.h"
#include <arpa/inet.h>
#include <pty.h>
#include <sys/wait.h>
char *home = NULL;
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

void read_config() {
	size_t read_size;
	char *config_path = malloc(strlen(home) + strlen(CONFIG_FILE_NAME) + 0x10);
	sprintf(config_path, "%s/%s", home, CONFIG_FILE_NAME);
	char *config_read = read_file(config_path, &read_size);
	if (read_size < ARG_MAX && config_read) {
		parse_call(config_read);
		free(config_path);
		free(config_read);
	}

}


int main(int argc, char *args[]) {
	int opt = 0;
	int remote = 0;
	while ((opt = getopt(argc, args, "r")) != -1) {
		switch (opt) {
			case 'r':
				remote = 1;
				break;
			default:
				fprintf(stderr, "Usage: %s [-r] source dest\n", args[0]);
				return 1;
		}
	}
	if (remote > 0) {
		int sock = socket(AF_INET, SOCK_STREAM, 0);  // TCP
		return 0;
	}
	if (argc == 1) {
		rl_attempted_completion_function = my_completion;
		// for (int c = 32; c <= 126; ++c) {
		//     rl_bind_key(c, key_logger);
		// }
		home = getenv("HOME");
		read_config();
		shell_print("wellcome to my shell \n");
		signal(SIGINT, handle_sigint);
		char *psi_set = getenv("PSI_RSHELL");
		if (psi_set == NULL) psi_set = GENERIC_PSI;
		while ((input = readline(GENERIC_PSI)) != NULL) {
			if (*input) {
				add_history(input);  // Enable up/down arrow history
				parse_call(input);
				// shell_print("%s", input);
			}
			free(input);
			input = NULL;
		}
	} else {
			for (int i = 1; i < argc; i++) {
				if (args[i]) {
					parse_call(args[i]);
				}
			}

	}

	return 0;
}

