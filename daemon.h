#include <signal.h>     // for size_t
#include <sys/types.h>  // for pid_t
extern char *home;
extern pid_t child_pid;
extern char *input;
extern int sockfd;
extern size_t home_len;
void read_config();
void local_shell();
void remote_shell();
void setup_reap_child();


