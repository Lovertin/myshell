#include "shell.h"
#include "job.h"
#include <pwd.h>

char *get_prompt(void)
{
    static char prompt[MAX_CMD_LEN];
    char hostname[256];
    char cwd[1024];
    const char *username = getenv("USER");
    const char *home = getenv("HOME");

    if (username == NULL)
    {
        struct passwd *pw = getpwuid(getuid());
        username = pw ? pw->pw_name : "user";
    }

    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        strcpy(hostname, "localhost");
    }

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        strcpy(cwd, "~");
    }
    else if (home != NULL && strncmp(cwd, home, strlen(home)) == 0)
    {
        // 将 HOME 目录替换为 ~
        char temp[1024];
        snprintf(temp, sizeof(temp), "~%s", cwd + strlen(home));
        strcpy(cwd, temp);
    }

    snprintf(prompt, sizeof(prompt), "%s@%s:%s$ ", username, hostname, cwd);
    return strdup(prompt);
}

void setup_signals(void)
{
    // Shell 主进程忽略 SIGINT (Ctrl+C)，防止 Shell 自身被终止
    signal(SIGINT, SIG_IGN);
    // 忽略 SIGTSTP (Ctrl+Z)
    signal(SIGTSTP, SIG_IGN);
    // 忽略 SIGTTOU 和 SIGTTIN，防止 tcsetpgrp 时 Shell 自身被停止
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    // 处理 SIGCHLD，回收后台子进程，避免僵尸进程
    signal(SIGCHLD, sigchld_handler);
}

void sigchld_handler(int sig)
{
    // 非阻塞回收所有已终止的子进程，并更新作业状态
    (void)sig;
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        update_job_status(pid, status);
    }
}
