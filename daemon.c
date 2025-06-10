#if __has_include(<linux/limits.h>)
#include <linux/limits.h>
#endif
#include "daemon.h"
#include <stdio.h>
#include <arpa/inet.h>          // for htons
#include <fcntl.h>              // for open, O_RDONLY
#include <netinet/in.h>         // for sockaddr_in, INADDR_ANY, in_addr
#include <pty.h>                // for forkpty
#include <readline/history.h>   // for add_history
#include <readline/readline.h>  // for readline, rl_crlf, rl_on_new_line
#include <signal.h>             // for kill, sigaction, size_t, SIGINT, sige...
#include <stdio.h>              // for perror, NULL, printf
#include <stdlib.h>             // for exit, free, malloc, getenv
#include <string.h>             // for memcpy, memset, strtok, memcmp, strlen
#include <sys/ioctl.h>          // for winsize, ioctl, TIOCSWINSZ
#include <sys/select.h>         // for FD_ISSET, FD_SET, select, FD_ZERO
#include <sys/socket.h>         // for recv, AF_INET, accept, bind, listen
#include <sys/wait.h>           // for waitpid, WNOHANG
#include <unistd.h>             // for close, read, write, fork
#include "command.h"            // for shell_print, CONFIG_FILE_NAME, GENERI...
#include "complete.h"           // for completion_setup
#include "config.h"             // for read_file
#include "parser.h"             // for parse_call
#define PORT 4444
char *home = NULL;
size_t home_len = 0;
pid_t child_pid = -1;
char *input;
int sockfd = -1;
enum {
	TRUE, 
	FALSE,
	NO_AUTH, 
	AUTH,
	START_AUTH,
	START_SHELL,
	RESIZE,
	TTY_BUFFER,
	FILES,
};
void handle_sigint(int sig) {
    if (child_pid > 0) {
        kill(child_pid, SIGINT);
		return;
    }

    rl_crlf(); 
    rl_on_new_line(); 
    rl_replace_line("", 0);
    rl_redisplay();
	if (sockfd != 0) {
		close(sockfd);
	}
}
void read_config() {
	size_t read_size;
	size_t file_len = sizeof(CONFIG_FILE_NAME);
	char *config_path = malloc(home_len + file_len + 0x10);
	memcpy(config_path, home, home_len);
	memcpy(&config_path[home_len], "/", 1);
	memcpy(&config_path[home_len + 1], CONFIG_FILE_NAME, file_len); 
	char *config_read = read_file(config_path, &read_size);
	if (config_read) {
		parse_call(config_read);

	}
	free(config_path);
	free(config_read);

}
void local_shell() {
	read_config();
	shell_print("wellcome to my shell \n");
	rl_catch_signals = 0;
	signal(SIGINT, handle_sigint);
	char *psi_set = getenv("PSI_RSHELL");
	if (psi_set == NULL) psi_set = GENERIC_PSI;
	completion_setup();
	while ((input = readline(psi_set)) != NULL) {
		if (*input) {
			add_history(input);  
			parse_call(input);
		}
		free(input);
		input = NULL;
	}
}
void handle_client(int client_fd) {
    int pty_master;
    pid_t pid;
    struct winsize ws;

    if (read(client_fd, &ws, sizeof(ws)) != sizeof(ws)) {
        perror("read winsize");
        close(client_fd);
        return;
    }

    pid = forkpty(&pty_master, NULL, NULL, &ws);
    if (pid < 0) {
        perror("forkpty");
        close(client_fd);
        return;
    }

    if (pid == 0) {
		local_shell();
        perror("execl");
        exit(1);
    }

    char buf[ARG_MAX];
    fd_set fds;
	memset(buf, 0, sizeof(buf));

    while (1) {
        FD_ZERO(&fds);
        FD_SET(client_fd, &fds);
        FD_SET(pty_master, &fds);
        int maxfd = (client_fd > pty_master ? client_fd : pty_master) + 1;

        if (select(maxfd, &fds, NULL, NULL, NULL) < 0) break;

        if (FD_ISSET(client_fd, &fds)) {
            unsigned char msg_type;
            ssize_t n = read(client_fd, &msg_type, 1);
            if (n <= 0) break;

            if (msg_type == TTY_BUFFER) {
                n = read(client_fd, buf, sizeof(buf) - 1);
                if (n <= 0) break;
                write(pty_master, buf, n);
            } else if (msg_type == RESIZE) {
                struct winsize new_ws;
                if (read(client_fd, &new_ws, sizeof(new_ws)) == sizeof(new_ws)) {
                    ioctl(pty_master, TIOCSWINSZ, &new_ws);
                    kill(pid, SIGWINCH);
                }
            }
        }

        if (FD_ISSET(pty_master, &fds)) {
            ssize_t n = read(pty_master, buf, sizeof(buf) - 1);
            if (n <= 0) break;
            write(client_fd, buf, n);
        }
    }

    kill(pid, SIGKILL);
    close(pty_master);
    close(client_fd);
    waitpid(pid, NULL, 0);
}

int basic_authencation(int client_fd) {
	char path_file[] = ".rpass";
	size_t file_len = sizeof(path_file);
	char *rpass_full = malloc(file_len + home_len + 2);
	char password[64];
	char user[64];
	char result = 0;
	rpass_full[0] = 0;
	memcpy(rpass_full, home, home_len);
	memcpy(rpass_full + home_len, "/", 1);
	memcpy(rpass_full + home_len + 1, path_file, file_len);
	memset(password, 0, 64);
	memset(user, 0, 64);
	int fd = open(rpass_full, O_RDONLY);
	if (fd < 0) {
		perror("open");
		result = NO_AUTH;
	} else {
		char auth = AUTH;
		write(client_fd, &auth, 1);
		char start_auth = 0;
		recv(client_fd, &start_auth, 1, 0);
		shell_print("user %d authencation activated ! \n", client_fd);
		read(fd, password, 63);
		recv(client_fd, user, 63, 0);
		strtok(user, "\n");
		strtok(password, "\n");
		if (!memcmp(user, password, strlen(password))) {
			result = TRUE;
		} else {
			result = FALSE;
		}
	}
	write(client_fd, &result, 1);
	free(rpass_full);
	return result;
}
#ifdef DAEMON
void daemonize() {
	int pid = fork();

	if (pid > 0) exit(EXIT_SUCCESS);
	if (pid < 0) exit(EXIT_FAILURE);
	
	if (setsid() < 0) {
		exit(EXIT_FAILURE);
	}
	if (pid > 0) exit(EXIT_SUCCESS);
	if (pid < 0) exit(EXIT_FAILURE);
	umask(0);
	chdir("/");
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDOUT_FILENO);
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);
	
}
#endif
void reap_children(int sig) {
    // reap all dead children
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
void setup_reap_child() {
	struct sigaction sa;
    sa.sa_handler = reap_children;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &sa, NULL);
}
void remote_shell() {

    int server_fd, client_fd;
    struct sockaddr_in addr;

    socklen_t addrlen = sizeof(addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
#ifdef DAEMON
	daemonize();
#endif

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
		int auth = basic_authencation(client_fd);
		if (auth == FALSE) {
			close(client_fd);
			shell_print("wrong password, refuse the client\n");
			continue;
		} 
		char start_shell = 0;
		recv(client_fd, &start_shell, 1, 0);

        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
			shell_print("new client connected %d \n", client_fd);
            handle_client(client_fd);
            exit(0);
        }
        close(client_fd);
    }

    close(server_fd);
}
