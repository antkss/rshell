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
#include "user.h"
#ifdef DEBUG
void handler(int sig) {
    printf("signal: %d\n", sig);
}
#endif

int main(int argc, char *args[]) {
	setup_reap_child();
	int opt = 0;
	char *home_path = NULL;
	char *user = NULL;
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

	while ((opt = getopt(argc, args, "r::s::")) != -1) {
		switch (opt) {
			case 'r':
				home_path = optarg;
				if (home_path) {
					setenv("HOME", home_path, 1);
					home = home_path;
					home_len = strlen(home_path);
				}
				remote_shell();
				return 0;
			case 's':
				user = optarg;
				if (user) switch_user(optarg);
				break;
			default:
				fprintf(stderr, "Usage: %s [-r] [-s] home\n", args[0]);
				return 1;
		}
	}
	local_shell();

	return 0;
}

