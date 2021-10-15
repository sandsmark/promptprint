#define _GNU_SOURCE

#define MAX_LENGTH 10

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void print_elided(char *path)
{
    if (strlen(path) < MAX_LENGTH) {
        printf("%s", path);
        return;
    }
    int pieces = 0;
    for (int i=0; path[i]; i++) {
        if (path[i] == '/') {
            pieces++;
        }
    }
    if (pieces <= 2) {
        printf("%s", path);
        return;
    }

    int printPre = 0;
    int printPost = 0;
    if (pieces > 4) {
        printPre = 2;
        printPost = 2;
    } else if (pieces > 3) {
        printPre = 2;
        printPost = 1;
    } else {
        printPre = 1;
        printPost = 1;
    }

    const int skip = pieces - printPre - printPost;

    char *saveptr;
    char *part = strtok_r(path, "/", &saveptr);
    printf("/%s/", part);
    for (int i=0; i<printPre - 1; i++) {
        char *part = strtok_r(NULL, "/", &saveptr);
        printf("%s/", part);
    }
    for (int i=0; i<skip; i++) {
        strtok_r(NULL, "/", &saveptr);
    }
    printf("...");
    for (int i=0; i<printPost; i++) {
        char *part = strtok_r(NULL, "/", &saveptr);
        printf("/%s", part);
    }
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    char *cwd = get_current_dir_name();
    const int cwd_len = strlen(cwd);
    char *home = getenv("HOME");
    const int home_len = strlen(home);
    if (home && strncmp(cwd, home, home_len) == 0) {
        if (cwd_len > home_len) {
            memmove(cwd + 1, cwd + home_len + 1, cwd_len - home_len);
        } else {
            cwd[0] = '\0';
        }
        printf("~");
    }

    print_elided(cwd);
    printf("/");
    free(cwd);

    return 0;
}
