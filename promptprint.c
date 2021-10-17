#define _GNU_SOURCE

//#define DEBUG_GIT

#define MAX_LENGTH 10

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>

#define GRAY    "\\[\033[00;37m\\]"
#define RED     "\\[\033[01;31m\\]"
#define GREEN   "\\[\033[01;32m\\]"
#define WHITE   "\\[\033[01;37m\\]"
#define CADET   "\\[\033[00;36m\\]"
#define RESET   "\\[\033[0m\\]"


#ifdef DEBUG_GIT
static struct timespec t;
static double start;

static void print_timer(const char *what)
{
    clock_gettime(CLOCK_MONOTONIC, &t);
    double now = t.tv_sec * 1000. + t.tv_nsec * 0.000001;
    fprintf(stderr, "\n - %s %.2f ms\n", what, now - start);
    start = now;
}
#else
#define print_timer(x)
#endif

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

#ifdef DEBUG_GIT
#define GIT_ERR(x) perror(x)
#else
#define GIT_ERR(x)
#endif

FILE *launch_git_status(int *pid)
{
    int fds[2];
    int ret = pipe(fds);
    if (ret == -1) {
        GIT_ERR("Failed to open pipe");
        return NULL;
    }
    int inRead = fds[0];
    int inWrite = fds[1];

    ret = fork();
    if (ret == -1) {
        GIT_ERR("failed to fork");
        return NULL;
    }
    if (ret == 0) { // Child
        close(inRead);
        dup2(inWrite, STDOUT_FILENO);
        close(inWrite);
        freopen("/dev/null", "w", stderr);
        execlp("git", "git",
                "status", "--ignore-submodules=all", "--branch", "--ahead-behind", "--porcelain", "--untracked-files=no",
                NULL);
        exit(0); // should never get here
    }
    *pid = ret;

    int flags = fcntl(inRead, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(inRead, F_SETFL, flags);

    return fdopen(inRead, "r");
}

int print_git_branch(char *buff)
{
    if (strlen(buff) < strlen("## *...*/*")) {
        return 0;
    }

    char *branch_end = strstr(buff, "...");
    if (branch_end == NULL) {
        return 0;
    }
    *branch_end = '\0';
    char *branch = buff + 3;
    printf("%s", branch);

    char *ahead_behind = strchr(branch_end + 1, '[');
    if (ahead_behind && strchr(ahead_behind, ']')) {
        printf(" %s", ahead_behind);
    }

    return 1;
}

void print_git_status(FILE *git_output)
{
    char buff[4096];
    if (fgets(buff, sizeof(buff), git_output) == NULL) {
        return;
    }
    size_t line_len = strlen(buff);
    if (!line_len) {
        return;
    }
    buff[line_len - 1] = '\0';
    printf(" " CADET "(");
    if (strcmp(buff, "## HEAD (no branch)") == 0) {
        printf("detached");
    } else if (strstr(buff, "## ") != buff || !print_git_branch(buff)) {
        printf("%s", buff);
    }

    // If there's more than one line, there's dirty files
    if (fgets(buff, sizeof(buff), git_output) != NULL) {
        printf(" dirty");
    }
    printf(")");
}

int main(int argc, char *argv[])
{
#ifdef DEBUG_GIT
    clock_gettime(CLOCK_MONOTONIC, &t);
    start = t.tv_sec * 1000. + t.tv_nsec * 0.000001;
#endif

    int git_pid = -1;
    FILE *git_output = launch_git_status(&git_pid);

    if (argc > 1) {
        // Assume argument is return code from previous command
        int ret = atoi(argv[1]);
        if (ret != 0) {
            printf("returned " RED "%d" RESET "\n", ret);
        }
    }
    print_timer("git launch");

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
    print_timer("basic stuff");

    print_path();
    print_timer("path");

    if (git_output) {
        int stat_val = -1;
        waitpid(git_pid, &stat_val, 0);
        if (WIFEXITED(stat_val)) {
            print_git_status(git_output);
        }
        fclose(git_output);
    }

    print_timer("git total");

    printf(RESET " ");
#ifdef DEBUG_GIT
    puts("");
#endif

    return 0;
}
