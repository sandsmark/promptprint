#define _GNU_SOURCE

// All my precious optimizations are wasted because of libgit2.  The rest of
// the code runs in less than 0.5ms, and then just git_libgit2_init takes
// dozens of milliseconds...
//#define LIBGIT2_SUCKS

#define MAX_LENGTH 10

#include <git2/status.h>
#include <git2/global.h>
#include <git2/repository.h>
#include <git2/buffer.h>
#include <git2/errors.h>
#include <git2/graph.h>
#include <git2/branch.h>

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

#ifdef LIBGIT2_SUCKS
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

void print_git_status(git_repository *repo)
{
    git_status_options statusopt;
    git_status_options_init(&statusopt, GIT_STATUS_OPTIONS_VERSION);
    statusopt.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    statusopt.flags = GIT_STATUS_OPT_EXCLUDE_SUBMODULES | GIT_STATUS_OPT_NO_REFRESH;

    git_status_list *status = NULL;
    int ret = git_status_list_new(&status, repo, &statusopt);
    if (ret == 0 && git_status_list_entrycount(status)) {
        printf(" dirty");
    }
    git_status_list_free(status);
}

void print_git_ahead_behind(git_repository *repo, git_reference *head)
{
    git_reference *upstream_branch = NULL;
    int ret = git_branch_upstream(&upstream_branch, head);
    if (ret != 0 || !upstream_branch) {
        return;
    }

    const git_oid *local = git_reference_target(head);
    const git_oid *upstream = git_reference_target(upstream_branch);

    if (!local || !upstream) {
        return;
    }

    size_t ahead = 0, behind = 0;
    ret = git_graph_ahead_behind(&ahead, &behind, repo, local, upstream);
    if (ret != 0) {
        return;
    }
    git_reference_free(upstream_branch);
    if (!ahead && !behind) {
        return;
    }
    printf(" [");
    if (ahead) {
        printf("%d ahead", (int)ahead);

        if (behind) {
            printf(", ");
        }
    }
    if (behind) {
        printf("%d behind", (int)behind);
    }
    printf("]");
}

void print_git_branch(git_repository *repo)
{
    if (git_repository_head_detached(repo)) {
        printf("detached");
        return;
    }

    git_reference *head = NULL;
    int ret = git_repository_head(&head, repo);
    if (ret != 0) {
        return;
    }
    const char *name = git_reference_shorthand(head);
    printf("%s", name);
    print_git_ahead_behind(repo, head);
    git_reference_free(head);
}

void print_git()
{
    print_timer("pre git init");
    git_libgit2_init();
    print_timer("git init");

    char *cwd = get_current_dir_name();
    git_repository *repo = NULL;
    int ret = git_repository_open_ext(&repo, cwd, 0, cwd);
    free(cwd);
    if (ret != 0) {
        return;
    }
    printf(CADET " (");
    print_git_branch(repo);
    print_timer("branch");
    print_git_status(repo);
    print_timer("status");
    git_repository_free(repo);
    printf(") ");
}

int main(int argc, char *argv[])
{
#ifdef LIBGIT2_SUCKS
    clock_gettime(CLOCK_MONOTONIC, &t);
    start = t.tv_sec * 1000. + t.tv_nsec * 0.000001;
#endif

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
    print_timer("asic");

    print_path();
    print_timer("path");
    print_git();
    print_timer("git total");

    printf(RESET_COLOR);

    return 0;
}
