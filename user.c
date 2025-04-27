
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include "user.h"
#include <string.h>
extern char *home;
extern size_t home_len;

int switch_user(const char *target_user) {
    struct passwd *pwd = getpwnam(target_user);
    if (pwd == NULL) {
        perror("getpwnam failed");
        return -1;  
    }

    printf("Switching to user: %s\n", target_user);
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

    printf("Successfully switched to user: %s\n", target_user);

    return 0;
}

