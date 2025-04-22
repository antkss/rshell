#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "command.h"
#include "config.h"
#include "complete.h"
#include <arpa/inet.h>
#include <pty.h>
#include <sys/wait.h>
#include "daemon.h"
#ifdef DEBUG
void handler(int sig) {
    printf("signal: %d\n", sig);
}
#endif

int main(int argc, char *args[]) {
	int opt = 0;
	int remote = 0;
	char *home_path = NULL;
#ifdef DEBUG
	signal(SIGUSR1, handler);
	pause();
#endif
	home = getenv("HOME");
	if (!home) {
		home = "/"; // or some known writable path
		home_len = 1;
		setenv("HOME", home, 1);
		shell_print("no HOME env found \n");
	} else {
		home_len = strlen(home);
	}

	while ((opt = getopt(argc, args, "r::")) != -1) {
		switch (opt) {
			case 'r':
				remote = 1;
				home_path = optarg;
				break;
			default:
				fprintf(stderr, "Usage: %s [-r] home\n", args[0]);
				return 1;
		}
	}
	if (remote != 0) {
		if (home_path) {
			setenv("HOME", home_path, 1);
			home = home_path;
			home_len = strlen(home_path);
		}
		remote_shell();
		return 0;
	}
	if (argc == 1) {
		local_shell();
	} else {
		for (int i = 1; i < argc; i++) {
			if (args[i]) {
				parse_call(args[i]);
			}
		}

	}

	return 0;
}

