#include <arpa/inet.h>
#include <fcntl.h>
#include <pty.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <termios.h>
#include <sys/select.h>
#include <wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "complete.h"
#include "terminal.h"
#include "command.h"
#include "parser.h"
#include "command.h"
#include "config.h"
extern char *home;
extern pid_t child_pid;
extern char *input;
extern int sockfd;
extern size_t home_len;
void read_config();
void local_shell();
void remote_shell();
void setup_reap_child();


