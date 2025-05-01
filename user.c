#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include "user.h"
#include "command.h"
#include <string.h>
extern char *home;
extern size_t home_len;
int strtoi(const char *nptr, int base, int *out);
int switch_user(const char *target) {
#if !STATIC
    struct passwd *pwd = getpwnam(target);
    if (pwd == NULL) {
        perror("getpwnam failed");
        return -1;  
    }
#else 
	shell_print("Use uid instead of username: \n");
	struct passwd *pwd = malloc(sizeof(struct passwd));
	char *end;
    pwd->pw_gid = strtol(target, &end, 10);
	pwd->pw_uid = pwd->pw_gid;
	pwd->pw_dir = "/";
#endif
    shell_print("Switching to user: %s\n", target);
    if (setgid(pwd->pw_gid) != 0) {
        perror("Failed to set GID");
        return -1;
    }

    if (setuid(pwd->pw_uid) != 0) {
        perror("Failed to set UID");
        return -1;
    }

    if (chdir(pwd->pw_dir) != 0) {
        perror("Failed to change directory");
        return -1;
    }
	setenv("HOME", pwd->pw_dir, 1);
	home = pwd->pw_dir;
	home_len = strlen(home);
    shell_print("Successfully switched to user: %s\n", target);

    return 0;
}

