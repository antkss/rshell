#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>
#include <pty.h>

#define PORT 4444
int sockfd = -1;
struct termios orig_termios;
// predefined functions
struct winsize get_terminal_size();
void send_ws(int sockfd);
void handle_socket(int sockfd); 
void disable_raw_mode();
void enable_raw_mode();
void handle_sigint(int sig);
void handle_sigwinch(int sig);

enum {
	TRUE, 
	FALSE,
	NO_AUTH, 
	AUTH,
	START_AUTH,
	START_SHELL,
	RESIZE,
	TTY_BUFFER,
};
int main(int argc, char *args[]) {
	char ip[16] = "127.0.0.1";
	if (argc > 1) {
		size_t len = strlen(args[1]);
		strncpy(ip, args[1], len);
		ip[len] = 0;
	}
	int pty_master;
	char cmd = -1;
	char password[64];
	printf("ip: %s \n", ip);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = inet_addr(ip),
    };
	signal(SIGWINCH, handle_sigwinch);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        return 1;
    }
	recv(sockfd, &cmd, 1, 0);
	// printf("%d \n", cmd);
	if (cmd == AUTH) {
		char result = -1;
		fflush(stdout);
		printf("input password: ");
		fgets(password, 63, stdin);
		fflush(stdout);
		char start_auth = START_AUTH;
		send(sockfd, &start_auth, 1, 0);
		send(sockfd, password, 63, 0);
		recv(sockfd, &result, 1, 0);
		if (result == TRUE) {
			printf("starting shell \n");
		} else {
			printf("wrong password ! \n");
			exit(EXIT_FAILURE);
		}
	}
	cmd = START_SHELL;
	write(sockfd, &cmd, 1);
	send_ws(sockfd);
	handle_socket(sockfd);
	close(sockfd);

	return 0;
}
struct winsize get_terminal_size(){
	struct winsize w;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
		perror("ioctl error");
	}
	return w;
}
void send_ws(int sockfd) {
	struct winsize ws = get_terminal_size();
	write(sockfd, &ws, sizeof(ws));
}
void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_oflag &= ~(OPOST);
    tcsetattr(STDIN_FILENO, TCSADRAIN, &raw);
}
void handle_sigwinch(int sig) {
	char resize_type = RESIZE;
	write(sockfd, &resize_type, 1);
	send_ws(sockfd);
}

void handle_socket(int sockfd) {
	enable_raw_mode();
    char buffer[ARG_MAX];
	fd_set fds;
	memset(buffer, 0, sizeof(buffer));
    while (1) {
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        FD_SET(STDIN_FILENO, &fds);
        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

        if (select(maxfd + 1, &fds, NULL, NULL, NULL) < 0) continue;

        if (FD_ISSET(sockfd, &fds)) {
            ssize_t n = recv(sockfd, buffer, sizeof(buffer), 0);
            if (n <= 0) break;
            if (buffer[0] == RESIZE && n >= 9) {
                struct winsize ws;
                memcpy(&ws, buffer + 1, sizeof(ws));
                ioctl(STDOUT_FILENO, TIOCSWINSZ, &ws);
            } else {
                write(STDOUT_FILENO, buffer, n);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &fds)) {
            ssize_t n = read(STDIN_FILENO, buffer + 1, sizeof(buffer) - 1);
            if (n <= 0) break;
            buffer[0] = TTY_BUFFER;
            send(sockfd, buffer, n + 1, 0);
        }
    }
	disable_raw_mode();
}
