#define _GNU_SOURCE

#define MAX_LENGTH 10

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define GRAY "\033[00;37m"
#define RED "\033[01;31m"
#define GREEN "\033[01;32m"
#define WHITE "\033[01;37m"
#define CADET "\033[00;36m"
#define RESET_COLOR "\033[00m"

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

static void print_path()
{
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
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    char buf[200];

    {
        time_t now;
        struct tm result;
        time(&now);

        strftime(buf, sizeof buf, "%T", localtime_r(&now, &result));

        printf("%s[%s] ", GRAY, buf);
    }
    if (gethostname(buf, sizeof buf) == 0) {
        buf[(sizeof buf) - 1] = '\0';
        printf("%s", buf);
    }
    printf("%s: " WHITE, geteuid() == 0 ? RED : GREEN);

    print_path();

    printf(RESET_COLOR);

    return 0;
}
